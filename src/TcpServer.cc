#include <Acceptor.h>
#include <Callbacks.h>
#include <cassert>
#include <EventLoop.h>
#include <EventLoopThreadPool.h>
#include <InetAddress.h>
#include <Logger.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <TcpConnection.h>
#include <TcpServer.h>
#include <unistd.h>

namespace {

/* 确保loop不为空 */
EventLoop *CHECK_NOTNULL(EventLoop *loop) {
    if (!loop) {
        log_fatal("mainLoop is nullptr");
    }
    return loop;
}

/* 获取与sockfd连接的本地ip和端口 */
struct sockaddr_in6 getLocalAddr(int sockfd) {
    struct sockaddr_in6 localaddr;
    bzero(&localaddr, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(
            sockfd, reinterpret_cast<sockaddr *>(&localaddr), &addrlen)
        < 0) {
        log_error("getLocalAddr Error");
    }
    return localaddr;
}

/* 默认高水位回调 */
void defaultWaterMarkCallback(TcpConnectionPtr const &conn, size_t len) {
    if (len > 256 * 1024 * 1024) {
        log_error("{}-严重堆积:{} 强制断连", conn->name(), len);
        conn->forceClose();
    } else if (len > 128 * 1024 * 1024) {
        log_warning("{}-高水位:{} 暂停读取", conn->name(), len);
        conn->stopRead();
    } else {
        log_info("{}-达到水位", conn->name());
    }
}

} // namespace

TcpServer::TcpServer(EventLoop *loop, InetAddress const &listenAddr,
    std::string const &name, Option option)
    : _loop(CHECK_NOTNULL(loop))
    , _ipPort(listenAddr.toIpPort())
    , _name(name)
    , _listenAddr(listenAddr)
    , _acceptor(std::make_unique<Acceptor>(
          loop, listenAddr, option == Option::ReusePort))
    , _threadPool(std::make_shared<EventLoopThreadPool>(_loop, _name))
    , _connectionCallback()
    , _messageCallback()
    , _started(0)
    , _nextConnId(0) {
    _acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection,
        this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer() {
    log_trace("TcpServer::~TcpServer [{}]", _name);
    for (auto &item: _connections) {
        /* 由局部对象持有 */
        auto conn = item.second;
        item.second.reset();
        conn->getLoop()->runInLoop(
            [this, conn]() { conn->connectDestroyed(); });
    }
}

void TcpServer::start() {
    if (_started++ == 0) {
        _threadPool->start(_threadInitCallback);
        assert(!_acceptor->listening());
        _loop->runInLoop(std::bind(&Acceptor::listen, _acceptor.get()));
    }
}

void TcpServer::newConnection(int sockfd, InetAddress const &peerAddr) {
    EventLoop *ioloop = _threadPool->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", _ipPort.c_str(), _nextConnId);
    ++_nextConnId;
    std::string connName = _name + buf;

    log_debug("TcpServer::newConnection [{}] - new connection [{}] from {}",
        _name, connName, peerAddr.toIpPort());
    InetAddress localAddr(_listenAddr);
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(
        ioloop, connName, sockfd, localAddr, peerAddr);
    _connections[connName] = conn;
    conn->setConnectionCallback(_connectionCallback);
    conn->setMessageCallback(_messageCallback);
    conn->setWriteCompleteCallback(_writeCompleteCallback);
    conn->setCloseCallback(
        [this](TcpConnectionPtr conn) { this->removeConnection(conn); });

    conn->setHighWaterMarkCallback(defaultWaterMarkCallback, 64 * 1024 * 1024);
    ioloop->runInLoop([conn]() { conn->connectEstablished(); });
}

void TcpServer::removeConnection(TcpConnectionPtr const &conn) {
    _loop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(TcpConnectionPtr const &conn) {
    log_debug("TcpServer::removeConnectionInLoop [{}] - connction {}", _name,
        conn->name());

    [[maybe_unused]] size_t n = _connections.erase(conn->name());

    assert(n == 1);
    EventLoop *ioloop = conn->getLoop();
    ioloop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
