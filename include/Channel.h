#pragma once

/**
 * @brief Channel通道,封装了socketfd和事件类型,以及事件发生后的回调操作
 *
 */
#include "noncapyable.h"
#include <functional>
#include <memory>
#include <Timestamp.h>

class EventLoop;

class Channel : noncopyable {
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    /* 得到poller通知,处理事件 */
    void handleEvent(Timestamp receiveTime);

    /* 设置回调 */
    __always_inline void setReadCallback(ReadEventCallback cb) {
        _readCallback = std::move(cb);
    }

    __always_inline void setWriteCallback(EventCallback cb) {
        _writeCallback = std::move(cb);
    }

    __always_inline void setCloseCallback(EventCallback cb) {
        _closeCallback = std::move(cb);
    }

    __always_inline void setErrorCallback(EventCallback cb) {
        _errorCallback = std::move(cb);
    }

    __always_inline void tie(std::shared_ptr<void> const &tie) {
        _tie = tie;
    }

    __always_inline int fd() const noexcept {
        return _fd;
    }

    __always_inline int events() const noexcept {
        return _events;
    }

    __always_inline void set_revents(int revt) {
        _revents = revt;
    }

    __always_inline bool isNoneEvent() const noexcept {
        return _events == _noneEvent;
    }

    /* 设置相应的事件 */
    void enableReading() {
        _events |= _readEvent;
        update();
    }

    void enableWriting() {
        _events |= _writeEvent;
        update();
    }

    void disableWriting() {
        _events &= ~_writeEvent;
        update();
    }

    void disableAll() {
        _events = _noneEvent;
        update();
    }

    __always_inline bool isWriting() const noexcept {
        return (_events & _writeEvent) != 0;
    }

    __always_inline bool isReading() const noexcept {
        return (_events & _readEvent) != 0;
    }

    __always_inline int index() const noexcept {
        return _index;
    }

    __always_inline void set_index(int idx) {
        _index = idx;
    }

    __always_inline EventLoop *ownerLoop() const noexcept {
        return _loop;
    }

    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

private:
    static int const _noneEvent;
    static int const _readEvent;
    static int const _writeEvent;

    EventLoop *_loop;
    int const _fd;
    int _events;  /* 注册fd感兴趣的事件 */
    int _revents; /* poller返回的具体发生的事件 */
    int _index;
    /*
        用于管理 Channel 对象的状态。它通常有以下几种取值：
        int const _new = -1;
        int const _added = 1;
        int const _deleted = 2;
    */

    std::weak_ptr<void> _tie; /* 用于防止对象提前销毁导致的程序崩溃。它通常指向
                                 TcpConnection 对象 */
    bool _tied;               /* 标志是否已经绑定了对象 */

    /* Channel通道里面可以获知fd最终发生的具体的事件revents,所以他负责调用具体事件的回调
     */
    ReadEventCallback _readCallback;
    EventCallback _writeCallback;
    EventCallback _closeCallback;
    EventCallback _errorCallback;
};
