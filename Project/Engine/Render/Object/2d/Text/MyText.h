#pragma once
#include "TextManager.h"

namespace MyText {

/// @brief 管理方法と描画レイヤーを指定してTextを作成する
/// @param name Textの識別名
/// @param text 表示するUTF-8文字列
/// @param sceneType 描画対象Scene
/// @param managementMode Textの管理方法
/// @param layer 描画Layer
/// @return 作成されたText。所有権はTextManagerが保持
inline MadoEngine::Text* Create(
	const std::string& name,
	const std::string& text,
	SceneType sceneType = SceneType::None,
	MadoEngine::EditorManagementMode managementMode = MadoEngine::EditorManagementMode::RuntimeOnly,
	MadoEngine::Render::RenderLayer layer = MadoEngine::Render::RenderLayer::Default) {
	MadoEngine::Text* created = MadoEngine::TextManager::GetInstance().Create(name, sceneType, managementMode);
	if (created) {
		created->SetText(text);
		created->SetRenderLayer(layer);
	}
	return created;
}

/// @brief 従来の引数順で描画レイヤーを指定して実行時専用Textを作成する
/// @param name Textの識別名
/// @param text 表示するUTF-8文字列
/// @param sceneType 描画対象Scene
/// @param layer 描画Layer
/// @return 作成されたText。所有権はTextManagerが保持
inline MadoEngine::Text* Create(
	const std::string& name,
	const std::string& text,
	SceneType sceneType,
	MadoEngine::Render::RenderLayer layer) {
	return Create(name, text, sceneType, MadoEngine::EditorManagementMode::RuntimeOnly, layer);
}

/// @brief 指定名のTextを取得
/// @param name Textの識別名
/// @return Text。存在しない場合はnullptr
inline MadoEngine::Text* Get(const std::string& name) {
	return MadoEngine::TextManager::GetInstance().Get(name);
}

/// @brief 指定名のTextを破棄
/// @param name Textの識別名
inline void Destroy(const std::string& name) {
	MadoEngine::TextManager::GetInstance().Destroy(name);
}

/// @brief 指定Sceneに属するTextを破棄
/// @param sceneType 破棄対象Scene
inline void DestroyByScene(SceneType sceneType) {
	MadoEngine::TextManager::GetInstance().DestroyByScene(sceneType);
}

} // namespace MyText
