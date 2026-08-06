#pragma once
#include <RenderHeaders.h>
#include <cstddef>
#include <string>
#include <vector>

namespace Weapon {
	class Inventory;
}

namespace UI::Game {
	
	/// @brief 武器アイコンの表示を管理するクラス
	class WeaponIconUI {
	public:
		/// @brief 武器アイコンUIを初期化
		/// @param slotCount 表示する武器スロットの数
		void Initialize(int slotCount);

		/// @brief 指定した武器アイコンの発射アニメーションを開始
		/// @param slotIndex 発射した武器のスロット番号
		void PlayFireAnimation(std::size_t slotIndex);
		
		/// @brief 装備中の武器に合わせて武器アイコンの表示を更新
		/// @param deltaTime 前フレームからの経過時間
		/// @param inventory 表示対象の武器インベントリ
		void Update(float deltaTime, const Weapon::Inventory& inventory);

	private:
		struct IconAnimationState {
			float elapsedTime = 0.0f;
			bool isPlaying = false;
		};

		/// @brief 指定した武器アイコンの発射アニメーションを更新
		/// @param deltaTime 前フレームからの経過時間
		/// @param slotIndex 更新する武器スロットの番号
		void UpdateFireAnimation(float deltaTime, std::size_t slotIndex);

		float fireAnimationDuration_ = 0.15f;    // 発射アニメーションの合計時間（秒）
		Vector2 startIconScale = { 0.9f, 0.9f }; // 武器アイコンの通常時の拡縮率
		Vector2 endIconScale = { 1.25f, 1.25f }; // 武器アイコンの発射アニメーション時の拡縮率
		float shotGaugeSize_ = 28.0f;            // 射撃待機ゲージの最大サイズ

		std::vector<MadoEngine::SpriteHandle> weaponIcons_;
		std::vector<MadoEngine::SpriteHandle> weaponIconsShotGauge_;
		std::vector<MadoEngine::SpriteHandle> weaponIconsBG_;
		std::vector<MadoEngine::SpriteHandle> weaponIconFrames_;
		std::vector<IconAnimationState> animationStates_;
	};
}
