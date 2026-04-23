#pragma once

/**
 * @brief 不可拷贝的基类，禁止拷贝构造和赋值操作
 *
 */
class noncopyable {
public:
    noncopyable(noncopyable const &) = delete;
    noncopyable &operator=(noncopyable const &) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};
