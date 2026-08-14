#pragma once
#include "EnemyBase.h"

namespace Enemy {

	/// @brief 状態遷移による追跡と突進を持つBoss Enemyクラス
	class Boss final : public Base {
	protected:
		/// @brief Bossの行動状態を初期化
		void OnInitialized() override;

		/// @brief Boss固有の状態遷移と行動を更新
		/// @param deltaTime 前フレームからの経過時間
		void UpdateBehavior(float deltaTime) override;

		/// @brief Boss専用Modelアセット名を取得
		/// @return Modelアセット名
		std::string GetModelAssetName() const override;

		/// @brief BossのModel表示倍率を取得
		/// @return Model表示倍率
		Vector3 GetModelScale() const override;

		/// @brief BossのModel原点補正量を取得
		/// @return Model原点補正量
		Vector3 GetModelOffset() const override;

		/// @brief Bossの移動解決用Sphereを作成
		/// @return 移動解決用Sphere
		Sphere CreateMovementCollider() const override;

		/// @brief Bossの被弾判定用AABBを作成
		/// @return 被弾判定用AABB
		AABB CreateHitCollider() const override;

		/// @brief Player接触時にBossを消滅させるか判定
		/// @return 常にfalse
		bool ShouldDisappearOnPlayerCollision() const override;

		/// @brief Bossの接触ダメージ待機時間を取得
		/// @return 接触ダメージの待機時間
		float GetPlayerDamageInterval() const override;

	private:
		enum class State {
			Chase,
			Windup,
			Rush,
			Recovery,
		};

		/// @brief Bossの行動状態を切り替え
		/// @param nextState 遷移先の行動状態
		void ChangeState(State nextState);

		State state_ = State::Chase;
		float stateTimer_ = 0.0f;
		Vector3 rushTargetPosition_ = { 0.0f, 0.0f, 0.0f };
	};

} // namespace Enemy
