#include "Logger.h"
#include <Windows.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <mutex>

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

#ifdef NDEBUG
    std::ofstream g_logFile;
    std::mutex g_logMutex;
    std::string g_currentLogPath;

	/// @brief UTF-8文字列をワイド文字列（UTF-16）に変換する関数
    std::wstring Utf8ToWide(const std::string& utf8Str) {
        if (utf8Str.empty()) return L"";
        int size = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
        std::wstring wideStr(size - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], size);
        return wideStr;
    }

	/// @brief ログファイルのパスを生成する関数。既に同名のファイルが存在する場合は番号を付けて重複を避ける。
    std::string GetLogFilePath() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm localTime;
        if (localtime_s(&localTime, &time) != 0) {
            return "";
        }

		// ファイル名に使用するタイムスタンプを生成
        char buf[64];
        sprintf_s(buf, "%04d-%02d-%02d_%02d-%02d-%02d",
            localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday,
            localTime.tm_hour, localTime.tm_min, localTime.tm_sec);

        std::string baseDir = "Assets/.EngineResource/Log/";
        std::string baseName = std::string(buf);

        // ファイルが存在しない場合は番号なしで返す
        std::string testPath = baseDir + baseName + ".Log";
        if (!std::filesystem::exists(Utf8ToWide(testPath))) {
            return testPath;
        }

        // ファイルが存在する場合は(1)(2)...と番号を付ける
        int counter = 1;
        while (true) {
            std::string numberedPath = baseDir + baseName + "(" + std::to_string(counter) + ").Log";
            if (!std::filesystem::exists(Utf8ToWide(numberedPath))) {
                return numberedPath;
            }
            counter++;
            if (counter > 1000) { // 安全のため上限を設定
                return testPath;
            }
        }
    }

	/// @brief ログファイルを開く関数。必要に応じてディレクトリも作成する。
    void OpenLogFile() {
        std::lock_guard<std::mutex> lock(g_logMutex);

        std::string logPath = GetLogFilePath();
        if (logPath.empty()) {
            return;
        }

        try {
            std::wstring wideLogPath = Utf8ToWide(logPath);
            std::filesystem::path filePath(wideLogPath);
            std::filesystem::create_directories(filePath.parent_path());

            g_logFile.open(wideLogPath, std::ios::out);
            if (g_logFile.is_open()) {
                g_currentLogPath = logPath;
            }
        }
        catch (const std::exception&) {
            // ディレクトリ作成やファイルオープンに失敗した場合は何もしない
        }
    }

	/// @brief ログメッセージをファイルに書き込む関数。スレッドセーフに実装されている。
    void WriteToFile(const std::string& message) {
        std::lock_guard<std::mutex> lock(g_logMutex);

        if (!g_logFile.is_open()) {
            return;
        }

        g_logFile << message;
        g_logFile.flush();
    }

	/// @brief ログファイルを閉じる関数。スレッドセーフに実装されている。
    void CloseLogFile() {
        std::lock_guard<std::mutex> lock(g_logMutex);

        if (g_logFile.is_open()) {
            g_logFile.close();
        }
    }
#endif

}

namespace Logger {

    void Initialize() {
#ifdef NDEBUG
        OpenLogFile();
#endif
    }

    void Finalize() {
#ifdef NDEBUG
        CloseLogFile();
#endif
    }

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

#ifdef NDEBUG
        // Releaseビルド時はファイルに出力
        WriteToFile(utf8Str);
#else
        // Debugビルド時はデバッグウィンドウに出力
        // 2. UTF-8 -> ワイド文字列 (UTF-16) への変換
        int size = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
        std::wstring wideStr(size, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], size);

        // 3. ワイド版の関数で出力
        OutputDebugStringW(wideStr.c_str());
#endif
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