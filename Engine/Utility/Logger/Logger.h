#pragma once
#include <string>
#include <source_location>

namespace Logger {

	/// @brief ログレベルを表す列挙型
    enum class Level {
        Info,    // 緑
        Warning, // 黄
		Error,   // 赤
		Debug    // 青
    };

	/// @brief ログを出力する関数
    void Output(
        const std::string& message,
        Level level = Level::Info,
        const std::source_location& location = std::source_location::current()
    );
	
    void Info(const std::string& message, const std::source_location& location = std::source_location::current());
    void Warning(const std::string& message, const std::source_location& location = std::source_location::current());
    void Error(const std::string& message, const std::source_location& location = std::source_location::current());
    void Debug(const std::string& message, const std::source_location& location = std::source_location::current());

}