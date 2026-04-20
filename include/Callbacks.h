#pragma once

#include "Timestamp.h"
#include <functional>
#include <memory>

class Buffer;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(TcpConnectionPtr const &)>;
using CloseCallback = std::function<void(TcpConnectionPtr const &)>;
using WriteCompleteCallback = std::function<void(TcpConnectionPtr const &)>;
using HighWaterMarkCallback
    = std::function<void(TcpConnectionPtr const &, size_t)>;
using MessageCallback
    = std::function<void(TcpConnectionPtr const &, Buffer *, Timestamp)>;
