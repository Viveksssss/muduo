#include "InetAddress.h"
#include <Acceptor.h>
#include <Logger.h>
#include <sys/socket.h>

static int createNonblocking(sa_family_t family) {
    int sockfd = ::socket(
        family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        log_fatal("createNonblocking error");
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, InetAddress const &addr, bool reuseport)
    : _loop(loop)
    , _acceptSocket(createNonblocking(addr.family()))
    , _acceptChannel(loop, _acceptSocket.fd())
    , _listening(false) {
    _acceptSocket.setReusePort(reuseport);
    _acceptSocket.setReuseAddr(true);
    _acceptSocket.bindAddress(addr);
    _acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
    _acceptChannel.disableAll();
    _acceptChannel.remove();
}

bool Acceptor::listening() const {
    return _listening;
}

void Acceptor::listen() {
    _listening = true;
    _acceptSocket.listen();
    _acceptChannel.enableReading();
}

void Acceptor::handleRead() {
    InetAddress peerAddr;
    int connfd = _acceptSocket.accept(&peerAddr);
    if (connfd >= 0) {
        if (_newConnectionCallback) {
            _newConnectionCallback(connfd, peerAddr);
        } else {
            ::close(connfd);
        }
    } else {
        log_error("Acceptor::listen error");
    }
}
