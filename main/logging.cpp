#include "logging.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace progressia {
namespace main {

namespace detail {

class LogSinkBackend {
  private:
    bool writing = false;
    std::stringstream buffer;

    void flush();

  public:
    LogSinkBackend() {}

    std::ostream &getOutput() { return buffer; }

    void setWriting(bool writing) {
        if (this->writing == writing) {
            std::cerr << "Attempting to write two log messages at once"
                      << std::endl;
            // REPORT_ERROR
            exit(1);
        }

        this->writing = writing;

        if (!writing) {
            flush();
        }
    }
};

} // namespace detail

namespace {
std::ofstream openLogFile() {
    // FIXME this is relative to bin, not root dir
    std::filesystem::create_directories("run");
    std::filesystem::create_directories("run/logs");
    return std::ofstream("run/logs/latest.log");
}
} // namespace

std::mutex logFileMutex;
std::ofstream logFile = openLogFile();
thread_local detail::LogSinkBackend theBackend;

std::ostream &detail::LogSink::getStream() const {
    if (!isCurrentSink) {
        std::cerr << "LogSink::getStream() while !isCurrentSink" << std::endl;
        // REPORT_ERROR
        exit(1);
    }

    return theBackend.getOutput();
}

detail::LogSink::LogSink(bool isCurrentSink) : isCurrentSink(isCurrentSink) {
    if (isCurrentSink) {
        theBackend.setWriting(true);
    }
}

detail::LogSink::~LogSink() {
    if (isCurrentSink) {
        theBackend.setWriting(false);
    }
}

detail::LogSink::LogSink(LogSink &&moveFrom)
    : isCurrentSink(moveFrom.isCurrentSink) {
    moveFrom.isCurrentSink = false;
}

void detail::LogSinkBackend::flush() {
    auto message = buffer.str();
    buffer.str("");
    buffer.clear();

    {
        std::lock_guard<std::mutex> lock(logFileMutex);
        // TODO flush less often?
        logFile << message << std::endl;
        std::cout << message << std::endl;
    }
}

namespace {
// FIXME This approach is horribly inefficient. It is also unsafe if any
// other piece of code wants access to std::localtime.
std::mutex getLocalTimeMutex;
std::tm getLocalTimeAndDontExplodePlease() {
    std::lock_guard<std::mutex> lock(getLocalTimeMutex);

    std::time_t t = std::time(nullptr);
    return *std::localtime(&t);
}
} // namespace

detail::LogSink log(LogLevel level, const char *start) {

#ifdef NDEBUG
    if (level == LogLevel::DEBUG) {
        return detail::LogSink(false);
    }
#endif

    detail::LogSink sink(true);
    auto tm = getLocalTimeAndDontExplodePlease();
    sink << std::put_time(&tm, "%T") << " ";

    switch (level) {
    case LogLevel::DEBUG:
        sink << "DEBUG";
        break;
    case LogLevel::INFO:
        sink << "INFO ";
        break;
    case LogLevel::WARN:
        sink << "WARN ";
        break;
    case LogLevel::ERROR:
        sink << "ERROR";
        break;
    default:
        sink << "FATAL";
        break;
    }

    sink << "    ";

    if (start != nullptr) {
        sink << start;
    }

    return sink;
}

namespace logging {

#ifdef NDEBUG
detail::LogSink debug(const char *) { return detail::LogSink(false); }
#else
detail::LogSink debug(const char *start) { return log(LogLevel::DEBUG, start); }
#endif

detail::LogSink info(const char *start) { return log(LogLevel::INFO, start); }

detail::LogSink warn(const char *start) { return log(LogLevel::WARN, start); }

detail::LogSink error(const char *start) { return log(LogLevel::ERROR, start); }

detail::LogSink fatal(const char *start) { return log(LogLevel::FATAL, start); }

} // namespace logging

} // namespace main
} // namespace progressia
