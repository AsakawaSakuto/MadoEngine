#pragma once
#include "../../IGameObject.h"
#include <cstdint>
#include <string>
#include <string_view>

namespace Player {
	class Base;
}

/// @brief Mapイベント相互作用からGameへ要求する処理種別
enum class MapEventAction {
	None,
	SpawnBoss,
};

/// @brief Mapイベント相互作用によって発生した処理要求
struct MapEventRequest {
	MapEventAction action = MapEventAction::None;
	Vector3 position = { 0.0f, 0.0f, 0.0f };
};

/// @brief Map上でPlayerと相互作用できるイベントオブジェクトの基底クラス
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

	/// @brief 現在のPlayer状態で相互作用可能か判定
	/// @param player 相互作用するPlayer
	/// @return 相互作用可能な場合はtrue
	virtual bool CanInteract(const Player::Base& player) const {
		(void)player;
		return true;
	}

	/// @brief Player接触中に表示する操作案内文を取得
	/// @return 操作案内に表示するUTF-8文字列
	virtual std::string_view GetInteractionText() const = 0;

	/// @brief 相互作用成立時にGameへ渡す処理要求を取得
	/// @return 相互作用によって発生する処理要求
	virtual MapEventRequest GetInteractionRequest() const { return {}; }

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
