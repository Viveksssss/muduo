#pragma once

#include "Buffer.h"
#include "copyable.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Logger.h"
#include "Timestamp.h"
#include <cstdint>
#include <memory>
#include <unordered_map>

class HttpContext : public copyable {
public:
    enum class HttpRequestParseState : int8_t {
        ExpectRequestLine,
        ExpectHeaders,
        ExpectBody,
        GotAll
    };

    enum class ParseResult : int8_t {
        Error = -1,
        NeedMore = 0,
        HeadersComplete = 1,
        GotRequest = 2
    };

    HttpContext();
    ~HttpContext();

    ParseResult parseRequest(Buffer *buf, Timestamp receiveTime);

    __attribute__((always_inline)) bool gotAll() const {
        return _state == HttpRequestParseState::GotAll;
    }

    __attribute__((always_inline)) bool expectBody() const {
        return _state == HttpRequestParseState::ExpectBody;
    }

    __attribute__((always_inline)) bool headersComplete() const {
        return _state == HttpRequestParseState::ExpectBody
               || _state == HttpRequestParseState::GotAll;
    }

    __attribute__((always_inline)) size_t remainingLength() const {
        return _contentLength - _bodyReceived;
    }

    __attribute__((always_inline)) bool isChunked() const {
        return _isChunked;
    }

    void reset();

    __attribute__((always_inline)) HttpRequest const &request() const {
        return _request;
    }

    __attribute__((always_inline)) HttpRequest &request() {
        return _request;
    }

    __attribute__((always_inline)) HttpRequestParseState state() const {
        return _state;
    }

    template <typename T>
    std::shared_ptr<T> getContext() const {
        return std::static_pointer_cast<T>(_customContext);
    }

    __attribute__((always_inline)) void setContext(std::shared_ptr<void> context) {
        _customContext = std::move(context);
    }

private:
    bool processRequestLine(std::string const &line);
    bool processRequestLine(char const *begin, char const *end);
    bool processHeader(Buffer *buf);
    bool processBody(Buffer *buf);

private:
    HttpRequestParseState _state;
    HttpRequest _request;
    size_t _contentLength;
    size_t _bodyReceived;
    bool _isChunked;
    std::shared_ptr<void> _customContext;
};
