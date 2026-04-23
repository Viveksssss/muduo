#pragma once

#include <atomic>
#include <functional>
#include <latch>
#include <memory>
#include <noncopyable.h>
#include <thread>
#include <unistd.h>

class Thread : noncopyable {
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func, std::string const &name = std::string());
    ~Thread();

    void start();
    void join();

    __attribute__((always_inline)) bool started() const {
        return _started;
    }

    __attribute__((always_inline)) pid_t tid() const noexcept {
        return _tid;
    }

    __attribute__((always_inline)) std::string const &name() const {
        return _name;
    }

    __attribute__((always_inline)) static uint32_t numCreated() {
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
