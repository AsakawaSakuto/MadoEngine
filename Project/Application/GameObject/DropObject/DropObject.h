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

		/// @brief DropObjectを破棄します
		~Base() override;
		
		/// @brief DropObjectを初期化します
		/// @param type DropObjectの種類です
		/// @param position 生成位置です
		/// @param index 識別番号です
		void Initialize(Type type, const Vector3& position, int index);

		/// @brief DropObjectを更新します
		/// @param deltaTime 1フレームの経過時間です
		void Update(float deltaTime) override;

		/// @brief Playerとの衝突判定を含めてDropObjectを更新します。
		/// @param deltaTime 1フレームの経過時間です。
		/// @param player 経験値を受け取るPlayerです。
		void Update(float deltaTime, Player::Base& player);

		/// @brief 移動状態を設定します
		/// @param isMoving trueの場合、TargetPositionへ向かいます
		void SetMoving(bool isMoving) { isMoving_ = isMoving; }

		/// @brief 移動先座標を設定します
		/// @param targetPosition 移動先座標です
		void SetTargetPosition(const Vector3& targetPosition) { targetPosition_ = targetPosition; }

		/// @brief 生存状態を取得します
		/// @return 生存中であればtrueを返します
		bool IsAlive() const { return isAlive_; }

	private:

		/// @brief ColliderとModelを破棄します
		void Release();

		/// @brief Player参照を指定してDropObjectを更新します。
		/// @param deltaTime 1フレームの経過時間です。
		/// @param player 経験値を受け取るPlayerです。nullptrの場合は経験値加算を行いません。
		void UpdateInternal(float deltaTime, Player::Base* player);

		/// @brief Playerへ経験値を渡してDropObjectを取得済みにします。
		/// @param player 経験値を受け取るPlayerです。
		void CollectExp(Player::Base& player);

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
