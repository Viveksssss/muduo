#include "Logger.h"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <Socket.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

Socket::Socket(int fd) : _sockfd(fd) { }

Socket::~Socket() {
    ::close(_sockfd);
}

int Socket::fd() const {
    return _sockfd;
}

void Socket::bindAddress(InetAddress const &localaddr) {
    if (bind(_sockfd, static_cast<sockaddr const *>(localaddr.getSockAddr()),
            sizeof(sockaddr_in6))
        != 0) {
        log_fatal("bind sockfd:{} fail\n", _sockfd);
    }
}

void Socket::listen() {
    if (::listen(_sockfd, 1024) != 0) {
        log_fatal("listen sockfd:{} faile\n", _sockfd);
    }
}

int Socket::accept(InetAddress *peeraddr) {
    sockaddr_in6 addr;
    socklen_t addrlen = sizeof(addr);
    bzero(&addr, sizeof addr);
    int connfd
        = ::accept(_sockfd, reinterpret_cast<sockaddr *>(&addr), &addrlen);
    if (connfd > 0) {
        /* 统一用v6来接受对段 */
        peeraddr->setSockAddrInet6(addr);
    }
    return connfd;
}

void Socket::shutdownWrite() {
    if (::shutdown(_sockfd, SHUT_WR) < 0) {
        log_error("shutwodn write error");
    }
}

void Socket::setTcpNoDelay(bool on) {
    /*
        level 值	说明	典型选项
        SOL_SOCKET	Socket 层（通用）	SO_REUSEADDR, SO_KEEPALIVE, SO_RCVBUF
        IPPROTO_IP	IPv4 协议层	IP_TTL, IP_MULTICAST_LOOP
        IPPROTO_IPV6	IPv6 协议层	IPV6_V6ONLY, IPV6_MULTICAST_LOOP
        IPPROTO_TCP	TCP 协议层	TCP_NODELAY, TCP_KEEPIDLE, TCP_CORK
        IPPROTO_UDP	UDP 协议层	UDP_CORK（Linux 特有）
    */
    int optval = on ? 1 : 0;
    ::setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}

void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

void Socket::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}

void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}
