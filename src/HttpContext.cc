#include "HttpContext.h"
#include "HttpRequest.h"
#include "Logger.h"
#include <strings.h>

HttpContext::HttpContext()
    : _state(HttpRequestParseState::ExpectRequestLine)
    , _contentLength(0)
    , _bodyReceived(0)
    , _isChunked(false) { }

HttpContext::~HttpContext() {
    log_info("HttpContext destroyed");
    reset();
}

HttpContext::ParseResult HttpContext::parseRequest(Buffer *buf, Timestamp receiveTime) {
    bool ok = true;
    bool hasMore = true;
    ParseResult result = ParseResult::NeedMore;
    log_debug("Parsing HTTP request, current state: {},result: {}", static_cast<int>(_state),
        static_cast<int>(result));
    log_debug("Buffer content: {}", std::string(buf->peek(), buf->readableBytes()));

    while (hasMore) {
        if (_state == HttpRequestParseState::ExpectRequestLine) {
            char const *crlf = buf->findCRLF();
            if (crlf) {
                ok = processRequestLine(buf->peek(), crlf);
                if (ok) {
                    _request.setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf + 2);
                    _state = HttpRequestParseState::ExpectHeaders;
                } else {
                    result = ParseResult::Error;
                    hasMore = false;
                }
            } else {
                hasMore = false;
            }
        } else if (_state == HttpRequestParseState::ExpectHeaders) {
            ok = processHeader(buf);
            if (ok) {
                if (_state == HttpRequestParseState::ExpectBody) {
                    if (_contentLength == 0 && !_isChunked) {
                        _state = HttpRequestParseState::GotAll;
                        result = ParseResult::GotRequest;
                        hasMore = false;
                    } else {
                        result = ParseResult::HeadersComplete;
                        if (buf->readableBytes() > 0) {
                            continue;
                        }
                        hasMore = false;
                    }
                }
            } else {
                result = ParseResult::Error;
                hasMore = false;
            }
        } else if (_state == HttpRequestParseState::ExpectBody) {
            if (processBody(buf)) {
                _state = HttpRequestParseState::GotAll;
                result = ParseResult::GotRequest;
            } else if (_bodyReceived < _contentLength) {
                result = ParseResult::NeedMore;
            }
            hasMore = false;
        }
    }
    return result;
}

void HttpContext::reset() {
    _state = HttpRequestParseState::ExpectRequestLine;
    HttpRequest{}.swap(_request);
    _contentLength = 0;
    _bodyReceived = 0;
    _isChunked = false;
    _customContext.reset();
}

bool HttpContext::processRequestLine(std::string const &line) {
    return processRequestLine(line.data(), line.data() + line.size());
}

bool HttpContext::processRequestLine(char const *begin, char const *end) {
    bool success = false;
    char const *start = begin;
    char const *space = std::find(start, end, ' ');
    if (space != end && _request.setMethod(start, space)) {
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end) {
            char const *question = std::find(start, space, '?');
            if (question != space) {
                _request.setPath(start, question);
                _request.setQuery(question, space);
            } else {
                _request.setPath(start, space);
            }
            start = space + 1;
            success = (end - start == 8 && std::equal(start, end - 1, "HTTP/1."));
            if (success) {
                if (*(end - 1) == '1') {
                    _request.setVersion(HttpRequest::Version::Http11);
                } else if (*(end - 1) == '0') {
                    _request.setVersion(HttpRequest::Version::Http10);
                } else {
                    success = false;
                }
            }
        }
    }
    return success;
}

bool HttpContext::processHeader(Buffer *buf) {
    bool ok = true;
    bool hasMore = true;
    while (hasMore) {
        char const *crlf = buf->findCRLF();
        if (crlf) {
            char const *colon = std::find(buf->peek(), crlf, ':');
            if (colon != crlf) {
                _request.addHeader(buf->peek(), colon, crlf);
                std::string field(buf->peek(), colon);
                if (strcasecmp(field.c_str(), "Content-Length") == 0) {
                    ++colon;
                    while (colon < crlf && isspace(*colon)) {
                        ++colon;
                    }
                    _contentLength = static_cast<size_t>(std::atoi(colon));
                } else if (strcasecmp(field.c_str(), "Transfer-Encoding") == 0) {
                    ++colon;
                    while (colon < crlf && isspace(*colon)) {
                        ++colon;
                    }
                    std::string encoding(colon, crlf); // 构造临时字符串
                    if (strcasecmp(encoding.c_str(), "chunked") == 0) {
                        _isChunked = true;
                    }
                }
                buf->retrieveUntil(crlf + 2);
            } else {
                // 空行解析完毕
                buf->retrieveUntil(crlf + 2);
                _state = HttpRequestParseState::ExpectBody;
                hasMore = false;
            }

        } else {
            hasMore = false;
        }
    }
    return ok;
}

bool HttpContext::processBody(Buffer *buf) {
    if (_isChunked) {
        return false;
    } else {
        size_t readable = buf->readableBytes();
        size_t remaining = remainingLength();

        // 自适应单次处理量
        size_t limit;
        if (remaining <= 1024 * 1024) {
            // 小文件：一次读完剩余内容
            limit = remaining;
        } else if (remaining <= 10 * 1024 * 1024) {
            // 中等文件：一次读 1MB
            limit = 1024 * 1024;
        } else {
            // 大文件：一次读 16MB
            limit = 16UL * 1024 * 1024;
        }

        size_t toRead = std::min({readable, remaining, limit});

        if (toRead > 0) {
            _request.appendToBody(buf->peek(), toRead);
            _bodyReceived += toRead;
            buf->retrieve(toRead);
        }

        return _bodyReceived >= _contentLength;

        // size_t toRead = std::min(readable, remainingLength());
        // if (toRead > 0) {
        //     _request.appendToBody(buf->peek(), toRead);
        //     _bodyReceived += toRead;
        //     buf->retrieve(toRead);
        // }
        // return _bodyReceived >= _contentLength;
    }
}
