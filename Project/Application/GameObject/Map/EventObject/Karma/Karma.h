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

	/// @brief 設定付きでKarmaを初期化
	/// @param desc 初期化に使用する設定
	void Initialize(const InitializeDesc& desc);

	/// @brief Karmaを更新
	/// @param deltaTime 前フレームからの経過時間
	void Update(float deltaTime) override;

	/// @brief Karmaに相互作用した時の処理を実行
	/// @param player 相互作用したPlayer
	/// @return KarmaをMapから削除するためtrue
	bool Interact(Player::Base& player) override;

	/// @brief Karmaの操作案内文を取得
	/// @return 操作案内に表示するUTF-8文字列
	std::string_view GetInteractionText() const override;
};
