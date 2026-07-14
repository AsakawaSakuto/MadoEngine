#pragma once
#include "../MapEventObjectBase.h"
#include "ChestType.h"

namespace Player {
	class Base;
}

class Chest : public MapEventObjectBase {
public:
	struct InitializeDesc {
		Vector3 position = { 0.0f, 0.0f, 0.0f };
		Vector3 rotation = { 0.0f, 0.0f, 0.0f };
		ChestType type = ChestType::Normal;
		std::string modelName = "Chest";
		std::string colliderName = "ChestAABB";
	};

	~Chest();

	/// @brief 設定付きでChestを初期化
	/// @param desc 初期化に使用する設定
	void Initialize(const InitializeDesc& desc);

	/// @brief Chestを更新
	/// @param deltaTime 前フレームからの経過時間
	void Update(float deltaTime) override;

	/// @brief Chestに相互作用した時の処理を実行
	/// @param player 相互作用したPlayer
	/// @return ChestをMapから削除するためtrueを返す
	bool Interact(Player::Base& player) override;

private:
	ChestType type_ = ChestType::Normal;
	std::string modelName_ = "Chest";
};
