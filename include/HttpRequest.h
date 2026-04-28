#pragma once

#include "copyable.h"
#include "Timestamp.h"
#include <string>
#include <unordered_map>

class HttpRequest : public copyable {
public:
    /* 标准化HTTP头键名，转换为小写 */
    static std::string normalizeHeaderKey(std::string key);

    enum class Method {
        Invalid,
        Get,
        Post,
        Head,
        Put,
        Delete
    };

    enum class Version {
        Unknown,
        Http10,
        Http11
    };

    HttpRequest() : _method(Method::Invalid), _version(Version::Unknown) { }

    __attribute__((always_inline)) void setVersion(Version v) {
        this->_version = v;
    }

    bool setMethod(std::string const &m);
    bool setMethod(char const *start, char const *end);

    __attribute__((always_inline)) Method method() const noexcept {
        return _method;
    }

    char const *methodString() const noexcept;

    __attribute__((always_inline)) void setPath(std::string const &path) {
        _path = path;
    }

    __attribute__((always_inline)) void setPath(char const *start, char const *end) {
        _path = std::string(start, end);
    }

    __attribute__((always_inline)) std::string const &path() const {
        return _path;
    }

    __attribute__((always_inline)) void setQuery(std::string const &query) {
        _query = query;
    }

    __attribute__((always_inline)) void setQuery(char const *start, char const *end) {
        _query = std::string(start, end);
    }

    __attribute__((always_inline)) std::string const &query() const {
        return _query;
    }

    __attribute__((always_inline)) void setBody(std::string const &body) {
        _body = body;
    }

    __attribute__((always_inline)) void appendToBody(std::string const &data) {
        _body.append(data);
    }

    __attribute__((always_inline)) void appendToBody(char const *data, size_t len) {
        _body.append(data, len);
    }

    __attribute__((always_inline)) std::string const &body() const {
        return _body;
    }

    __attribute__((always_inline)) void setReceiveTime(Timestamp receiveTime) {
        _receiveTime = receiveTime;
    }

    void addHeader(std::string const &field, std::string const &value);

    void addHeader(char const *start, char const *colon, char const *end);

    std::string getHeader(std::string const &field) const {
        std::string normalizedKey = normalizeHeaderKey(field);
        auto it = _headers.find(normalizedKey);
        if (it != _headers.end()) {
            return it->second;
        }
        return "";
    }

    __attribute__((always_inline)) std::unordered_map<std::string, std::string> const &
    headers() const {
        return _headers;
    }

    void swap(HttpRequest &other);
    std::string getQuery(std::string const &key, std::string const &defaultValue = "") const;

    // 获取所有查询参数
    std::unordered_map<std::string, std::string> getAllQueries() const;

    // 获取路径参数
    std::string getPathParam(std::string const &name) const {
        auto it = _pathParams.find(name);
        if (it != _pathParams.end()) {
            return it->second;
        }
        return "";
    }

    // 设置路径参数
    __attribute__((always_inline)) void setPathParams(
        std::unordered_map<std::string, std::string> const &params) {
        _pathParams = params;
    }

    __attribute__((always_inline)) void setPathParam(
        std::string const &name, std::string const &value) {
        _pathParams[name] = value;
    }

    static std::string urlDecode(std::string const &str);

private:
    void parseQueryParam(
        std::string const &param, std::unordered_map<std::string, std::string> &result) const;

private:
    Method _method;
    Version _version;
    std::string _path;
    std::string _query;
    std::string _body;
    Timestamp _receiveTime;
    std::unordered_map<std::string, std::string> _headers;
    std::unordered_map<std::string, std::string> _pathParams;
};
