#include "Logger.h"
#include <Windows.h>
#include <chrono>
#include <iomanip>

namespace {
    const char* LevelToString(Logger::Level level) {
        switch (level) {
        case Logger::Level::Info:    return "「INFO」";
        case Logger::Level::Warning: return "「WARNING」";
        case Logger::Level::Error:   return "「ERROR」";
        case Logger::Level::Debug:   return "「DEBUG」";
        default:                  return "「UNKNOWN」";
        }
    }

    std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ) % 1000;

        std::tm localTime;
        localtime_s(&localTime, &time);

        std::ostringstream oss;
        oss << std::put_time(&localTime, "%H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }
}

namespace Logger {

    void Output(
        const std::string& message,
        Level level,
        const std::source_location& location
    ) {
        std::string timestamp = GetTimestamp();
        std::string levelStr = LevelToString(level);
        std::string filename = location.file_name();

        size_t lastSlash = filename.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            filename = filename.substr(lastSlash + 1);
        }

        std::string output = std::format(
            "[{}] {} {}({}): {}\n",
            timestamp,
            levelStr,
            filename,
            location.line(),
            message
        );

        OutputDebugStringA(output.c_str());
    }

    void Info(const std::string& message) {
        Output(message, Level::Info);
    }

    void Warning(const std::string& message) {
        Output(message, Level::Warning);
    }

    void Error(const std::string& message) {
        Output(message, Level::Error);
    }

    void Debug(const std::string& message) {
        Output(message, Level::Debug);
    }

}
