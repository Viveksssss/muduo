#pragma once
#include <Poller.h>
#include <sys/epoll.h>

class EpollPoller : public Poller {
public:
    EpollPoller(EventLoop *loop);
    ~EpollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    /* _events.初始长度 */
    static int const initEventListSize = 16;
    using EpollList = std::vector<epoll_event>;
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    void update(int operation, Channel *channel);

private:
    int _epollfd;
    EpollList _events;
};
