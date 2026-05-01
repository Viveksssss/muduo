#pragma once

#include <atomic>
#include <Buffer.h>
#include <Callbacks.h>
#include <InetAddress.h>
#include <memory>
#include <noncopyable.h>
#include <string>
#include <sys/types.h>
#include <Timestamp.h>

class EventLoop;
class Channel;
class Socket;

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop *loop, std::string const &name, int sockfd,
        InetAddress const &localAddr, InetAddress const &peerAddr);

    ~TcpConnection();

    void send(std::string const &msg);
    void send(Buffer *buf);

    void shutdown();

    void forceClose();

    void stopRead();

    void startRead();

    void connectEstablished();

    void connectDestroyed();

    __attribute__((always_inline)) std::string const &name() {
        return _name;
    }

    __attribute__((always_inline)) EventLoop *getLoop() {
        return _loop;
    }

    __attribute__((always_inline)) const InetAddress &localAddress() const {
        return _localAddr;
    }

    __attribute__((always_inline)) const InetAddress &peerAddress() const {
        return _peerAddr;
    }

    __attribute__((always_inline)) bool connected() const {
        return _state.load(std::memory_order_acquire) == State::Connected;
    }

    __attribute__((always_inline)) bool disconnected() const {
        return _state.load(std::memory_order_acquire) == State::Disconnected;
    }

    __attribute__((always_inline)) void setConnectionCallback(ConnectionCallback const &cb) {
        _connectionCallback = cb;
    }

    __attribute__((always_inline)) void setMessageCallback(MessageCallback const &cb) {
        _messageCallback = cb;
    }

    __attribute__((always_inline)) void setCloseCallback(CloseCallback const &cb) {
        _closeCallback = cb;
    }

    __attribute__((always_inline)) void setWriteCompleteCallback(WriteCompleteCallback const &cb) {
        _writeCompleteCallback = cb;
    }

    __attribute__((always_inline)) void setHighWaterMarkCallback(
        HighWaterMarkCallback const &cb, size_t mark) {
        _highWaterMarkCallback = cb;
        _highWaterMark = mark;
    }

    __attribute__((always_inline)) void setContext(std::shared_ptr<void> const &context) {
        _context = context;
    }

    __attribute__((always_inline)) std::shared_ptr<void> const &getContext() const {
        return _context;
    }

private:
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(void const *, size_t);
    void shutdownInLoop();
    void forceCloseInLoop();

private:
    enum class State : uint8_t {
        Disconnected,
        Connecting,
        Connected,
        Disconnecting
    };
    EventLoop *_loop;
    std::string const _name;
    std::atomic<State> _state;
    bool _reading;
    std::unique_ptr<Socket> _socket;
    std::unique_ptr<Channel> _channel;
    InetAddress const _localAddr;
    InetAddress const _peerAddr;
    bool _pausedByHighWaterMark;

    ConnectionCallback _connectionCallback;
    MessageCallback _messageCallback;
    WriteCompleteCallback _writeCompleteCallback;
    HighWaterMarkCallback _highWaterMarkCallback;
    CloseCallback _closeCallback;
    size_t _highWaterMark;

    Buffer _inputBuffer;
    Buffer _outputBuffer;

    std::shared_ptr<void> _context;
};
