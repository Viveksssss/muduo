# muduo

Muduo 是一个基于 Reactor 模式的高性能 C++ 网络库，采用非阻塞 I/O 和事件驱动架构，适用于开发高并发的网络应用程序。

## 特性

- ✅ Reactor 模式：基于事件驱动的网络编程模型

- ✅ 非阻塞 I/O：使用 epoll/poll/select 实现高并发

- ✅ 多线程支持：EventLoopThreadPool 实现多线程事件循环

- ✅ 线程安全：精心设计的线程模型，避免锁竞争

- ✅ 现代 C++：使用 C++20 标准，RAII 资源管理

- ✅ 跨平台：支持 Linux、macOS 等 Unix-like 系统

## 快速开始

```bash

git clone https://github.com/Viveksssss/muduo.git
cd muduo

./auto_build.sh
```

## 使用实例:Echo服务器
```cpp
#include <muduo/TcpServer.h>
#include <muduo/EventLoop.h>
#include <muduo/InetAddress.h>
#include <muduo/TcpConnection.h>

using namespace muduo;

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
{
    std::string msg = buf->retrieveAllAsString();
    conn->send(msg);  // echo back
}

int main()
{
    EventLoop loop;
    InetAddress listenAddr(8888);
    TcpServer server(&loop, listenAddr, "EchoServer");
    
    server.setMessageCallback(onMessage);
    server.start();
    
    loop.loop();
    
    return 0;
}
```