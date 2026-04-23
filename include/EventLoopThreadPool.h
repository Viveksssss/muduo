#pragma once

#include "EventLoopThread.h"
#include <functional>
#include <noncopyable.h>
#include <string>

class EventLoop;

class EventLoopThreadPool : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    EventLoopThreadPool(EventLoop *baseLoop, std::string const &name);
    ~EventLoopThreadPool();

    void start(ThreadInitCallback const &cb = ThreadInitCallback());

    EventLoop *getNextLoop();

    std::vector<EventLoop *> getAllLoops();

    __attribute__((always_inline)) void setThreadNum(int numThreads) {
        _numThreads = numThreads;
    }

    __attribute__((always_inline)) bool started() const {
        return _started;
    }

    __attribute__((always_inline)) std::string const &name() const {
        return _name;
    }

private:
    /* 如果不设置线程数,默认只有一个线程即_baseLoop */
    EventLoop *_baseLoop;
    std::string _name;
    bool _started;
    int _numThreads;
    int _next;
    std::vector<std::unique_ptr<EventLoopThread>> _threads;
    std::vector<EventLoop *> _loops;
};
