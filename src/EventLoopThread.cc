#include <cassert>
#include <EventLoop.h>
#include <EventLoopThread.h>

EventLoopThread::EventLoopThread(
    ThreadInitCallback const &cb, std::string const &name)
    : _loop(nullptr)
    , _exit(false)
    , _thread(std::bind(&EventLoopThread::threadFunc, this), name)
    , _mutex()
    , _cv()
    , _callback(cb) { }

EventLoopThread::~EventLoopThread() {
    _exit = true;
    if (_loop) {
        _loop->quit();
        _thread.join();
    }
}

EventLoop *EventLoopThread::startLoop() {
    assert(!_thread.started());
    _thread.start();
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (_loop == nullptr) {
            _cv.wait(lock);
        }
    }
    return _loop;
}

/*
    真正的线程函数,函数内部运行了一个EventLoop,即one loop per thread
        线程内部:
            首先执行ThreadInitCallback进行初始化
            然后通知外部(也就是startLoop函数),线程已经初始化完毕,EventLoop也创建好了
            最后调用EventLoop的loop函数,进入事件循环(在thread内部一直运行)
*/
void EventLoopThread::threadFunc() {
    EventLoop loop;
    if (_callback) {
        _callback(&loop);
    }
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _loop = &loop;
        _cv.notify_one();
    }
    loop.loop();
    // assert(_exit);
    std::lock_guard<std::mutex> lock(_mutex);
    _loop = nullptr;
}
