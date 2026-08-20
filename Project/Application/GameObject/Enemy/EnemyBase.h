#pragma once
#include "../IGameObject.h"
#include "EnemyMovement.h"
#include "EnemyStatus.h"
#include <cstdint>
#include <string>
#include <unordered_map>

namespace Player {
	class Base;
}

namespace Enemy {

	/// @brief Enemyが死亡状態へ移行した理由
	enum class DeathReason {
		Defeated,
		OutsideMap,
		OutsidePlayerRange,
	};

	/// @brief Enemy生成時に使用する初期化情報
	struct SpawnDesc {
		Vector3 position = { 0.0f, 0.0f, 0.0f };
		Data::Status status;
		Data::Type type = Data::Type::Normal;
		Data::BonusType bonusType = Data::BonusType::None;
		SceneType sceneType = SceneType::None;
		float groundSurfaceY = 0.0f;
		bool emergeFromGround = false;
	};

	/// @brief Enemyへ適用したProjectileダメージの結果
	struct ProjectileDamageResult {
		float appliedDamage = 0.0f;
		bool wasApplied = false;
		bool wasKilled = false;
	};

	/// @brief Enemy単体の共通状態と振る舞いを管理する基底クラス
	class Base : public IGameObject {
	public:
		/// @brief Enemy::Baseを破棄
		~Base() override;

		/// @brief Enemy::Baseを初期化
		/// @param enemyId Enemyの識別番号
		/// @param desc Enemyの生成情報
		void Initialize(std::uint32_t enemyId, const SpawnDesc& desc);

		/// @brief Enemy単体の状態と移動を更新
		/// @param deltaTime 前フレームからの経過時間
		void Update(float deltaTime) override;

		/// @brief 追跡対象のPlayerを設定
		/// @param player 追跡対象のPlayer
		void SetTargetPlayer(Player::Base* player) { targetPlayer_ = player; }

		/// @brief Collider更新後に地形との接触状態を解決
		void ResolveAfterCollision();

		/// @brief EnemyのColliderをDebugLineへ登録
		void DrawDebugLine() const;

		/// @brief Playerと接触しているか判定
		/// @return Playerと接触していればtrue
		bool IsHitPlayer() const;

		/// @brief Playerに追従するEnemy削除範囲内に存在するか判定
		/// @return Enemy削除範囲内に存在する場合はtrue
		bool IsInsidePlayerDeleteRange() const;

		/// @brief Playerとの接触ダメージを解決
		/// @param player 接触ダメージを受けるPlayer
		/// @return ダメージを適用した場合はtrue
		bool ResolvePlayerCollision(Player::Base& player);

		/// @brief Projectileからのダメージを適用
		/// @param projectileId Projectileの識別番号
		/// @param damage 適用するダメージ量
		/// @return 実際に適用されたダメージと死亡状態
		ProjectileDamageResult TakeProjectileDamage(std::uint64_t projectileId, float damage);

		/// @brief Enemyを理由に応じた死亡状態へ移行
		/// @param reason Enemyが死亡状態へ移行した理由
		void Kill(DeathReason reason = DeathReason::Defeated);

		/// @brief Enemyの有効状態を取得
		/// @return 有効であればtrue
		bool IsActive() const { return isActive_; }

		/// @brief Enemyが地中からの出現中か判定
		/// @return 地中からの出現中であればtrue
		bool IsEmerging() const { return isEmerging_; }

		/// @brief Enemyの識別番号を取得
		/// @return Enemyの識別番号
		std::uint32_t GetEnemyId() const { return enemyId_; }

		/// @brief Enemyの種類を取得
		/// @return Enemyの種類
		Data::Type GetType() const { return type_; }

		/// @brief Enemyのボーナス種類を取得
		/// @return Enemyのボーナス種類
		Data::BonusType GetBonusType() const { return bonusType_; }

		/// @brief Enemyの現在座標を取得
		/// @return Enemyの現在座標
		Vector3 GetPosition() const { return transform_.translate; }

		/// @brief Enemyの当たり判定用Collider名を取得
		/// @return Enemyの当たり判定用Collider名
		const std::string& GetHitColliderName() const { return hitColliderName_; }

		/// @brief Enemyの現在HPを取得
		/// @return Enemyの現在HP
		float GetCurrentHealth() const { return status_.currentHealth; }

		/// @brief Playerへ与えるダメージ量を取得
		/// @return Playerへ与えるダメージ量
		float GetPower() const { return status_.power; }

	protected:
		/// @brief Enemy種類固有の初期状態を設定
		virtual void OnInitialized() {}

		/// @brief Enemy種類固有の行動を更新
		/// @param deltaTime 前フレームからの経過時間
		virtual void UpdateBehavior(float deltaTime) = 0;

		/// @brief 使用するModelアセット名を取得
		/// @return Modelアセット名
		virtual std::string GetModelAssetName() const = 0;

		/// @brief Modelの表示倍率を取得
		/// @return Modelの表示倍率
		virtual Vector3 GetModelScale() const = 0;

		/// @brief Transform原点からModel原点への補正量を取得
		/// @return Model原点の補正量
		virtual Vector3 GetModelOffset() const = 0;

		/// @brief 移動解決用Sphereを作成
		/// @return 移動解決用Sphere
		virtual Sphere CreateMovementCollider() const = 0;

		/// @brief 被弾判定用AABBを作成
		/// @return 被弾判定用AABB
		virtual AABB CreateHitCollider() const = 0;

		/// @brief Player接触時にEnemyを消滅させるか判定
		/// @return Player接触時に消滅させる場合はtrue
		virtual bool ShouldDisappearOnPlayerCollision() const = 0;

		/// @brief Playerへ接触ダメージを再適用できるまでの時間を取得
		/// @return 接触ダメージの待機時間
		virtual float GetPlayerDamageInterval() const { return 0.5f; }

		/// @brief 指定座標へ向かう移動と重力を更新
		/// @param deltaTime 前フレームからの経過時間
		/// @param targetPosition 移動目標のワールド座標
		/// @param speedMultiplier 基礎移動速度へ適用する倍率
		/// @return EnemyがMap内に存在していればtrue
		bool MoveTowardPosition(float deltaTime, const Vector3& targetPosition, float speedMultiplier = 1.0f);

		/// @brief Playerへ向かう移動と重力を更新
		/// @param deltaTime 前フレームからの経過時間
		/// @param speedMultiplier 基礎移動速度へ適用する倍率
		/// @return EnemyがMap内に存在していればtrue
		bool MoveTowardPlayer(float deltaTime, float speedMultiplier = 1.0f);

		/// @brief 追跡対象Playerの座標を取得
		/// @return Playerの座標、未設定の場合はEnemyの現在座標
		Vector3 GetTargetPlayerPosition() const;

	private:
		/// @brief Projectileごとの再ダメージ待機時間を更新
		/// @param deltaTime 前フレームからの経過時間
		void UpdateProjectileDamageCooldowns(float deltaTime);

		/// @brief 被ダメージ時の白色点滅を開始
		void StartDamageFlash();

		/// @brief 被ダメージEffect Sequenceを現在座標に再生
		void PlayDamageEffect() const;

		/// @brief 経過時間に応じて敵モデルの表示色を更新
		/// @param deltaTime 前フレームからの経過時間
		void UpdateAppearance(float deltaTime);

		/// @brief 死亡報酬を生成
		void SpawnDeathReward();

		/// @brief 地中から地表面までの出現移動を更新
		/// @param deltaTime 前フレームからの経過時間
		void UpdateEmergence(float deltaTime);

		/// @brief 移動用と被弾用のColliderを登録
		void RegisterColliders();

		/// @brief Modelへ現在のTransformを反映
		void ApplyModelTransform();

		/// @brief ColliderとModelを破棄
		void Release();

		/// @brief Collider登録名を作成
		/// @param prefix 登録名の先頭文字列
		/// @return Collider登録名
		std::string CreateColliderName(const std::string& prefix) const;

		/// @brief Model登録名を作成
		/// @return Model登録名
		std::string CreateModelName() const;

		std::uint32_t enemyId_ = 0;
		Data::Status status_;
		Data::Type type_ = Data::Type::Normal;
		Data::BonusType bonusType_ = Data::BonusType::None;
		SceneType sceneType_ = SceneType::None;
		Movement movement_;
		ColliderShape hitAABB_;
		Player::Base* targetPlayer_ = nullptr;
		std::string movementColliderName_;
		std::string hitColliderName_;
		std::string modelName_;
		float projectileDamageInterval_ = 0.5f; // Projectileからのダメージを受ける間隔（秒）
		std::unordered_map<std::uint64_t, float> projectileDamageCooldowns_;
		float playerDamageCooldown_ = 0.0f;
		float damageFlashRemainingTime_ = 0.0f;
		float emergenceTargetY_ = 0.0f;
		bool isActive_ = true;
		bool isEmerging_ = false;
		bool areCollidersRegistered_ = false;
		bool isDeathRewardSpawned_ = false;
		bool isReleased_ = false;
		GamingColor gamingColor_;
	};
} // namespace Enemy
