#pragma once

#include "noncapyable.h"
#include "Timestamp.h"
#include <unordered_map>
#include <vector>

class Channel;
class EventLoop;

/* muduo中核心的IO复用模块 */
class Poller : noncopyable {
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;
    /* 类比event_wait */
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;
    bool hasChannel(Channel *channel) const;
    static Poller *newDefaultPoller(EventLoop *loop);

protected:
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap _channels;

private:
    EventLoop *_ownerLoop;
};
