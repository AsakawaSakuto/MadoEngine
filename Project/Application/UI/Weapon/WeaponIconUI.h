#pragma once
#include <RenderHeaders.h>
#include <vector>
#include <string>

namespace Weapon {
	class Inventory;
}

namespace UI {
	
	/// @brief 武器アイコンの表示を管理するクラス
	class WeaponIconUI {
	public:
		/// @brief 武器アイコンUIを初期化
		/// @param slotCount 表示する武器スロットの数
		void Initialize(int slotCount);
		
		/// @brief 装備中の武器に合わせて武器アイコンの表示を更新
		/// @param inventory 表示対象の武器インベントリ
		void Update(const Weapon::Inventory& inventory);

	private:
		std::vector<Sprite*> weaponIcons_;
		std::vector<Sprite*> weaponFrames_;
	};
}
