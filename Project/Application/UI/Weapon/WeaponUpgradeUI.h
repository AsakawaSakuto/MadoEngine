#pragma once
#include <cstddef>
#include <cstdint>

namespace Weapon {

	class Inventory;
	class UpgradeSystem;

	/// @brief 武器アップグレードの選択操作と表示を管理するクラス
	class UpgradeUI {
	public:
		/// @brief 武器アップグレードUIを初期化します。
		void Initialize();

		/// @brief 武器アップグレードの選択入力を更新します。
		/// @param upgradeSystem 武器アップグレード進行を管理するシステムです。
		/// @param inventory 選択結果を適用する武器インベントリです。
		void UpdateInput(UpgradeSystem& upgradeSystem, Inventory& inventory);

		/// @brief 武器アップグレード確認用のImGuiを描画します。
		/// @param upgradeSystem 表示する武器アップグレードシステムです。
		/// @param inventory 選択結果を適用する武器インベントリです。
		void DrawImGui(UpgradeSystem& upgradeSystem, Inventory& inventory);

	private:
		/// @brief 現在の候補世代に合わせて選択状態を同期します。
		/// @param upgradeSystem 同期元の武器アップグレードシステムです。
		void SynchronizeSelection(const UpgradeSystem& upgradeSystem);

		/// @brief 選択状態を初期値へ戻します。
		void ResetSelection();

		std::size_t selectedChoiceIndex_ = 0;
		std::uint64_t selectedGeneration_ = 0;
	};
}
