#pragma once

#include "Buffer.h"
#include "InetAddress.h"
#include <atomic>
#include <Callbacks.h>
#include <memory>
#include <noncapyable.h>
#include <string>
#include <sys/types.h>

class EventLoop;
class Channel;
class Socket;

class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop *loop, std::string const &name, int sockfd,
        InetAddress const &localAddr, InetAddress const &peerAddr);

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
        return _state == State::Connected;
    }

    __attribute__((always_inline)) bool disconnected() const {
        return _state == State::Disconnected;
    }

    void send(void const *message, int len);

    void shutdown();

    void forceClose();

    void setConnectionCallback(ConnectionCallback const &cb) {
        _connectionCallback = cb;
    }

    void setMessageCallback(MessageCallback const &cb) {
        _messageCallback = cb;
    }

    void setCloseCallback(CloseCallback const &cb) {
        _closeCallback = cb;
    }

    void setWriteCompleteCallback(WriteCompleteCallback const &cb) {
        _writeCompleteCallback = cb;
    }

    void setHighWaterMarkCallback(HighWaterMarkCallback const &cb) {
        _highWaterMarkCallback = cb;
    }

    void connectDestroyed();

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

    ConnectionCallback _connectionCallback;
    MessageCallback _messageCallback;
    WriteCompleteCallback _writeCompleteCallback;
    HighWaterMarkCallback _highWaterMarkCallback;
    CloseCallback _closeCallback;
    size_t _highWaterMark;

    Buffer _inputBuffer;
    Buffer _outputBUffer;
};
