#include <Channel.h>
#include <EpollPoller.h>
#include <EventLoop.h>
#include <Logger.h>
#include <Poller.h>
#include <sys/eventfd.h>

namespace {

thread_local EventLoop *t_loopInThisThread = nullptr;

int const kPollTimeMs = 10000; /* 默认超时时间 */

int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        log_fatal("Failed in eventfd");
    }
    return evtfd;
}

} // namespace

EventLoop::EventLoop()
    : _looping(false)
    , _quit(false)
    , _threadId(CurrentThread::tid())
    , _pollReturnTime()
    , _poller(Poller::newDefaultPoller(this))
    , _wakeupFd(createEventfd())
    , _wakeupChannel(std::make_unique<Channel>(this, _wakeupFd))
    , _currentActiveChannel(nullptr)
    , _activeChannels()
    , _pendingFunctors()
    , _callingPendingFunctors(false)
    , _mutex() {
    log_debug("EventLoop created {} in thread %d", static_cast<void *>(this),
        _threadId);
    if (t_loopInThisThread) {
        log_fatal("Another EventLoop {} exists in this thread {}",
            static_cast<void *>(t_loopInThisThread), _threadId);
    } else {
        t_loopInThisThread = this;
    }

    /* 设置wakeupfd的事件类型以及回调 */
    _wakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead, this));
    _wakeupChannel->enableReading();
}

EventLoop::~EventLoop() {
    log_debug("EventLoop {} of thread {} destructs", static_cast<void *>(this),
        _threadId);

    _wakeupChannel->disableAll();
    _wakeupChannel->remove();
    ::close(_wakeupFd);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop() {
    _looping = true;
    _quit = false;
    log_trace("EventLoop {} start looping", static_cast<void *>(this));
    while (!_quit) {
        _activeChannels.clear();
        _pollReturnTime = _poller->poll(kPollTimeMs, &_activeChannels);
        for (Channel *channel: _activeChannels) {
            log_trace("EventLoop::loop() active channel: {}", channel->fd());
            _currentActiveChannel = channel;
            _currentActiveChannel->handleEvent(_pollReturnTime);
        }
        _currentActiveChannel = nullptr;
        /*
            _activeChannels是操作系统返回的网络IO事件
            _pendingFunctors是用户通过runInLoop()或者queueInLoop()方法提交的回调函数
            来源不同,目的不同,生命周期不同,所以分开来处理
        */
        doPendingFunctors();
    }
    log_trace("EventLoop {} stop looping", static_cast<void *>(this));
    _looping = false;
}

void EventLoop::quit() {
    _quit = true;
    if (!isInLoopThread()) {
        /*
            如果不在当前线程, 想要让loop退出,先要唤醒才能退出
            否则就好比打游戏睡着的玩家,只有他醒来,你让他下线才能下线,否则他一直挂机
        */
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _pendingFunctors.emplace_back(std::move(cb));
    }
    if (!isInLoopThread() || _callingPendingFunctors) {
        wakeup();
    }
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = ::write(_wakeupFd, &one, sizeof one);
    if (n != sizeof one) {
        log_error("EventLoop::wakeup() writes {} bytes instead of 8", n);
    }
}

void EventLoop::updateChannel(Channel *channel) {
    _poller->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    _poller->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) {
    return _poller->hasChannel(channel);
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = ::read(_wakeupFd, &one, sizeof one);
    if (n != sizeof one) {
        log_error("EventLoop::handleRead() reads {} bytes instead of 8", n);
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    _callingPendingFunctors = true;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        functors.swap(_pendingFunctors);
        /*
            这里直接将待处理的任务移动到局部变量,常量速度
            然后释放锁,执行局部变量中的任务,这样就不会阻塞其他线程往队列中添加任务了
        */
    }

    for (Functor const &functor: functors) {
        functor();
    }
    _callingPendingFunctors = false;
}
