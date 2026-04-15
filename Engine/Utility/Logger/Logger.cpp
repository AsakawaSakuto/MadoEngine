#include "Logger.h"
#include <Windows.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>

namespace {

    // std::stringを返すことでメモリ安全性を確保
    std::string LevelToString(Logger::Level level) {
        switch (level) {
        case Logger::Level::Info:    return " --- [INFO] --- ";
        case Logger::Level::Warning: return " --- [WARNING] --- ";
        case Logger::Level::Error:   return " --- [ERROR] --- ";
        case Logger::Level::Debug:   return " --- [DEBUG] --- ";
        default:                     return " --- [UNKNOWN] --- ";
        }
    }

    // 安定したタイムスタンプ取得
    std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ) % 1000;

        std::tm localTime;
        if (localtime_s(&localTime, &time) != 0) {
            return "00:00:00.000";
        }

        char buf[32];
        sprintf_s(buf, "%02d:%02d:%02d.%03lld",
            localTime.tm_hour, localTime.tm_min, localTime.tm_sec, ms.count());
        return std::string(buf);
    }
}

namespace Logger {

    void Logger::Output(const std::string& message, Level level, const std::source_location& location) {

        // 1. 文字列を組み立てる (UTF-8のまま)
        std::string ts = GetTimestamp();
        std::string lv = LevelToString(level);
        std::string file = location.file_name();
        size_t lastSlash = file.find_last_of("\\/");
        if (lastSlash != std::string::npos) file = file.substr(lastSlash + 1);

        std::ostringstream oss;
        oss << "Logger --- " << "[" << ts << "]" << lv << file << "(" << location.line() << ") : " << message << "\n";
        std::string utf8Str = oss.str();

        // 2. UTF-8 -> ワイド文字列 (UTF-16) への変換
        int size = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
        std::wstring wideStr(size, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], size);

        // 3. ワイド版の関数で出力
        OutputDebugStringW(wideStr.c_str());
    }

    void Info(const std::string& message, const std::source_location& location) {
        Output(message, Level::Info, location);
    }

    void Warning(const std::string& message, const std::source_location& location) {
        Output(message, Level::Warning, location);
    }

    void Error(const std::string& message, const std::source_location& location) {
        Output(message, Level::Error, location);
    }

    void Debug(const std::string& message, const std::source_location& location) {
        Output(message, Level::Debug, location);
    }
}