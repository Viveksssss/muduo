#pragma once

#include "InetAddress.h"
#include "noncopyable.h"

class Socket : noncopyable {
public:
    explicit Socket(int fd);

    ~Socket();
    int fd() const;
    void bindAddress(InetAddress const &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);
    void shutdownWrite();
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    int const _sockfd;
};
