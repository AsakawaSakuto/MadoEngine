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

	/// @brief ログシステムを初期化する
	void Initialize();

	/// @brief ログシステムを終了する
	void Finalize();

	/// @brief ログを出力する関数
	void Output(
		const std::string& message,
		Level level = Level::Info,
		const std::source_location& location = std::source_location::current()
	);

	
	/// @brief 情報レベルのログを出力する関数
	void Info(const std::string& message, const std::source_location& location = std::source_location::current());

	/// @brief 警告レベルのログを出力する関数
	void Warning(const std::string& message, const std::source_location& location = std::source_location::current());

	/// @brief エラーレベルのログを出力する関数
	void Error(const std::string& message, const std::source_location& location = std::source_location::current());

	/// @brief デバッグレベルのログを出力する関数
	void Debug(const std::string& message, const std::source_location& location = std::source_location::current());

}