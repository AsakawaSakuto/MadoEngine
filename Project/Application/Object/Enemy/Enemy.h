#pragma once
#include "../IGameObject.h"
#include <cstdint>
#include <string>

namespace Player {
	class Base;
}

class Enemy : public IGameObject {
public:
	/// @brief Enemyを破棄します。
	~Enemy() override;

	/// @brief Enemyを初期化します。
	/// @param enemyId Enemyの識別番号です。
	/// @param spawnPosition 生成位置です。
	/// @param sceneType 所属するシーン種別です。
	void Initialize(uint32_t enemyId, const Vector3& spawnPosition, SceneType sceneType);

	/// @brief Enemyの移動処理を更新します。
	/// @param deltaTime デルタタイムです。
	void Update(float deltaTime) override;

	/// @brief Playerを追跡するための参照先を設定します。
	/// @param player 追跡対象のPlayerです。
	void SetTargetPlayer(Player::Base* player) { targetPlayer_ = player; }

	/// @brief 地形との当たり判定後にEnemyの状態を更新します。
	void ResolveAfterCollision();

	/// @brief PlayerのAABBと接触しているか判定します。
	/// @return Playerと接触していればtrueを返します。
	bool IsHitPlayer() const;

	/// @brief Enemyの有効状態を取得します。
	/// @return 有効であればtrueを返します。
	bool IsActive() const { return isActive_; }

	/// @brief Enemyを削除対象にします。
	void Kill() { isActive_ = false; }

private:
	/// @brief ColliderとModelを破棄します。
	void Release();

	/// @brief Collider登録名を作成します。
	/// @param prefix 登録名の先頭文字列です。
	/// @return Collider登録名を返します。
	std::string CreateColliderName(const std::string& prefix) const;

	/// @brief Model登録名を作成します。
	/// @return Model登録名を返します。
	std::string CreateModelName() const;

	/// @brief Playerへ向かう水平方向を取得します。
	/// @return 正規化済みの水平方向です。
	Vector3 GetDirectionToPlayer() const;

	/// @brief 側面で詰まったEnemyを少しずつ上へ補助します。
	/// @param deltaTime 1フレームの経過時間です。
	/// @param isGroundContact AABB地面に接地していればtrueです。
	/// @param isSlopeGroundContact Slope地面に接地していればtrueです。
	void ApplySideClimbAssist(float deltaTime, bool isGroundContact, bool isSlopeGroundContact);

	/// @brief 側面上昇補助の状態を初期化します。
	void ResetSideClimbAssist();

	/// @brief 今回の移動が側面で止められたか判定します。
	/// @return 側面で前進量が落ちていればtrueを返します。
	bool IsSideBlockedThisFrame() const;

	/// @brief Modelの座標と回転を反映します。
	void UpdateModelTransform();

	uint32_t enemyId_ = 0;
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
	Vector3 lastMoveStartPosition_ = { 0.0f, 0.0f, 0.0f };
	Vector3 lastDesiredHorizontalMove_ = { 0.0f, 0.0f, 0.0f };
	bool isGrounded_ = false;
	bool isSideClimbing_ = false;
	bool isActive_ = true;
	bool isReleased_ = false;
};
