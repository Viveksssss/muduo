#include "HttpServer.h"
#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Logger.h"

namespace {
bool defaultHttpCallback(TcpConnectionPtr const &conn, HttpRequest &req, HttpResponse *resp) {
    resp->setStatusCode(HttpResponse::NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
    return false;
}

} // namespace

HttpServer::HttpServer(EventLoop *loop, InetAddress const &listenAddr, std::string const &name)
    : _server(loop, listenAddr, name)
    , _httpCallback(defaultHttpCallback) {
    _server.setConnectionCallback(
        std::bind(&HttpServer::OnConnection, this, std::placeholders::_1));

    _server.setMessageCallback(std::bind(&HttpServer::OnMessage, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3));
}

HttpServer::~HttpServer() = default;

void HttpServer::OnConnection(TcpConnectionPtr const &conn) {
    if (conn->connected()) {
        auto context = std::make_shared<HttpContext>();
        conn->setContext(context);
    }
}

void HttpServer::OnMessage(TcpConnectionPtr const &conn, Buffer *buf, Timestamp receiveTime) {
    auto context = std::static_pointer_cast<HttpContext>(conn->getContext());
    if (!context) {
        log_error("Context is null");
        return;
    }

    HttpContext::ParseResult result = context->parseRequest(buf, receiveTime);
    log_info("Parse result: {}", static_cast<int>(result));
    if (result == HttpContext::ParseResult::Error) {
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
        return;
    }

    if (result == HttpContext::ParseResult::HeadersComplete) {
        log_info("Headers complete, waiting for body if needed");
    } else if (result == HttpContext::ParseResult::NeedMore) {
        if (context->expectBody()
            || context->state() == HttpContext::HttpRequestParseState::ExpectBody) {
            HttpRequest &req = context->request();
            size_t bufSize = req.body().size();
            if (bufSize >= 1024 * 1024) {
                log_info("Stringing chunk:{}bytes", bufSize);
                HttpResponse response(false);
                bool syncProcessed = _httpCallback(conn, req, &response);
                if (!syncProcessed) {
                    log_info("Async request,waiting for callback");
                    return;
                } else {
                    log_info("Sync request completed");
                    req.clearBody();
                }
            }
        }

    } else if (result == HttpContext::ParseResult::GotRequest) {
        bool syncProcessed = OnRequest(conn, context->request());
        if (syncProcessed) {
            log_info("context->reset()");
            context->reset();
        }
    }

    log_debug("onMessage end");
}

bool HttpServer::OnRequest(TcpConnectionPtr const &conn, HttpRequest &request) {
    std::string const &connection = request.getHeader("Connection");
    bool close
        = connection == "close"
          || (request.getVersion() == HttpRequest::Version::Http10 && connection != "Keep-Alive");
    HttpResponse response(close);

    bool syncProcessed = _httpCallback(conn, request, &response);

    if (syncProcessed) {
        Buffer buf;
        response.appendToBuffer(&buf);
        conn->send(&buf);
        if (response.closeConnection()) {
            conn->shutdown();
        }
        log_info("Sync request completed");
    } else {
        log_info("Async request,waiting for callback");
    }
    return syncProcessed;
}
