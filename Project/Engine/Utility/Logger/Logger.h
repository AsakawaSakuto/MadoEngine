#pragma once
#include <string>
#include <source_location>

namespace Logger {

	/// @brief ログレベルを表す列挙型
	enum class Level {
		Engine,      // エンジン情報のログ
		Application, // アプリケーション情報のログ
		Assets,      // アセットの読み込み関連のログ
		Warning,     // 警告レベルのログ
		Error,       // エラーレベルのログ
		Debug,       // デバッグレベルのログ
	};

	/// @brief ログシステムを初期化する
	void Initialize();

	/// @brief ログシステムを終了する
	void Finalize();

	/// @brief ログを出力する関数
	void Output(
		const std::string& message,
		Level level = Level::Engine,
		const std::source_location& location = std::source_location::current()
	);
}