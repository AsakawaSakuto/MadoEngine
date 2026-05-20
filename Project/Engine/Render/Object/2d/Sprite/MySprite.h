#pragma once
#include "SpriteManager.h"

namespace MySprite {

	/// @brief Spriteを生成して管理下に登録する
	/// @param name Spriteの識別名
	/// @param textureName 使用するテクスチャ名
	/// @param sceneName 描画を許可するシーン名（空文字の場合は全シーンで描画）
	/// @return 生成したSpriteのポインタ（所有権はSpriteManagerが持つ）
	inline Sprite* Create(const std::string& name, const std::string& textureName, const std::string& sceneName) {
		return MadoEngine::SpriteManager::GetInstance()->Create(name, textureName, sceneName);
	};

	/// @brief 識別名でSpriteを取得する
	/// @param name Spriteの識別名
	/// @return Spriteのポインタ（存在しない場合はnullptr）
	inline Sprite* Get(const std::string& name) {
		return MadoEngine::SpriteManager::GetInstance()->Get(name);
	}

	/// @brief 指定したSpriteを破棄する
	/// @param name Spriteの識別名
	inline void Destroy(const std::string& name) {
		MadoEngine::SpriteManager::GetInstance()->Destroy(name);
	}
};