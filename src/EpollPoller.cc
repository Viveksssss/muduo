#include <cassert>
#include <Channel.h>
#include <cstring>
#include <EpollPoller.h>
#include <Logger.h>
#include <sys/epoll.h>
#include <unistd.h>

int const _new = -1;
int const _added = 1;
int const _deleted = 2;

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop)
    , _epollfd(::epoll_create1(EPOLL_CLOEXEC))
    , _events(initEventListSize) {
    if (_epollfd < 0) {
        log_fatal("epoll_create1 error:{}", std::strerror(errno));
    }
}

EpollPoller::~EpollPoller() {
    ::close(_epollfd);
}

Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
    log_trace("total fd:{} timeout={}ms", _channels.size(), timeoutMs);
    int numEvents = ::epoll_wait(
        _epollfd, _events.data(), static_cast<int>(_events.size()), timeoutMs);
    Timestamp now(Timestamp::now());
    if (numEvents > 0) {
        log_trace("{} events happened", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if (static_cast<size_t>(numEvents) == _events.size()) {
            _events.resize(_events.size() * 2);
        }
    } else if (numEvents == 0) {
        log_trace("nothing happened");
    } else {
        if (errno != EINTR) {
            log_error("epoll_wait error:{}", std::strerror(errno));
        }
    }
    return now;
}

void EpollPoller::updateChannel(Channel *channel) {
    int const index = channel->index();
    log_trace("func={} channel fd={} events={} index={}", __func__,
        channel->fd(), channel->events(), index);
    if (index == _new || index == _deleted) {
        int fd = channel->fd();
        if (index == _new) {
            assert(_channels.find(fd) == _channels.end());
            _channels[fd] = channel;
        } else { // index == _deleted
            assert(_channels.find(fd) != _channels.end());
            assert(_channels[fd] == channel);
        }
        channel->set_index(_added);
        update(EPOLL_CTL_ADD, channel);
    } else { // index == _added
        int fd = channel->fd();
        assert(_channels.find(fd) != _channels.end());
        assert(_channels[fd] == channel);
        assert(index == _added);
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(_deleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EpollPoller::removeChannel(Channel *channel) {
    int fd = channel->fd();
    log_trace("func={} channel fd={}", __func__, fd);
    assert(_channels.find(fd) != _channels.end());
    assert(_channels[fd] == channel);
    assert(channel->isNoneEvent());
    assert(channel->index() == _added || channel->index() == _deleted);
    size_t n = _channels.erase(fd);
    assert(n == 1);
    if (channel->index() == _added) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(_new);
}

void EpollPoller::fillActiveChannels(
    int numEvents, ChannelList *activeChannels) const {
    assert(static_cast<size_t>(numEvents) <= _events.size());
    for (int i = 0; i < numEvents; ++i) {
        auto *channel = static_cast<Channel *>(_events[i].data.ptr);
        channel->set_revents(_events[i].events);
        activeChannels->push_back(channel);
    }
}

void EpollPoller::update(int operation, Channel *channel) {
    struct epoll_event event;
    bzero(&event, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    log_trace("func={} op={} fd={} event={}", __func__, operation, fd,
        static_cast<uint32_t>(event.events));
    if (::epoll_ctl(_epollfd, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            log_error("epoll_ctl op={} fd={} error:{}", operation, fd,
                std::strerror(errno));
        } else {
            log_fatal("epoll_ctl op={} fd={} error:{}", operation, fd,
                std::strerror(errno));
        }
    }
}
