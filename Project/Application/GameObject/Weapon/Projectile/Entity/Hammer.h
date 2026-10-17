#pragma once
#include "../IProjectile.h"
#include <string>
#include <unordered_set>

namespace Projectile {

	/// @brief 武器所有者の頭上から対象方向へ振り下ろすProjectile
	class Hammer : public IProjectile {
	public:
		/// @brief Hammerのデストラクタ
		~Hammer() override;

		/// @brief Hammerの構えと攻撃方向を初期化
		/// @param context 初期化に使用する情報
		void Initialize(InitializeDesc context) override;

		/// @brief 構え、振り下ろし、終了待機を更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief 振り下ろしの支点を武器所有者へ追従
		/// @param ownerPosition 武器所有者の現在座標
		void SynchronizeOwnerPosition(const Vector3& ownerPosition) override;

		/// @brief 振り下ろし中のEnemy命中を一体につき一回だけ受付
		/// @param enemyId 衝突したEnemyの識別番号
		/// @param bounceTargetPosition 共通インターフェースの跳弾先座標、Hammerでは未使用
		/// @return ダメージを適用する新しい衝突の場合はtrue
		bool HandleEnemyCollision(std::uint32_t enemyId, const Vector3* bounceTargetPosition) override;

	private:
		enum class State {
			Windup,
			Swinging,
			Recovery,
		};

		/// @brief 支点と振り下ろし角度をModelとColliderへ反映
		void UpdatePose();

		/// @brief Modelの回転とサイズを根元から先端までのOBBへ反映
		void UpdateHitbox();

		MadoEngine::ModelHandle model_{};
		std::string objectName_;
		Vector3 hitboxPosition_;
		std::unordered_set<std::uint32_t> hitEnemyIds_;
		State state_ = State::Windup;
		GameTimer stateTimer_;
		float swingDuration_ = 0.0f;
		float swingEndAngle_ = 0.0f;
		float swingAngle_ = 0.0f;
		float aimYaw_ = 0.0f;
		bool isHitboxActive_ = false;
	};
}
