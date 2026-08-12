#include "Utility/Camera/CameraManager.h"
#include "Utility/Easing/Easing.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Json/Core/JsonSerializer.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <unordered_set>

namespace {

	constexpr float kMinProjectionValue = 0.0001f;
	constexpr float kMaxShakePower = 100000.0f;
	constexpr float kMaxShakeDuration = 300.0f;
	constexpr int kCameraJsonVersion = 2;

	/// @brief 二つの値を指定率で線形補間
	/// @param source 補間開始値
	/// @param destination 補間終了値
	/// @param progress 補間率
	/// @return 補間結果
	float LerpValue(float source, float destination, float progress) {
		return source + (destination - source) * progress;
	}

	/// @brief 最短回転方向で角度を線形補間
	/// @param source 補間開始角度
	/// @param destination 補間終了角度
	/// @param progress 補間率
	/// @return 補間結果の角度
	float LerpAngle(float source, float destination, float progress) {
		const float fullRotation = std::numbers::pi_v<float> * 2.0f;
		const float shortestDifference = std::remainder(destination - source, fullRotation);
		return source + shortestDifference * progress;
	}

	/// @brief 有限なfloatをJsonから取得して範囲内へ制限
	/// @param json 読み込み元のJson
	/// @param key 取得するキー
	/// @param defaultValue 取得失敗時の既定値
	/// @param minValue 最小値
	/// @param maxValue 最大値
	/// @return 検証済みの値
	float ReadClampedFloat(
		const nlohmann::json& json,
		const char* key,
		float defaultValue,
		float minValue,
		float maxValue) {
		const float value = MadoEngine::Json::JsonSerializer::GetOrDefault<float>(
			json,
			key,
			defaultValue
		);
		return std::isfinite(value) ? std::clamp(value, minValue, maxValue) : defaultValue;
	}

	/// @brief Vector3の全成分が有限値か確認
	/// @param value 確認対象のVector3
	/// @return 全成分が有限値の場合はtrue
	bool IsFiniteVector3(const Vector3& value) {
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
	}

	/// @brief Camera遷移方式をJson表示名へ変換
	/// @param mode 変換対象の遷移方式
	/// @return Jsonへ保存する遷移方式名
	const char* CameraTransitionModeToString(CameraTransitionMode mode) {
		return mode == CameraTransitionMode::Blend ? "Blend" : "Cut";
	}

	/// @brief Json表示名からCamera遷移方式を取得
	/// @param value 読み込んだ遷移方式名
	/// @return 名前に対応する遷移方式
	CameraTransitionMode CameraTransitionModeFromString(const std::string& value) {
		return value == "Blend" ? CameraTransitionMode::Blend : CameraTransitionMode::Cut;
	}

	/// @brief Shake軸をJson表示名へ変換
	/// @param type 変換対象のShake軸
	/// @return Jsonへ保存するShake軸名
	const char* ShakeTypeToString(ShakeType type) {
		switch (type) {
		case ShakeType::X:
			return "X";
		case ShakeType::Y:
			return "Y";
		case ShakeType::Z:
			return "Z";
		case ShakeType::XY:
			return "XY";
		case ShakeType::XZ:
			return "XZ";
		case ShakeType::YZ:
			return "YZ";
		case ShakeType::XYZ:
		default:
			return "XYZ";
		}
	}

	/// @brief Json表示名からShake軸を取得
	/// @param value 読み込んだShake軸名
	/// @return 名前に対応するShake軸
	ShakeType ShakeTypeFromString(const std::string& value) {
		if (value == "X") {
			return ShakeType::X;
		}
		if (value == "Y") {
			return ShakeType::Y;
		}
		if (value == "Z") {
			return ShakeType::Z;
		}
		if (value == "XY") {
			return ShakeType::XY;
		}
		if (value == "XZ") {
			return ShakeType::XZ;
		}
		if (value == "YZ") {
			return ShakeType::YZ;
		}
		return ShakeType::XYZ;
	}

} // namespace

CameraHandle CameraManager::RegisterCamera(
	const std::string& name,
	std::unique_ptr<Camera> camera,
	MadoEngine::EditorManagementMode managementMode) {
	if (name.empty() || !camera || cameraHandlesByName_.contains(name)) {
		return {};
	}

	uint32_t index = 0;
	if (freeIndices_.empty()) {
		index = static_cast<uint32_t>(cameraEntries_.size());
		cameraEntries_.push_back({});
	} else {
		index = freeIndices_.back();
		freeIndices_.pop_back();
	}

	CameraEntry& entry = cameraEntries_[index];
	entry.camera = std::move(camera);
	entry.name = name;
	entry.transitionSettings = {};
	entry.shakeSettings = {};
	entry.managementMode = managementMode;
	const CameraHandle handle{ index, entry.generation };
	cameraHandlesByName_.emplace(name, handle);
	return handle;
}

Camera* CameraManager::TryGetCamera(CameraHandle handle) {
	CameraEntry* entry = TryGetEntry(handle);
	return entry ? entry->camera.get() : nullptr;
}

const Camera* CameraManager::TryGetCamera(CameraHandle handle) const {
	const CameraEntry* entry = TryGetEntry(handle);
	return entry ? entry->camera.get() : nullptr;
}

CameraHandle CameraManager::Find(const std::string& name) const {
	const auto cameraIt = cameraHandlesByName_.find(name);
	return cameraIt != cameraHandlesByName_.end() ? cameraIt->second : CameraHandle{};
}

std::vector<CameraHandle> CameraManager::GetCameraHandles() const {
	std::vector<CameraHandle> handles;
	handles.reserve(cameraHandlesByName_.size());
	for (uint32_t index = 0; index < static_cast<uint32_t>(cameraEntries_.size()); ++index) {
		const CameraEntry& entry = cameraEntries_[index];
		if (entry.camera) {
			handles.push_back({ index, entry.generation });
		}
	}
	return handles;
}

std::string CameraManager::GetCameraName(CameraHandle handle) const {
	const CameraEntry* entry = TryGetEntry(handle);
	return entry ? entry->name : std::string{};
}

bool CameraManager::RenameCamera(CameraHandle handle, const std::string& newName) {
	CameraEntry* entry = TryGetEntry(handle);
	if (!entry || newName.empty()) {
		return false;
	}

	if (entry->name == newName) {
		return true;
	}

	if (cameraHandlesByName_.contains(newName)) {
		return false;
	}

	cameraHandlesByName_.erase(entry->name);
	entry->name = newName;
	cameraHandlesByName_.emplace(entry->name, handle);
	return true;
}

bool CameraManager::IsValid(CameraHandle handle) const {
	return TryGetEntry(handle) != nullptr;
}

bool CameraManager::IsEditorManaged(CameraHandle handle) const {
	const CameraEntry* entry = TryGetEntry(handle);
	return entry && entry->managementMode == MadoEngine::EditorManagementMode::EditorManaged;
}

CameraTransitionSettings CameraManager::GetTransitionSettings(CameraHandle handle) const {
	const CameraEntry* entry = TryGetEntry(handle);
	return entry ? entry->transitionSettings : CameraTransitionSettings{};
}

bool CameraManager::SetTransitionSettings(
	CameraHandle handle,
	const CameraTransitionSettings& settings) {
	CameraEntry* entry = TryGetEntry(handle);
	if (!entry) {
		return false;
	}

	if (settings.destinationHandle.IsValid() &&
		(!IsValid(settings.destinationHandle) || settings.destinationHandle == handle)) {
		return false;
	}

	entry->transitionSettings = settings;
	entry->transitionSettings.blendDuration =
		std::isfinite(settings.blendDuration) ? (std::max)(settings.blendDuration, 0.0f) : 0.5f;
	return true;
}

bool CameraManager::ExecuteTransition(CameraHandle handle) {
	const CameraEntry* entry = TryGetEntry(handle);
	if (!entry || !IsValid(entry->transitionSettings.destinationHandle)) {
		return false;
	}

	const CameraTransitionSettings settings = entry->transitionSettings;
	if (settings.mode == CameraTransitionMode::Blend) {
		return BlendTo(
			settings.destinationHandle,
			settings.blendDuration,
			settings.blendEaseType
		);
	}
	return CutTo(settings.destinationHandle);
}

CameraShakeSettings CameraManager::GetShakeSettings(CameraHandle handle) const {
	const CameraEntry* entry = TryGetEntry(handle);
	return entry ? entry->shakeSettings : CameraShakeSettings{};
}

bool CameraManager::SetShakeSettings(
	CameraHandle handle,
	const CameraShakeSettings& settings) {
	CameraEntry* entry = TryGetEntry(handle);
	if (!entry) {
		return false;
	}

	entry->shakeSettings = settings;
	entry->shakeSettings.power =
		std::isfinite(settings.power)
		? std::clamp(settings.power, 0.0f, kMaxShakePower)
		: 0.25f;
	entry->shakeSettings.duration =
		std::isfinite(settings.duration)
		? std::clamp(settings.duration, 0.0f, kMaxShakeDuration)
		: 0.5f;
	return true;
}

bool CameraManager::ExecuteShake(CameraHandle handle) {
	CameraEntry* entry = TryGetEntry(handle);
	if (!entry || entry->shakeSettings.power <= 0.0f || entry->shakeSettings.duration <= 0.0f) {
		return false;
	}

	const CameraShakeSettings settings = entry->shakeSettings;
	entry->camera->Shake(settings.power, settings.duration, settings.type);
	return true;
}

float CameraManager::GetBlendProgress() const {
	if (!isBlending_ || blendDuration_ <= 0.0f) {
		return 0.0f;
	}
	return std::clamp(blendElapsedTime_ / blendDuration_, 0.0f, 1.0f);
}

bool CameraManager::DestroyCamera(CameraHandle handle) {
	CameraEntry* entry = TryGetEntry(handle);
	if (!entry) {
		return false;
	}
	const bool wasActive = handle == activeCameraHandle_;

	cameraHandlesByName_.erase(entry->name);
	entry->camera.reset();
	entry->name.clear();
	entry->transitionSettings = {};
	entry->shakeSettings = {};
	entry->managementMode = MadoEngine::EditorManagementMode::RuntimeOnly;
	entry->generation = MadoEngine::NextGeneration(entry->generation);
	freeIndices_.push_back(handle.index);

	// 削除Cameraを参照する遷移設定を無効化して世代更新後の別Cameraへの誤接続を防止
	for (CameraEntry& otherEntry : cameraEntries_) {
		if (otherEntry.camera && otherEntry.transitionSettings.destinationHandle == handle) {
			otherEntry.transitionSettings.destinationHandle = {};
		}
	}

	// 破棄対象を遷移先に含む演出は継続できないため現在の描画状態で停止
	if (handle == blendDestinationHandle_) {
		ResetBlend();
	}
	if (wasActive) {
		activeCameraHandle_ = {};

		// Active Camera削除後も描画を継続できるよう登録順で次のCameraへ切り替え
		const std::vector<CameraHandle> remainingHandles = GetCameraHandles();
		if (!remainingHandles.empty()) {
			CutTo(remainingHandles.front());
		}
	}
	return true;
}

void CameraManager::Clear() {
	cameraHandlesByName_.clear();
	freeIndices_.clear();

	// Clear前のHandleが再登録Cameraを参照しないよう全Slotの世代を更新
	for (uint32_t index = 0; index < static_cast<uint32_t>(cameraEntries_.size()); ++index) {
		CameraEntry& entry = cameraEntries_[index];
		entry.camera.reset();
		entry.name.clear();
		entry.transitionSettings = {};
		entry.shakeSettings = {};
		entry.managementMode = MadoEngine::EditorManagementMode::RuntimeOnly;
		entry.generation = MadoEngine::NextGeneration(entry.generation);
		freeIndices_.push_back(index);
	}

	activeCameraHandle_ = {};
	ResetBlend();
	renderCamera_ = Camera{};
}

std::filesystem::path CameraManager::CreateDefaultJsonPath(const std::string& sceneName) {
	const std::string validSceneName = sceneName.empty() ? "Unknown" : sceneName;
	return std::filesystem::path(kDefaultCameraJsonDirectory) / (validSceneName + ".json");
}

bool CameraManager::SaveToJson(const std::filesystem::path& filePath) const {
	nlohmann::json root = nlohmann::json::object();
	root["version"] = kCameraJsonVersion;
	root["activeCamera"] = IsValid(activeCameraHandle_)
		? GetCameraName(activeCameraHandle_)
		: std::string{};
	root["cameras"] = nlohmann::json::array();
	root["transitions"] = nlohmann::json::array();
	root["shakes"] = nlohmann::json::array();

	// Runtime Cameraを永続化せずEditorで配置した固定Cameraだけを保存
	for (const CameraEntry& entry : cameraEntries_) {
		if (!entry.camera || entry.managementMode != MadoEngine::EditorManagementMode::EditorManaged) {
			continue;
		}

		const Camera& camera = *entry.camera;
		nlohmann::json cameraJson = {
			{ "name", entry.name },
			{ "position", MadoEngine::Json::JsonSerializer::ToJson(camera.GetPosition()) },
			{ "rotation", MadoEngine::Json::JsonSerializer::ToJson(camera.GetRotation()) },
			{ "fovY", camera.GetFovY() },
			{ "aspectRatio", camera.GetAspectRatio() },
			{ "nearClip", camera.GetNearClip() },
			{ "farClip", camera.GetFarClip() },
		};
		root["cameras"].push_back(std::move(cameraJson));
	}

	// Runtime Cameraを含む全Cameraの演出プリセットを安定したCamera名で保存
	for (const CameraEntry& entry : cameraEntries_) {
		if (!entry.camera) {
			continue;
		}

		const CameraTransitionSettings& transition = entry.transitionSettings;
		root["transitions"].push_back({
			{ "camera", entry.name },
			{ "mode", CameraTransitionModeToString(transition.mode) },
			{ "destination", IsValid(transition.destinationHandle)
				? GetCameraName(transition.destinationHandle)
				: std::string{} },
			{ "blendDuration", transition.blendDuration },
			{ "blendEaseType", static_cast<int>(transition.blendEaseType) },
		});

		const CameraShakeSettings& shake = entry.shakeSettings;
		root["shakes"].push_back({
			{ "camera", entry.name },
			{ "power", shake.power },
			{ "duration", shake.duration },
			{ "type", ShakeTypeToString(shake.type) },
		});
	}

	const bool isSaved = MadoEngine::Json::JsonFile::Save(filePath, root, 4, true);
	Logger::Output(
		isSaved
			? "Camera設定をJsonへ保存しました : " + filePath.generic_string()
			: "Camera設定のJson保存に失敗しました : " + filePath.generic_string(),
		isSaved ? Logger::Level::Application : Logger::Level::Error
	);
	return isSaved;
}

bool CameraManager::LoadFromJson(const std::filesystem::path& filePath) {
	nlohmann::json root;
	if (!MadoEngine::Json::JsonFile::Load(filePath, root)) {
		Logger::Output(
			"Camera設定をJsonから読み込めませんでした : " + filePath.generic_string(),
			Logger::Level::Error
		);
		return false;
	}

	if (!root.is_object() || !root.contains("cameras") || !root.at("cameras").is_array()) {
		Logger::Output(
			"Camera設定のJson形式が不正です : " + filePath.generic_string(),
			Logger::Level::Error
		);
		return false;
	}

	struct CameraLoadDesc {
		std::string name;
		Vector3 position;
		Vector3 rotation;
		float fovY = 0.45f;
		float aspectRatio = 16.0f / 9.0f;
		float nearClip = 0.1f;
		float farClip = 1000.0f;
		CameraTransitionMode transitionMode = CameraTransitionMode::Cut;
		std::string transitionDestinationName;
		float blendDuration = 0.5f;
		EaseType blendEaseType = EaseType::EaseInOutCubic;
	};

	std::unordered_set<std::string> runtimeCameraNames;
	for (const CameraEntry& entry : cameraEntries_) {
		if (entry.camera && entry.managementMode == MadoEngine::EditorManagementMode::RuntimeOnly) {
			runtimeCameraNames.insert(entry.name);
		}
	}

	std::vector<CameraLoadDesc> loadDescs;
	std::unordered_set<std::string> loadedNames;
	for (const nlohmann::json& cameraJson : root.at("cameras")) {
		if (!cameraJson.is_object()) {
			Logger::Output("Camera設定Json内の不正な要素をスキップしました", Logger::Level::Warning);
			continue;
		}

		CameraLoadDesc desc;
		desc.name = MadoEngine::Json::JsonSerializer::GetOrDefault<std::string>(cameraJson, "name", "");
		if (desc.name.empty() || runtimeCameraNames.contains(desc.name) || !loadedNames.insert(desc.name).second) {
			Logger::Output(
				"重複または空のCamera名をスキップしました : " + desc.name,
				Logger::Level::Warning
			);
			continue;
		}

		const Camera defaultCamera;
		desc.position = cameraJson.contains("position")
			? MadoEngine::Json::JsonSerializer::ToVector3(cameraJson.at("position"), defaultCamera.GetPosition())
			: defaultCamera.GetPosition();
		desc.rotation = cameraJson.contains("rotation")
			? MadoEngine::Json::JsonSerializer::ToVector3(cameraJson.at("rotation"), defaultCamera.GetRotation())
			: defaultCamera.GetRotation();
		if (!IsFiniteVector3(desc.position)) {
			desc.position = defaultCamera.GetPosition();
		}
		if (!IsFiniteVector3(desc.rotation)) {
			desc.rotation = defaultCamera.GetRotation();
		}

		desc.fovY = ReadClampedFloat(cameraJson, "fovY", defaultCamera.GetFovY(), kMinProjectionValue, std::numbers::pi_v<float> - kMinProjectionValue);
		desc.aspectRatio = ReadClampedFloat(cameraJson, "aspectRatio", defaultCamera.GetAspectRatio(), kMinProjectionValue, 100.0f);
		desc.nearClip = ReadClampedFloat(cameraJson, "nearClip", defaultCamera.GetNearClip(), kMinProjectionValue, 100000.0f);
		desc.farClip = ReadClampedFloat(cameraJson, "farClip", defaultCamera.GetFarClip(), desc.nearClip + kMinProjectionValue, 1000000.0f);

		if (cameraJson.contains("transition") && cameraJson.at("transition").is_object()) {
			const nlohmann::json& transitionJson = cameraJson.at("transition");
			desc.transitionMode = CameraTransitionModeFromString(
				MadoEngine::Json::JsonSerializer::GetOrDefault<std::string>(
					transitionJson,
					"mode",
					"Cut"
				)
			);
			desc.transitionDestinationName =
				MadoEngine::Json::JsonSerializer::GetOrDefault<std::string>(
					transitionJson,
					"destination",
					""
				);
			desc.blendDuration = ReadClampedFloat(
				transitionJson,
				"blendDuration",
				0.5f,
				0.0f,
				300.0f
			);
			const int easeTypeValue = std::clamp(
				MadoEngine::Json::JsonSerializer::GetOrDefault<int>(
					transitionJson,
					"blendEaseType",
					static_cast<int>(EaseType::EaseInOutCubic)
				),
				static_cast<int>(EaseType::Linear),
				static_cast<int>(EaseType::None)
			);
			desc.blendEaseType = static_cast<EaseType>(easeTypeValue);
		}
		loadDescs.push_back(std::move(desc));
	}

	const std::string activeCameraName =
		MadoEngine::Json::JsonSerializer::GetOrDefault<std::string>(root, "activeCamera", "");

	// Json形式の検証完了後に既存Editor Cameraだけを読み込み内容へ置換
	ClearEditorManagedCameras();
	for (const CameraLoadDesc& desc : loadDescs) {
		const CameraHandle handle = CreateEditorCamera<Camera>(desc.name);
		Camera* camera = TryGetCamera(handle);
		if (!camera) {
			continue;
		}
		camera->SetPosition(desc.position);
		camera->SetRotation(desc.rotation);
		camera->SetFovY(desc.fovY);
		camera->SetAspectRatio(desc.aspectRatio);
		camera->SetNearClip(desc.nearClip);
		camera->SetFarClip(desc.farClip);
		camera->Update(0.0f);
	}

	// 全Camera生成後に名前をHandleへ解決して前方参照を含む遷移設定を復元
	for (const CameraLoadDesc& desc : loadDescs) {
		const CameraHandle ownerHandle = Find(desc.name);
		if (!ownerHandle.IsValid()) {
			continue;
		}

		CameraTransitionSettings transition;
		transition.mode = desc.transitionMode;
		transition.blendDuration = desc.blendDuration;
		transition.blendEaseType = desc.blendEaseType;
		transition.destinationHandle = Find(desc.transitionDestinationName);
		if (transition.destinationHandle == ownerHandle) {
			transition.destinationHandle = {};
		}
		SetTransitionSettings(ownerHandle, transition);
	}

	if (root.contains("transitions") && root.at("transitions").is_array()) {
		for (const nlohmann::json& transitionJson : root.at("transitions")) {
			if (!transitionJson.is_object()) {
				continue;
			}

			const std::string ownerName =
				MadoEngine::Json::JsonSerializer::GetOrDefault<std::string>(
					transitionJson,
					"camera",
					""
				);
			const CameraHandle ownerHandle = Find(ownerName);
			if (!ownerHandle.IsValid()) {
				continue;
			}

			CameraTransitionSettings transition;
			transition.mode = CameraTransitionModeFromString(
				MadoEngine::Json::JsonSerializer::GetOrDefault<std::string>(
					transitionJson,
					"mode",
					"Cut"
				)
			);
			const std::string destinationName =
				MadoEngine::Json::JsonSerializer::GetOrDefault<std::string>(
					transitionJson,
					"destination",
					""
				);
			transition.destinationHandle = Find(destinationName);
			if (transition.destinationHandle == ownerHandle) {
				transition.destinationHandle = {};
			}
			transition.blendDuration = ReadClampedFloat(
				transitionJson,
				"blendDuration",
				0.5f,
				0.0f,
				300.0f
			);
			const int easeTypeValue = std::clamp(
				MadoEngine::Json::JsonSerializer::GetOrDefault<int>(
					transitionJson,
					"blendEaseType",
					static_cast<int>(EaseType::EaseInOutCubic)
				),
				static_cast<int>(EaseType::Linear),
				static_cast<int>(EaseType::None)
			);
			transition.blendEaseType = static_cast<EaseType>(easeTypeValue);
			SetTransitionSettings(ownerHandle, transition);
		}
	}

	if (root.contains("shakes") && root.at("shakes").is_array()) {
		for (const nlohmann::json& shakeJson : root.at("shakes")) {
			if (!shakeJson.is_object()) {
				continue;
			}

			const std::string ownerName =
				MadoEngine::Json::JsonSerializer::GetOrDefault<std::string>(
					shakeJson,
					"camera",
					""
				);
			const CameraHandle ownerHandle = Find(ownerName);
			if (!ownerHandle.IsValid()) {
				continue;
			}

			CameraShakeSettings shake;
			shake.power = ReadClampedFloat(
				shakeJson,
				"power",
				0.25f,
				0.0f,
				kMaxShakePower
			);
			shake.duration = ReadClampedFloat(
				shakeJson,
				"duration",
				0.5f,
				0.0f,
				kMaxShakeDuration
			);
			shake.type = ShakeTypeFromString(
				MadoEngine::Json::JsonSerializer::GetOrDefault<std::string>(
					shakeJson,
					"type",
					"XYZ"
				)
			);
			SetShakeSettings(ownerHandle, shake);
		}
	}

	if (const CameraHandle savedActiveHandle = Find(activeCameraName); savedActiveHandle.IsValid()) {
		CutTo(savedActiveHandle);
	} else if (!IsValid(activeCameraHandle_)) {
		const std::vector<CameraHandle> handles = GetCameraHandles();
		if (!handles.empty()) {
			CutTo(handles.front());
		}
	}

	Logger::Output(
		"Camera設定をJsonから読み込みました : " + filePath.generic_string() +
		" 件数 : " + std::to_string(loadDescs.size()),
		Logger::Level::Application
	);
	return true;
}

bool CameraManager::CutTo(CameraHandle handle) {
	Camera* camera = TryGetCamera(handle);
	if (!camera) {
		return false;
	}

	activeCameraHandle_ = handle;
	ResetBlend();

	// 即時切り替えではCamera Shakeを含む計算済み描画状態をそのまま採用
	renderCamera_ = *camera;
	return true;
}

bool CameraManager::BlendTo(CameraHandle handle, float duration, EaseType easeType) {
	Camera* destinationCamera = TryGetCamera(handle);
	if (!destinationCamera) {
		return false;
	}

	if (duration <= 0.0f || !IsValid(activeCameraHandle_)) {
		return CutTo(handle);
	}

	if (!isBlending_ && handle == activeCameraHandle_) {
		renderCamera_ = *destinationCamera;
		return true;
	}

	// 補間中の再切り替えでも見た目が飛ばないよう現在の描画Cameraを開始状態として固定
	blendSourceSnapshot_ = renderCamera_;
	blendDestinationHandle_ = handle;
	blendDuration_ = duration;
	blendElapsedTime_ = 0.0f;
	blendEaseType_ = easeType;
	isBlending_ = true;
	return true;
}

void CameraManager::Update(float deltaTime) {

	// 非アクティブCameraも追従状態を維持し、切り替え直後の位置飛びを防止
	for (CameraEntry& entry : cameraEntries_) {
		if (entry.camera) {
			entry.camera->Update(deltaTime);
		}
	}

	if (!isBlending_) {
		if (Camera* activeCamera = TryGetCamera(activeCameraHandle_)) {
			renderCamera_ = *activeCamera;
		}
		return;
	}

	Camera* destinationCamera = TryGetCamera(blendDestinationHandle_);
	if (!destinationCamera) {
		ResetBlend();
		return;
	}

	blendElapsedTime_ += (std::max)(deltaTime, 0.0f);
	const float normalizedProgress = std::clamp(blendElapsedTime_ / blendDuration_, 0.0f, 1.0f);
	const float easedProgress = Easing::Apply(normalizedProgress, blendEaseType_);
	UpdateBlendedCamera(blendSourceSnapshot_, *destinationCamera, easedProgress);

	if (normalizedProgress >= 1.0f) {

		// 補間終了Frameは遷移先のCamera状態へ完全一致させて以後の追従へ接続
		activeCameraHandle_ = blendDestinationHandle_;
		renderCamera_ = *destinationCamera;
		ResetBlend();
	}
}

CameraManager::CameraEntry* CameraManager::TryGetEntry(CameraHandle handle) {
	if (!handle.IsValid() || handle.index >= cameraEntries_.size()) {
		return nullptr;
	}

	CameraEntry& entry = cameraEntries_[handle.index];
	if (!entry.camera || entry.generation != handle.generation) {
		return nullptr;
	}
	return &entry;
}

const CameraManager::CameraEntry* CameraManager::TryGetEntry(CameraHandle handle) const {
	if (!handle.IsValid() || handle.index >= cameraEntries_.size()) {
		return nullptr;
	}

	const CameraEntry& entry = cameraEntries_[handle.index];
	if (!entry.camera || entry.generation != handle.generation) {
		return nullptr;
	}
	return &entry;
}

void CameraManager::ResetBlend() {
	blendDestinationHandle_ = {};
	blendDuration_ = 0.0f;
	blendElapsedTime_ = 0.0f;
	blendEaseType_ = EaseType::Linear;
	isBlending_ = false;
}

void CameraManager::ClearEditorManagedCameras() {
	std::vector<CameraHandle> editorCameraHandles;
	for (uint32_t index = 0; index < static_cast<uint32_t>(cameraEntries_.size()); ++index) {
		const CameraEntry& entry = cameraEntries_[index];
		if (entry.camera && entry.managementMode == MadoEngine::EditorManagementMode::EditorManaged) {
			editorCameraHandles.push_back({ index, entry.generation });
		}
	}

	// 列挙中のSlot変更を避けるため削除対象を確定してから個別に破棄
	for (CameraHandle handle : editorCameraHandles) {
		DestroyCamera(handle);
	}
}

void CameraManager::UpdateBlendedCamera(
	const Camera& source,
	const Camera& destination,
	float progress) {
	const Vector3 sourceRotation = source.GetRotation();
	const Vector3 destinationRotation = destination.GetRotation();
	const Vector3 blendedRotation = {
		LerpAngle(sourceRotation.x, destinationRotation.x, progress),
		LerpAngle(sourceRotation.y, destinationRotation.y, progress),
		LerpAngle(sourceRotation.z, destinationRotation.z, progress),
	};

	const float nearClip = (std::max)(
		LerpValue(source.GetNearClip(), destination.GetNearClip(), progress),
		kMinProjectionValue
	);
	const float farClip = (std::max)(
		LerpValue(source.GetFarClip(), destination.GetFarClip(), progress),
		nearClip + kMinProjectionValue
	);

	// BackやElasticの行き過ぎを位置と回転へ許可しつつ射影値だけ有効範囲へ制限
	renderCamera_.SetPosition(Math::Lerp(source.GetPosition(), destination.GetPosition(), progress));
	renderCamera_.SetRotation(blendedRotation);
	renderCamera_.SetFovY((std::max)(
		LerpValue(source.GetFovY(), destination.GetFovY(), progress),
		kMinProjectionValue
	));
	renderCamera_.SetAspectRatio((std::max)(
		LerpValue(source.GetAspectRatio(), destination.GetAspectRatio(), progress),
		kMinProjectionValue
	));
	renderCamera_.SetNearClip(nearClip);
	renderCamera_.SetFarClip(farClip);
	renderCamera_.Update(0.0f);
}
