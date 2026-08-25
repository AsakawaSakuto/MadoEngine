#pragma once
#include "EnemyBase.h"

namespace Enemy {

	/// @brief 高耐久と低速移動を特徴とするTank Enemyクラス
	class Tank final : public Base {
	protected:
		/// @brief Playerへ向かうTank固有の低速追跡を更新
		/// @param deltaTime 前フレームからの経過時間
		void UpdateBehavior(float deltaTime) override;

		/// @brief TankのModelアセット名を取得
		/// @return Modelアセット名
		std::string GetModelAssetName() const override;

		/// @brief TankのModel表示倍率を取得
		/// @return Model表示倍率
		Vector3 GetModelScale() const override;

		/// @brief TankのModel原点補正量を取得
		/// @return Model原点補正量
		Vector3 GetModelOffset() const override;

		/// @brief Tankの移動解決用Sphereを作成
		/// @return 移動解決用Sphere
		Sphere CreateMovementCollider() const override;

		/// @brief Tankの被弾判定用AABBを作成
		/// @return 被弾判定用AABB
		AABB CreateHitCollider() const override;

		/// @brief Player接触時にTankを消滅させるか判定
		/// @return 常にtrue
		bool ShouldDisappearOnPlayerCollision() const override;
	};

} // namespace Enemy
