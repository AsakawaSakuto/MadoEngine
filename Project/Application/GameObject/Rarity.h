#pragma once

// @brief アイテムのレアリティや武器のアップグレードレアリティを表す列挙型
enum class Rarity {
	Uncommon = 0,  // 緑
	Rare,          // 青
	Epic,          // 紫
	Legendary,     // 黄 // 強化の選択肢に出るのはここまで
			       
	Unique,        // 赤	 // キャラ専用アイテム用
};