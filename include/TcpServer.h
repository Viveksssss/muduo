#pragma once

#include <Acceptor.h>
#include <Callbacks.h>
#include <Channel.h>
#include <EventLoop.h>
#include <EventLoopThreadPool.h>
#include <InetAddress.h>
#include <memory>
#include <noncapyable.h>
#include <string>
#include <TcpConnection.h>
#include <unordered_map>

/**
 * @brief 面向用户使用的接口
 *
 */

class TcpServer : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    enum class Option {
        NoUsePort,
        ReusePort
    };

    TcpServer(EventLoop *loop, InetAddress const &listenAddr,
        std::string const &name, Option option = Option::NoUsePort);
    ~TcpServer();

    void start();

    __attribute__((always_inline)) void setThreadNum(int numThreads) {
        _threadPool->setThreadNum(numThreads);
    }

    __attribute__((always_inline)) void setThreadInitCallback(
        ThreadInitCallback cb) {
        _threadInitCallback = cb;
    }

    __attribute__((always_inline)) void setConnectionCallback(
        ConnectionCallback const &cb) {
        _connectionCallback = cb;
    }

    __attribute__((always_inline)) void setMessageCallback(
        MessageCallback const &cb) {
        _messageCallback = cb;
    }

    __attribute__((always_inline)) void setWriteCompleteCallback(
        WriteCompleteCallback const &cb) {
        _writeCompleteCallback = cb;
    }

private:
    void newConnection(int sockfd, InetAddress const &peerAddr);
    void removeConnection(TcpConnectionPtr const &conn);
    void removeConnectionInLoop(TcpConnectionPtr const &conn);

private:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
    EventLoop *_loop;                                 /* baseLoop */
    std::string const _ipPort;                        /* ipport127.0.0.1:9999 */
    std::string const _name;                          /* 服务器实例名称 */
    InetAddress const _listenAddr;                    /* 监听地址 */
    std::unique_ptr<Acceptor> _acceptor;              /* 接受新的连接 */
    std::shared_ptr<EventLoopThreadPool> _threadPool; /* loop池 */
    ConnectionCallback _connectionCallback;           /* 连接/断开连接回调 */
    MessageCallback _messageCallback;                 /* 消息到达回调 */
    WriteCompleteCallback _writeCompleteCallback;     /* 写完成回调 */
    ThreadInitCallback _threadInitCallback;           /* 工作线程初始化回调 */
    std::atomic<int> _started;                        /* 服务器启动状态 */
    int _nextConnId;                                  /* 下一个连接的ID计数器 */
    ConnectionMap _connections;                       /* 管理name和connection */
};
