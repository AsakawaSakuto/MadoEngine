#pragma once
#include "WeaponUpgradeCardUI.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace Weapon {

	class Inventory;
	class UpgradeSystem;
}

namespace UI::Game {

	/// @brief 武器アップグレードの選択操作とカード表示を管理するクラス
	class UpgradeUI {
	public:
		/// @brief 武器アップグレードUIを初期化
		void Initialize();

		/// @brief 武器アップグレードUIが所有する描画オブジェクトを破棄
		void Finalize();

		/// @brief 武器アップグレードの選択入力とカード表示を更新
		/// @param deltaTime 前フレームからの経過時間
		/// @param upgradeSystem 武器アップグレード進行を管理するシステム
		/// @param inventory 選択結果を適用する武器インベントリ
		void Update(
			float deltaTime,
			Weapon::UpgradeSystem& upgradeSystem,
			Weapon::Inventory& inventory
		);

		/// @brief 武器アップグレード確認用のImGuiを描画
		/// @param upgradeSystem 表示する武器アップグレードシステム
		/// @param inventory 選択結果を適用する武器インベントリ
		void DrawImGui(Weapon::UpgradeSystem& upgradeSystem, Weapon::Inventory& inventory);

	private:
		/// @brief 現在の候補世代に合わせて選択状態とカード内容を同期
		/// @param upgradeSystem 同期元の武器アップグレードシステム
		void SynchronizeSelection(const Weapon::UpgradeSystem& upgradeSystem);

		/// @brief 各カードへ選択状態と表示演出を反映
		/// @param deltaTime 前フレームからの経過時間
		void UpdateCards(float deltaTime);

		/// @brief 選択状態を初期値へ戻してカードを非表示化
		void ResetSelection();

		static constexpr std::size_t kUpgradeCardCount = 3;

		std::array<WeaponUpgradeCardUI, kUpgradeCardCount> upgradeCards_;
		std::size_t selectedChoiceIndex_ = 0;
		std::size_t visibleChoiceCount_ = 0;
		std::uint64_t selectedGeneration_ = 0;
	};
}
