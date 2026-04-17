#pragma once

/**
 * @brief
 * 用于获取当前线程的ID，并将其缓存在线程局部存储中，以避免每次调用时都进行系统调用获取线程ID。
 *
 */
namespace CurrentThread {

extern thread_local int t_cachedTid;

void cacheTid();

inline int tid() {
    if (t_cachedTid == 0) [[unlikely]] {
        cacheTid();
    }
    return t_cachedTid;
}

} // namespace CurrentThread
