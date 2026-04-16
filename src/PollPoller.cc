#include <PollPoller.h>

PollPoller::PollPoller(EventLoop *loop) : Poller(loop) { }

PollPoller::~PollPoller() { }

Timestamp PollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
    return {};
}

void PollPoller::updateChannel(Channel *channel) { }

void PollPoller::removeChannel(Channel *channel) { }
