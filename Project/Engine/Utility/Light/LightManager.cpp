#include "LightManager.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>

namespace {

const std::string kEmptyLightName;

/// @brief シーン種別の表示名を取得する
/// @param sceneType 表示するシーン種別
/// @return シーン種別の表示名
std::string GetSceneLabel(SceneType sceneType) {
	if (sceneType == SceneType::None) {
		return "全シーン";
	}
	return SceneTypeToString(sceneType);
}

} // namespace

LightManager& LightManager::GetInstance() {
	static LightManager instance;
	return instance;
}

LightHandle LightManager::CreateDirectionalLight(
	const std::string& name,
	const DirectionalLight& light,
	SceneType sceneType,
	LightLayerMask layerMask) {
	DirectionalLightEntry entry;
	entry.light = light;
	entry.meta.name = name;
	entry.meta.sceneType = sceneType;
	entry.meta.layerMask = layerMask;
	entry.meta.enabled = light.useLight != 0;

	return CreateLight(directionalLights_, LightType::Directional, name, entry);
}

LightHandle LightManager::CreatePointLight(
	const std::string& name,
	const PointLight& light,
	SceneType sceneType,
	LightLayerMask layerMask) {
	PointLightEntry entry;
	entry.light = light;
	entry.meta.name = name;
	entry.meta.sceneType = sceneType;
	entry.meta.layerMask = layerMask;
	entry.meta.enabled = light.useLight != 0;

	return CreateLight(pointLights_, LightType::Point, name, entry);
}

LightHandle LightManager::CreateSpotLight(
	const std::string& name,
	const SpotLight& light,
	SceneType sceneType,
	LightLayerMask layerMask) {
	SpotLightEntry entry;
	entry.light = light;
	entry.meta.name = name;
	entry.meta.sceneType = sceneType;
	entry.meta.layerMask = layerMask;
	entry.meta.enabled = light.useLight != 0;

	return CreateLight(spotLights_, LightType::Spot, name, entry);
}

LightHandle LightManager::Find(const std::string& name) const {
	auto it = nameToHandle_.find(name);
	if (it == nameToHandle_.end()) {
		return {};
	}
	return it->second;
}

bool LightManager::IsValid(LightHandle handle) const {
	if (!handle.IsValid()) {
		return false;
	}

	switch (handle.type) {
	case LightType::Directional:
		return handle.index < directionalLights_.size() &&
			directionalLights_[handle.index].active &&
			directionalLights_[handle.index].generation == handle.generation;
	case LightType::Point:
		return handle.index < pointLights_.size() &&
			pointLights_[handle.index].active &&
			pointLights_[handle.index].generation == handle.generation;
	case LightType::Spot:
		return handle.index < spotLights_.size() &&
			spotLights_[handle.index].active &&
			spotLights_[handle.index].generation == handle.generation;
	default:
		return false;
	}
}

bool LightManager::Destroy(LightHandle handle) {
	if (!IsValid(handle)) {
		Logger::Output("ライト削除に失敗しました。無効なハンドルです。", Logger::Level::Warning);
		return false;
	}

	EraseName(handle);

	switch (handle.type) {
	case LightType::Directional:
		directionalLights_[handle.index].active = false;
		++directionalLights_[handle.index].generation;
		break;
	case LightType::Point:
		pointLights_[handle.index].active = false;
		++pointLights_[handle.index].generation;
		break;
	case LightType::Spot:
		spotLights_[handle.index].active = false;
		++spotLights_[handle.index].generation;
		break;
	default:
		return false;
	}
	AdvanceRevision();

	Logger::Output("ライトを削除しました。", Logger::Level::Application);
	return true;
}

bool LightManager::Destroy(const std::string& name) {
	return Destroy(Find(name));
}

void LightManager::Clear() {
	ClearSlots(directionalLights_);
	ClearSlots(pointLights_);
	ClearSlots(spotLights_);
	nameToHandle_.clear();
	AdvanceRevision();
	Logger::Output("登録済みライトをすべて削除しました。", Logger::Level::Application);
}

bool LightManager::SetEnabled(LightHandle handle, bool enabled) {
	LightMetaData* meta = GetMetaData(handle);
	if (!meta) {
		Logger::Output("ライトの有効状態を変更できません。無効なハンドルです。", Logger::Level::Warning);
		return false;
	}

	meta->enabled = enabled;
	ApplyEnabledToLight(handle, enabled);
	AdvanceRevision();
	Logger::Output("ライトの有効状態を変更しました : " + meta->name + " -> " + (enabled ? "有効" : "無効"), Logger::Level::Application);
	return true;
}

bool LightManager::IsEnabled(LightHandle handle) const {
	const LightMetaData* meta = GetMetaData(handle);
	return meta ? meta->enabled : false;
}

bool LightManager::SetSceneType(LightHandle handle, SceneType sceneType) {
	LightMetaData* meta = GetMetaData(handle);
	if (!meta) {
		Logger::Output("ライトのシーン設定を変更できません。無効なハンドルです。", Logger::Level::Warning);
		return false;
	}

	meta->sceneType = sceneType;
	AdvanceRevision();
	Logger::Output("ライトのシーンを変更しました : " + meta->name + " -> " + GetSceneLabel(sceneType), Logger::Level::Application);
	return true;
}

SceneType LightManager::GetSceneType(LightHandle handle) const {
	const LightMetaData* meta = GetMetaData(handle);
	return meta ? meta->sceneType : SceneType::None;
}

bool LightManager::SetLayerMask(LightHandle handle, LightLayerMask layerMask) {
	LightMetaData* meta = GetMetaData(handle);
	if (!meta) {
		Logger::Output("ライトのレイヤー設定を変更できません。無効なハンドルです。", Logger::Level::Warning);
		return false;
	}

	meta->layerMask = layerMask;
	AdvanceRevision();
	Logger::Output("ライトのレイヤーマスクを変更しました : " + meta->name + " -> " + std::to_string(layerMask), Logger::Level::Application);
	return true;
}

LightLayerMask LightManager::GetLayerMask(LightHandle handle) const {
	const LightMetaData* meta = GetMetaData(handle);
	return meta ? meta->layerMask : ToLightLayerMask(LightLayer::None);
}

const std::string& LightManager::GetName(LightHandle handle) const {
	const LightMetaData* meta = GetMetaData(handle);
	return meta ? meta->name : kEmptyLightName;
}

const DirectionalLight* LightManager::GetDirectionalLightData(LightHandle handle) const {
	const DirectionalLightEntry* entry = GetDirectionalLight(handle);
	return entry ? &entry->light : nullptr;
}

const PointLight* LightManager::GetPointLightData(LightHandle handle) const {
	const PointLightEntry* entry = GetPointLight(handle);
	return entry ? &entry->light : nullptr;
}

const SpotLight* LightManager::GetSpotLightData(LightHandle handle) const {
	const SpotLightEntry* entry = GetSpotLight(handle);
	return entry ? &entry->light : nullptr;
}

bool LightManager::SetDirectionalLight(LightHandle handle, const DirectionalLight& light) {
	DirectionalLight* targetLight = MutableDirectionalLight(handle);
	if (!targetLight) {
		return false;
	}

	*targetLight = light;
	return true;
}

bool LightManager::SetPointLight(LightHandle handle, const PointLight& light) {
	PointLight* targetLight = MutablePointLight(handle);
	if (!targetLight) {
		return false;
	}

	*targetLight = light;
	return true;
}

bool LightManager::SetSpotLight(LightHandle handle, const SpotLight& light) {
	SpotLight* targetLight = MutableSpotLight(handle);
	if (!targetLight) {
		return false;
	}

	*targetLight = light;
	return true;
}

DirectionalLight* LightManager::MutableDirectionalLight(LightHandle handle) {
	DirectionalLightEntry* entry = GetDirectionalLight(handle);
	return entry ? &entry->light : nullptr;
}

PointLight* LightManager::MutablePointLight(LightHandle handle) {
	PointLightEntry* entry = GetPointLight(handle);
	return entry ? &entry->light : nullptr;
}

SpotLight* LightManager::MutableSpotLight(LightHandle handle) {
	SpotLightEntry* entry = GetSpotLight(handle);
	return entry ? &entry->light : nullptr;
}

DirectionalLightEntry* LightManager::GetDirectionalLight(LightHandle handle) {
	if (!IsValid(handle) || handle.type != LightType::Directional) {
		return nullptr;
	}
	AdvanceRevision();
	return &directionalLights_[handle.index].entry;
}

const DirectionalLightEntry* LightManager::GetDirectionalLight(LightHandle handle) const {
	if (!IsValid(handle) || handle.type != LightType::Directional) {
		return nullptr;
	}
	return &directionalLights_[handle.index].entry;
}

PointLightEntry* LightManager::GetPointLight(LightHandle handle) {
	if (!IsValid(handle) || handle.type != LightType::Point) {
		return nullptr;
	}
	AdvanceRevision();
	return &pointLights_[handle.index].entry;
}

const PointLightEntry* LightManager::GetPointLight(LightHandle handle) const {
	if (!IsValid(handle) || handle.type != LightType::Point) {
		return nullptr;
	}
	return &pointLights_[handle.index].entry;
}

SpotLightEntry* LightManager::GetSpotLight(LightHandle handle) {
	if (!IsValid(handle) || handle.type != LightType::Spot) {
		return nullptr;
	}
	AdvanceRevision();
	return &spotLights_[handle.index].entry;
}

const SpotLightEntry* LightManager::GetSpotLight(LightHandle handle) const {
	if (!IsValid(handle) || handle.type != LightType::Spot) {
		return nullptr;
	}
	return &spotLights_[handle.index].entry;
}

size_t LightManager::GetDirectionalLightCount() const {
	return static_cast<size_t>(std::count_if(directionalLights_.begin(), directionalLights_.end(), [](const DirectionalSlot& slot) {
		return slot.active;
	}));
}

size_t LightManager::GetPointLightCount() const {
	return static_cast<size_t>(std::count_if(pointLights_.begin(), pointLights_.end(), [](const PointSlot& slot) {
		return slot.active;
	}));
}

size_t LightManager::GetSpotLightCount() const {
	return static_cast<size_t>(std::count_if(spotLights_.begin(), spotLights_.end(), [](const SpotSlot& slot) {
		return slot.active;
	}));
}

std::vector<DirectionalLight> LightManager::GetFilteredDirectionalLights(SceneType sceneType, LightLayerMask receiveLightMask) const {
	return GetFilteredLights<DirectionalLight>(directionalLights_, sceneType, receiveLightMask);
}

std::vector<PointLight> LightManager::GetFilteredPointLights(SceneType sceneType, LightLayerMask receiveLightMask) const {
	return GetFilteredLights<PointLight>(pointLights_, sceneType, receiveLightMask);
}

std::vector<SpotLight> LightManager::GetFilteredSpotLights(SceneType sceneType, LightLayerMask receiveLightMask) const {
	return GetFilteredLights<SpotLight>(spotLights_, sceneType, receiveLightMask);
}

LightGpuData LightManager::BuildGpuData(SceneType sceneType, LightLayerMask receiveLightMask) const {
	LightGpuData gpuData;
	gpuData.directionalLightCount = FillGpuLights(directionalLights_, sceneType, receiveLightMask, gpuData.directionalLights);
	gpuData.pointLightCount = FillGpuLights(pointLights_, sceneType, receiveLightMask, gpuData.pointLights);
	gpuData.spotLightCount = FillGpuLights(spotLights_, sceneType, receiveLightMask, gpuData.spotLights);
	return gpuData;
}

const LightGpuData& LightManager::GetCachedGpuData(SceneType sceneType, LightLayerMask receiveLightMask) {
	const uint64_t cacheKey = MakeGpuDataCacheKey(sceneType, receiveLightMask);
	LightGpuDataCache& cache = gpuDataCache_[cacheKey];
	if (cache.revision != revision_) {
		cache.gpuData = BuildGpuData(sceneType, receiveLightMask);
		cache.revision = revision_;
	}

	return cache.gpuData;
}

template <typename TSlot>
LightHandle LightManager::CreateLight(
	std::vector<TSlot>& slots,
	LightType type,
	const std::string& name,
	const typename TSlot::entry_type& entry) {
	if (name.empty()) {
		Logger::Output("ライト名が空のため登録できません。", Logger::Level::Warning);
		return {};
	}

	auto nameIt = nameToHandle_.find(name);
	if (nameIt != nameToHandle_.end()) {
		Logger::Output("同名のライトがすでに登録されています : " + name, Logger::Level::Warning);
		return nameIt->second;
	}

	uint32_t index = 0;
	for (; index < slots.size(); ++index) {
		if (!slots[index].active) {
			break;
		}
	}

	if (index == slots.size()) {
		slots.emplace_back();
	}

	TSlot& slot = slots[index];
	slot.entry = entry;
	slot.active = true;

	LightHandle handle;
	handle.type = type;
	handle.index = index;
	handle.generation = slot.generation;
	nameToHandle_.emplace(name, handle);
	AdvanceRevision();

	Logger::Output("ライトを登録しました : " + name + " シーン : " + GetSceneLabel(entry.meta.sceneType), Logger::Level::Application);
	return handle;
}

LightMetaData* LightManager::GetMetaData(LightHandle handle) {
	if (!IsValid(handle)) {
		return nullptr;
	}

	switch (handle.type) {
	case LightType::Directional:
		return &directionalLights_[handle.index].entry.meta;
	case LightType::Point:
		return &pointLights_[handle.index].entry.meta;
	case LightType::Spot:
		return &spotLights_[handle.index].entry.meta;
	default:
		return nullptr;
	}
}

const LightMetaData* LightManager::GetMetaData(LightHandle handle) const {
	if (!IsValid(handle)) {
		return nullptr;
	}

	switch (handle.type) {
	case LightType::Directional:
		return &directionalLights_[handle.index].entry.meta;
	case LightType::Point:
		return &pointLights_[handle.index].entry.meta;
	case LightType::Spot:
		return &spotLights_[handle.index].entry.meta;
	default:
		return nullptr;
	}
}

void LightManager::ApplyEnabledToLight(LightHandle handle, bool enabled) {
	const uint32_t useLight = enabled ? 1u : 0u;

	switch (handle.type) {
	case LightType::Directional:
		directionalLights_[handle.index].entry.light.useLight = useLight;
		break;
	case LightType::Point:
		pointLights_[handle.index].entry.light.useLight = useLight;
		break;
	case LightType::Spot:
		spotLights_[handle.index].entry.light.useLight = useLight;
		break;
	default:
		break;
	}
}

void LightManager::EraseName(LightHandle handle) {
	const std::string name = GetName(handle);
	if (!name.empty()) {
		nameToHandle_.erase(name);
	}
}

void LightManager::AdvanceRevision() {
	++revision_;
	if (revision_ == 0) {
		revision_ = 1;
		gpuDataCache_.clear();
	}
}

uint64_t LightManager::MakeGpuDataCacheKey(SceneType sceneType, LightLayerMask receiveLightMask) const {
	const uint64_t sceneKey = static_cast<uint64_t>(static_cast<uint32_t>(sceneType));
	return (sceneKey << 32) | static_cast<uint64_t>(receiveLightMask);
}

template <typename TSlot>
void LightManager::ClearSlots(std::vector<TSlot>& slots) {
	for (TSlot& slot : slots) {
		if (slot.active) {
			slot.active = false;
			++slot.generation;
		}
	}
}

bool LightManager::IsLightMatched(const LightMetaData& meta, SceneType sceneType, LightLayerMask receiveLightMask) const {
	if (!meta.enabled) {
		return false;
	}

	const bool isSceneMatched =
		meta.sceneType == SceneType::None ||
		sceneType == SceneType::None ||
		meta.sceneType == sceneType;
	if (!isSceneMatched) {
		return false;
	}

	return (meta.layerMask & receiveLightMask) != 0;
}

template <typename TLight, typename TSlot>
std::vector<TLight> LightManager::GetFilteredLights(
	const std::vector<TSlot>& slots,
	SceneType sceneType,
	LightLayerMask receiveLightMask) const {
	std::vector<TLight> filteredLights;

	for (const TSlot& slot : slots) {
		if (!slot.active) {
			continue;
		}
		if (!IsLightMatched(slot.entry.meta, sceneType, receiveLightMask)) {
			continue;
		}
		if (slot.entry.light.useLight == 0) {
			continue;
		}

		filteredLights.push_back(slot.entry.light);
	}

	return filteredLights;
}

template <typename TLight, typename TSlot, size_t MaxLightCount>
uint32_t LightManager::FillGpuLights(
	const std::vector<TSlot>& slots,
	SceneType sceneType,
	LightLayerMask receiveLightMask,
	std::array<TLight, MaxLightCount>& outLights) const {
	uint32_t count = 0;

	for (const TSlot& slot : slots) {
		if (count >= MaxLightCount) {
			break;
		}
		if (!slot.active) {
			continue;
		}
		if (!IsLightMatched(slot.entry.meta, sceneType, receiveLightMask)) {
			continue;
		}
		if (slot.entry.light.useLight == 0) {
			continue;
		}

		outLights[count] = slot.entry.light;
		++count;
	}

	return count;
}
