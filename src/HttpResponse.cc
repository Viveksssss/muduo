#include "HttpResponse.h"

HttpResponse::HttpResponse(bool close)
    : _statusCode(Unknown)
    , _closeConnection(close)
    , _async(false) { }

void HttpResponse::appendToBuffer(Buffer *output) const {
    char buf[32];
    snprintf(buf, sizeof(buf), "HTTP/1.1 %d ", _statusCode);
    output->append(buf);
    output->append(_statusMessage);
    output->append("\r\n");

    if (_headers.find("Connection") == _headers.end()
        && _headers.find("connection") == _headers.end()) {
        if (_closeConnection) {
            output->append("Connection: close\r\n");
        } else {
            output->append("Connection: keep-alive\r\n");
        }

        if (_headers.find("Content_length") == _headers.end()
            && _headers.find("Content-Length") == _headers.end()
            && _headers.find("content-length") == _headers.end()) {
            output->append("Content-Length: ");
            output->append(std::to_string(_body.size()));
            output->append("\r\n");
        }

        for (auto const &header: _headers) {
            output->append(header.first);
            output->append(": ");
            output->append(header.second);
            output->append("\r\n");
        }

        output->append("\r\n");
        output->append(_body);
    }
}
