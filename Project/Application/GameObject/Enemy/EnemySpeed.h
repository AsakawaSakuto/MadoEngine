#pragma once
#include "EnemyBase.h"

namespace Enemy {

	/// @brief 高速でPlayerを追跡するSpeed Enemyクラス
	class Speed final : public Base {
	protected:
		/// @brief Playerへ向かう高速追跡行動を更新
		/// @param deltaTime 前フレームからの経過時間
		void UpdateBehavior(float deltaTime) override;

		/// @brief Speed EnemyのModelアセット名を取得
		/// @return Modelアセット名
		std::string GetModelAssetName() const override;

		/// @brief Speed EnemyのModel表示倍率を取得
		/// @return Model表示倍率
		Vector3 GetModelScale() const override;

		/// @brief Speed EnemyのModel原点補正量を取得
		/// @return Model原点補正量
		Vector3 GetModelOffset() const override;

		/// @brief Speed Enemyの移動解決用Sphereを作成
		/// @return 移動解決用Sphere
		Sphere CreateMovementCollider() const override;

		/// @brief Speed Enemyの被弾判定用AABBを作成
		/// @return 被弾判定用AABB
		AABB CreateHitCollider() const override;

		/// @brief Player接触時にSpeed Enemyを消滅させるか判定
		/// @return 常にtrue
		bool ShouldDisappearOnPlayerCollision() const override;
	};

} // namespace Enemy
