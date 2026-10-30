#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace System {

	/// @brief ゲームオーバー時の総スコア上位10件の永続化
	class ScoreRanking final {
	public:
		static constexpr std::size_t kMaxEntryCount = 10;

		/// @brief ランキングの保存先と初回移行元を設定
		/// @param filePath ランキングJSONの保存先
		/// @param playLogDirectory 初回起動時に取り込む既存プレイログの保存先
		explicit ScoreRanking(std::filesystem::path filePath = "SavedData/ScoreRanking.json",
			std::filesystem::path playLogDirectory = "SavedData/PlayLogs");

		/// @brief 保存済みランキングを読み込み、未作成の場合は既存プレイログを移行
		void Initialize();

		/// @brief 新しい総スコアを上位10件へ反映して保存
		/// @param score ゲームオーバー時に確定した総スコア
		/// @return 保存不要、または保存に成功した場合はtrue
		bool RecordScore(std::uint64_t score);

		/// @brief 現在のランキングを読み取り専用で取得
		/// @return 総スコアの降順に並んだ最大10件のランキング
		const std::vector<std::uint64_t>& GetScores() const { return scores_; }

		/// @brief 直近のスコア登録でランクインした位置を取得
		/// @return 0始まりの順位、圏外または読み込み直後の場合は空
		std::optional<std::size_t> GetLastRecordedRank() const { return lastRecordedRank_; }

	private:
		/// @brief ゲームオーバーとして保存された既存ログからスコアを復元
		void ImportPlayLogs();

		/// @brief スコアを降順に整列して上位10件へ制限
		void SortAndTrim();

		/// @brief 現在のランキングをJSONへ保存
		/// @return JSONの保存に成功した場合はtrue
		bool Save() const;

		std::filesystem::path filePath_;
		std::filesystem::path playLogDirectory_;
		std::vector<std::uint64_t> scores_;
		std::optional<std::size_t> lastRecordedRank_;
	};

}
