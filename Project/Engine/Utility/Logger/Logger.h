#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <source_location>
#include <vector>

namespace Logger {

	/// @brief ログレベルを表す列挙型
	enum class Level {
		Engine,      // エンジン情報のログ
		Application, // アプリケーション情報のログ
		Assets,      // アセットの読み込み関連のログ
		Warning,     // 警告レベルのログ
		Error,       // エラーレベルのログ
		Debug,       // デバッグレベルのログ
		Count,       // ログレベル数
	};

	/// @brief ImGui表示用に保持するログ情報
	struct LogEntry {
		std::uint64_t sequence = 0;      // ログの通し番号
		Level level = Level::Engine;     // ログレベル
		std::string timestamp;           // 出力時刻
		std::string fileName;            // 出力元ファイル名
		std::uint_least32_t line = 0;    // 出力元行番号
		std::string message;             // ログ本文
		std::string formattedText;       // 表示用に整形済みのログ文字列
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

	/// @brief メモリ上に保持しているログ履歴を取得する
	/// @return ログ履歴のコピー
	std::vector<LogEntry> GetEntries();

	/// @brief メモリ上に保持しているログ履歴を消去する
	void ClearEntries();

	/// @brief メモリ上に保持するログ履歴の最大件数を設定する
	/// @param maxEntries 最大保持件数。0を指定した場合は履歴を保持しない
	void SetMaxBufferedEntries(std::size_t maxEntries);
}
