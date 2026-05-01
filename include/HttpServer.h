#pragma once

#include "Callbacks.h"
#include "EventLoop.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "TcpServer.h"
#include <functional>

class HttpServer {
public:
    using HttpCallback
        = std::function<bool(TcpConnectionPtr const &, HttpRequest &, HttpResponse *)>;
    HttpServer(EventLoop *loop, InetAddress const &listenAddr, std::string const &name);
    ~HttpServer();

    __attribute__((always_inline)) void setHttpCallback(HttpCallback const &cb) {
        _httpCallback = cb;
    }

    __attribute__((always_inline)) void setConnectionCallback(ConnectionCallback const &cb) {
        _server.setConnectionCallback(cb);
    }

    __attribute__((always_inline)) void setThreadNum(int numThreads) {
        _server.setThreadNum(numThreads);
    }

    __attribute__((always_inline)) void start() {
        _server.start();
    }

private:
    void OnConnection(TcpConnectionPtr const &conn);
    void OnMessage(TcpConnectionPtr const &conn, Buffer *buf, Timestamp receiveTime);
    bool OnRequest(TcpConnectionPtr const &conn, HttpRequest &request);

private:
    TcpServer _server;
    HttpCallback _httpCallback;
};
