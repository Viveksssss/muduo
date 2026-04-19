#pragma once

#include "Timestamp.h"
#include <functional>
#include <memory>

class Buffer;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(TcpConnection const &)>;
using CloseCallback = std::function<void(TcpConnection const &)>;
using WriteCompleteCallback = std::function<void(TcpConnection const &)>;
using HighWaterMarkCallback
    = std::function<void(TcpConnection const &, size_t)>;
using MessageCallback
    = std::function<void(TcpConnection const &, Buffer *, Timestamp)>;
