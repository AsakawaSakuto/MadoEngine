#pragma once
#include "SpriteManager.h"

namespace MySprite {

	/// @brief 描画レイヤーを指定してSpriteを作成する
	/// @param name Spriteの識別名
	/// @param textureName 使用するテクスチャ名
	/// @param sceneType 描画を許可するシーンの種類
	/// @param layer 設定する描画レイヤー
	/// @return 生成したSpriteのポインタ（所有権はSpriteManagerが持つ）
	inline Sprite* Create(
		const std::string& name,
		const std::string& textureName,
		SceneType sceneType,
		MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::Default)
	{
		Sprite* sprite = MadoEngine::SpriteManager::GetInstance().Create(name, textureName, sceneType);
		if (sprite) {
			sprite->SetRenderLayer(layer);
		}
		return sprite;
	}

	/// @brief 識別名でSpriteを取得する
	/// @param name Spriteの識別名
	/// @return Spriteのポインタ（存在しない場合はnullptr）
	inline Sprite* Get(const std::string& name) {
		return MadoEngine::SpriteManager::GetInstance().Get(name);
	}

	/// @brief 指定したSpriteを破棄する
	/// @param name Spriteの識別名
	inline void Destroy(const std::string& name) {
		MadoEngine::SpriteManager::GetInstance().Destroy(name);
	}

	/// @brief 指定したシーンに属するSpriteインスタンスをすべて破棄する
	/// @param sceneType 破棄対象のシーン種別
	inline void DestroyByScene(SceneType sceneType) {
		MadoEngine::SpriteManager::GetInstance().DestroyByScene(sceneType);
	}
};
