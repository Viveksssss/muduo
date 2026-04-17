#pragma once

#include <atomic>
#include <functional>
#include <latch>
#include <memory>
#include <noncapyable.h>
#include <thread>
#include <unistd.h>

class Thread : noncopyable {
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func, std::string const &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const {
        return _started;
    }

    pid_t tid() const {
        return _tid;
    }

    std::string const &name() const {
        return _name;
    }

    static uint32_t numCreated() {
        return _numCreated.load();
    }

private:
    void setDefaultName();

private:
    bool _started;
    bool _joined;
    pid_t _tid;
    std::unique_ptr<std::thread> _thread;
    ThreadFunc _func;
    std::string _name;
    std::latch _latch;
    static std::atomic<uint32_t> _numCreated;
};
