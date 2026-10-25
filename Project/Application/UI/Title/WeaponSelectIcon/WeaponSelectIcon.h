#pragma once
#include "GameObject/Weapon/Projectile/ProjectileStatus.h"
#include "RenderHeaders.h"

namespace UI::Title {

	/// @brief 初期武器の選択枠とアイコンと武器名の表示
	class WeaponSelectIcon final {
	public:
		/// @brief 武器種類と配置座標を指定したアイコンの初期化
		/// @param weaponType 表示対象の武器種類
		/// @param position アイコン中心の画面座標
		void Initialize(Projectile::Type weaponType, const Vector2& position);

		/// @brief 選択枠とアイコンの強調状態の設定
		/// @param isSelected 選択中の表示にする場合はtrue
		void SetSelected(bool isSelected);

		/// @brief Scene所有リソースへの参照の解放
		void Finalize();

	private:
		MadoEngine::SpriteHandle frameSprite_{};
		MadoEngine::SpriteHandle backgroundSprite_{};
		MadoEngine::SpriteHandle iconSprite_{};
		MadoEngine::TextHandle nameText_{};
	};
}
