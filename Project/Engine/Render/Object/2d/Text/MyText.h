#pragma once
#include "TextManager.h"

namespace MyText {

/// @brief Textを作成して管理下に登録します。
/// @param name Textの識別名。
/// @param text 表示するUTF-8文字列。
/// @param sceneType 描画対象Scene。
/// @return 作成されたText。所有権はTextManagerが保持します。
inline MadoEngine::Text* Create(
	const std::string& name,
	const std::string& text,
	SceneType sceneType = SceneType::None) {
	MadoEngine::Text* created = MadoEngine::TextManager::GetInstance().Create(name, sceneType);
	if (created) {
		created->SetText(text);
	}
	return created;
}

/// @brief Layerを指定してTextを作成します。
/// @param name Textの識別名。
/// @param text 表示するUTF-8文字列。
/// @param sceneType 描画対象Scene。
/// @param layer 描画Layer。
/// @return 作成されたText。所有権はTextManagerが保持します。
inline MadoEngine::Text* Create(
	const std::string& name,
	const std::string& text,
	SceneType sceneType,
	MadoEngine::Render::RenderLayer layer) {
	MadoEngine::Text* created = Create(name, text, sceneType);
	if (created) {
		created->SetRenderLayer(layer);
	}
	return created;
}

/// @brief 指定名のTextを取得します。
/// @param name Textの識別名。
/// @return Text。存在しない場合はnullptr。
inline MadoEngine::Text* Get(const std::string& name) {
	return MadoEngine::TextManager::GetInstance().Get(name);
}

/// @brief 指定名のTextを破棄します。
/// @param name Textの識別名。
inline void Destroy(const std::string& name) {
	MadoEngine::TextManager::GetInstance().Destroy(name);
}

/// @brief 指定Sceneに属するTextを破棄します。
/// @param sceneType 破棄対象Scene。
inline void DestroyByScene(SceneType sceneType) {
	MadoEngine::TextManager::GetInstance().DestroyByScene(sceneType);
}

} // namespace MyText
