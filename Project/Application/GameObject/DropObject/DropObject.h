#pragma once
#include "../IGameObject.h"
#include "DropObjectStatus.h"
#include <string>

namespace Player {
	class Base;
}

namespace DropObject {
	
	class Base : public IGameObject {
	public:

		/// @brief DropObjectを破棄
		~Base() override;
		
		/// @brief DropObjectを初期化
		/// @param type DropObjectの種類
		/// @param position 生成位置
		/// @param index 識別番号
		void Initialize(Type type, const Vector3& position, int index);

		/// @brief 確保済みのModelとColliderを再利用して配置
		/// @param position 配置先のワールド座標
		void Activate(const Vector3& position);

		/// @brief ModelとColliderを保持したまま回収と表示を停止
		void Deactivate();

		/// @brief Poolで管理するDrop一個分の報酬量を取得
		/// @return Expの場合は経験値量、Moneyの場合は金額、それ以外の場合は0
		int GetRewardAmount() const;

		/// @brief 回収成立による未処理の報酬量を取得
		/// @return 回収した経験値量または金額、未回収または処理済みの場合は0
		int ConsumeCollectedRewardAmount();

		/// @brief DropObjectを更新
		/// @param deltaTime 1フレームの経過時間
		void Update(float deltaTime) override;

		/// @brief Playerとの衝突判定を含めてDropObjectを更新
		/// @param deltaTime 1フレームの経過時間
		/// @param player 報酬の受取先Player
		void Update(float deltaTime, Player::Base& player);

		/// @brief 移動状態を設定
		/// @param isMoving TargetPositionへ移動する場合はtrue
		void SetMoving(bool isMoving) { isMoving_ = isMoving; }

		/// @brief 移動先座標を設定
		/// @param targetPosition 移動先座標
		void SetTargetPosition(const Vector3& targetPosition) { targetPosition_ = targetPosition; }

		/// @brief 生存状態を取得
		/// @return 生存中であればtrue
		bool IsAlive() const { return isAlive_; }

	private:

		/// @brief ColliderとModelを破棄
		void Release();

		/// @brief Player参照を指定してDropObjectを更新
		/// @param deltaTime 1フレームの経過時間
		/// @param player 報酬の受取先Player、nullptrの場合は報酬加算なし
		void UpdateInternal(float deltaTime, Player::Base* player);

		/// @brief Pool報酬の回収通知またはPlayerの回復を確定して取得済み状態へ変更
		/// @param player 報酬の受取先Player
		void Collect(Player::Base& player);

		Type type_ = Type::Exp;
		bool isMoving_ = false;
		bool isAlive_ = false;
		bool isReleased_ = false;
		int collectedRewardAmount_ = 0;

		Vector3 targetPosition_ = { 0.0f, 0.0f, 0.0f };
		std::string colliderName_;
		std::string modelName_;

		GameTimer backTimer_;
	};
}
