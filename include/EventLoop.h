#pragma once

#include <atomic>
#include <bits/types/timer_t.h>
#include <CurrentThread.h>
#include <functional>
#include <memory>
#include <noncapyable.h>
#include <Timestamp.h>

class Channel;
class Poller;

class EventLoop : noncopyable {
public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();
    void loop();
    void quit();

    Timestamp pollReturnTime() const {
        return _pollReturnTime;
    }

    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);

    void wakeup();
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    bool isInLoopThread() const {
        return _threadId == CurrentThread::tid();
    };

private:
    void handleRead();        /* wakeupFd发生读事件时的回调函数 */
    void doPendingFunctors(); /* 执行回调函数列表 */
private:
    using ChannelList = std::vector<Channel *>;

    std::atomic<bool> _looping;      /* 是否正在事件循环 */
    std::atomic<bool> _quit;         /* 是否退出事件循环 */
    pid_t const _threadId;           /* 事件循环所在的线程ID */
    Timestamp _pollReturnTime;       /* 上一次调用poll()返回的时间点 */
    std::unique_ptr<Poller> _poller; /* 事件循环使用的Poller */

    int _wakeupFd;                   /* eventfd, 用于唤醒事件循环所在的线程 */
    std::unique_ptr<Channel> _wakeupChannel;   /* 用于监视_wakeupFd的Channel */
    Channel *_currentActiveChannel;            /* 当前正在处理的Channel */
    ChannelList _activeChannels;               /* 活跃的Channel列表 */

    std::vector<Functor> _pendingFunctors;     /* 待执行的函数列表 */
    std::atomic<bool> _callingPendingFunctors; /* 是否正在调用待处理的函数 */
    mutable std::mutex _mutex; /* 保护_pendingFunctors的互斥锁 */
};

// clang-format off
/*
    1.关于这个_wakeupFd的说明

    // TcpServer 中，主 Reactor 拿到新连接后
    void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
        // 1. 轮询选择一个子 Reactor（选定了目标对象！）
        EventLoop* ioLoop = threadPool_->getNextLoop();

        // 2. 直接把任务塞给这个选定的子 Reactor
        //    注意：这里调用的是 ioLoop->runInLoop()，
        //    这个 ioLoop 就是那个被选中的子 Reactor 对象！
        ioLoop->runInLoop([=] {
            // 这个 lambda 会在 ioLoop 的线程里执行
            TcpConnectionPtr conn(new TcpConnection(ioLoop, sockfd, ...));
            connections_[sockfd] = conn;
            conn->connectEstablished();
        });
    }

    // runInLoop 内部，最终会调用目标 EventLoop 自己的 wakeup()
    void EventLoop::queueInLoop(Functor cb) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _pendingFunctors.push_back(std::move(cb));
        }

        if (!isInLoopThread() || _callingPendingFunctors) {
            // 重点：这里调用的是 this->wakeup()
            // this 就是被选中的那个子 Reactor 对象
            // 所以写的是这个子 Reactor 自己的 _wakeupFd
            wakeup();  // 向 this->_wakeupFd 写入数据
        }
    }

    2. _currentActiveChannel 的核心用处是解决"在处理事件时删除自己"的棘手问题。
        1)如果没有此变量
            此时，removeChannel 需要把这个 Channel 从 Poller 中移除。但问题是：这个 Channel 此刻正在 _activeChannels 列表中被遍历。如果直接erase(it).遍历 _activeChannels 的循环会因为迭代器失效而崩溃或产生未定义行为。
        2)有了此变量
            void EventLoop::removeChannel(Channel* channel) {
                // 如果正在被处理的正是这个 Channel
                if (channel == _currentActiveChannel) {
                    // 不做实际删除，只打个标记，等 handleEvent 执行完再说
                    // 这样可以安全地让当前事件处理完成
                } else {
                    // 不是当前正在处理的，可以直接从 _activeChannels 移除
                }
            }

*/
// clang-format on
