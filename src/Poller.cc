#include <Channel.h>
#include <Poller.h>

Poller::Poller(EventLoop *loop) : _ownerLoop(loop) { }

bool Poller::hasChannel(Channel *channel) const {
    auto it = _channels.find(channel->fd());
    return it != _channels.end() && it->second == channel;
}
