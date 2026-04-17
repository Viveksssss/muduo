#include "CurrentThread.h"
#include <cassert>
#include <Thread.h>

std::atomic<uint32_t> Thread::_numCreated = 0;

Thread::Thread(ThreadFunc func, std::string const &name)
    : _started(false)
    , _joined(false)
    , _tid(0)
    , _thread()
    , _func(std::move(func))
    , _name(name)
    , _latch(1) {
    setDefaultName();
}

Thread::~Thread() {
    if (_started && !_joined) {
        _thread->detach();
    }
}

void Thread::start() {
    _thread = std::make_unique<std::thread>([this] {
        _tid = CurrentThread::tid();
        _latch.count_down();
        _func();
    });
    /* 这里必须等待获取_tid */
    _latch.wait();
}

void Thread::join() {
    assert(_started);
    assert(!_joined);
    _joined = true;
    _thread->join();
}

void Thread::setDefaultName() {
    int num = _numCreated.fetch_add(1, std::memory_order_relaxed);
    if (_name.empty()) {
        char buf[32];
        std::snprintf(buf, sizeof buf, "Thread%d", num);
        _name = buf;
    }
}
