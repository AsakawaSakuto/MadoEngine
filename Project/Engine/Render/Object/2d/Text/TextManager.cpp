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
	Logger::Output("TextManagerを初期化しました", Logger::Level::Engine);
}

void TextManager::Finalize() {
	pendingDestroyHandles_.clear();
	nameToHandle_.clear();
	freeSlots_.clear();
	freeSlots_.reserve(slots_.size());
	for (uint32_t index = 0; index < slots_.size(); ++index) {
		TextSlot& slot = slots_[index];
		if (slot.text) {
			slot.text->ReleaseTexture();
		}
		slot.text.reset();
		slot.name.clear();
		slot.managementMode = EditorManagementMode::RuntimeOnly;
		slot.active = false;
		slot.generation = NextObjectGeneration(slot.generation);
		freeSlots_.push_back(index);
	}
	sharedGeometry_.Finalize();
	TextTextureGenerator::GetInstance().Finalize();
	device_ = nullptr;
	commandList_ = nullptr;
	psoRegistry_ = nullptr;
	Logger::Output("TextManagerの全リソースを解放しました", Logger::Level::Engine);
}

void TextManager::SetScreenSize(float width, float height) {
	screenWidth_ = width;
	screenHeight_ = height;
	for (TextSlot& slot : slots_) {
		if (slot.active) {
			slot.text->SetScreenSize(screenWidth_, screenHeight_);
		}
	}
}

TextHandle TextManager::Create(const std::string& name, SceneType sceneType, EditorManagementMode managementMode) {
	return Create(TextCreateDesc{ name, sceneType, managementMode });
}

TextHandle TextManager::Create(const TextCreateDesc& desc) {
	if (desc.name.empty()) {
		Logger::Output("名前が空のTextは生成できません", Logger::Level::Warning);
		return {};
	}
	if (nameToHandle_.contains(desc.name)) {
		Logger::Output("同名のTextが既に存在するため生成に失敗しました: " + desc.name, Logger::Level::Warning);
		return {};
	}

	auto text = std::make_unique<Text>(desc.name);
	text->Initialize(device_, commandList_, sharedGeometry_);
	text->SetPSORegistry(psoRegistry_);
	text->SetSceneType(desc.sceneType);
	text->SetScreenSize(screenWidth_, screenHeight_);

	uint32_t index = 0;
	if (freeSlots_.empty()) {
		index = static_cast<uint32_t>(slots_.size());
		slots_.emplace_back();
	} else {
		index = freeSlots_.back();
		freeSlots_.pop_back();
	}

	TextSlot& slot = slots_[index];
	slot.text = std::move(text);
	slot.name = desc.name;
	slot.managementMode = desc.managementMode;
	slot.active = true;
	const TextHandle handle{ index, slot.generation };
	nameToHandle_.emplace(slot.name, handle);

	Logger::Output("Textを生成しました: " + desc.name, Logger::Level::Application);
	return handle;
}

TextHandle TextManager::FindOrCreate(const TextCreateDesc& desc) {
	const TextHandle existing = Find(desc.name);
	if (!existing.IsValid()) {
		return Create(desc);
	}

	const TextSlot& slot = slots_[existing.index];
	if (slot.text->GetSceneType() != desc.sceneType || slot.managementMode != desc.managementMode) {
		Logger::Output("既存Textの生成条件が一致しません: " + desc.name, Logger::Level::Warning);
		return {};
	}
	return existing;
}

TextHandle TextManager::CreateFromJson(const nlohmann::json& json) {
	if (!json.is_object()) {
		Logger::Output("TextのJSON要素がオブジェクトではありません", Logger::Level::Warning);
		return {};
	}

	const std::string name = json.value("name", "Text");
	TextHandle handle = Find(name);
	if (handle.IsValid() && slots_[handle.index].managementMode == EditorManagementMode::RuntimeOnly) {
		Logger::Output("実行時専用Textと同名のためJSON読み込みをスキップしました: " + name, Logger::Level::Warning);
		return {};
	}
	if (!handle.IsValid()) {
		handle = Create(name, SceneType::None, EditorManagementMode::EditorManaged);
	}

	Text* text = TryGet(handle);
	if (!text) {
		return {};
	}
	text->FromJson(json);
	return handle;
}

Text* TextManager::TryGet(TextHandle handle) {
	return const_cast<Text*>(std::as_const(*this).TryGet(handle));
}

const Text* TextManager::TryGet(TextHandle handle) const {
	if (!handle.IsValid() || handle.index >= slots_.size()) {
		return nullptr;
	}
	const TextSlot& slot = slots_[handle.index];
	if (!slot.active || slot.generation != handle.generation) {
		return nullptr;
	}
	return slot.text.get();
}

TextHandle TextManager::Find(const std::string& name) const {
	const auto it = nameToHandle_.find(name);
	return it == nameToHandle_.end() ? TextHandle{} : it->second;
}

bool TextManager::IsValid(TextHandle handle) const {
	return TryGet(handle) != nullptr;
}

Text* TextManager::Get(const std::string& name) {
	return TryGet(Find(name));
}

const Text* TextManager::Get(const std::string& name) const {
	return TryGet(Find(name));
}

bool TextManager::Rename(TextHandle handle, const std::string& newName) {
	Text* text = TryGet(handle);
	if (!text) {
		Logger::Output("名前を変更するTextのHandleが無効です", Logger::Level::Warning);
		return false;
	}
	if (newName.empty()) {
		Logger::Output("Text名を空文字へ変更できません", Logger::Level::Warning);
		return false;
	}

	TextSlot& slot = slots_[handle.index];
	if (slot.name == newName) {
		return true;
	}
	if (nameToHandle_.contains(newName)) {
		Logger::Output("同名のTextが既に存在します: " + newName, Logger::Level::Warning);
		return false;
	}

	const std::string oldName = slot.name;
	nameToHandle_.erase(oldName);
	slot.name = newName;
	nameToHandle_.emplace(newName, handle);
	text->SetObjectName(newName);
	Logger::Output("Text名を変更しました: " + oldName + " -> " + newName, Logger::Level::Application);
	return true;
}

bool TextManager::Rename(const std::string& currentName, const std::string& newName) {
	return Rename(Find(currentName), newName);
}

bool TextManager::Destroy(TextHandle handle) {
	Text* text = TryGet(handle);
	if (!text) {
		return false;
	}

	TextSlot& slot = slots_[handle.index];
	const std::string name = slot.name;
	pendingDestroyHandles_.erase(
		std::remove(pendingDestroyHandles_.begin(), pendingDestroyHandles_.end(), handle),
		pendingDestroyHandles_.end());
	const auto nameIt = nameToHandle_.find(name);
	if (nameIt != nameToHandle_.end() && nameIt->second == handle) {
		nameToHandle_.erase(nameIt);
	}
	text->ReleaseTexture();
	slot.text.reset();
	slot.name.clear();
	slot.managementMode = EditorManagementMode::RuntimeOnly;
	slot.active = false;
	slot.generation = NextObjectGeneration(slot.generation);
	freeSlots_.push_back(handle.index);

	Logger::Output("Textを削除しました: " + name, Logger::Level::Application);
	return true;
}

bool TextManager::Destroy(const std::string& name) {
	return Destroy(Find(name));
}

void TextManager::RequestDestroy(TextHandle handle) {
	if (!IsValid(handle)) {
		return;
	}
	if (std::find(pendingDestroyHandles_.begin(), pendingDestroyHandles_.end(), handle) == pendingDestroyHandles_.end()) {
		pendingDestroyHandles_.push_back(handle);
	}
}

void TextManager::RequestDestroy(const std::string& name) {
	RequestDestroy(Find(name));
}

void TextManager::FlushPendingDestroys() {
	if (pendingDestroyHandles_.empty()) {
		return;
	}
	std::vector<TextHandle> destroyHandles = std::move(pendingDestroyHandles_);
	pendingDestroyHandles_.clear();
	for (TextHandle handle : destroyHandles) {
		Destroy(handle);
	}
}

void TextManager::DestroyByScene(SceneType sceneType) {
	if (sceneType == SceneType::None) {
		Logger::Output("SceneType::NoneのTextはScene遷移で削除しません", Logger::Level::Warning);
		return;
	}

	std::vector<TextHandle> destroyHandles;
	for (const auto& [name, handle] : nameToHandle_) {
		(void)name;
		const Text* text = TryGet(handle);
		if (text && text->GetSceneType() == sceneType) {
			destroyHandles.push_back(handle);
		}
	}
	for (TextHandle handle : destroyHandles) {
		Destroy(handle);
	}
	Logger::Output("Scene内のTextを削除しました: " + SceneTypeToString(sceneType) + " 件数: " + std::to_string(destroyHandles.size()), Logger::Level::Application);
}

void TextManager::UpdateAll(SceneType currentSceneType) {
	for (const auto& [name, handle] : nameToHandle_) {
		(void)name;
		Text* text = TryGet(handle);
		if (!text) {
			continue;
		}
		const SceneType textScene = text->GetSceneType();
		if (!text->IsVisible()) {
			continue;
		}
		if (textScene != SceneType::None && textScene != currentSceneType) {
			continue;
		}
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
	for (const auto& [name, handle] : nameToHandle_) {
		(void)name;
		Text* text = TryGet(handle);
		if (!text) {
			continue;
		}
		const SceneType textScene = text->GetSceneType();
		if (!text->IsVisible()) {
			continue;
		}
		if (textScene != SceneType::None && textScene != currentSceneType) {
			continue;
		}
		if (!text->IsRenderLayerIncluded(layerMask)) {
			continue;
		}
		if (!targetScreen.empty() && text->GetTargetScreen() != targetScreen) {
			continue;
		}

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
	for (const auto& [name, handle] : nameToHandle_) {
		(void)name;
		const Text* text = TryGet(handle);
		if (!text || slots_[handle.index].managementMode != EditorManagementMode::EditorManaged) {
			continue;
		}
		texts.push_back(text->ToJson());
	}
	return { { "texts", texts } };
}

void TextManager::FromJson(const nlohmann::json& json) {
	const nlohmann::json* textArray = nullptr;
	if (json.is_array()) {
		textArray = &json;
	} else if (json.contains("texts") && json.at("texts").is_array()) {
		textArray = &json.at("texts");
	}
	if (!textArray) {
		Logger::Output("TextのJSONにtexts配列がありません", Logger::Level::Warning);
		return;
	}

	for (const nlohmann::json& textJson : *textArray) {
		const TextHandle handle = CreateFromJson(textJson);
		(void)handle;
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
	names.reserve(nameToHandle_.size());
	for (const auto& [name, handle] : nameToHandle_) {
		(void)handle;
		names.push_back(name);
	}
	std::sort(names.begin(), names.end());
	return names;
}

std::vector<std::string> TextManager::GetEditorManagedNames() const {
	std::vector<std::string> names;
	names.reserve(nameToHandle_.size());
	for (const auto& [name, handle] : nameToHandle_) {
		if (IsValid(handle) && slots_[handle.index].managementMode == EditorManagementMode::EditorManaged) {
			names.push_back(name);
		}
	}
	std::sort(names.begin(), names.end());
	return names;
}

} // namespace MadoEngine
