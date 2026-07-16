#include "LightManager.h"
#include "Utility/Json/JsonHeaders.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>

namespace {

const std::string kEmptyLightName;
constexpr float kMinLightDirectionLengthSq = 0.000001f;

/// @brief ライト方向として使用できる有限値か確認する
/// @param direction 確認する方向ベクトル
/// @return 有限値のみで構成されている場合はtrue
bool IsFiniteDirection(const Vector3& direction) {
	return std::isfinite(direction.x) && std::isfinite(direction.y) && std::isfinite(direction.z);
}

/// @brief ライト方向を正規化する
/// @param direction 正規化する方向ベクトル
/// @return 正規化済みの方向ベクトル
Vector3 NormalizeLightDirection(const Vector3& direction) {
	if (!IsFiniteDirection(direction)) {
		return { 0.0f, -1.0f, 0.0f };
	}

	const float lengthSq = direction.LengthSq();
	if (lengthSq <= kMinLightDirectionLengthSq) {
		return { 0.0f, -1.0f, 0.0f };
	}

	const float invLength = 1.0f / std::sqrt(lengthSq);
	return { direction.x * invLength, direction.y * invLength, direction.z * invLength };
}

/// @brief 平行光源の方向を正規化する
/// @param light 正規化対象の平行光源
void NormalizeDirectionalLight(DirectionalLight& light) {
	light.direction = NormalizeLightDirection(light.direction);
}

/// @brief スポットライトの方向を正規化する
/// @param light 正規化対象のスポットライト
void NormalizeSpotLight(SpotLight& light) {
	light.direction = NormalizeLightDirection(light.direction);
}

/// @brief シーン種別の表示名を取得する
/// @param sceneType 表示するシーン種別
/// @return シーン種別の表示名
std::string GetSceneLabel(SceneType sceneType) {
	if (sceneType == SceneType::None) {
		return "全シーン";
	}
	return SceneTypeToString(sceneType);
}

/// @brief Json保存用のシーン名を取得する
/// @param sceneType 保存するSceneType
/// @return Jsonへ保存するシーン名
std::string GetSceneJsonName(SceneType sceneType) {
	if (sceneType == SceneType::None) {
		return "None";
	}

	return SceneTypeToString(sceneType);
}

/// @brief 文字列からSceneTypeを取得する
/// @param sceneName 読み込むシーン名
/// @return 文字列に対応するSceneType
SceneType ParseSceneType(const std::string& sceneName) {
	if (sceneName.empty() || sceneName == "None" || sceneName == "All" || sceneName == "全シーン") {
		return SceneType::None;
	}

	for (uint32_t index = 0; index < kSceneTypeCount; ++index) {
		const SceneType sceneType = GetSceneTypeByIndex(index);
		if (SceneTypeToString(sceneType) == sceneName) {
			return sceneType;
		}
	}

	return SceneType::None;
}

/// @brief LightTypeをJson保存用の文字列へ変換する
/// @param type 変換するLightType
/// @return LightTypeの文字列
const char* GetLightTypeJsonName(LightType type) {
	switch (type) {
	case LightType::Directional:
		return "Directional";
	case LightType::Point:
		return "Point";
	case LightType::Spot:
		return "Spot";
	default:
		return "None";
	}
}

/// @brief 文字列からLightTypeを取得する
/// @param typeName 読み込むライト種別名
/// @return 文字列に対応するLightType
LightType ParseLightType(const std::string& typeName) {
	if (typeName == "Directional") {
		return LightType::Directional;
	}
	if (typeName == "Point") {
		return LightType::Point;
	}
	if (typeName == "Spot") {
		return LightType::Spot;
	}

	return LightType::None;
}

/// @brief 平行光源をJsonへ変換する
/// @param light 変換する平行光源
/// @return 変換されたJson
nlohmann::json DirectionalLightToJson(const DirectionalLight& light) {
	return nlohmann::json{
		{ "color", MadoEngine::Json::JsonSerializer::ToJson(light.color) },
		{ "direction", MadoEngine::Json::JsonSerializer::ToJson(light.direction) },
		{ "intensity", light.intensity },
		{ "useHalfLambert", light.useHalfLambert != 0 }
	};
}

/// @brief 点光源をJsonへ変換する
/// @param light 変換する点光源
/// @return 変換されたJson
nlohmann::json PointLightToJson(const PointLight& light) {
	return nlohmann::json{
		{ "color", MadoEngine::Json::JsonSerializer::ToJson(light.color) },
		{ "position", MadoEngine::Json::JsonSerializer::ToJson(light.position) },
		{ "intensity", light.intensity },
		{ "radius", light.radius },
		{ "decay", light.decay }
	};
}

/// @brief スポットライトをJsonへ変換する
/// @param light 変換するスポットライト
/// @return 変換されたJson
nlohmann::json SpotLightToJson(const SpotLight& light) {
	return nlohmann::json{
		{ "color", MadoEngine::Json::JsonSerializer::ToJson(light.color) },
		{ "position", MadoEngine::Json::JsonSerializer::ToJson(light.position) },
		{ "direction", MadoEngine::Json::JsonSerializer::ToJson(light.direction) },
		{ "intensity", light.intensity },
		{ "distance", light.distance },
		{ "decay", light.decay },
		{ "cosAngle", light.cosAngle },
		{ "cosFalloffStart", light.cosFalloffStart }
	};
}

/// @brief ライト共通情報をJsonへ追加する
/// @param json 追加先のJson
/// @param type 保存するライト種別
/// @param meta 保存するライト共通情報
void AddLightMetaDataToJson(nlohmann::json& json, LightType type, const LightMetaData& meta) {
	json["type"] = GetLightTypeJsonName(type);
	json["name"] = meta.name;
	json["scene"] = GetSceneJsonName(meta.sceneType);
	json["layerMask"] = meta.layerMask;
	json["enabled"] = meta.enabled;
}

/// @brief Jsonから平行光源を読み込む
/// @param json 読み込み元のJson
/// @return 読み込んだ平行光源
DirectionalLight DirectionalLightFromJson(const nlohmann::json& json) {
	DirectionalLight light;
	if (!json.is_object()) {
		return light;
	}

	light.color = json.contains("color") ? MadoEngine::Json::JsonSerializer::ToVector4(json.at("color"), light.color) : light.color;
	light.direction = json.contains("direction") ? MadoEngine::Json::JsonSerializer::ToVector3(json.at("direction"), light.direction) : light.direction;
	light.intensity = MadoEngine::Json::JsonSerializer::GetOrDefault<float>(json, "intensity", light.intensity);
	light.useHalfLambert = MadoEngine::Json::JsonSerializer::GetOrDefault<bool>(json, "useHalfLambert", light.useHalfLambert != 0) ? 1u : 0u;
	NormalizeDirectionalLight(light);
	return light;
}

/// @brief Jsonから点光源を読み込む
/// @param json 読み込み元のJson
/// @return 読み込んだ点光源
PointLight PointLightFromJson(const nlohmann::json& json) {
	PointLight light;
	if (!json.is_object()) {
		return light;
	}

	light.color = json.contains("color") ? MadoEngine::Json::JsonSerializer::ToVector4(json.at("color"), light.color) : light.color;
	light.position = json.contains("position") ? MadoEngine::Json::JsonSerializer::ToVector3(json.at("position"), light.position) : light.position;
	light.intensity = MadoEngine::Json::JsonSerializer::GetOrDefault<float>(json, "intensity", light.intensity);
	light.radius = MadoEngine::Json::JsonSerializer::GetOrDefault<float>(json, "radius", light.radius);
	light.decay = MadoEngine::Json::JsonSerializer::GetOrDefault<float>(json, "decay", light.decay);
	return light;
}

/// @brief Jsonからスポットライトを読み込む
/// @param json 読み込み元のJson
/// @return 読み込んだスポットライト
SpotLight SpotLightFromJson(const nlohmann::json& json) {
	SpotLight light;
	if (!json.is_object()) {
		return light;
	}

	light.color = json.contains("color") ? MadoEngine::Json::JsonSerializer::ToVector4(json.at("color"), light.color) : light.color;
	light.position = json.contains("position") ? MadoEngine::Json::JsonSerializer::ToVector3(json.at("position"), light.position) : light.position;
	light.direction = json.contains("direction") ? MadoEngine::Json::JsonSerializer::ToVector3(json.at("direction"), light.direction) : light.direction;
	light.intensity = MadoEngine::Json::JsonSerializer::GetOrDefault<float>(json, "intensity", light.intensity);
	light.distance = MadoEngine::Json::JsonSerializer::GetOrDefault<float>(json, "distance", light.distance);
	light.decay = MadoEngine::Json::JsonSerializer::GetOrDefault<float>(json, "decay", light.decay);
	light.cosAngle = MadoEngine::Json::JsonSerializer::GetOrDefault<float>(json, "cosAngle", light.cosAngle);
	light.cosFalloffStart = MadoEngine::Json::JsonSerializer::GetOrDefault<float>(json, "cosFalloffStart", light.cosFalloffStart);
	NormalizeSpotLight(light);
	return light;
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
	LightLayerMask layerMask,
	MadoEngine::EditorManagementMode managementMode) {
	DirectionalLightEntry entry;
	entry.light = light;
	NormalizeDirectionalLight(entry.light);
	entry.meta.name = name;
	entry.meta.sceneType = sceneType;
	entry.meta.layerMask = layerMask;
	entry.meta.managementMode = managementMode;
	entry.meta.enabled = light.useLight != 0;

	return CreateLight(directionalLights_, LightType::Directional, name, entry);
}

LightHandle LightManager::CreatePointLight(
	const std::string& name,
	const PointLight& light,
	SceneType sceneType,
	LightLayerMask layerMask,
	MadoEngine::EditorManagementMode managementMode) {
	PointLightEntry entry;
	entry.light = light;
	entry.meta.name = name;
	entry.meta.sceneType = sceneType;
	entry.meta.layerMask = layerMask;
	entry.meta.managementMode = managementMode;
	entry.meta.enabled = light.useLight != 0;

	return CreateLight(pointLights_, LightType::Point, name, entry);
}

LightHandle LightManager::CreateSpotLight(
	const std::string& name,
	const SpotLight& light,
	SceneType sceneType,
	LightLayerMask layerMask,
	MadoEngine::EditorManagementMode managementMode) {
	SpotLightEntry entry;
	entry.light = light;
	NormalizeSpotLight(entry.light);
	entry.meta.name = name;
	entry.meta.sceneType = sceneType;
	entry.meta.layerMask = layerMask;
	entry.meta.managementMode = managementMode;
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

std::vector<LightHandle> LightManager::GetDirectionalLightHandles() const {
	return GetActiveHandles(directionalLights_, LightType::Directional);
}

std::vector<LightHandle> LightManager::GetPointLightHandles() const {
	return GetActiveHandles(pointLights_, LightType::Point);
}

std::vector<LightHandle> LightManager::GetSpotLightHandles() const {
	return GetActiveHandles(spotLights_, LightType::Spot);
}

std::vector<LightHandle> LightManager::GetEditorManagedDirectionalLightHandles() const {
	return GetActiveHandles(directionalLights_, LightType::Directional, true);
}

std::vector<LightHandle> LightManager::GetEditorManagedPointLightHandles() const {
	return GetActiveHandles(pointLights_, LightType::Point, true);
}

std::vector<LightHandle> LightManager::GetEditorManagedSpotLightHandles() const {
	return GetActiveHandles(spotLights_, LightType::Spot, true);
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

void LightManager::DestroyByScene(SceneType sceneType) {
	if (sceneType == SceneType::None) {
		Logger::Output("SceneType::Noneは全シーン共通のため、Lightのシーン単位削除をスキップしました", Logger::Level::Warning);
		return;
	}

	size_t destroyCount = 0;

	for (DirectionalSlot& slot : directionalLights_) {
		if (!slot.active || slot.entry.meta.sceneType != sceneType) {
			continue;
		}

		nameToHandle_.erase(slot.entry.meta.name);
		slot.active = false;
		++slot.generation;
		++destroyCount;
	}

	for (PointSlot& slot : pointLights_) {
		if (!slot.active || slot.entry.meta.sceneType != sceneType) {
			continue;
		}

		nameToHandle_.erase(slot.entry.meta.name);
		slot.active = false;
		++slot.generation;
		++destroyCount;
	}

	for (SpotSlot& slot : spotLights_) {
		if (!slot.active || slot.entry.meta.sceneType != sceneType) {
			continue;
		}

		nameToHandle_.erase(slot.entry.meta.name);
		slot.active = false;
		++slot.generation;
		++destroyCount;
	}

	AdvanceRevision();
	Logger::Output("シーン内のLightを削除しました : " + SceneTypeToString(sceneType) + " 件数 : " + std::to_string(destroyCount), Logger::Level::Application);
}

bool LightManager::RenameLight(LightHandle handle, const std::string& newName) {
	LightMetaData* meta = GetMetaData(handle);
	if (!meta) {
		Logger::Output("ライト名を変更できません。無効なハンドルです。", Logger::Level::Warning);
		return false;
	}

	if (newName.empty()) {
		Logger::Output("ライト名を変更できません。新しい名前が空です。", Logger::Level::Warning);
		return false;
	}

	if (meta->name == newName) {
		return true;
	}

	auto nameIt = nameToHandle_.find(newName);
	if (nameIt != nameToHandle_.end()) {
		Logger::Output("ライト名を変更できません。同名のライトが存在します : " + newName, Logger::Level::Warning);
		return false;
	}

	nameToHandle_.erase(meta->name);
	meta->name = newName;
	nameToHandle_.emplace(meta->name, handle);
	AdvanceRevision();
	Logger::Output("ライト名を変更しました : " + meta->name, Logger::Level::Application);
	return true;
}

void LightManager::Clear() {
	ClearSlots(directionalLights_);
	ClearSlots(pointLights_);
	ClearSlots(spotLights_);
	nameToHandle_.clear();
	AdvanceRevision();
	Logger::Output("登録済みライトをすべて削除しました。", Logger::Level::Application);
}

void LightManager::ClearEditorManagedLights() {
	ClearEditorManagedSlots(directionalLights_);
	ClearEditorManagedSlots(pointLights_);
	ClearEditorManagedSlots(spotLights_);
	AdvanceRevision();
}

bool LightManager::SaveToJson(const std::filesystem::path& filePath) const {
	nlohmann::json root = nlohmann::json::object();
	root["version"] = 1;
	root["lights"] = nlohmann::json::array();

	for (const DirectionalSlot& slot : directionalLights_) {
		if (!slot.active || slot.entry.meta.managementMode != MadoEngine::EditorManagementMode::EditorManaged) {
			continue;
		}

		nlohmann::json lightJson = nlohmann::json::object();
		AddLightMetaDataToJson(lightJson, LightType::Directional, slot.entry.meta);
		lightJson["data"] = DirectionalLightToJson(slot.entry.light);
		root["lights"].push_back(lightJson);
	}

	for (const PointSlot& slot : pointLights_) {
		if (!slot.active || slot.entry.meta.managementMode != MadoEngine::EditorManagementMode::EditorManaged) {
			continue;
		}

		nlohmann::json lightJson = nlohmann::json::object();
		AddLightMetaDataToJson(lightJson, LightType::Point, slot.entry.meta);
		lightJson["data"] = PointLightToJson(slot.entry.light);
		root["lights"].push_back(lightJson);
	}

	for (const SpotSlot& slot : spotLights_) {
		if (!slot.active || slot.entry.meta.managementMode != MadoEngine::EditorManagementMode::EditorManaged) {
			continue;
		}

		nlohmann::json lightJson = nlohmann::json::object();
		AddLightMetaDataToJson(lightJson, LightType::Spot, slot.entry.meta);
		lightJson["data"] = SpotLightToJson(slot.entry.light);
		root["lights"].push_back(lightJson);
	}

	const bool isSaved = MadoEngine::Json::JsonFile::Save(filePath, root, 4, true);
	if (isSaved) {
		Logger::Output("LightManagerの設定をJsonへ保存しました : " + filePath.generic_string(), Logger::Level::Application);
	} else {
		Logger::Output("LightManagerの設定保存に失敗しました : " + filePath.generic_string(), Logger::Level::Error);
	}

	return isSaved;
}

bool LightManager::LoadFromJson(const std::filesystem::path& filePath) {
	nlohmann::json root;
	if (!MadoEngine::Json::JsonFile::Load(filePath, root)) {
		Logger::Output("LightManagerの設定をJsonから読み込めませんでした : " + filePath.generic_string(), Logger::Level::Error);
		return false;
	}

	if (!root.is_object() || !root.contains("lights") || !root.at("lights").is_array()) {
		Logger::Output("LightManagerのJson形式が不正です : " + filePath.generic_string(), Logger::Level::Error);
		return false;
	}

	ClearEditorManagedLights();

	size_t loadCount = 0;
	for (const nlohmann::json& lightJson : root.at("lights")) {
		if (!lightJson.is_object()) {
			Logger::Output("LightManagerのJson内に不正なライト情報があります", Logger::Level::Warning);
			continue;
		}

		const std::string typeName = MadoEngine::Json::JsonSerializer::GetOrDefault<std::string>(lightJson, "type", "");
		const LightType type = ParseLightType(typeName);
		if (type == LightType::None) {
			Logger::Output("未対応のライト種別をスキップしました : " + typeName, Logger::Level::Warning);
			continue;
		}

		const std::string name = MadoEngine::Json::JsonSerializer::GetOrDefault<std::string>(lightJson, "name", "");
		if (name.empty()) {
			Logger::Output("名前が空のライトをスキップしました", Logger::Level::Warning);
			continue;
		}

		const std::string sceneName = MadoEngine::Json::JsonSerializer::GetOrDefault<std::string>(lightJson, "scene", "None");
		const SceneType sceneType = ParseSceneType(sceneName);
		const LightLayerMask layerMask = MadoEngine::Json::JsonSerializer::GetOrDefault<LightLayerMask>(lightJson, "layerMask", ToLightLayerMask(LightLayer::World));
		const bool enabled = MadoEngine::Json::JsonSerializer::GetOrDefault<bool>(lightJson, "enabled", true);
		const nlohmann::json dataJson = lightJson.contains("data") ? lightJson.at("data") : nlohmann::json::object();

		LightHandle handle;
		switch (type) {
		case LightType::Directional:
		{
			DirectionalLight light = DirectionalLightFromJson(dataJson);
			light.useLight = enabled ? 1u : 0u;
			handle = CreateDirectionalLight(
				name,
				light,
				sceneType,
				layerMask,
				MadoEngine::EditorManagementMode::EditorManaged);
			break;
		}
		case LightType::Point:
		{
			PointLight light = PointLightFromJson(dataJson);
			light.useLight = enabled ? 1u : 0u;
			handle = CreatePointLight(
				name,
				light,
				sceneType,
				layerMask,
				MadoEngine::EditorManagementMode::EditorManaged);
			break;
		}
		case LightType::Spot:
		{
			SpotLight light = SpotLightFromJson(dataJson);
			light.useLight = enabled ? 1u : 0u;
			handle = CreateSpotLight(
				name,
				light,
				sceneType,
				layerMask,
				MadoEngine::EditorManagementMode::EditorManaged);
			break;
		}
		default:
			break;
		}

		if (handle.IsValid()) {
			++loadCount;
		}
	}

	AdvanceRevision();
	Logger::Output("LightManagerの設定をJsonから読み込みました : " + filePath.generic_string() + " 件数 : " + std::to_string(loadCount), Logger::Level::Application);
	return true;
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

	DirectionalLight normalizedLight = light;
	NormalizeDirectionalLight(normalizedLight);
	*targetLight = normalizedLight;
	AdvanceRevision();
	return true;
}

bool LightManager::SetPointLight(LightHandle handle, const PointLight& light) {
	PointLight* targetLight = MutablePointLight(handle);
	if (!targetLight) {
		return false;
	}

	*targetLight = light;
	AdvanceRevision();
	return true;
}

bool LightManager::SetSpotLight(LightHandle handle, const SpotLight& light) {
	SpotLight* targetLight = MutableSpotLight(handle);
	if (!targetLight) {
		return false;
	}

	SpotLight normalizedLight = light;
	NormalizeSpotLight(normalizedLight);
	*targetLight = normalizedLight;
	AdvanceRevision();
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
		const LightMetaData* existingMeta = GetMetaData(nameIt->second);
		if (existingMeta && existingMeta->managementMode != entry.meta.managementMode) {
			Logger::Output("同名のライトが異なる管理方法で既に登録されています : " + name, Logger::Level::Warning);
			return {};
		}

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

template <typename TSlot>
void LightManager::ClearEditorManagedSlots(std::vector<TSlot>& slots) {
	for (TSlot& slot : slots) {
		if (!slot.active || slot.entry.meta.managementMode != MadoEngine::EditorManagementMode::EditorManaged) {
			continue;
		}

		nameToHandle_.erase(slot.entry.meta.name);
		slot.active = false;
		++slot.generation;
	}
}

template <typename TSlot>
std::vector<LightHandle> LightManager::GetActiveHandles(
	const std::vector<TSlot>& slots,
	LightType type,
	bool editorManagedOnly) const {
	std::vector<LightHandle> handles;
	handles.reserve(slots.size());

	for (uint32_t index = 0; index < static_cast<uint32_t>(slots.size()); ++index) {
		const TSlot& slot = slots[index];
		if (!slot.active) {
			continue;
		}
		if (editorManagedOnly && slot.entry.meta.managementMode != MadoEngine::EditorManagementMode::EditorManaged) {
			continue;
		}

		LightHandle handle;
		handle.type = type;
		handle.index = index;
		handle.generation = slot.generation;
		handles.push_back(handle);
	}

	return handles;
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
