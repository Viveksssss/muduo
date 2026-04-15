#include "Timestamp.h"
#include <chrono>
#include <iomanip> // std::put_time
#include <sstream> // std::ostringstream

Timestamp::Timestamp() : _microSecondsSinceEpoch(0) { }

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : _microSecondsSinceEpoch(microSecondsSinceEpoch) { }

std::string Timestamp::toString() const {
    return std::to_string(_microSecondsSinceEpoch);
}

std::string Timestamp::toFormattedString() const {
    std::time_t sec = _microSecondsSinceEpoch / 1'000'000;
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&sec), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

Timestamp Timestamp::now() {
    return Timestamp(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch())
            .count());
}
