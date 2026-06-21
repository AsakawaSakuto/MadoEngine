#pragma once
#include "../../IGameObject.h"
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

	std::string colliderName_;
	bool isHighlighted_ = false;
};
