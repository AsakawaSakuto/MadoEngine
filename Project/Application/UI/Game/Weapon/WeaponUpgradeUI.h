#pragma once
#include <cstddef>
#include <cstdint>

namespace Weapon {

	class Inventory;
	class UpgradeSystem;
}

namespace UI::Game {

	/// @brief 武器アップグレードの選択操作と表示を管理するクラス
	class UpgradeUI {
	public:
		/// @brief 武器アップグレードUIを初期化
		void Initialize();

		/// @brief 武器アップグレードの選択入力を更新
		/// @param upgradeSystem 武器アップグレード進行を管理するシステム
		/// @param inventory 選択結果を適用する武器インベントリ
		void UpdateInput(Weapon::UpgradeSystem& upgradeSystem, Weapon::Inventory& inventory);

		/// @brief 武器アップグレード確認用のImGuiを描画
		/// @param upgradeSystem 表示する武器アップグレードシステム
		/// @param inventory 選択結果を適用する武器インベントリ
		void DrawImGui(Weapon::UpgradeSystem& upgradeSystem, Weapon::Inventory& inventory);

	private:
		/// @brief 現在の候補世代に合わせて選択状態を同期
		/// @param upgradeSystem 同期元の武器アップグレードシステム
		void SynchronizeSelection(const Weapon::UpgradeSystem& upgradeSystem);

		/// @brief 選択状態を初期値へ戻す
		void ResetSelection();

		std::size_t selectedChoiceIndex_ = 0;
		std::uint64_t selectedGeneration_ = 0;
	};
}
