#pragma once
#include "../../IGameObject.h"
#include <cstdint>
#include <string>

class Player;

/// @brief Map上でPlayerと相互作用できるイベントオブジェクトの基底クラスです。
class MapEventObjectBase : public IGameObject {
public:
	virtual ~MapEventObjectBase() = default;

	/// @brief Playerの当たり判定と衝突しているか判定します。
	/// @return Playerと衝突していればtrueを返します。
	bool IsHitPlayer() const;

	/// @brief Player接触時の強調表示状態を設定します。
	/// @param isHighlighted 強調表示する場合はtrueです。
	void SetHighlighted(bool isHighlighted);

	/// @brief Playerが相互作用した時の処理を実行します。
	/// @param player 相互作用するPlayerです。
	/// @return 相互作用後にMapから削除する場合はtrueを返します。
	virtual bool Interact(Player& player) = 0;

protected:
	/// @brief Collider登録名を設定します。
	/// @param colliderName Collider登録名です。
	void SetColliderName(const std::string& colliderName);

	/// @brief インスタンス描画用の通常表示と強調表示を登録します。
	/// @param normalModel 通常表示に使用するインスタンス描画モデルです。
	/// @param normalHandle 通常表示のインスタンスハンドルです。
	/// @param outlineModel 強調表示に使用するインスタンス描画モデルです。
	/// @param outlineHandle 強調表示のインスタンスハンドルです。
	void SetInstancedDraw(
		InstancedModel* normalModel,
		uint32_t normalHandle,
		InstancedModel* outlineModel,
		uint32_t outlineHandle);

	/// @brief インスタンス描画の表示を解除します。
	void HideInstancedDraw();

	std::string colliderName_;
	bool isHighlighted_ = false;
	InstancedModel* normalInstancedModel_ = nullptr;
	InstancedModel* outlineInstancedModel_ = nullptr;
	uint32_t normalInstanceHandle_ = UINT32_MAX;
	uint32_t outlineInstanceHandle_ = UINT32_MAX;
};
