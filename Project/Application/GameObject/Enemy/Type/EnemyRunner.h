#pragma once
#include "../EnemyBase.h"

namespace Enemy {

	/// @brief 低耐久と高速移動を特徴とするRunner Enemyクラス
	class Runner final : public Base {
	protected:
		/// @brief Playerへ向かうRunner固有の高速追跡を更新
		/// @param deltaTime 前フレームからの経過時間
		void UpdateBehavior(float deltaTime) override;

		/// @brief RunnerのModelアセット名を取得
		/// @return Modelアセット名
		std::string GetModelAssetName() const override;

		/// @brief RunnerのModel表示倍率を取得
		/// @return Model表示倍率
		Vector3 GetModelScale() const override;

		/// @brief RunnerのModel原点補正量を取得
		/// @return Model原点補正量
		Vector3 GetModelOffset() const override;

		/// @brief Runnerの移動解決用Sphereを作成
		/// @return 移動解決用Sphere
		Sphere CreateMovementCollider() const override;

		/// @brief Runnerの被弾判定用AABBを作成
		/// @return 被弾判定用AABB
		AABB CreateHitCollider() const override;

		/// @brief Player接触時にRunnerを消滅させるか判定
		/// @return 常にtrue
		bool ShouldDisappearOnPlayerCollision() const override;
	};

} // namespace Enemy
