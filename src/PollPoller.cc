#include <Channel.h>
#include <PollPoller.h>

PollPoller::PollPoller(EventLoop *loop) : Poller(loop) { }

PollPoller::~PollPoller() { }

Timestamp PollPoller::poll([[maybe_unused]] int timeoutMs,
    [[maybe_unused]] ChannelList *activeChannels) {
    return {};
}

void PollPoller::updateChannel([[maybe_unused]] Channel *channel) { }

void PollPoller::removeChannel([[maybe_unused]] Channel *channel) { }
