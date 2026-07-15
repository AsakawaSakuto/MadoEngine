#pragma once
#include "../IGameObject.h"
#include "EnemyStatus.h"
#include <cstdint>
#include <string>
#include <unordered_map>

namespace Player {
	class Base;
}

class Enemy : public IGameObject {
public:
	/// @brief Enemyを破棄
	~Enemy() override;

	/// @brief Enemyを初期化
	/// @param enemyId Enemyの識別番号
	/// @param spawnPosition 生成位置
	/// @param sceneType 所属するシーン種別
	void Initialize(uint32_t enemyId, const Vector3& spawnPosition, SceneType sceneType);

	/// @brief Enemyの移動処理を更新
	/// @param deltaTime デルタタイム
	void Update(float deltaTime) override;

	/// @brief Playerを追跡するための参照先を設定
	/// @param player 追跡対象のPlayer
	void SetTargetPlayer(Player::Base* player) { targetPlayer_ = player; }

	/// @brief 地形との当たり判定後にEnemyの状態を更新
	void ResolveAfterCollision();

	/// @brief PlayerのAABBと接触しているか判定する
	/// @return Playerと接触していればtrueを返す
	bool IsHitPlayer() const;

	/// @brief Projectileからのダメージを適用
	/// @param projectileId Projectileの識別番号
	/// @param damage 適用するダメージ量
	/// @return このダメージでEnemyが倒された場合はtrueを返す
	bool TakeProjectileDamage(std::uint64_t projectileId, float damage);

	/// @brief Enemyの有効状態を取得
	/// @return 有効であればtrueを返す
	bool IsActive() const { return isActive_; }

	/// @brief Enemyの現在座標を取得
	/// @return Enemyの現在座標
	Vector3 GetPosition() const { return transform_.translate; }

	/// @brief Enemyの当たり判定用コライダー名を取得
	/// @return Enemyの当たり判定用コライダー名
	const std::string& GetHitColliderName() const { return hitColliderName_; }

	/// @brief Enemyの現在HPを取得
	/// @return Enemyの現在HP
	float GetCurrentHealth() const { return status_.currentHealth; }

	/// @brief Enemyを削除対象にする
	void Kill() { isActive_ = false; }

private:
	/// @brief Projectileごとの再ダメージ待機時間を更新
	/// @param deltaTime 前フレームからの経過時間
	void UpdateProjectileDamageCooldowns(float deltaTime);

	/// @brief ColliderとModelを破棄する
	void Release();

	/// @brief Collider登録名を作成
	/// @param prefix 登録名の先頭文字列
	/// @return Collider登録名を返す
	std::string CreateColliderName(const std::string& prefix) const;

	/// @brief Model登録名を作成
	/// @return Model登録名を返す
	std::string CreateModelName() const;

	/// @brief Playerへ向かう水平方向を取得
	/// @return 正規化済みの水平方向
	Vector3 GetDirectionToPlayer() const;

	/// @brief 側面で詰まったEnemyを少しずつ上へ補助する
	/// @param deltaTime 1フレームの経過時間
	/// @param isGroundContact AABB地面に接地していればtrue
	/// @param isSlopeGroundContact Slope地面に接地していればtrue
	void ApplySideClimbAssist(float deltaTime, bool isGroundContact, bool isSlopeGroundContact);

	/// @brief 側面上昇補助の状態を初期化
	void ResetSideClimbAssist();

	/// @brief 今回の移動が側面で止められたか判定
	/// @return 側面で前進量が落ちていればtrueを返す
	bool IsSideBlockedThisFrame() const;

	/// @brief Modelの座標と回転を接地状態に合わせて反映
	/// @param isSlopeGroundContact Slopeに接地していればtrue
	void UpdateModelTransform(bool isSlopeGroundContact = false);

	uint32_t enemyId_ = 0;
	EnemyData::Status status_;
	ColliderShape hitAABB_;
	Player::Base* targetPlayer_ = nullptr;
	std::string movementColliderName_;
	std::string hitColliderName_;
	std::string modelName_;
	float moveSpeed_ = 3.0f;
	float gravity_ = 30.0f;
	float velocityY_ = 0.0f;
	float lastDeltaTime_ = 0.0f;
	float sideClimbBaseY_ = 0.0f;
	float sideClimbAmount_ = 0.0f;
	float sideClimbCrestTimer_ = 0.0f;
	float faceYaw_ = 0.0f;
	float modelGroundNormalFollowSpeed_ = 16.0f;
	float projectileDamageInterval_ = 0.2f;
	Vector3 lastMoveStartPosition_ = { 0.0f, 0.0f, 0.0f };
	Vector3 lastDesiredHorizontalMove_ = { 0.0f, 0.0f, 0.0f };
	Vector3 currentGroundNormal_ = { 0.0f, 1.0f, 0.0f };
	std::unordered_map<std::uint64_t, float> projectileDamageCooldowns_;
	bool isGrounded_ = false;
	bool isSideClimbing_ = false;
	bool isActive_ = true;
	bool isReleased_ = false;

	GamingColor gamingColor_;
};