#pragma once

#include "util.h"
#include <ostream>

namespace progressia::main {

namespace detail {

class LogSink : private progressia::main::NonCopyable {
  private:
    bool isCurrentSink;

    std::ostream &getStream() const;

    template <typename T> void put(const T &x) const {
        if (isCurrentSink) {
            getStream() << x;
        }
    }

  public:
    LogSink(bool isCurrentSink);
    ~LogSink();

    LogSink(LogSink &&) noexcept;

    template <typename T>
    friend const LogSink &operator<<(const LogSink &sink, const T &x) {
        sink.put(x);
        return sink;
    }
};
} // namespace detail

enum class LogLevel {
    // Only interesting when investigating a problem.
    DEBUG,
    // A regular user may want to see this.
    INFO,
    // All users should read this; if message persists, something is wrong.
    WARN,
    // Something is definitely wrong, but we're not crashing (yet).
    ERROR,
    // This is why the game is about to crash.
    FATAL
};

detail::LogSink log(LogLevel, const char *start = nullptr);

namespace logging {
detail::LogSink debug(const char *start = nullptr);
detail::LogSink info(const char *start = nullptr);
detail::LogSink warn(const char *start = nullptr);
detail::LogSink error(const char *start = nullptr);
detail::LogSink fatal(const char *start = nullptr);
} // namespace logging

void initializeLogging();
void shutdownLogging();

} // namespace progressia::main
