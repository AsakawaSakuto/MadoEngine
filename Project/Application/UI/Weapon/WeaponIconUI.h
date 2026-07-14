#pragma once
#include <RenderHeaders.h>
#include <vector>
#include <string>

namespace Weapon {
	class Inventory;
	
	/// @brief 武器アイコンの表示を管理するクラスです。
	class WeaponIconUI {
	public:
		/// @brief 武器アイコンUIを初期化します。
		/// @param slotCount 表示する武器スロットの数です。
		void Initialize(int slotCount);
		
		/// @brief 装備中の武器に合わせて武器アイコンの表示を更新します。
		/// @param inventory 表示対象の武器インベントリです。
		void Update(const Inventory& inventory);

	private:
		std::vector<Sprite*> weaponIcons_;
		std::vector<Sprite*> weaponFrames_;
	};
}
