#pragma once
#include "../MapEventObjectBase.h"
#include "JarType.h"

class Jar : public MapEventObjectBase {
public:
	struct InitializeDesc {
		Vector3 position = { 0.0f, 0.0f, 0.0f };
		Vector3 rotation = { 0.0f, 0.0f, 0.0f };
		std::string modelName = "Jar";
		std::string colliderName = "JarAABB";
	};

	~Jar();

	/// @brief 設定付きでJarを初期化します。
	/// @param desc 初期化に使用する設定です。
	void Initialize(const InitializeDesc& desc);

	/// @brief Jarを更新します。
	/// @param deltaTime 前フレームからの経過時間です。
	void Update(float deltaTime) override;

	/// @brief Jarを取得した時の処理を実行します。
	/// @param player 取得するPlayerです。
	/// @return JarをMapから削除するためtrueを返します。
	bool Interact(Player& player) override;

private:
	JarType type_ = JarType::Money;
	JarSize size_ = JarSize::Small;
	std::string modelName_ = "Jar";
};
