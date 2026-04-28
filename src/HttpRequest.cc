
#include "HttpRequest.h"
#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

std::string HttpRequest::normalizeHeaderKey(std::string key) {
    std::transform(
        key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });
    return key;
}

char const *HttpRequest::methodString() const noexcept {
    char const *result = "UNKNOWN";
    switch (_method) {
    case Method::Get:    result = "GET"; break;
    case Method::Post:   result = "POST"; break;
    case Method::Head:   result = "HEAD"; break;
    case Method::Put:    result = "PUT"; break;
    case Method::Delete: result = "DELETE"; break;
    default:             break;
    }
    return result;
}

void HttpRequest::addHeader(std::string const &field, std::string const &value) {
    std::string normalizedKey = normalizeHeaderKey(field);
    _headers[normalizedKey] = value;
}

void HttpRequest::addHeader(char const *start, char const *colon, char const *end) {
    std::string field(start, colon);
    std::string normalizedField = normalizeHeaderKey(field);
    ++colon;
    while (colon < end && std::isspace(*colon)) {
        ++colon;
    }
    std::string value(colon, end);
    while (!value.empty() && std::isspace(value.back())) {
        value.resize(value.size() - 1);
    }

    _headers[normalizedField] = value;
}

void HttpRequest::swap(HttpRequest &other) {
    std::swap(_method, other._method);
    std::swap(_version, other._version);
    _path.swap(other._path);
    _query.swap(other._query);
    _body.swap(other._body);
    std::swap(_receiveTime, other._receiveTime);
    _headers.swap(other._headers);
    _pathParams.swap(other._pathParams);
}

std::unordered_map<std::string, std::string> HttpRequest::getAllQueries() const {
    std::unordered_map<std::string, std::string> result;
    std::string q = _query;
    if (q.empty()) {
        return result;
    }
    if (q[0] == '?') {
        q = q.substr(1);
    }

    size_t start = 0;
    size_t end = q.find('&');
    while (end != std::string::npos) {
        parseQueryParam(q.substr(start, end - start), result);
        start = end + 1;
        end = q.find('&', start);
    }
    parseQueryParam(q.substr(start), result);

    return result;
}

std::string HttpRequest::getQuery(std::string const &key, std::string const &defaultValue) const {
    std::string q = _query;
    if (q.empty()) {
        return defaultValue;
    }
    if (q[0] == '?') {
        q = q.substr(1); // 移除开头的?
    }

    std::vector<std::string> params;
    size_t start = 0;
    size_t end = q.find('&');
    while (end != std::string::npos) {
        params.push_back(q.substr(start, end - start));
        start = end + 1;
        end = q.find('&', start);
    }
    params.push_back(q.substr(start));

    for (auto const &param: params) {
        size_t pos = param.find('=');
        if (pos != std::string::npos) {
            std::string paramKey = param.substr(0, pos);
            if (paramKey == key) {
                return urlDecode(param.substr(pos + 1));
            }
        } else if (param == key) {
            return "true";
        }
    }

    return defaultValue;
}

std::string HttpRequest::urlDecode(std::string const &str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] != '%') {
            if (str[i] == '+') {
                result += ' ';
            } else {
                result += str[i];
            }
        } else if (i + 2 < str.size()) {
            char hex[3] = {str[i + 1], str[i + 2], '\0'};
            result += static_cast<char>(std::strtol(hex, nullptr, 16));
            i += 2;
        }
    }
    return result;
}

void HttpRequest::parseQueryParam(
    std::string const &param, std::unordered_map<std::string, std::string> &result) const {
    size_t pos = param.find('=');
    if (pos != std::string::npos) {
        result[urlDecode(param.substr(0, pos))] = urlDecode(param.substr(pos + 1));
    } else {
        result[urlDecode(param)] = "true";
    }
}

bool HttpRequest::setMethod(std::string const &m) {
    assert(_method == Method::Invalid);
    if (m == "GET") {
        _method = Method::Get;
    } else if (m == "POST") {
        _method = Method::Post;
    } else if (m == "HEAD") {
        _method = Method::Head;
    } else if (m == "PUT") {
        _method = Method::Put;
    } else if (m == "DELETE") {
        _method = Method::Delete;
    } else {
        _method = Method::Invalid;
    }
    return _method != Method::Invalid;
}

bool HttpRequest::setMethod(char const *start, char const *end) {
    std::string m(start, end);
    return setMethod(m);
}
