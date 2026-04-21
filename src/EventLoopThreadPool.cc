#include "EventLoop.h"
#include "EventLoopThread.h"
#include <cassert>
#include <EventLoopThreadPool.h>
#include <memory>

EventLoopThreadPool::EventLoopThreadPool(
    EventLoop *baseLoop, std::string const &name)
    : _baseLoop(baseLoop)
    , _name(name)
    , _started(false)
    , _numThreads(0)
    , _next(0)
    , _threads()
    , _loops()

{
    _threads.reserve(16);
    _loops.reserve(16);
}

EventLoopThreadPool::~EventLoopThreadPool() { }

void EventLoopThreadPool::start(ThreadInitCallback const &cb) {
    assert(!_started);
    _started = true;
    for (auto i = 0; i < _numThreads; ++i) {
        std::vector<char> buf;
        buf.reserve(_name.size() + 32);
        snprintf(buf.data(), buf.capacity(), "%s%d", _name.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, std::string(buf.data()));
        _threads.push_back(std::unique_ptr<EventLoopThread>(t));
        _loops.push_back(t->startLoop());
    }
    if (_numThreads == 0 && cb) {
        cb(_baseLoop);
    }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
    assert(_started);
    EventLoop *loop = _baseLoop;
    if (!_loops.empty()) [[likely]] {
        loop = _loops[_next];
        /*
            _next = (_next + 1) % _numThreads;
            // 除法是 CPU最慢的基本运算之一, 20-30周期
        */
        ++_next;
        if (static_cast<size_t>(_next) >= _numThreads)
            [[unlikely]] { /* 基本零开销,预测失败10-20周期 */
            _next = 0;
        }
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() {
    assert(_started);
    if (_loops.empty()) {
        return std::vector<EventLoop *>(1, _baseLoop);
    }
    return _loops;
}
