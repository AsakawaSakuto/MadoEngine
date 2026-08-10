#pragma once

#include "SpriteManager.h"

namespace MySprite {

/// @brief 描画Layerを指定してSpriteを生成
/// @param name Sprite名
/// @param textureName テクスチャ名
/// @param sceneType 所属Scene
/// @param managementMode 管理方式
/// @param layer 描画Layer
/// @return 生成したSpriteのHandle、失敗した場合は無効Handle
[[nodiscard]] inline MadoEngine::SpriteHandle Create(
	const std::string& name,
	const std::string& textureName,
	SceneType sceneType,
	MadoEngine::EditorManagementMode managementMode = MadoEngine::EditorManagementMode::RuntimeOnly,
	MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::Default) {
	MadoEngine::SpriteManager& manager = MadoEngine::SpriteManager::GetInstance();
	const MadoEngine::SpriteHandle handle = manager.Create(name, textureName, sceneType, managementMode);
	if (Sprite* sprite = manager.TryGet(handle)) {
		sprite->SetRenderLayer(layer);
	}
	return handle;
}

/// @brief 描画Layerを指定して実行時専用Spriteを生成
/// @param name Sprite名
/// @param textureName テクスチャ名
/// @param sceneType 所属Scene
/// @param layer 描画Layer
/// @return 生成したSpriteのHandle、失敗した場合は無効Handle
[[nodiscard]] inline MadoEngine::SpriteHandle Create(
	const std::string& name,
	const std::string& textureName,
	SceneType sceneType,
	MadoEngine::Render::RenderLayer layer) {
	return Create(name, textureName, sceneType, MadoEngine::EditorManagementMode::RuntimeOnly, layer);
}

/// @brief 名前からSpriteのHandleを取得
/// @param name Sprite名
/// @return 見つかったSpriteのHandle、見つからない場合は無効Handle
[[nodiscard]] inline MadoEngine::SpriteHandle Find(const std::string& name) {
	return MadoEngine::SpriteManager::GetInstance().Find(name);
}

/// @brief 名前からSpriteのHandleを取得する互換API
/// @param name Sprite名
/// @return 見つかったSpriteのHandle、見つからない場合は無効Handle
[[nodiscard]] inline MadoEngine::SpriteHandle Get(const std::string& name) {
	return Find(name);
}

/// @brief HandleからSpriteを一時参照として取得
/// @param handle SpriteのHandle
/// @return 有効な場合はSprite、無効な場合はnullptr
inline Sprite* TryGet(MadoEngine::SpriteHandle handle) {
	return MadoEngine::SpriteManager::GetInstance().TryGet(handle);
}

/// @brief Handleを指定してSpriteを即時削除
/// @param handle 削除対象のHandle
inline void Destroy(MadoEngine::SpriteHandle handle) {
	MadoEngine::SpriteManager::GetInstance().Destroy(handle);
}

/// @brief 名前を指定してSpriteを即時削除する互換API
/// @param name 削除対象の名前
inline void Destroy(const std::string& name) {
	MadoEngine::SpriteManager::GetInstance().Destroy(name);
}

/// @brief 指定SceneのSpriteを一括削除する互換API
/// @param sceneType 削除対象のScene
inline void DestroyByScene(SceneType sceneType) {
	MadoEngine::SpriteManager::GetInstance().DestroyByScene(sceneType);
}

} // namespace MySprite
