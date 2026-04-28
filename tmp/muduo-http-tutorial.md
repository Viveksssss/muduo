# Muduo HTTP 开发教程

## 教程概述

本教程将指导你逐步为 Muduo 网络库实现 HTTP 功能，从最基础的 HTTP 协议解析到完整的 HTTP 服务器。

## 目录

1. [第一阶段：HTTP 基础解析](#第一阶段http-基础解析)
2. [第二阶段：HTTP 服务器基础](#第二阶段http-服务器基础)
3. [第三阶段：路由和功能增强](#第三阶段路由和功能增强)
4. [第四阶段：高级特性](#第四阶段高级特性)
5. [实施建议](#实施建议)

---

## 第一阶段：HTTP 基础解析（核心组件）

### 任务 1：实现 HTTP 请求解析

**文件：** `HttpRequest.h` 和 `HttpRequest.cc`

**功能需求：**
```cpp
// HttpRequest.h
class HttpRequest {
public:
    // 构造函数
    HttpRequest();
    
    // 重置请求
    void reset();
    
    // 解析请求（返回0表示成功，-1表示需要更多数据，-2表示解析错误）
    int parse(Buffer& buf);
    
    // 解析请求行（例如："GET /path HTTP/1.1"）
    bool parseRequestLine(const char* begin, const char* end);
    
    // 解析头部（例如："Host: example.com"）
    bool parseHeaders(const char* begin, const char* end);
    
    // 获取方法（GET/POST/PUT/DELETE等）
    const string& method() const { return method_; }
    
    // 获取URI
    const string& uri() const { return uri_; }
    
    // 获取版本
    const string& version() const { return version_; }
    
    // 获取头部值
    const string& getHeader(const string& field) const;
    
    // 获取查询参数
    string getQueryParameter(const string& key) const;
    
    // 获取请求体
    const string& body() const { return body_; }
    
private:
    string method_;      // HTTP方法
    string uri_;        // URI
    string version_;    // HTTP版本
    map<string, string> headers_;  // HTTP头部
    string body_;       // 请求体
    bool parsed_;       // 是否已完成解析
};
```

**关键实现点：**
- 处理 `\r\n` 换行符
- 正确处理半包和粘包问题
- 解析查询参数（`?key=value&key2=value2`）
- 支持多种 HTTP 方法

### 任务 2：实现 HTTP 响应生成

**文件：** `HttpResponse.h` 和 `HttpResponse.cc`

**功能需求：**
```cpp
// HttpResponse.h
class HttpResponse {
public:
    HttpResponse();
    
    // 设置状态码和描述
    void setStatusCode(HttpStatusCode code);
    
    // 设置响应头
    void setHeader(const string& field, const string& value);
    
    // 设置响应体
    void setBody(const string& body);
    
    // 序列化响应到 Buffer
    void appendToBuffer(Buffer& output) const;
    
private:
    HttpStatusCode statusCode_;    // HTTP状态码
    string statusMessage_;        // 状态描述
    map<string, string> headers_; // 响应头
    string body_;                 // 响应体
};
```

**HTTP 状态码定义：**
```cpp
enum HttpStatusCode {
    kUnknown = 0,
    k200OK = 200,
    k301MovedPermanently = 301,
    k400BadRequest = 400,
    k404NotFound = 404,
    k500InternalServerError = 500
};
```

### 任务 3：HTTP 连接上下文

**文件：** `HttpContext.h` 和 `HttpContext.cc`

**功能需求：**
```cpp
// HttpContext.h
class HttpContext {
public:
    enum ParseState {
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll
    };
    
    HttpContext();
    
    // 重置状态
    void reset();
    
    // 解析HTTP请求
    ParseState parse(Buffer& buf);
    
    // 获取解析完成的HTTP请求
    HttpRequest* getRequest();
    
    // 设置请求完成回调
    void onRequestComplete(function<void(HttpRequest&, TcpConnection*)> callback);
    
private:
    ParseState state_;                              // 解析状态
    HttpRequest request_;                           // HTTP请求对象
    function<void(HttpRequest&, TcpConnection*)> callback_; // 完成回调
};
```

---

## 第二阶段：HTTP 服务器基础

### 任务 4：简单 HTTP 服务器

**文件：** `HttpServer.h` 和 `HttpServer.cc`

**功能需求：**
```cpp
// HttpServer.h
class HttpServer : public TcpServer {
public:
    HttpServer(EventLoop* loop, const InetAddress& listenAddr);
    
    // 设置HTTP请求回调
    void setHttpCallback(const HttpCallback& cb) { httpCallback_ = cb; }
    
private:
    // 新连接建立
    void onConnection(const TcpConnectionPtr& conn);
    
    // 消息到达
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime);
    
    // 请求处理完成
    void onRequestComplete(const TcpConnectionPtr& conn, const HttpRequest& req);
    
    HttpCallback httpCallback_;  // HTTP请求回调函数
};
```

**使用示例：**
```cpp
EventLoop loop;
InetAddress addr(8080);
HttpServer server(&loop, addr);

// 设置HTTP请求处理回调
server.setHttpCallback([](const HttpRequest& req, HttpResponse* resp) {
    if (req.method() == "GET" && req.uri() == "/") {
        resp->setStatusCode(HttpResponse::k200OK);
        resp->setHeader("Content-Type", "text/html");
        resp->setBody("<h1>Hello World!</h1>");
    }
    else {
        resp->setStatusCode(HttpResponse::k404NotFound);
        resp->setHeader("Content-Type", "text/html");
        resp->setBody("<h1>404 Not Found</h1>");
    }
});

server.start();
loop.loop();
```

---

## 第三阶段：路由和功能增强

### 任务 5：路由系统

**文件：** `RouteTrie.h` 和 `RouteTrie.cc`

**功能需求：**
```cpp
// RouteTrie.h
class RouteNode {
public:
    map<string, RouteNode*> children;  // 子节点
    RouteNode* wildcardChild;         // 通配符子节点
    function<void(HttpRequest&, HttpResponse*)> handler; // 处理函数
    string pathParam;                 // 路径参数名
};

class RouteTrie {
public:
    // 插入路由
    void insert(const string& path, const string& method, 
                function<void(HttpRequest&, HttpResponse*)> handler);
    
    // 查找路由
    pair<RouteNode*, map<string, string>> find(const string& path, const string& method);
    
private:
    RouteNode* root_;
};
```

**使用示例：**
```cpp
RouteTrie routeTrie;

// 注册路由
routeTrie.insert("/users/:id", "GET", [](const HttpRequest& req, HttpResponse* resp) {
    string userId = req.getHeader("user-id");
    resp->setBody("User ID: " + userId);
});

// 查找路由
auto [node, params] = routeTrie.find("/users/123", "GET");
if (node) {
    // 找到路由，执行处理函数
    node->handler(req, resp);
}
```

### 任务 6：多线程支持

**扩展 `HttpServer` 以支持线程池：**
- 使用 `EventLoopThreadPool`
- 为每个连接分配处理线程
- 支持请求并发处理

---

## 第四阶段：高级特性

### 任务 7：HTTP/1.1 支持

**Keep-Alive 连接：**
```cpp
// 在 HttpResponse 中支持
void setKeepAlive(bool on) {
    if (on) {
        setHeader("Connection", "keep-alive");
    } else {
        setHeader("Connection", "close");
    }
}
```

**分块传输编码：**
```cpp
// 支持流式响应
void setChunkedEncoding(bool on);
void appendChunk(Buffer& output, const string& data);
```

### 任务 8：性能优化

**连接池：**
- 重用 TCP 连接
- 减少连接建立开销

**响应缓存：**
- 缓存静态资源
- 减少 CPU 和 I/O 开销

**限流功能：**
- 基于 IP 的限流
- 基于 URL 的限流

---

## 实施建议

### 1. 开发顺序
1. **先实现 HttpRequest** - 能够解析简单的 GET 请求
2. **实现 HttpResponse** - 能够生成 HTTP 响应
3. **实现 HttpContext** - 处理连接状态
4. **实现 HttpServer** - 组合所有组件
5. **添加路由功能** - 支持多个 URL 路径
6. **添加多线程支持** - 提高并发性能

### 2. 测试策略
- **单元测试**：测试每个组件的独立功能
- **集成测试**：测试整个 HTTP 服务器的功能
- **性能测试**：测试并发处理能力

### 3. 使用现有基础设施
- **Buffer**：用于数据缓冲和解析
- **TcpConnection**：处理网络连接
- **EventLoop**：处理事件循环
- **InetAddress**：处理网络地址

### 4. 调试技巧
- 使用 `printf` 或日志查看解析过程
- 使用 `curl` 或浏览器测试 HTTP 服务器
- 使用 Wireshark 抓包分析 HTTP 流量

### 5. 参考资源
- RFC 2616：HTTP/1.1 协议规范
- Muduo 现有代码：学习其设计模式
- 开源 HTTP 服务器：如 mongoose, civetweb

---

## 总结

本教程提供了一个完整的 Muduo HTTP 开发路线图。从基础的 HTTP 协议解析开始，逐步构建功能完整的 HTTP 服务器。每个阶段都有明确的任务和实现指导，帮助你逐步掌握 Muduo 网络库的 HTTP 开发。

记住：**先实现最简单的版本，然后逐步添加功能**。每完成一个阶段，都要进行充分测试，确保代码质量。