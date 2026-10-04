#pragma once
#include <RenderHeaders.h>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Relic {
	class System;
	struct Definition;
}

namespace UI::Game {

	/// @brief 所持レリックを画面下部へ横一列に表示するUI
	class RelicInventoryUI {
	public:
		/// @brief 描画オブジェクトを初期化
		/// @param relicSystem 初期表示に使用するレリックシステム
		void Initialize(const Relic::System& relicSystem);

		/// @brief 描画オブジェクトの参照を解放
		void Finalize();

		/// @brief 所持レリックの変更を表示へ同期
		/// @param relicSystem 表示対象のレリックシステム
		void Update(const Relic::System& relicSystem);

	private:
		struct Slot {
			MadoEngine::SpriteHandle icon{};
			MadoEngine::TextHandle countText{};
		};

		/// @brief 必要な数まで再利用可能な表示スロットを生成
		/// @param requiredCount 必要な表示スロット数
		void EnsureSlotCount(std::size_t requiredCount);

		/// @brief 一個の表示スロットへレリック情報を設定
		/// @param slotIndex 設定対象のスロット番号
		/// @param definition 表示対象のレリック定義
		/// @param stackCount 現在の所持数
		/// @param position スロット中心の画面座標
		void ConfigureSlot(
			std::size_t slotIndex,
			const Relic::Definition& definition,
			int stackCount,
			const Vector2& position);

		/// @brief 指定した表示スロットの表示状態を設定
		/// @param slotIndex 設定対象のスロット番号
		/// @param isVisible 表示する場合はtrue
		void SetSlotVisible(std::size_t slotIndex, bool isVisible);

		std::vector<Slot> slots_;
		std::size_t activeSlotCount_ = 0;
		std::uint64_t displayedRevision_ = 0;
		bool isInitialized_ = false;
	};

}
