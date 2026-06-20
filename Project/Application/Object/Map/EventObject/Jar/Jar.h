#pragma once
#include "../../../IGameObject.h"
#include "JarType.h"

class Jar : public IGameObject {
public:
	struct InitializeDesc {
		Vector3 position = { 0.0f, 0.0f, 0.0f };
		Vector3 rotation = { 0.0f, 0.0f, 0.0f };
		std::string modelName = "Jar";
		std::string colliderName = "JarAABB";
	};

	~Jar();
	
	/// @brief Jarを初期化します。
	void Initialize() override;

	/// @brief Jarを設定付きで初期化します。
	/// @param desc 初期化に使用する設定です。
	void Initialize(const InitializeDesc& desc);

	/// @brief Jarを更新します。
	/// @param deltaTime 前フレームからの経過時間です。
	void Update(float deltaTime) override;

	/// @brief Playerの当たり判定と衝突しているか判定します。
	/// @return Playerと衝突していればtrueです。
	bool IsHitPlayer() const;

private:
	JarType type_ = JarType::Money;
	JarSize size_ = JarSize::Small;
	std::string modelName_ = "Jar";
	std::string colliderName_ = "JarAABB";
};
