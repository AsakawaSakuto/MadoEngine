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
		/// @return 削除対象の場合はtrueを返す
		bool IsDead() const { return isDead_; }

		/// @brief Projectileの識別番号を取得
		/// @return Projectileの識別番号
		std::uint64_t GetProjectileId() const { return projectileId_; }

		/// @brief Projectileのダメージ量を取得
		/// @return Projectileのダメージ量
		float GetDamage() const { return damage_; }

		/// @brief Projectileのコライダー名を取得
		/// @return Projectileのコライダー名
		const std::string& GetColliderName() const { return colliderName_; }

	protected:
		/// @brief Projectile共通情報を初期化
		/// @param context Projectileの初期化情報
		/// @param colliderName Projectileのコライダー名
		void InitializeCommonProperties(const InitializeDesc& context, const std::string& colliderName) {
			projectileId_ = context.projectileId;
			damage_ = context.damage;
			colliderName_ = colliderName;
			ownerPosition = context.ownerPosition;
			targetPosition = context.targetPosition;
		}

		std::uint64_t projectileId_ = 0;
		float damage_ = 10.0f;
		std::string colliderName_;

		bool isDead_ = false;
		Transform3D transform_;
		ColliderShape hitbox_;

		Vector3 ownerPosition = { 0.0f, 0.0f, 0.0f };
		Vector3 targetPosition = { 0.0f, 0.0f, 0.0f };
	};
}
