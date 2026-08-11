#include "SpriteManager.h"
#include "Render/Object/2d/IRenderLayerBatchContext.h"
#include "Shader/RootSignatureManager.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cassert>
#include <utility>

namespace MadoEngine {

SpriteManager& SpriteManager::GetInstance() {
	static SpriteManager instance;
	return instance;
}

void SpriteManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Render::PSORegistry* psoRegistry) {
	assert(device);
	assert(commandList);
	assert(psoRegistry);

	device_ = device;
	commandList_ = commandList;
	psoRegistry_ = psoRegistry;
	sharedGeometry_.Initialize(device_);

	Logger::Output("SpriteManagerを初期化しました", Logger::Level::Engine);
}

void SpriteManager::Finalize() {
	pendingDestroyHandles_.clear();
	nameToHandle_.clear();
	drawOrder_.clear();
	freeSlots_.clear();
	freeSlots_.reserve(slots_.size());
	for (uint32_t index = 0; index < slots_.size(); ++index) {
		SpriteSlot& slot = slots_[index];
		slot.sprite.reset();
		slot.name.clear();
		slot.textureName.clear();
		slot.managementMode = EditorManagementMode::RuntimeOnly;
		slot.active = false;
		slot.generation = NextObjectGeneration(slot.generation);
		freeSlots_.push_back(index);
	}
	sharedGeometry_.Finalize();
	device_ = nullptr;
	commandList_ = nullptr;
	psoRegistry_ = nullptr;
	Logger::Output("SpriteManagerの全リソースを解放しました", Logger::Level::Engine);
}

void SpriteManager::SetScreenSize(float width, float height) {
	screenWidth_ = width;
	screenHeight_ = height;
	for (SpriteSlot& slot : slots_) {
		if (slot.active) {
			slot.sprite->SetScreenSize(screenWidth_, screenHeight_);
		}
	}
}

SpriteHandle SpriteManager::Create(
	const std::string& name,
	const std::string& textureName,
	SceneType sceneType,
	EditorManagementMode managementMode) {
	return Create(SpriteCreateDesc{ name, textureName, sceneType, managementMode });
}

SpriteHandle SpriteManager::Create(const SpriteCreateDesc& desc) {
	if (desc.name.empty()) {
		Logger::Output("名前が空のSpriteは生成できません", Logger::Level::Warning);
		return {};
	}
	if (nameToHandle_.contains(desc.name)) {
		Logger::Output("同名のSpriteが既に存在するため生成に失敗しました: " + desc.name, Logger::Level::Warning);
		return {};
	}
	if (TextureManager::GetInstance().GetTextureIndex(desc.textureName) == UINT32_MAX) {
		Logger::Output("存在しないテクスチャを指定したためSpriteの生成に失敗しました: " + desc.textureName, Logger::Level::Warning);
		return {};
	}

	auto sprite = std::make_unique<Sprite>(desc.name);
	sprite->Initialize(device_, commandList_, desc.textureName, sharedGeometry_);
	sprite->SetPSORegistry(psoRegistry_);
	sprite->SetSceneType(desc.sceneType);
	sprite->SetScreenSize(screenWidth_, screenHeight_);

	// 破棄済みSlotを再利用してHandle Indexの増加を抑制
	uint32_t index = 0;
	if (freeSlots_.empty()) {
		index = static_cast<uint32_t>(slots_.size());
		slots_.emplace_back();
	} else {
		index = freeSlots_.back();
		freeSlots_.pop_back();
	}

	SpriteSlot& slot = slots_[index];
	slot.sprite = std::move(sprite);
	slot.name = desc.name;
	slot.textureName = desc.textureName;
	slot.managementMode = desc.managementMode;
	slot.active = true;
	const SpriteHandle handle{ index, slot.generation };
	nameToHandle_.emplace(slot.name, handle);
	drawOrder_.push_back(handle);

	Logger::Output("Spriteを生成しました: " + desc.name, Logger::Level::Application);
	return handle;
}

SpriteHandle SpriteManager::FindOrCreate(const SpriteCreateDesc& desc) {
	const SpriteHandle existing = Find(desc.name);
	if (!existing.IsValid()) {
		return Create(desc);
	}

	const SpriteSlot& slot = slots_[existing.index];
	if (slot.textureName != desc.textureName ||
		slot.sprite->GetSceneType() != desc.sceneType ||
		slot.managementMode != desc.managementMode) {
		Logger::Output("既存Spriteの生成条件が一致しません: " + desc.name, Logger::Level::Warning);
		return {};
	}
	return existing;
}

SpriteHandle SpriteManager::CreateFromJson(const nlohmann::json& json) {
	if (!json.is_object()) {
		Logger::Output("SpriteのJSON要素がオブジェクトではありません", Logger::Level::Warning);
		return {};
	}

	const std::string name = json.value("name", "Sprite");
	const std::string textureName = json.value("texture", "");
	SpriteHandle handle = Find(name);
	if (handle.IsValid()) {
		SpriteSlot& slot = slots_[handle.index];

		// JSON復元で実行時専用Spriteを上書きしない管理境界
		if (slot.managementMode == EditorManagementMode::RuntimeOnly) {
			Logger::Output("実行時専用Spriteと同名のためJSON読み込みをスキップしました: " + name, Logger::Level::Warning);
			return {};
		}
		if (slot.textureName != textureName) {
			Destroy(handle);
			handle = {};
		}
	}
	if (!handle.IsValid()) {
		handle = Create(name, textureName, SceneType::None, EditorManagementMode::EditorManaged);
	}

	Sprite* sprite = TryGet(handle);
	if (!sprite) {
		return {};
	}
	sprite->FromJson(json);
	return handle;
}

Sprite* SpriteManager::TryGet(SpriteHandle handle) {
	return const_cast<Sprite*>(std::as_const(*this).TryGet(handle));
}

const Sprite* SpriteManager::TryGet(SpriteHandle handle) const {
	if (!handle.IsValid() || handle.index >= slots_.size()) {
		return nullptr;
	}
	const SpriteSlot& slot = slots_[handle.index];
	if (!slot.active || slot.generation != handle.generation) {
		return nullptr;
	}
	return slot.sprite.get();
}

SpriteHandle SpriteManager::Find(const std::string& name) const {
	const auto it = nameToHandle_.find(name);
	return it == nameToHandle_.end() ? SpriteHandle{} : it->second;
}

bool SpriteManager::IsValid(SpriteHandle handle) const {
	return TryGet(handle) != nullptr;
}

Sprite* SpriteManager::Get(const std::string& name) {
	return TryGet(Find(name));
}

const Sprite* SpriteManager::Get(const std::string& name) const {
	return TryGet(Find(name));
}

bool SpriteManager::Rename(SpriteHandle handle, const std::string& newName) {
	Sprite* sprite = TryGet(handle);
	if (!sprite) {
		Logger::Output("名前を変更するSpriteのHandleが無効です", Logger::Level::Warning);
		return false;
	}
	if (newName.empty()) {
		Logger::Output("Sprite名を空文字へ変更できません", Logger::Level::Warning);
		return false;
	}

	SpriteSlot& slot = slots_[handle.index];
	if (slot.name == newName) {
		return true;
	}
	if (nameToHandle_.contains(newName)) {
		Logger::Output("同名のSpriteが既に存在します: " + newName, Logger::Level::Warning);
		return false;
	}

	const std::string oldName = slot.name;

	// Sprite内部名と検索索引を同時に更新してHandle検索の一貫性を維持
	nameToHandle_.erase(oldName);
	slot.name = newName;
	nameToHandle_.emplace(newName, handle);
	sprite->SetObjectName(newName);
	Logger::Output("Sprite名を変更しました: " + oldName + " -> " + newName, Logger::Level::Application);
	return true;
}

bool SpriteManager::Rename(const std::string& currentName, const std::string& newName) {
	return Rename(Find(currentName), newName);
}

bool SpriteManager::Destroy(SpriteHandle handle) {
	Sprite* sprite = TryGet(handle);
	if (!sprite) {
		return false;
	}

	SpriteSlot& slot = slots_[handle.index];
	const std::string name = slot.name;

	// 遅延削除Queueと描画順から対象Handleを外して破棄後の参照を防止
	pendingDestroyHandles_.erase(
		std::remove(pendingDestroyHandles_.begin(), pendingDestroyHandles_.end(), handle),
		pendingDestroyHandles_.end());
	drawOrder_.erase(std::remove(drawOrder_.begin(), drawOrder_.end(), handle), drawOrder_.end());
	const auto nameIt = nameToHandle_.find(name);
	if (nameIt != nameToHandle_.end() && nameIt->second == handle) {
		nameToHandle_.erase(nameIt);
	}
	slot.sprite.reset();
	slot.name.clear();
	slot.textureName.clear();
	slot.managementMode = EditorManagementMode::RuntimeOnly;
	slot.active = false;

	// 旧Handleを無効化してからSlotを再利用候補へ返却
	slot.generation = NextObjectGeneration(slot.generation);
	freeSlots_.push_back(handle.index);

	Logger::Output("Spriteを削除しました: " + name, Logger::Level::Application);
	return true;
}

bool SpriteManager::Destroy(const std::string& name) {
	return Destroy(Find(name));
}

void SpriteManager::RequestDestroy(SpriteHandle handle) {
	if (!IsValid(handle)) {
		return;
	}
	if (std::find(pendingDestroyHandles_.begin(), pendingDestroyHandles_.end(), handle) == pendingDestroyHandles_.end()) {
		pendingDestroyHandles_.push_back(handle);
	}
}

void SpriteManager::RequestDestroy(const std::string& name) {
	RequestDestroy(Find(name));
}

void SpriteManager::FlushPendingDestroys() {
	if (pendingDestroyHandles_.empty()) {
		return;
	}

	std::vector<SpriteHandle> destroyHandles = std::move(pendingDestroyHandles_);
	pendingDestroyHandles_.clear();
	for (SpriteHandle handle : destroyHandles) {
		Destroy(handle);
	}
}

void SpriteManager::DestroyByScene(SceneType sceneType) {
	if (sceneType == SceneType::None) {
		Logger::Output("SceneType::NoneのSpriteはScene遷移で削除しません", Logger::Level::Warning);
		return;
	}

	// Draw Orderの走査中にContainerを変更しないよう対象Handleを先に収集
	std::vector<SpriteHandle> destroyHandles;
	for (SpriteHandle handle : drawOrder_) {
		const Sprite* sprite = TryGet(handle);
		if (sprite && sprite->GetSceneType() == sceneType) {
			destroyHandles.push_back(handle);
		}
	}
	for (SpriteHandle handle : destroyHandles) {
		Destroy(handle);
	}
	Logger::Output("Scene内のSpriteを削除しました: " + SceneTypeToString(sceneType) + " 件数: " + std::to_string(destroyHandles.size()), Logger::Level::Application);
}

void SpriteManager::UpdateAll(SceneType currentSceneType) {

	// Globalまたは現在Sceneに属する表示中Spriteだけを更新
	for (SpriteHandle handle : drawOrder_) {
		Sprite* sprite = TryGet(handle);
		if (!sprite) {
			continue;
		}
		const SceneType spriteScene = sprite->GetSceneType();
		if (!sprite->IsVisible()) {
			continue;
		}
		if (spriteScene != SceneType::None && spriteScene != currentSceneType) {
			continue;
		}
		sprite->Update();
	}
}

void SpriteManager::DrawAll(SceneType currentSceneType) {
	DrawLayerMask(currentSceneType, Render::kAllRenderLayers);
}

void SpriteManager::DrawLayer(SceneType currentSceneType, Render::RenderLayer layer) {
	DrawLayerMask(currentSceneType, Render::ToRenderLayerMask(layer));
}

void SpriteManager::DrawLayerMask(SceneType currentSceneType, Render::RenderLayerMask layerMask) {
	bool isStateSet = false;

	// 描画対象が見つかった時点で共通PSOとGeometryを一度だけBind
	for (SpriteHandle handle : drawOrder_) {
		Sprite* sprite = TryGet(handle);
		if (!sprite) {
			continue;
		}
		const SceneType spriteScene = sprite->GetSceneType();
		if (!sprite->IsVisible()) {
			continue;
		}
		if (spriteScene != SceneType::None && spriteScene != currentSceneType) {
			continue;
		}
		if (!sprite->IsRenderLayerIncluded(layerMask)) {
			continue;
		}

		if (!isStateSet) {
			commandList_->SetGraphicsRootSignature(RootSignatureManager::GetInstance().Get(sprite->GetRootSigKey()));
			commandList_->SetPipelineState(psoRegistry_->Get(sprite->GetPSODesc()));
			commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList_->IASetVertexBuffers(0, 1, &sharedGeometry_.vbv);
			commandList_->IASetIndexBuffer(&sharedGeometry_.ibv);
			isStateSet = true;
		}
		sprite->Draw();
	}
}

void SpriteManager::DrawInOrder(
	SceneType currentSceneType,
	Render::IRenderLayerBatchContext& batchContext)
{
	bool isBatchActive = false;
	bool isStateSet = false;
	Render::RenderLayer currentLayer = Render::RenderLayer::Default;
	for (SpriteHandle handle : drawOrder_) {
		Sprite* sprite = TryGet(handle);
		if (!sprite) {
			continue;
		}
		const SceneType spriteScene = sprite->GetSceneType();
		if (!sprite->IsVisible()) {
			continue;
		}
		if (spriteScene != SceneType::None && spriteScene != currentSceneType) {
			continue;
		}

		const Render::RenderLayer layer = sprite->GetRenderLayer();
		if (!isBatchActive || layer != currentLayer) {
			if (isBatchActive) {
				batchContext.EndRenderLayerBatch(currentLayer);
			}
			currentLayer = layer;
			batchContext.BeginRenderLayerBatch(currentLayer);
			isBatchActive = true;
			isStateSet = false;
		}

		if (!isStateSet) {
			commandList_->SetGraphicsRootSignature(RootSignatureManager::GetInstance().Get(sprite->GetRootSigKey()));
			commandList_->SetPipelineState(psoRegistry_->Get(sprite->GetPSODesc()));
			commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList_->IASetVertexBuffers(0, 1, &sharedGeometry_.vbv);
			commandList_->IASetIndexBuffer(&sharedGeometry_.ibv);
			isStateSet = true;
		}
		sprite->Draw();
	}

	if (isBatchActive) {
		batchContext.EndRenderLayerBatch(currentLayer);
	}
}

nlohmann::json SpriteManager::ToJson() const {
	nlohmann::json sprites = nlohmann::json::array();
	for (SpriteHandle handle : drawOrder_) {
		const Sprite* sprite = TryGet(handle);
		if (!sprite || slots_[handle.index].managementMode != EditorManagementMode::EditorManaged) {
			continue;
		}
		sprites.push_back(sprite->ToJson());
	}
	return { { "sprites", sprites } };
}

void SpriteManager::FromJson(const nlohmann::json& json) {
	FromJsonInternal(json, std::nullopt);
}

void SpriteManager::FromJson(const nlohmann::json& json, SceneType sceneType) {
	FromJsonInternal(json, sceneType);
}

void SpriteManager::FromJsonInternal(
	const nlohmann::json& json,
	std::optional<SceneType> sceneType) {
	const nlohmann::json* spriteArray = nullptr;
	if (json.is_array()) {
		spriteArray = &json;
	} else if (json.contains("sprites") && json.at("sprites").is_array()) {
		spriteArray = &json.at("sprites");
	}
	if (!spriteArray) {
		Logger::Output("SpriteのJSONにsprites配列がありません", Logger::Level::Warning);
		return;
	}

	for (const nlohmann::json& spriteJson : *spriteArray) {
		try {
			const SceneType spriteSceneType = SceneTypeFromString(
				spriteJson.value("scene", SceneTypeToString(SceneType::None)));
			if (sceneType &&
				spriteSceneType != SceneType::None &&
				spriteSceneType != *sceneType) {
				continue;
			}
			const SpriteHandle handle = CreateFromJson(spriteJson);
			(void)handle;
		}
		catch (const nlohmann::json::exception& exception) {
			Logger::Output("SpriteのJSON要素を読み込めませんでした: " + std::string(exception.what()), Logger::Level::Error);
		}
	}
}

bool SpriteManager::SaveToFile(const std::filesystem::path& filePath) const {
	return Json::JsonFile::Save(filePath, ToJson(), 4, true);
}

bool SpriteManager::SaveToFile(
	const std::filesystem::path& filePath,
	SceneType sceneType) const {
	nlohmann::json outputJson = nlohmann::json::object();
	nlohmann::json mergedSprites = nlohmann::json::array();

	if (Json::JsonFile::Exists(filePath)) {
		nlohmann::json existingJson;
		if (!Json::JsonFile::Load(filePath, existingJson)) {
			return false;
		}

		const nlohmann::json* existingSprites = nullptr;
		if (existingJson.is_array()) {
			existingSprites = &existingJson;
		} else if (existingJson.contains("sprites") && existingJson.at("sprites").is_array()) {
			existingSprites = &existingJson.at("sprites");
		}
		if (!existingSprites) {
			Logger::Output(
				"SpriteのJSONにsprites配列がないため、シーン単位の保存を中止しました",
				Logger::Level::Warning);
			return false;
		}

		if (existingJson.is_object()) {
			outputJson = existingJson;
		}

		for (const nlohmann::json& spriteJson : *existingSprites) {
			try {
				const SceneType spriteSceneType = SceneTypeFromString(
					spriteJson.value("scene", SceneTypeToString(SceneType::None)));
				if (spriteSceneType != SceneType::None && spriteSceneType != sceneType) {
					mergedSprites.push_back(spriteJson);
				}
			}
			catch (const nlohmann::json::exception& exception) {
				Logger::Output(
					"Spriteのシーン設定を判定できないため保存を中止しました: " +
					std::string(exception.what()),
					Logger::Level::Error);
				return false;
			}
		}
	}

	const nlohmann::json activeEditorJson = ToJson();
	for (const nlohmann::json& spriteJson : activeEditorJson.at("sprites")) {
		mergedSprites.push_back(spriteJson);
	}
	outputJson["sprites"] = std::move(mergedSprites);
	return Json::JsonFile::Save(filePath, outputJson, 4, true);
}

bool SpriteManager::LoadFromFile(const std::filesystem::path& filePath) {
	nlohmann::json json;
	if (!Json::JsonFile::Load(filePath, json)) {
		return false;
	}
	FromJson(json);
	return true;
}

bool SpriteManager::LoadFromFile(
	const std::filesystem::path& filePath,
	SceneType sceneType) {
	nlohmann::json json;
	if (!Json::JsonFile::Load(filePath, json)) {
		return false;
	}
	FromJson(json, sceneType);
	return true;
}

std::vector<std::string> SpriteManager::GetNames() const {
	std::vector<std::string> names;
	names.reserve(drawOrder_.size());
	for (SpriteHandle handle : drawOrder_) {
		if (IsValid(handle)) {
			names.push_back(slots_[handle.index].name);
		}
	}
	return names;
}

std::size_t SpriteManager::GetSpriteCount() const {
	return nameToHandle_.size();
}

std::vector<std::string> SpriteManager::GetEditorManagedNames() const {
	std::vector<std::string> names;
	names.reserve(drawOrder_.size());
	for (SpriteHandle handle : drawOrder_) {
		if (IsValid(handle) && slots_[handle.index].managementMode == EditorManagementMode::EditorManaged) {
			names.push_back(slots_[handle.index].name);
		}
	}
	return names;
}

} // namespace MadoEngine
