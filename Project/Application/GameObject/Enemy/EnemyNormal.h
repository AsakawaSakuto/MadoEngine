#pragma once
#include "EnemyBase.h"

namespace Enemy {

	/// @brief Playerを継続追跡する通常Enemyクラス
	class Normal final : public Base {
	protected:
		/// @brief Playerへ向かう通常行動を更新
		/// @param deltaTime 前フレームからの経過時間
		void UpdateBehavior(float deltaTime) override;

		/// @brief 通常EnemyのModelアセット名を取得
		/// @return Modelアセット名
		std::string GetModelAssetName() const override;

		/// @brief 通常EnemyのModel表示倍率を取得
		/// @return Model表示倍率
		Vector3 GetModelScale() const override;

		/// @brief 通常EnemyのModel原点補正量を取得
		/// @return Model原点補正量
		Vector3 GetModelOffset() const override;

		/// @brief 通常Enemyの移動解決用Sphereを作成
		/// @return 移動解決用Sphere
		Sphere CreateMovementCollider() const override;

		/// @brief 通常Enemyの被弾判定用AABBを作成
		/// @return 被弾判定用AABB
		AABB CreateHitCollider() const override;

		/// @brief Player接触時に通常Enemyを消滅させるか判定
		/// @return 常にtrue
		bool ShouldDisappearOnPlayerCollision() const override;
	};

} // namespace Enemy
