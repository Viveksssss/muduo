#pragma once

/*

    @code
    +-------------------+------------------+------------------+
    | prependable bytes |  readable bytes  |  writable bytes  |
    |                   |     (CONTENT)    |                  |
    +-------------------+------------------+------------------+
    |                   |                  |                  |
    0      <=      readerIndex   <=   writerIndex    <=     size
    @endcode

*/
#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

class Buffer {
public:
    static size_t const CheapPrepend = 8;
    static size_t const InitialSize = 1024;

    explicit Buffer(size_t initialSize = InitialSize)
        : _buffer(CheapPrepend + initialSize)
        , _readerIndex(CheapPrepend)
        , _writerIndex(CheapPrepend) { }

    __attribute__((__always_inline__)) size_t readableBytes() const noexcept {
        return _writerIndex - _readerIndex;
    }

    __attribute__((__always_inline__)) size_t writeableBytes() const noexcept {
        return _buffer.size() - _writerIndex;
    }

    __attribute__((__always_inline__)) size_t
    prependableBytes() const noexcept {
        return _readerIndex;
    }

    void append(char const *data, size_t len) {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        _writerIndex += len;
    }

    /* 从fd读取数据 */
    ssize_t readFd(int fd, int *saveErrno);

    /* 往fd写入数据 */
    ssize_t writeFd(int fd, int *saveErrno);

    __attribute__((__always_inline__)) char *begin() noexcept {
        return _buffer.data();
    }

    __attribute__((__always_inline__)) char const *begin() const noexcept {
        return _buffer.data();
    }

    __attribute__((__always_inline__)) char *beginWrite() {
        return begin() + _writerIndex;
    }

    __attribute__((__always_inline__)) const char *beginWrite() const {
        return begin() + _writerIndex;
    }

    __attribute__((__always_inline__)) char const *peek() const noexcept {
        return begin() + _readerIndex;
    }

    __attribute__((__always_inline__)) void retrieve(size_t len) noexcept {
        assert(len <= readableBytes());
        if (len < readableBytes()) {
            _readerIndex += len;
        } else {
            retrieveAll();
        }
    }

    __attribute__((__always_inline__)) void retrieveAll() {
        _readerIndex = CheapPrepend;
        _writerIndex = CheapPrepend;
    }

    __attribute__((__always_inline__)) std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    __attribute__((__always_inline__)) std::string retrieveAsString(
        size_t len) noexcept {
        assert(len <= readableBytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void ensureWriteableBytes(size_t len) {
        if (writeableBytes() < len) {
            makeSpace(len);
        }
        assert(writeableBytes() >= len);
    }

private:
    /*
        | prepend | read | write |
        | prepend    |     len      |
                     |
                     ↓
        | prepend | read |    len   |
    */
    void makeSpace(size_t len) {
        if (writeableBytes() + prependableBytes() - CheapPrepend < len) {
            _buffer.resize(_writerIndex + len);
        } else {
            assert(CheapPrepend < _readerIndex);
            size_t readable = readableBytes();
            std::copy(begin() + _readerIndex, begin() + _writerIndex,
                begin() + CheapPrepend);

            _readerIndex = CheapPrepend;
            _writerIndex = _readerIndex + readable;
            assert(readable == readableBytes());
        }
    }

    std::vector<char> _buffer;
    std::size_t _readerIndex;
    std::size_t _writerIndex;
};
