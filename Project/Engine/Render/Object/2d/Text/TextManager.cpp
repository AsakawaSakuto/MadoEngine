#include "TextManager.h"
#include "Shader/RootSignatureManager.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cassert>
#include <utility>

namespace MadoEngine {

TextManager& TextManager::GetInstance() {
	static TextManager instance;
	return instance;
}

void TextManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Render::PSORegistry* psoRegistry) {
	assert(device != nullptr);
	assert(commandList != nullptr);
	assert(psoRegistry != nullptr);

	device_ = device;
	commandList_ = commandList;
	psoRegistry_ = psoRegistry;
	sharedGeometry_.Initialize(device_);
	TextTextureGenerator::GetInstance().Initialize();

	Logger::Output("[Engine] TextManagerを初期化しました。", Logger::Level::Engine);
}

void TextManager::Finalize() {
	for (auto& [name, entry] : texts_) {
		(void)name;
		entry.text->ReleaseTexture();
	}
	texts_.clear();
	sharedGeometry_.Finalize();
	TextTextureGenerator::GetInstance().Finalize();
	device_ = nullptr;
	commandList_ = nullptr;
	psoRegistry_ = nullptr;

	Logger::Output("[Engine] TextManagerを終了しました。", Logger::Level::Engine);
}

void TextManager::SetScreenSize(float width, float height) {
	screenWidth_ = width;
	screenHeight_ = height;
	for (auto& [name, entry] : texts_) {
		(void)name;
		entry.text->SetScreenSize(screenWidth_, screenHeight_);
	}
}

Text* TextManager::Create(
	const std::string& name,
	SceneType sceneType,
	EditorManagementMode managementMode) {
	auto existingIt = texts_.find(name);
	if (existingIt != texts_.end()) {
		if (existingIt->second.managementMode != managementMode) {
			Logger::Output("[Engine] 同名のTextが異なる管理方法で既に存在します: " + name, Logger::Level::Warning);
			return nullptr;
		}

		Logger::Output("[Engine] 同名のTextが既に存在します: " + name, Logger::Level::Warning);
		return existingIt->second.text.get();
	}

	auto text = std::make_unique<Text>(name);
	text->Initialize(device_, commandList_, sharedGeometry_);
	text->SetPSORegistry(psoRegistry_);
	text->SetSceneType(sceneType);
	text->SetScreenSize(screenWidth_, screenHeight_);

	Text* created = text.get();
	texts_.emplace(name, TextEntry{ std::move(text), managementMode });

	Logger::Output("[Engine] Textを作成しました: " + name, Logger::Level::Application);
	return created;
}

Text* TextManager::CreateFromJson(const nlohmann::json& json) {
	if (!json.is_object()) {
		Logger::Output("[Engine] Text Jsonの要素がオブジェクトではありません。", Logger::Level::Warning);
		return nullptr;
	}

	const std::string name = json.value("name", "Text");
	Text* text = nullptr;
	auto existingIt = texts_.find(name);
	if (existingIt != texts_.end()) {
		if (existingIt->second.managementMode == EditorManagementMode::RuntimeOnly) {
			Logger::Output(
				"[Engine] 実行時専用Textと同名のためJsonの読み込みをスキップしました: " + name,
				Logger::Level::Warning);
			return nullptr;
		}
		text = existingIt->second.text.get();
	} else {
		text = Create(name, SceneType::None, EditorManagementMode::EditorManaged);
	}

	if (!text) {
		return nullptr;
	}

	text->FromJson(json);
	return text;
}

Text* TextManager::Get(const std::string& name) const {
	auto it = texts_.find(name);
	if (it == texts_.end()) {
		return nullptr;
	}
	return it->second.text.get();
}

bool TextManager::Rename(const std::string& currentName, const std::string& newName) {
	auto currentIt = texts_.find(currentName);
	if (currentIt == texts_.end()) {
		Logger::Output("名前を変更するTextが見つかりません : " + currentName, Logger::Level::Warning);
		return false;
	}
	if (newName.empty()) {
		Logger::Output("Text名を空文字へ変更できません", Logger::Level::Warning);
		return false;
	}
	if (currentName == newName) {
		return true;
	}
	if (texts_.contains(newName)) {
		Logger::Output("同名のTextが既に存在します : " + newName, Logger::Level::Warning);
		return false;
	}

	auto node = texts_.extract(currentIt);
	Text* text = node.mapped().text.get();
	node.key() = newName;
	texts_.insert(std::move(node));
	text->SetObjectName(newName);

	Logger::Output("Text名を変更しました : " + currentName + " -> " + newName, Logger::Level::Application);
	return true;
}

void TextManager::Destroy(const std::string& name) {
	auto it = texts_.find(name);
	if (it != texts_.end()) {
		it->second.text->ReleaseTexture();
		texts_.erase(it);
		Logger::Output("[Engine] Textを破棄しました: " + name, Logger::Level::Application);
	}
}

void TextManager::DestroyByScene(SceneType sceneType) {
	if (sceneType == SceneType::None) {
		Logger::Output("[Engine] SceneType::Noneは全Scene共通のためText破棄をスキップしました。", Logger::Level::Warning);
		return;
	}

	size_t destroyCount = 0;
	for (auto it = texts_.begin(); it != texts_.end();) {
		if (it->second.text->GetSceneType() == sceneType) {
			it->second.text->ReleaseTexture();
			it = texts_.erase(it);
			++destroyCount;
		} else {
			++it;
		}
	}

	Logger::Output("[Engine] Scene内のTextを破棄しました。件数: " + std::to_string(destroyCount), Logger::Level::Application);
}

void TextManager::UpdateAll(SceneType currentSceneType) {
	for (auto& [name, entry] : texts_) {
		(void)name;
		Text* text = entry.text.get();
		const SceneType textScene = text->GetSceneType();
		if (!text->IsVisible()) { continue; }
		if (textScene != SceneType::None && textScene != currentSceneType) { continue; }
		text->Update();
	}
}

void TextManager::DrawAll(SceneType currentSceneType, const std::string& targetScreen) {
	DrawLayerMask(currentSceneType, Render::kAllRenderLayers, targetScreen);
}

void TextManager::DrawLayer(SceneType currentSceneType, Render::RenderLayer layer, const std::string& targetScreen) {
	DrawLayerMask(currentSceneType, Render::ToRenderLayerMask(layer), targetScreen);
}

void TextManager::DrawLayerMask(SceneType currentSceneType, Render::RenderLayerMask layerMask, const std::string& targetScreen) {
	bool isStateSet = false;

	for (auto& [name, entry] : texts_) {
		(void)name;
		Text* text = entry.text.get();
		const SceneType textScene = text->GetSceneType();
		if (!text->IsVisible()) { continue; }
		if (textScene != SceneType::None && textScene != currentSceneType) { continue; }
		if (!text->IsRenderLayerIncluded(layerMask)) { continue; }
		if (!targetScreen.empty() && text->GetTargetScreen() != targetScreen) { continue; }

		if (!isStateSet) {
			commandList_->SetGraphicsRootSignature(RootSignatureManager::GetInstance().Get(text->GetRootSigKey()));
			commandList_->SetPipelineState(psoRegistry_->Get(text->GetPSODesc()));
			commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList_->IASetVertexBuffers(0, 1, &sharedGeometry_.vbv);
			commandList_->IASetIndexBuffer(&sharedGeometry_.ibv);
			isStateSet = true;
		}

		text->Draw();
	}
}

nlohmann::json TextManager::ToJson() const {
	nlohmann::json texts = nlohmann::json::array();
	for (const auto& [name, entry] : texts_) {
		(void)name;
		if (entry.managementMode != EditorManagementMode::EditorManaged) {
			continue;
		}
		texts.push_back(entry.text->ToJson());
	}

	return {
		{ "texts", texts },
	};
}

void TextManager::FromJson(const nlohmann::json& json) {
	const nlohmann::json* textArray = nullptr;
	if (json.is_array()) {
		textArray = &json;
	} else if (json.contains("texts") && json.at("texts").is_array()) {
		textArray = &json.at("texts");
	}

	if (!textArray) {
		Logger::Output("[Engine] Text Jsonにtexts配列がありません。", Logger::Level::Warning);
		return;
	}

	for (const nlohmann::json& textJson : *textArray) {
		CreateFromJson(textJson);
	}
}

bool TextManager::SaveToFile(const std::filesystem::path& filePath) const {
	return Json::JsonFile::Save(filePath, ToJson(), 4, true);
}

bool TextManager::LoadFromFile(const std::filesystem::path& filePath) {
	nlohmann::json json;
	if (!Json::JsonFile::Load(filePath, json)) {
		return false;
	}

	FromJson(json);
	return true;
}

std::vector<std::string> TextManager::GetNames() const {
	std::vector<std::string> names;
	names.reserve(texts_.size());
	for (const auto& [name, entry] : texts_) {
		(void)entry;
		names.push_back(name);
	}
	std::sort(names.begin(), names.end());
	return names;
}

std::vector<std::string> TextManager::GetEditorManagedNames() const {
	std::vector<std::string> names;
	names.reserve(texts_.size());
	for (const auto& [name, entry] : texts_) {
		if (entry.managementMode != EditorManagementMode::EditorManaged) {
			continue;
		}
		names.push_back(name);
	}
	std::sort(names.begin(), names.end());
	return names;
}

} // namespace MadoEngine
