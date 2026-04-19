#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "TcpConnection.h"
#include <cassert>
#include <Logger.h>
#include <TcpServer.h>

namespace {

EventLoop *CHECK_NOTNULL(EventLoop *loop) {
    if (!loop) {
        log_fatal("mainLoop is nullptr");
    }
    return loop;
}

} // namespace

TcpServer::TcpServer(EventLoop *loop, InetAddress const &listenAddr,
    std::string const &name, Option option)
    : _loop(CHECK_NOTNULL(loop))
    , _ipPort(listenAddr.toIpPort())
    , _name(name)
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

TcpServer::~TcpServer() { }

void TcpServer::start() {
    if (_started++ == 0) {
        _threadPool->start(_threadInitCallback);
        assert(!_acceptor->listening());
        _loop->runInLoop(std::bind(&Acceptor::listen, _acceptor.get()));
    }
}

void TcpServer::newConnection(
    [[maybe_unused]] int sockfd, InetAddress const &peerAddr) {
    EventLoop *ioloop = _threadPool->getNextLoop();
    ioloop->runInLoop([]() { });
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", _ipPort.c_str(), _nextConnId);
    ++_nextConnId;
    std::string connName = _name + buf;

    log_debug("TcpServer::newConnection [{}] - new connection [{}] from {}",
        _name, connName, peerAddr.toIpPort());
    // TODO:
    // InetAddress localAddr();
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
