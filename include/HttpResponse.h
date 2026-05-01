#pragma once

#include "Buffer.h"
#include <functional>
#include <string>
#include <unordered_map>

class HttpResponse {
public:
    using ResponseCallback = std::function<void(HttpResponse const &)>;

    enum HttpStatusCode {
        Unknown = 0,
        OK = 200,
        PartialContent = 206,
        MovedPermanentyly = 301,
        BadRequest = 400,
        Unauthorized = 401,
        Forbidden = 403,
        NotFound = 404,
        RangeNotSatisfiable = 416,
        InternalServerError = 500,
    };

    explicit HttpResponse(bool close);

    ~HttpResponse() = default;

    __attribute__((always_inline)) void setStatusCode(HttpStatusCode code) {
        _statusCode = code;
    }

    __attribute__((always_inline)) void setStatusMessage(std::string const &message) {
        _statusMessage = message;
    }

    __attribute__((always_inline)) void setCloseConnection(bool on) {
        _closeConnection = on;
    }

    __attribute__((always_inline)) bool closeConnection() const {
        return _closeConnection;
    }

    __attribute__((always_inline)) void setContentType(std::string const &contentType) {
        addHeader("Content-Type", contentType);
    }

    __attribute__((always_inline)) void addHeader(
        std::string const &key, std::string const &value) {
        _headers[key] = value;
    }

    __attribute__((always_inline)) void setBody(std::string const &body) {
        _body = body;
    }

    __attribute__((always_inline)) void setAsync(bool async) {
        _async = async;
    }

    __attribute__((always_inline)) bool isAsync() const {
        return _async;
    }

    __attribute__((always_inline)) void setCallback(ResponseCallback callback) {
        _callback = std::move(callback);
    }

    __attribute__((always_inline)) ResponseCallback const &callback() const {
        return _callback;
    }

    void appendToBuffer(Buffer *output) const;

private:
    std::unordered_map<std::string, std::string> _headers;
    HttpStatusCode _statusCode;
    std::string _statusMessage;
    bool _closeConnection;
    std::string _body;
    bool _async;
    ResponseCallback _callback;
};
