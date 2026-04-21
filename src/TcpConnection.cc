#include "Callbacks.h"
#include "Timestamp.h"
#include <asm-generic/socket.h>
#include <atomic>
#include <cerrno>
#include <Channel.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <EventLoop.h>
#include <InetAddress.h>
#include <Logger.h>
#include <Socket.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <TcpConnection.h>
#include <unistd.h>

namespace {

EventLoop *CHECK_NOTNULL(EventLoop *loop) {
    if (!loop) {
        log_fatal("mainLoop is nullptr");
    }
    return loop;
}

int getSocketError(int socket) {
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);
    if (::getsockopt(socket, SOL_SOCKET, SO_ERROR, &optval, &optlen)) {
        return errno;
    } else {
        return optval;
    }
}

} // namespace

TcpConnection::TcpConnection(EventLoop *loop, std::string const &name,
    int sockfd, InetAddress const &localAddr, InetAddress const &peerAddr)
    : _loop(CHECK_NOTNULL(loop))
    , _name(name)
    , _state(State::Connecting)
    , _reading(false)
    , _socket(std::make_unique<Socket>(sockfd))
    , _channel(std::make_unique<Channel>(loop, sockfd))
    , _localAddr(localAddr)
    , _peerAddr(peerAddr)
    , _pausedByHighWaterMark(false)
    , _highWaterMark(64 * 1024 * 1024) {
    _channel->setReadCallback(
        [this](Timestamp receiveTime) { this->handleRead(receiveTime); });
    _channel->setCloseCallback([this]() { this->handleClose(); });
    _channel->setErrorCallback([this]() { this->handleError(); });
    _channel->setWriteCallback([this]() { this->handleWrite(); });
    _socket->setKeepAlive(true);

    log_debug("TcpConnection::constructor[{}] at fd={}",
        static_cast<void *>(this), sockfd);
}

TcpConnection::~TcpConnection() {
    log_debug("TcpConnection:destructor[{}] at fd={}",
        static_cast<void *>(this), _channel->fd());
    assert(_state == State::Disconnected);
}

void TcpConnection::send(std::string const &msg) {
    if (_state == State::Connected) {
        if (_loop->isInLoopThread()) {
            sendInLoop(msg.c_str(), msg.size());
            // _outputBuffer.retrieveAll();
        } else {
            _loop->queueInLoop([this, msg = std::move(msg)]() mutable {
                this->sendInLoop(msg.data(), msg.size());
            });
        }
    }
}

void TcpConnection::sendInLoop(void const *data, size_t len) {
    if (_state != State::Connected) {
        log_error("disconnected,give up writing");
        return;
    }

    /*
        这个nwrote一定是ssize_t有符号.否则write即使返回-1,赋给size_t也会转成正数
        导致nwrote >= 0 恒为true
    */
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (!_channel->isWriting() && _outputBuffer.readableBytes() == 0) {
        nwrote = ::write(_channel->fd(), data, len);
        if (nwrote >= 0) /* ok */ {
            remaining = len - nwrote;
            if (remaining == 0 && _writeCompleteCallback) {
                _loop->queueInLoop([this, self = shared_from_this()]() {
                    this->_writeCompleteCallback(self);
                });
            }
        } else /* error */ {
            nwrote = 0;
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                log_error("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }
    assert(remaining <= len);
    if (!faultError && remaining > 0) /* write成功,但是没发送完毕  */ {
        /*
            1.  如果待需要发送的数据 >= _highWaterMark,也就是数据太大,比较紧张
            2.  这时候,就把剩余数据添加进入Buffer ,同时监听EPOLLOUT
                每当写缓冲区ok的时候,就会触发Channel::_writeCallback,也就是this->handleWrite.handleWrite会接着从Buffer内读取数据写入缓冲区发送
                所以我们需要写给对端,就将内容写入Buffer,EPOLLOUT写回调会自动将数据写出
        */

        size_t old_len = _outputBuffer.readableBytes();
        /*
            这里的&& old_len
           < _highWaterMark如果不满足,说明上次已经高水位了,已经调用
        */
        if (old_len + remaining >= _highWaterMark && old_len < _highWaterMark
            && _highWaterMarkCallback) {
            _loop->queueInLoop(
                [this, self = shared_from_this(), old_len, remaining]() {
                    this->_highWaterMarkCallback(self, old_len + remaining);
                });
        }
        _outputBuffer.append(
            static_cast<char const *>(data) + nwrote, remaining);
        if (!_channel->isWriting()) {
            _channel->enableWriting();
        }
    }

    /*
        反压链条
            服务端 outputBuffer_ 堆积
                    │
                    ▼
            说明客户端接收慢（或者网络慢）
                    │
                    ▼
            服务端应该 stopRead，不让客户端发更多数据
                    │
                    ▼
            客户端被反压，它的 outputBuffer_ 也会堆积
                    │
                    ▼
            客户端的业务层发现发不动了，自然就慢下来了
                    │
                    ▼
            服务端趁机把 outputBuffer_ 发完
    */
}

void TcpConnection::shutdown() {
    auto expeceted = State::Connected;
    if (_state.compare_exchange_strong(expeceted, State::Disconnecting)) {
        _loop->runInLoop(
            [this, self = shared_from_this()]() { this->shutdownInLoop(); });
    }
}

void TcpConnection::shutdownInLoop() {
    if (!_channel->isWriting()) {
        _socket->shutdownWrite();
    }
}

void TcpConnection::forceClose() {
    State old = _state.exchange(State::Disconnecting);
    if (old == State::Connected || old == State::Connecting) {
        _loop->queueInLoop(
            [self = shared_from_this()]() { self->forceCloseInLoop(); });
    }
}

void TcpConnection::forceCloseInLoop() {
    if (_state.load(std::memory_order_acquire) == State::Connected
        || _state.load(std::memory_order_acquire) == State::Disconnecting) {
        handleClose();
    }
}

void TcpConnection::stopRead() {
    if (_channel->isReading()) {
        _channel->disableReading();
        _pausedByHighWaterMark = true;
    }
}

void TcpConnection::startRead() {
    if (!_channel->isReading()) {
        _channel->enableReading();
        _pausedByHighWaterMark = false;
    }
}

void TcpConnection::connectEstablished() {
    assert(_state.load(std::memory_order_acquire) == State::Connecting);
    _state = State::Connected;
    _channel->tie(shared_from_this());
    _channel->enableReading();
    _connectionCallback(shared_from_this());
}

void TcpConnection::connectDestroyed() {
    if (_state.load(std::memory_order_acquire) == State::Connected) {
        _state = State::Disconnected;
        _channel->disableAll();
        _connectionCallback(shared_from_this());
    }
    _channel->remove();
}

void TcpConnection::handleRead(Timestamp receiveTime) {
    int saveErrno = 0;
    ssize_t n = _inputBuffer.readFd(_socket->fd(), &saveErrno);
    if (n > 0) {
        _messageCallback(shared_from_this(), &_inputBuffer, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        errno = saveErrno;
        log_error("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite() {
    if (_channel->isWriting()) {
        int saveErrno = 0;
        ssize_t n = _outputBuffer.writeFd(_channel->fd(), &saveErrno);
        if (n > 0) {
            _outputBuffer.retrieve(n);
            if (_outputBuffer.readableBytes() == 0) {
                _channel->disableWriting();
                if (_writeCompleteCallback) {
                    /*
                        这里使用queueInLoop因为写回调事件不是紧急事件
                        放在队尾,不必立刻执行
                     */
                    _loop->queueInLoop([this, self = shared_from_this()]() {
                        this->_writeCompleteCallback(self);
                    });
                    if (_pausedByHighWaterMark) {
                        startRead();
                    }
                }
                if (_state.load(std::memory_order_acquire)
                    == State::Disconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            log_error("TcpConnection::handleWrite");
        }
    } else {
        log_error("Connecting fd = {} is down,no more writing", _channel->fd());
    }
}

void TcpConnection::handleClose() {
    log_trace("fd = {} state = {}", _channel->fd(),
        static_cast<int>(_state.load(std::memory_order_acquire)));
    assert(_state.load(std::memory_order_acquire) == State::Connected
           || _state.load(std::memory_order_acquire) == State::Disconnecting);
    _state = State::Disconnected;
    _channel->disableAll();
    TcpConnectionPtr guardThis(shared_from_this());
    _connectionCallback(guardThis);
    _closeCallback(guardThis);
    // clang-format off
    /* 
        1. 为什么没有调用_channel->remove()
        调用链是这样的
        handleClose()
                │
                ├─ setState(kDisconnected)
                ├─ _channel->disableAll()
                │
                ├─ _connectionCallback(guardThis)  ──► 用户回调：conn->connected() == false
                │
                └─ _closeCallback(guardThis)       ──►   TcpServer::removeConnection
                                                            │
                                                            ▼
                                                    从 _connections 移除

        主要是为了线程安全.TcpServer::_connections 这个 map 由主线程管理，必须由主线程来删除。_connections 的所有操作（增、删、查）都必须在主线程执行。
        2. 为什么需要这个guardThis
            _closeCallback最终会调用到TcpServer的removeConnection,然后内部会_connections.erase(conn->name()).
            如果没有为什么需要这个guardThis,TcpServer内部erase之后,计数为0,
            这时候这个TcpConnection的this对象被销毁,回调返回后，this 对象已经被销毁
    */
    // clang-format on
}

void TcpConnection::handleError() {
    int err = getSocketError(_channel->fd());
    log_error("TcpConnection::handleRrror [{}] - SO_ERROR = {}", _name, err,
        std::strerror(err));
}
