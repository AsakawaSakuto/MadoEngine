#pragma once
#include "../MapEventObjectBase.h"

class Karma : public MapEventObjectBase {
public:
	struct InitializeDesc {
		Vector3 position = { 0.0f, 0.0f, 0.0f };
		Vector3 rotation = { 0.0f, 0.0f, 0.0f };
		std::string colliderName = "KarmaAABB";
	};

	~Karma();

	/// @brief 設定付きでKarmaを初期化します。
	/// @param desc 初期化に使用する設定です。
	void Initialize(const InitializeDesc& desc);

	/// @brief Karmaを更新します。
	/// @param deltaTime 前フレームからの経過時間です。
	void Update(float deltaTime) override;

	/// @brief Karmaに相互作用した時の処理を実行します。
	/// @param player 相互作用したPlayerです。
	/// @return KarmaをMapから削除するためtrueを返します。
	bool Interact(Player::Base& player) override;
};
