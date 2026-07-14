#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "ProjectileStatus.h"
#include "../WeaponStatus.h"
#include <vector>
#include <memory>
#include <string>

namespace Projectile {

	class IProjectile {
	public:
		/// @brief Projectileのデストラクタ
		virtual ~IProjectile() = default;

		/// @brief Projectileを初期化
		/// @param context 初期化に使用する情報
		virtual void Initialize(InitializeDesc context) = 0;

		/// @brief Projectileを更新
		/// @param deltaTime 前フレームからの経過時間
		virtual void Update(float deltaTime) = 0;

		/// @brief 削除対象かを取得
		/// @return 削除対象の場合はtrue
		bool IsDead() const { return isDead_; }

	protected:

		bool isDead_ = false;
		Transform3D transform_;
		ColliderShape hitbox_;

		Vector3 ownerPosition = { 0.0f, 0.0f, 0.0f };
		Vector3 targetPosition = { 0.0f, 0.0f, 0.0f };
	};
}
