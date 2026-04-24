#pragma once
#include <string>
#include <source_location>

namespace Logger {

	/// @brief ログレベルを表す列挙型
	enum class Level {
		Info,       // 情報レベルのログ
		Warning,    // 警告レベルのログ
		Error,      // エラーレベルのログ
		Debug,      // デバッグレベルのログ
		Application // アプリケーションレベルのログ
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
}