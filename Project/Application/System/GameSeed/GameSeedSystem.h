#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace System {

	/// @brief Game開始時のSeed確定と履歴の永続化を管理
	class GameSeedSystem final {
	public:
		/// @brief 保存対象となるSeed履歴の一件
		struct HistoryEntry {
			std::uint32_t seed = 0;
			bool isFavorite = false;
		};

		static constexpr std::size_t kMaxHistoryCount = 10;

		/// @brief 永続化されたSeed履歴の読み込み
		void Initialize();

		/// @brief 次回のGameで使用するSeed要求を設定
		/// @param requestedSeed 次回使用するSeed値、未指定時は新規抽選
		void RequestSeed(std::optional<std::uint32_t> requestedSeed);

		/// @brief Game開始時に使用するSeed値を確定
		/// @return Game開始に使用するSeed値
		std::uint32_t BeginGame();

		/// @brief 指定したSeed履歴のお気に入り状態を変更
		/// @param historyIndex 変更する履歴の位置
		/// @param isFavorite お気に入りとして登録する場合はtrue
		/// @return 履歴更新と保存に成功した場合はtrue
		bool SetFavorite(std::size_t historyIndex, bool isFavorite);

		/// @brief 保存されているSeed履歴を取得
		/// @return 古い順に並んだSeed履歴
		const std::vector<HistoryEntry>& GetHistory() const;

	private:
		/// @brief 新規抽選したSeed値を履歴へ登録
		/// @param seed 登録するSeed値
		void RegisterGeneratedSeed(std::uint32_t seed);

		/// @brief Seed履歴をJSONへ保存
		/// @return 保存に成功した場合はtrue
		bool Save() const;

		std::vector<HistoryEntry> history_;
		std::optional<std::uint32_t> requestedSeed_;
	};

}
