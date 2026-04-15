#pragma once

#include <cstdint>
#include <string>

class Timestamp {
public:
    Timestamp();

    explicit Timestamp(int64_t microSecondsSinceEpoch);

    std::string toString() const;
    std::string toFormattedString() const;
    static Timestamp now();

private:
    int64_t _microSecondsSinceEpoch;
};
