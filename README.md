# muduo

Muduo 是一个基于 Reactor 模式的高性能 C++ 网络库，采用非阻塞 I/O 和事件驱动架构，适用于开发高并发的网络应用程序。

## 特性

- ✅ Reactor 模式：基于事件驱动的网络编程模型

- ✅ 非阻塞 I/O：使用 epoll/poll/select 实现高并发

- ✅ 多线程支持：EventLoopThreadPool 实现多线程事件循环

- ✅ 线程安全：精心设计的线程模型，避免锁竞争

- ✅ 现代 C++：使用 C++20 标准，RAII 资源管理

## 性能测试(redis-benchmark)

| 场景 | 并发连接 | Pipeline | QPS | 平均延迟 | P99延迟 |
|------|----------|----------|-----|----------|---------|
| 单连接一问一答 | 1 | 1 | 11.1万 | 5 μs | 15 μs |
| 多连接一问一答 | 15 | 1 | 21.6万 | 39 μs | 71 μs |
| 批量处理 | 16 | 15 | 244万 | 55 μs | 103 μs |


## 快速开始

```bash

git clone https://github.com/Viveksssss/muduo.git
cd muduo

./auto_build.sh
```

## 使用实例:Echo服务器
```cpp
#include <cctype>
#include <functional>
#include <muduo/Logger.h>
#include <muduo/TcpServer.h>
#include <Timestamp.h>

class EchoServer {
public:
    EchoServer(
        EventLoop *loop, InetAddress const &addr, std::string const &name)
        : _loop(loop)
        , _server(loop, addr, name) {
        _server.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        _server.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
        _server.setThreadNum(15);
    }

    void start() {
        _server.start();
    }

private:
    void onMessage(TcpConnectionPtr const &conn, Buffer *buf, Timestamp time) {
        std::string msg = buf->retrieveAllAsString();
        // log_debug("{}", msg);
        // for (auto &c: msg) {
        //     c = ::toupper(c);
        // }
        conn->send(msg);
    }

    void onConnection(TcpConnectionPtr const &conn) {
        if (conn->connected()) {
            log_info("new connection");
        } else if (conn->disconnected()) {
            log_info("connection closed");
        }
    }

private:
    EventLoop *_loop;
    TcpServer _server;
};

/*
	默认不开启日志,如若开启,可以定义 #define LOG_ENABLED true
	然后set_log_level调整登等级
*/
int main(int, char **) {
    EventLoop loop;
    InetAddress addr(8000);
    EchoServer server(&loop, addr, "echo_server");

    server.start();

    loop.loop();
    return 0;
}

```