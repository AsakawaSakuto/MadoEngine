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

	/// @brief 設定付きでChestを初期化します。
	/// @param desc 初期化に使用する設定です。
	void Initialize(const InitializeDesc& desc);

	/// @brief Chestを更新します。
	/// @param deltaTime 前フレームからの経過時間です。
	void Update(float deltaTime) override;

	/// @brief Chestに相互作用した時の処理を実行します。
	/// @param player 相互作用したPlayerです。
	/// @return ChestをMapから削除するためtrueを返します。
	bool Interact(Player::Base& player) override;

private:
	ChestType type_ = ChestType::Normal;
	std::string modelName_ = "Chest";
};
