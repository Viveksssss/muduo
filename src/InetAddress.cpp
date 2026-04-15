#include <arpa/inet.h>
#include <cstring>
#include <InetAddress.h>
#include <stdexcept>
#include <sys/socket.h>

InetAddress::InetAddress(uint16_t port, std::string const &ip) {
    ::bzero(&_addr, sizeof(_addr));
    if (::inet_pton(AF_INET, ip.c_str(), &_addr._addr.sin_addr) == 1) {
        _addr._addr.sin_family = AF_INET;
        _addr._addr.sin_port = htons(port);
    } else if (::inet_pton(AF_INET6, ip.c_str(), &_addr._addr6.sin6_addr)
               == 1) {
        _addr._addr6.sin6_family = AF_INET6;
        _addr._addr6.sin6_port = htons(port);
    } else {
        throw std::runtime_error("Invalid IP address");
    }
}

std::string InetAddress::toIp() const {
    char buf[INET6_ADDRSTRLEN] = {0};
    if (_addr._addr.sin_family == AF_INET) {
        ::inet_ntop(AF_INET, &_addr._addr.sin_addr, buf, sizeof(buf));
    } else if (_addr._addr6.sin6_family == AF_INET6) {
        ::inet_ntop(AF_INET6, &_addr._addr6.sin6_addr, buf, sizeof(buf));
    } else {
        return "unknown";
    }
    return std::string(buf);
}

std::string InetAddress::toIpPort() const {
    char buf[64] = {0};
    if (_addr._addr.sin_family == AF_INET) {
        ::inet_ntop(AF_INET, &_addr._addr.sin_addr, buf, sizeof(buf));
        uint16_t port = ::ntohs(_addr._addr.sin_port);
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ":%u", port);
    } else if (_addr._addr6.sin6_family == AF_INET6) {
        buf[0] = '[';
        ::inet_ntop(
            AF_INET6, &_addr._addr6.sin6_addr, buf + 1, sizeof(buf) - 1);
        uint16_t port = ::ntohs(_addr._addr6.sin6_port);
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "]:%u", port);
    }
    return std::string(buf);
}

uint16_t InetAddress::toPort() const {
    if (_addr._addr.sin_family == AF_INET) {
        return ntohs(_addr._addr.sin_port);
    } else if (_addr._addr6.sin6_family == AF_INET6) {
        return ntohs(_addr._addr6.sin6_port);
    }
    return 0;
}

sockaddr const *InetAddress::getSockAddr() const { 
    return reinterpret_cast<sockaddr const *>(&_addr);
}
