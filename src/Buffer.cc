#include <Buffer.h>
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

ssize_t Buffer::readFd(int fd, int *saveErrno) {
    char extrabuf[65536] = {0};
    iovec vec[2];
    size_t const writeable = writeableBytes();
    vec[0].iov_base = begin() + _writerIndex;
    vec[0].iov_len = writeable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    int const iovcnt = (writeable < sizeof extrabuf) ? 2 : 1;
    ssize_t const n = ::readv(fd, vec, iovcnt);
    if (n < 0) [[unlikely]] {
        *saveErrno = errno;
    } else if (static_cast<size_t>(n) <= writeable) [[likely]] {
        _writerIndex += n;
    } else [[unlikely]] {
        _writerIndex = _buffer.size();
        append(extrabuf, n - writeable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno) {
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0) {
        *saveErrno = errno;
    }
    return n;
}
