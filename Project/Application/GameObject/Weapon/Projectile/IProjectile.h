#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "ProjectileStatus.h"
#include "../WeaponStatus.h"
#include <cmath>
#include <string>
#include <unordered_set>

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

		/// @brief Enemy命中時の固有処理を実行
		virtual void OnEnemyHit() {
		}

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

		/// @brief Projectileの現在座標を取得
		/// @return Projectileの現在座標
		const Vector3& GetPosition() const { return transform_.translate; }

		/// @brief 跳弾可能かを取得
		/// @return 跳弾回数が残っていればtrueを返す
		bool CanBounce() const {
			return disappearsUponCollision_ && remainingBounceCount_ > 0;
		}

		/// @brief Enemyとの接触判定を開始
		void BeginEnemyCollisionFrame() {
			currentContactEnemyIds_.clear();
		}

		/// @brief Enemyとの衝突を処理
		/// @param enemyId 衝突したEnemyの識別番号
		/// @param bounceTargetPosition 跳弾先の座標。跳弾先が存在しない場合はnullptr
		/// @return ダメージを適用する新しい衝突の場合はtrueを返す
		bool HandleEnemyCollision(std::uint32_t enemyId, const Vector3* bounceTargetPosition) {
			currentContactEnemyIds_.insert(enemyId);

			if (isDead_) {
				return false;
			}

			// 敵との接触で消えない範囲攻撃は、既存のダメージ間隔に従って処理する。
			if (!disappearsUponCollision_) {
				return true;
			}

			// 接触し続けている同じ敵で回数を重複消費しない。
			if (previousContactEnemyIds_.contains(enemyId)) {
				return false;
			}

			if (remainingBounceCount_ > 0 && bounceTargetPosition &&
				SetMoveDirectionTowards(*bounceTargetPosition)) {
				--remainingBounceCount_;
				return true;
			}

			if (remainingPenetrationCount_ > 0) {
				--remainingPenetrationCount_;
				return true;
			}

			isDead_ = true;
			return true;
		}

		/// @brief Enemyとの接触判定を終了
		void EndEnemyCollisionFrame() {
			previousContactEnemyIds_ = currentContactEnemyIds_;
		}

	protected:
		/// @brief Projectile共通情報を初期化
		/// @param context Projectileの初期化情報
		/// @param colliderName Projectileのコライダー名
		void InitializeCommonProperties(const InitializeDesc& context, const std::string& colliderName) {
			projectileId_ = context.projectileId;
			colliderName_ = colliderName;
			ownerPosition = context.ownerPosition;
			targetPosition = context.targetPosition;

			damage_ = context.damage;
			moveSpeed_ = context.moveSpeed;
			sizeRate_ = context.sizeRate;
			lifeTime_ = context.lifeTime;
			remainingBounceCount_ = context.bounceCount;
			remainingPenetrationCount_ = context.penetrationCount;
			previousContactEnemyIds_.clear();
			currentContactEnemyIds_.clear();
		}

		/// @brief 指定座標へ向かうよう移動方向を変更
		/// @param position 新しい目標座標
		/// @return 有効な移動方向を設定できた場合はtrueを返す
		bool SetMoveDirectionTowards(const Vector3& position) {
			constexpr float kMinDirectionLengthSq = 0.000001f;
			const Vector3 toTarget = position - transform_.translate;
			if (toTarget.LengthSq() <= kMinDirectionLengthSq) {
				return false;
			}

			targetPosition = position;
			moveDirection_ = toTarget.Normalized();
			const float horizontalLength = std::sqrt(
				moveDirection_.x * moveDirection_.x + moveDirection_.z * moveDirection_.z);
			transform_.rotate.x = std::atan2(-moveDirection_.y, horizontalLength);
			transform_.rotate.y = std::atan2(moveDirection_.x, moveDirection_.z);
			transform_.rotate.z = 0.0f;
			return true;
		}

		std::uint64_t projectileId_ = 0; // Projectileの識別番号
		std::string colliderName_;       // Projectileのコライダー名

		float damage_ = 10.0f;              // ダメージ量
		float moveSpeed_ = 25.0f;           // 移動速度
		float sizeRate_ = 1.0f;             // サイズ倍率
		int remainingBounceCount_ = 0;      // 跳弾可能回数
		int remainingPenetrationCount_ = 0; // 貫通可能回数
		float lifeTime_ = 0.0f;             // 生存時間（秒）
		GameTimer lifeTimer_;               // 生存時間計測用タイマー

		bool isDead_ = false;                 // 生存フラグ
		bool disappearsUponCollision_ = true; // 敵との衝突時に消えるかどうか

		Transform3D transform_;
		ColliderShape hitbox_;

		Vector3 moveDirection_ = { 0.0f, 0.0f, 0.0f };
		Vector3 ownerPosition = { 0.0f, 0.0f, 0.0f };
		Vector3 targetPosition = { 0.0f, 0.0f, 0.0f };

		std::unordered_set<std::uint32_t> previousContactEnemyIds_; // 前フレームで接触していたEnemyの識別番号
		std::unordered_set<std::uint32_t> currentContactEnemyIds_;  // 今フレームで接触しているEnemyの識別番号
	};
}
