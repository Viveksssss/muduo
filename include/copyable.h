#pragma once

class copyable {
public:
    copyable &operator=(copyable const &) = default;
    copyable(copyable const &) = default;
    copyable() = default;

protected:
    /* 防止外部根据基类指针delete类对象,copyable本质类似一种属性 */
    ~copyable() = default;
};
