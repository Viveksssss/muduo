#pragma once
#include <arpa/inet.h>
#include <copyable.h>
#include <netinet/in.h>
#include <string>

class InetAddress : copyable {
public:
    explicit InetAddress(uint16_t port, std::string const &ip = "127.0.0.1");

    explicit InetAddress(struct sockaddr_in const &addr)
        : _addr{._addr = addr} { }

    explicit InetAddress(struct sockaddr_in6 const &addr)
        : _addr{._addr6 = addr} { }

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;
    sockaddr const *getSockAddr() const;

private:
    union {
        struct sockaddr_in _addr;
        struct sockaddr_in6 _addr6;
    } _addr;

    uint16_t _port;
};
