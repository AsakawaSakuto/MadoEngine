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

	/// @brief Enemy生成時に使用する初期化情報
	struct SpawnDesc {
		Vector3 position = { 0.0f, 0.0f, 0.0f };
		Data::Status status;
		Data::Type type = Data::Type::Normal;
		SceneType sceneType = SceneType::None;
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

		/// @brief Projectileからのダメージを適用
		/// @param projectileId Projectileの識別番号
		/// @param damage 適用するダメージ量
		/// @return 実際に適用されたダメージと死亡状態
		ProjectileDamageResult TakeProjectileDamage(std::uint64_t projectileId, float damage);

		/// @brief Enemyを死亡状態にして報酬を生成
		void Kill();

		/// @brief Enemyの有効状態を取得
		/// @return 有効であればtrue
		bool IsActive() const { return isActive_; }

		/// @brief Enemyの識別番号を取得
		/// @return Enemyの識別番号
		std::uint32_t GetEnemyId() const { return enemyId_; }

		/// @brief Enemyの種類を取得
		/// @return Enemyの種類
		Data::Type GetType() const { return type_; }

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

	private:
		/// @brief Projectileごとの再ダメージ待機時間を更新
		/// @param deltaTime 前フレームからの経過時間
		void UpdateProjectileDamageCooldowns(float deltaTime);

		/// @brief 被ダメージ時の白色点滅を開始
		void StartDamageFlash();

		/// @brief 経過時間に応じて敵モデルの表示色を更新
		/// @param deltaTime 前フレームからの経過時間
		void UpdateAppearance(float deltaTime);

		/// @brief 死亡報酬を生成
		void SpawnDeathReward();

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
		Movement movement_;
		ColliderShape hitAABB_;
		Player::Base* targetPlayer_ = nullptr;
		std::string movementColliderName_;
		std::string hitColliderName_;
		std::string modelName_;
		float projectileDamageInterval_ = 0.5f; // Projectileからのダメージを受ける間隔（秒）
		std::unordered_map<std::uint64_t, float> projectileDamageCooldowns_;
		float damageFlashRemainingTime_ = 0.0f;
		bool isActive_ = true;
		bool isDeathRewardSpawned_ = false;
		bool isReleased_ = false;
		GamingColor gamingColor_;
	};
} // namespace Enemy
