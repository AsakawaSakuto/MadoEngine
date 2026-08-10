#pragma once

#include "TextManager.h"

namespace MyText {

/// @brief 表示内容と描画Layerを指定してTextを生成
/// @param name Text名
/// @param text 表示するUTF-8文字列
/// @param sceneType 所属Scene
/// @param managementMode 管理方式
/// @param layer 描画Layer
/// @return 生成したTextのHandle、失敗した場合は無効Handle
[[nodiscard]] inline MadoEngine::TextHandle Create(
	const std::string& name,
	const std::string& text,
	SceneType sceneType = SceneType::None,
	MadoEngine::EditorManagementMode managementMode = MadoEngine::EditorManagementMode::RuntimeOnly,
	MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::Default) {
	MadoEngine::TextManager& manager = MadoEngine::TextManager::GetInstance();
	const MadoEngine::TextHandle handle = manager.Create(name, sceneType, managementMode);
	if (MadoEngine::Text* created = manager.TryGet(handle)) {
		created->SetText(text);
		created->SetRenderLayer(layer);
	}
	return handle;
}

/// @brief 描画Layerを指定して実行時専用Textを生成
/// @param name Text名
/// @param text 表示するUTF-8文字列
/// @param sceneType 所属Scene
/// @param layer 描画Layer
/// @return 生成したTextのHandle、失敗した場合は無効Handle
[[nodiscard]] inline MadoEngine::TextHandle Create(
	const std::string& name,
	const std::string& text,
	SceneType sceneType,
	MadoEngine::Render::RenderLayer layer) {
	return Create(name, text, sceneType, MadoEngine::EditorManagementMode::RuntimeOnly, layer);
}

/// @brief 名前からTextのHandleを取得
/// @param name Text名
/// @return 見つかったTextのHandle、見つからない場合は無効Handle
[[nodiscard]] inline MadoEngine::TextHandle Find(const std::string& name) {
	return MadoEngine::TextManager::GetInstance().Find(name);
}

/// @brief 名前からTextのHandleを取得する互換API
/// @param name Text名
/// @return 見つかったTextのHandle、見つからない場合は無効Handle
[[nodiscard]] inline MadoEngine::TextHandle Get(const std::string& name) {
	return Find(name);
}

/// @brief HandleからTextを一時参照として取得
/// @param handle TextのHandle
/// @return 有効な場合はText、無効な場合はnullptr
inline MadoEngine::Text* TryGet(MadoEngine::TextHandle handle) {
	return MadoEngine::TextManager::GetInstance().TryGet(handle);
}

/// @brief Handleを指定してTextを即時削除
/// @param handle 削除対象のHandle
inline void Destroy(MadoEngine::TextHandle handle) {
	MadoEngine::TextManager::GetInstance().Destroy(handle);
}

/// @brief 名前を指定してTextを即時削除する互換API
/// @param name 削除対象の名前
inline void Destroy(const std::string& name) {
	MadoEngine::TextManager::GetInstance().Destroy(name);
}

/// @brief 指定SceneのTextを一括削除する互換API
/// @param sceneType 削除対象のScene
inline void DestroyByScene(SceneType sceneType) {
	MadoEngine::TextManager::GetInstance().DestroyByScene(sceneType);
}

} // namespace MyText
