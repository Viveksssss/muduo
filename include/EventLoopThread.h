#pragma once
#include "Thread.h"
#include <condition_variable>
#include <functional>
#include <noncopyable.h>

class EventLoop;

class EventLoopThread : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    EventLoopThread(ThreadInitCallback const &cb = ThreadInitCallback(),
        std::string const &name = std::string());
    ~EventLoopThread();
    EventLoop *startLoop();

private:
    void threadFunc();

private:
    EventLoop *_loop;
    bool _exit;
    Thread _thread;
    std::mutex _mutex;
    std::condition_variable _cv;
    ThreadInitCallback _callback;
};
