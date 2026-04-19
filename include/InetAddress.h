#pragma once
#include <arpa/inet.h>
#include <copyable.h>
#include <netinet/in.h>
#include <string>
#include <sys/cdefs.h>
#include <sys/socket.h>

class InetAddress : copyable {
public:
    explicit InetAddress(
        uint16_t port = 0, std::string const &ip = "127.0.0.1");

    explicit InetAddress(struct sockaddr_in const &addr)
        : _addr{._addr = addr} { }

    explicit InetAddress(struct sockaddr_in6 const &addr)
        : _addr{._addr6 = addr} { }

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;
    sockaddr const *getSockAddr() const;
    void setSockAddrInet6(sockaddr_in6 const &addr);

    __attribute__((always_inline)) sa_family_t family() const noexcept {
        return _addr._addr.sin_family;
    }

private:
    union {
        struct sockaddr_in _addr;
        struct sockaddr_in6 _addr6;
    } _addr;

    uint16_t _port;
};
