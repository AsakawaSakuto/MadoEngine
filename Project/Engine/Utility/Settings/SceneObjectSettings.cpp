#include "SceneObjectSettings.h"
#include "Render/Object/3d/Model/ModelManager.h"
#include "Render/Object/2d/Sprite/SpriteManager.h"
#include "Render/Object/2d/Text/TextManager.h"
#include "Render/Object/2d/UIGroup/UIGroupManager.h"
#include "Utility/Light/LightManager.h"
#include <nlohmann/json.hpp>

namespace MadoEngine::Editor {

bool LoadModelEditorJson() {
	return ModelManager::GetInstance().LoadFromFile(kModelEditorJsonPath);
}

bool LoadModelEditorJson(SceneType sceneType) {
	return ModelManager::GetInstance().LoadFromFile(kModelEditorJsonPath, sceneType);
}

bool LoadSpriteEditorJson() {
	return SpriteManager::GetInstance().LoadFromFile(kSpriteEditorJsonPath);
}

bool LoadSpriteEditorJson(SceneType sceneType) {
	return SpriteManager::GetInstance().LoadFromFile(kSpriteEditorJsonPath, sceneType);
}

bool LoadTextEditorJson() {
	return TextManager::GetInstance().LoadFromFile("Assets/Json/TextObjects.json");
}

bool LoadTextEditorJson(SceneType sceneType) {
	return TextManager::GetInstance().LoadFromFile("Assets/Json/TextObjects.json", sceneType);
}

bool LoadLightEditorJson() {
	return LightManager::GetInstance().LoadFromJson();
}

bool LoadUIGroupEditorJson(SceneType sceneType) {
	return UIGroupManager::GetInstance().LoadFromFile("Assets/Json/UIGroupObjects.json", sceneType);
}

bool SaveUIGroupEditorJson(SceneType sceneType) {

	// 参照先の生成情報も保存し、再起動時にGroupだけが復元される状態を回避
	bool success = SpriteManager::GetInstance().SaveToFile("Assets/Json/SpriteObjects.json", sceneType);
	success = TextManager::GetInstance().SaveToFile("Assets/Json/TextObjects.json", sceneType) && success;
	success = UIGroupManager::GetInstance().SaveToFile("Assets/Json/UIGroupObjects.json", sceneType) && success;
	return success;
}

std::string CaptureUIGroupEditorState() {
	return nlohmann::json{
		{ "groups", UIGroupManager::GetInstance().ToJson() },
		{ "sprites", SpriteManager::GetInstance().ToJson() },
		{ "texts", TextManager::GetInstance().ToJson() },
	}.dump();
}

bool RestoreUIGroupEditorState(const std::string& snapshot) {
	const auto json = nlohmann::json::parse(snapshot, nullptr, false);

	// 参照構成を先に検証し、不正なSnapshotで描画要素を破棄しないためのガード
	if (json.is_discarded() || !json.is_object() || !json.contains("groups") ||
		!json.contains("sprites") || !json.at("sprites").is_object() ||
		!json.at("sprites").contains("sprites") || !json.at("sprites").at("sprites").is_array() ||
		!json.contains("texts") || !json.at("texts").is_object() ||
		!json.at("texts").contains("texts") || !json.at("texts").at("texts").is_array() ||
		!UIGroupManager::GetInstance().FromJson(json.at("groups"))) {
		return false;
	}

	// 生成元の情報も復元し、新規要素のUndoで残存、Redoで欠落する状態を防止
	SpriteManager::GetInstance().RestoreEditorSnapshot(json.at("sprites"));
	TextManager::GetInstance().RestoreEditorSnapshot(json.at("texts"));
	UIGroupManager::GetInstance().RefreshBindings();
	return true;
}

} // namespace MadoEngine::Editor
