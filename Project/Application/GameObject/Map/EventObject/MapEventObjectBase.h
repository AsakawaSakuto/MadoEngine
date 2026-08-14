#pragma once
#include "../../IGameObject.h"
#include <cstdint>
#include <string>

namespace Player {
	class Base;
}

/// @brief Map上でPlayerと相互作用できるイベントオブジェクトの基底クラスです。
class MapEventObjectBase : public IGameObject {
public:
	virtual ~MapEventObjectBase() = default;

	/// @brief Playerの当たり判定と衝突しているか判定
	/// @return Playerと衝突していればtrue
	bool IsHitPlayer() const;

	/// @brief Objectのワールド座標を取得
	/// @return Objectのワールド座標
	Vector3 GetPosition() const;

	/// @brief Objectの回転を取得
	/// @return Objectの回転
	Vector3 GetRotation() const;

	/// @brief Player接触時の強調表示状態を設定
	/// @param isHighlighted 強調表示する場合はtrue
	void SetHighlighted(bool isHighlighted);

	/// @brief Playerが相互作用した時の処理を実行
	/// @param player 相互作用するPlayer
	/// @return 相互作用後にMapから削除する場合はtrue
	virtual bool Interact(Player::Base& player) = 0;

protected:
	/// @brief Collider登録名を設定
	/// @param colliderName Collider登録名
	void SetColliderName(const std::string& colliderName);

	/// @brief インスタンス描画用の通常表示と強調表示を登録
	/// @param normalModel 通常表示に使用するインスタンス描画モデル
	/// @param normalHandle 通常表示のインスタンスハンドル
	/// @param outlineModel 強調表示に使用するインスタンス描画モデル
	/// @param outlineHandle 強調表示のインスタンスハンドル
	void SetInstancedDraw(
		MadoEngine::InstancedModelHandle normalModel,
		uint32_t normalHandle,
		MadoEngine::InstancedModelHandle outlineModel,
		uint32_t outlineHandle);

	/// @brief インスタンス描画の表示を解除
	void HideInstancedDraw();

	std::string colliderName_;
	bool isHighlighted_ = false;
	MadoEngine::InstancedModelHandle normalInstancedModel_{};
	MadoEngine::InstancedModelHandle outlineInstancedModel_{};
	uint32_t normalInstanceHandle_ = UINT32_MAX;
	uint32_t outlineInstanceHandle_ = UINT32_MAX;
};
