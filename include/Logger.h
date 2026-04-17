#include <chrono>
#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <source_location>
#include <string>

#define FOREACH_LOG_LEVEL(f)                                                   \
    f(trace) f(debug) f(info) f(critical) f(warning) f(error) f(fatal)

enum class log_level : std::uint8_t {
#define _FUNCTION(x) x,
    FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
};

namespace details {

inline std::string log_level_name(log_level lev) {
    switch (lev) {
#define _FUNCTION(name)                                                        \
    case log_level::name: return #name;
        FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
    }
    return "unknown";
}

inline log_level log_level_from_name(std::string const &name) {
#define _FUNCTION(lev)                                                         \
    if (name == #lev)                                                          \
        return log_level::lev;
    FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
    return log_level::info;
}

#if (__linux__) || defined(__APPLE__)
inline constexpr char k_level_ansi_colors[(std::uint8_t)log_level::fatal + 1][8]
    = {
        "\033[37m",
        "\033[35m",
        "\033[32m",
        "\033[34m",
        "\033[33m",
        "\033[31m",
        "\033[31;1m",
};

inline constexpr char k_reset_ansi_color[4] = "\033[m";
# define LOG_IF_HAS_ANSI_COLORS(x) x
#else
# define LOG_IF_HAS_ANSI_COLORS(x) x
inline constexpr char k_level_ansi_colors[(std::uint8_t)log_level::fatal + 1][1]
    = {
        "",
        "",
        "",
        "",
        "",
        "",
        "",
};

inline constexpr char k_reset_ansi_color[1] = "";
#endif

inline log_level g_max_level = []() -> log_level {
    if (auto lev = std::getenv("LOG_LEVEL")) {
        return log_level_from_name(lev);
    }
    return log_level::info;
}();

inline std::ofstream g_log_file = []() -> std::ofstream {
    if (auto path = std::getenv("LOG_FILE")) {
        return std::ofstream(path, std::ios::app);
    }
    return std::ofstream();
}();

inline void output_log(
    log_level lev, std::string msg, std::source_location const &loc) {
    std::chrono::zoned_time now{
        std::chrono::current_zone(), std::chrono::system_clock::now()};
    msg = std::format("{} {}:{} [{}] {}", now, loc.file_name(), loc.line(),
        log_level_name(lev), msg);
    if (g_log_file) {
        g_log_file << msg + '\n';
    }
    if (lev >= g_max_level) {
        std::cout << LOG_IF_HAS_ANSI_COLORS(
                         k_level_ansi_colors[(std::uint8_t)lev] +)
                             msg LOG_IF_HAS_ANSI_COLORS(+k_reset_ansi_color)
                         + '\n';
    }
}

template <typename T>
struct with_source_location {
private:
    T inner;
    std::source_location loc;

public:
    template <typename U>
        requires std::constructible_from<T, U>
    consteval with_source_location(
        U &&inner, std::source_location loc = std::source_location::current())
        : inner(std::forward<U>(inner))
        , loc(std::move(loc)) { }

    constexpr T const &format() const {
        return inner;
    }

    constexpr std::source_location const &location() const {
        return loc;
    }
};

} // namespace details

template <typename... Args>
void log(log_level lev,
    details::with_source_location<std::format_string<Args...>> fmt,
    Args &&...args) {
    auto const &loc = fmt.location();
    // TODO:
    auto format
        = std::vformat(fmt.format().get(), std::make_format_args(args...));
    details::output_log(lev, std::move(format), loc);
    if (lev == log_level::fatal) {
        std::abort();
    }
}

#define _FUNCTION(name)                                                        \
    template <typename... Args>                                                \
    void log_##name(                                                           \
        details::with_source_location<std::format_string<Args...>> fmt,        \
        Args &&...args) {                                                      \
        return log(                                                            \
            log_level::name, std::move(fmt), std::forward<Args>(args)...);     \
    }
FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION

static inline void set_log_level(log_level lev) {
    details::g_max_level = lev;
}

static inline void set_log_file(std::string const &path) {
    details::g_log_file = std::ofstream(path, std::ios::app);
}
