#pragma once
#include "../EnemyBase.h"

namespace Enemy {

	/// @brief Playerを継続追跡するOverTime Enemyクラス
	class OverTime final : public Base {
	protected:
		/// @brief Playerへ向かうOverTimeの追跡行動を更新
		/// @param deltaTime 前フレームからの経過時間
		void UpdateBehavior(float deltaTime) override;

		/// @brief OverTimeのModelアセット名を取得
		/// @return Modelアセット名
		std::string GetModelAssetName() const override;

		/// @brief OverTimeのModel表示倍率を取得
		/// @return Model表示倍率
		Vector3 GetModelScale() const override;

		/// @brief OverTimeのModel原点補正量を取得
		/// @return Model原点補正量
		Vector3 GetModelOffset() const override;

		/// @brief OverTimeの移動解決用Sphereを作成
		/// @return 移動解決用Sphere
		Sphere CreateMovementCollider() const override;

		/// @brief OverTimeの被弾判定用AABBを作成
		/// @return 被弾判定用AABB
		AABB CreateHitCollider() const override;
	};

} // namespace Enemy
