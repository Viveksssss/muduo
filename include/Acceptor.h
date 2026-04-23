#pragma once

#include "Channel.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "Socket.h"

// clang-format off
/*
    1. 结构图
    ┌─────────────┐     持有      ┌─────────────┐
    │  Acceptor   │─────────────>│    Socket   │ (负责 bind/listen/accept)
    └─────────────┘               └─────────────┘
        │ 持有                         ↑
        │                              │ 使用
        ▼                              │
    ┌─────────────┐     持有      ┌─────────────┐
    │   Channel   │─────────────>│    fd       │ (同一个监听 socket)
    └─────────────┘               └─────────────┘
        │ 注册到
        ▼
    ┌─────────────┐
    │  EventLoop  │ (epoll 驱动)
    └─────────────┘

    InetAddress 作为参数在 bind 和 accept 时传递地址信息。

    2. Acceptor构造阶段
    Acceptor(loop, listenAddr, reuseport)
        │
        ├─> createNonblocking() 创建非阻塞 socket
        │
        ├─> _acceptSocket(该 fd)          // Socket 对象接管 fd
        │
        ├─> _acceptChannel(loop, fd)      // Channel 对象绑定同一个 fd 和 loop
        │
        ├─> _acceptSocket.setReuseAddr(true)
        ├─> _acceptSocket.setReusePort(true)
        ├─> _acceptSocket.bindAddress(listenAddr)
        │
        └─> _acceptChannel.setReadCallback( std::bind(&Acceptor::handleRead, this) )

        - 监听 socket 的 fd 被 两个对象同时持有：Socket 负责实际的系统调用，Channel 负责事件注册与回调。

    3. 启动监听 —— listen()

        void Acceptor::listen() {
            _listening = true;
            _acceptSocket.listen();               // 调用 ::listen
            _acceptChannel.enableReading();       // 将 fd 的 EPOLLIN 事件注册到 EventLoop
        }

        此时，EventLoop 中的 epoll 开始监控该 fd 的可读事件（即新连接到来）。

    4.  事件触发与接受连接
        epoll_wait 返回 → EventLoop 找到对应的 Channel
            │
            └─> Channel::handleEvent() → handleEventWithGuard()
                    │
                    └─> _readCallback(receiveTime)   // 即 Acceptor::handleRead()

        Acceptor::handleRead() 内部：

            InetAddress peerAddr;
            int connfd = _acceptSocket.accept(&peerAddr);   // 接受连接，获得客户端地址
            if (connfd >= 0) {
                _newConnectionCallback(connfd, peerAddr);    // 回调给 TcpServer 处理新连接
            } else {
                ::close(connfd);
            }

    > Acceptor 用 Socket 操作监听 fd，用 Channel 把 fd 的“可读事件”交给 EventLoop；当事件触发时，通过 Socket::accept 拿到连接 fd 和地址，再通过回调通知上层。
*/
// clang-formatter on

class EventLoop;
class InetAddress;

class Acceptor : noncopyable {
public:
    using NewFunctionCallback = std::function<void(int, InetAddress const &)>;

    explicit Acceptor(EventLoop *loop, InetAddress const &addr, bool reuseport = true);

    ~Acceptor();

    void setNewConnectionCallback(NewFunctionCallback cb) {
        _newConnectionCallback = cb;
    }

    bool listening() const;
    void listen();

private:
    void handleRead();

private:
    EventLoop *_loop;
    Socket _acceptSocket;
    Channel _acceptChannel;
    NewFunctionCallback _newConnectionCallback;
    bool _listening;
};
