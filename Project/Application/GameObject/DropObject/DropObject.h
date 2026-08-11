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

		/// @brief DropObjectの種類に応じた報酬をPlayerへ渡して取得済み状態へ変更
		/// @param player 報酬の受取先Player
		void Collect(Player::Base& player);

		Type type_ = Type::Exp;
		bool isMoving_ = false;
		bool isAlive_ = true;
		bool isReleased_ = false;

		Vector3 targetPosition_ = { 0.0f, 0.0f, 0.0f };
		std::string colliderName_;
		std::string modelName_;

		GameTimer backTimer_;
	};
}
