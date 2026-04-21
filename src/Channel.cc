#include <Channel.h>
#include <EventLoop.h>
#include <Logger.h>
#include <sys/epoll.h>
#include <Timestamp.h>

int const Channel::_noneEvent = 0;
int const Channel::_readEvent = EPOLLIN | EPOLLPRI;
int const Channel::_writeEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : _loop(loop)
    , _fd(fd)
    , _events(0)
    , _revents(0)
    , _index(-1)
    , _tied(false) { }

Channel::~Channel() { }

/* 当channel的fd的events更新之后,更新poller中的channel的状态 */
void Channel::update() {
    _loop->updateChannel(this);
}

/* 在poller中移除channel */
void Channel::remove() {
    _loop->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime) {
    std::shared_ptr<void> guard;
    /* 由TcpConnection绑定,所以需要确定对象是否存在 */
    if (_tied) [[likely]] {
        guard = _tie.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
    }
    /* 比如Accceptor等无需绑定TcpConnection,直接由EventLoop调用 */
    else {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime) {
    log_info("Channel::handleEventWithGuard revents:{}", _revents);
    if (_revents & EPOLLHUP && !(_revents & EPOLLIN)) {
        if (_closeCallback) {
            _closeCallback();
        }
    }
    if (_revents & EPOLLERR) {
        if (_errorCallback) {
            _errorCallback();
        }
    }
    if (_revents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (_readCallback) {
            _readCallback(receiveTime);
        }
    }
    if (_revents & EPOLLOUT) {
        if (_writeCallback) {
            _writeCallback();
        }
    }
}
