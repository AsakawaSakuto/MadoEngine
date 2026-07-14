#pragma once
#include "../MapEventObjectBase.h"
#include "JarType.h"

class Jar : public MapEventObjectBase {
public:
	struct InitializeDesc {
		Vector3 position = { 0.0f, 0.0f, 0.0f };
		Vector3 rotation = { 0.0f, 0.0f, 0.0f };
		JarType type = JarType::Money;
		JarSize size = JarSize::Small;
		std::string modelName = "Jar";
		std::string colliderName = "JarAABB";
	};

	~Jar();

	/// @brief 設定付きでJarを初期化
	/// @param desc 初期化に使用する設定
	void Initialize(const InitializeDesc& desc);

	/// @brief Jarを更新
	/// @param deltaTime 前フレームからの経過時間
	void Update(float deltaTime) override;

	/// @brief Jarを取得した時の処理を実行
	/// @param player 取得するPlayer
	/// @return JarをMapから削除するためtrueを返す
	bool Interact(Player::Base& player) override;

private:
	JarType type_ = JarType::Money;
	JarSize size_ = JarSize::Small;
	std::string modelName_ = "Jar";
};
