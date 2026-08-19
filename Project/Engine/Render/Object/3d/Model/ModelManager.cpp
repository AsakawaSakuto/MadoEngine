#include "ModelManager.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <utility>

namespace {

/// @brief 文字列を小文字へ変換
/// @param value 変換する文字列
/// @return 小文字へ変換した文字列
std::string ToLower(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});
	return value;
}

/// @brief ファイルパスを正規化
/// @param path 正規化するパス
/// @return 汎用形式へ正規化したパス
std::string NormalizePath(const std::filesystem::path& path) {
	return path.lexically_normal().generic_string();
}

/// @brief Modelが現在のSceneとLayerMaskの描画対象か判定
/// @param model 判定するModel
/// @param currentSceneType 現在のScene
/// @param layerMask 描画対象のLayerMask
/// @return 描画対象の場合はtrue
template <typename ModelObject>
bool IsDrawTarget(
	const ModelObject& model,
	SceneType currentSceneType,
	MadoEngine::Render::RenderLayerMask layerMask) {
	if (!model.IsVisible()) {
		return false;
	}

	const SceneType modelScene = model.GetSceneType();
	if (modelScene != SceneType::None && modelScene != currentSceneType) {
		return false;
	}

	return model.IsRenderLayerIncluded(layerMask);
}

/// @brief 対応しているModel拡張子か確認
/// @param path 確認するパス
/// @return 対応している場合はtrue
bool IsModelExtension(const std::filesystem::path& path) {
	const std::string extension = ToLower(path.extension().string());
	return extension == ".obj" || extension == ".gltf" || extension == ".glb";
}

/// @brief Modelの親ディレクトリからModel種別を取得
/// @param modelRoot Modelルート
/// @param path Modelファイルパス
/// @param outType 取得したModel種別
/// @return 種別を取得できた場合はtrue
bool TryGetModelType(const std::filesystem::path& modelRoot, const std::filesystem::path& path, ModelType& outType) {
	std::error_code error;
	const std::filesystem::path relative = std::filesystem::relative(path.parent_path(), modelRoot, error);
	if (error || relative.empty()) {
		return false;
	}

	const std::string firstDirectory = ToLower((*relative.begin()).string());
	if (firstDirectory == "static") {
		outType = ModelType::Static;
		return true;
	}
	if (firstDirectory == "animation" || firstDirectory == "animated") {
		outType = ModelType::Animated;
		return true;
	}
	if (firstDirectory == "skining" || firstDirectory == "skinning") {
		outType = ModelType::Skinning;
		return true;
	}
	return false;
}

/// @brief Animation専用として参照されるModelファイルを収集
/// @param modelRoot Modelルート
/// @return 通常Model読み込みから除外する正規化済みパス集合
std::unordered_set<std::string> CollectAnimationOnlyPaths(const std::filesystem::path& modelRoot) {
	std::unordered_set<std::string> animationOnlyPaths;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(modelRoot)) {
		if (!entry.is_regular_file() ||
			!entry.path().filename().string().ends_with(".animations.json")) {
			continue;
		}

		std::ifstream manifestFile(entry.path());
		if (!manifestFile.is_open()) {
			continue;
		}

		try {
			nlohmann::json manifestJson;
			manifestFile >> manifestJson;
			const std::string modelFile = manifestJson.value("model", std::string{});
			if (modelFile.empty()) {
				continue;
			}

			const std::filesystem::path manifestDirectory = entry.path().parent_path();
			const std::string baseModelPath = NormalizePath(manifestDirectory / modelFile);
			const auto clipsIterator = manifestJson.find("clips");
			if (clipsIterator == manifestJson.end() || !clipsIterator->is_object()) {
				continue;
			}

			// 基準Modelと異なる参照先だけをAnimation専用Assetとして除外
			for (const auto& [clipName, clipJson] : clipsIterator->items()) {
				(void)clipName;
				if (!clipJson.is_object()) {
					continue;
				}
				const std::string sourceFile = clipJson.value("file", std::string{});
				if (sourceFile.empty()) {
					continue;
				}
				const std::string sourcePath = NormalizePath(manifestDirectory / sourceFile);
				if (sourcePath != baseModelPath) {
					animationOnlyPaths.insert(sourcePath);
				}
			}
		} catch (const nlohmann::json::exception& exception) {
			Logger::Output(
				"Animation設定の解析に失敗しました: " + entry.path().generic_string() + " / " + exception.what(),
				Logger::Level::Error
			);
		}
	}

	return animationOnlyPaths;
}

} // namespace

namespace MadoEngine {

ModelManager& ModelManager::GetInstance() {
	static ModelManager instance;
	return instance;
}

void ModelManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Render::PSORegistry* psoRegistry) {
	assert(device);
	assert(commandList);
	assert(psoRegistry);
	device_ = device;
	commandList_ = commandList;
	psoRegistry_ = psoRegistry;
	LoadAllModels();
	Logger::Output(std::to_string(sharedData_.size()) + "件のModelアセットを読み込みました", Logger::Level::Engine);
}

void ModelManager::Finalize() {
	pendingDestroyModelHandles_.clear();
	modelNameToHandle_.clear();
	freeModelSlots_.clear();
	freeModelSlots_.reserve(modelSlots_.size());
	for (uint32_t index = 0; index < modelSlots_.size(); ++index) {
		ModelSlot& slot = modelSlots_[index];
		slot.model.reset();
		slot.name.clear();
		slot.assetName.clear();
		slot.managementMode = EditorManagementMode::RuntimeOnly;
		slot.active = false;
		slot.generation = NextObjectGeneration(slot.generation);
		freeModelSlots_.push_back(index);
	}

	pendingDestroyInstancedModelHandles_.clear();
	instancedModelNameToHandle_.clear();
	freeInstancedModelSlots_.clear();
	freeInstancedModelSlots_.reserve(instancedModelSlots_.size());
	for (uint32_t index = 0; index < instancedModelSlots_.size(); ++index) {
		InstancedModelSlot& slot = instancedModelSlots_[index];
		slot.model.reset();
		slot.name.clear();
		slot.assetName.clear();
		slot.managementMode = EditorManagementMode::RuntimeOnly;
		slot.active = false;
		slot.generation = NextObjectGeneration(slot.generation);
		freeInstancedModelSlots_.push_back(index);
	}

	for (auto& [key, data] : sharedData_) {
		(void)key;
		ModelResource::Finalize(*data);
	}
	sharedData_.clear();
	aliases_.clear();
	activeCamera_ = {};
	device_ = nullptr;
	commandList_ = nullptr;
	psoRegistry_ = nullptr;
	Logger::Output("ModelManagerの全リソースを解放しました", Logger::Level::Engine);
}

void ModelManager::LoadAllModels() {
	const std::filesystem::path modelRoot = "Assets/Model";
	if (!std::filesystem::exists(modelRoot)) {
		Logger::Output("Assets/Modelフォルダが見つかりません", Logger::Level::Warning);
		return;
	}
	const std::unordered_set<std::string> animationOnlyPaths = CollectAnimationOnlyPaths(modelRoot);

	for (const auto& entry : std::filesystem::recursive_directory_iterator(modelRoot)) {
		if (!entry.is_regular_file() || !IsModelExtension(entry.path())) {
			continue;
		}
		const std::string normalizedPath = NormalizePath(entry.path());
		if (animationOnlyPaths.contains(normalizedPath)) {
			continue;
		}
		ModelType type = ModelType::Static;
		if (!TryGetModelType(modelRoot, entry.path(), type)) {
			continue;
		}
		LoadModelFile(normalizedPath, type);
	}
}

void ModelManager::LoadModelFile(const std::string& path, ModelType type) {
	if (sharedData_.contains(path)) {
		return;
	}

	auto data = std::make_unique<ModelSharedData>();
	ModelResource::Initialize(*data, device_, path, type);
	const std::string stem = std::filesystem::path(path).stem().string();
	if (!aliases_.contains(stem)) {
		aliases_.emplace(stem, path);
	} else {
		Logger::Output("Modelアセットの別名が重複しています。パスを使用してください: " + stem, Logger::Level::Warning);
	}
	aliases_.emplace(path, path);
	sharedData_.emplace(path, std::move(data));
	Logger::Output("Modelアセットを読み込みました: " + path, Logger::Level::Assets);
}

ModelHandle ModelManager::Create(
	const std::string& name,
	const std::string& modelName,
	SceneType sceneType,
	EditorManagementMode managementMode) {
	return Create(ModelCreateDesc{ name, modelName, sceneType, managementMode });
}

ModelHandle ModelManager::Create(const ModelCreateDesc& desc) {
	if (desc.name.empty()) {
		Logger::Output("名前が空のModelは生成できません", Logger::Level::Warning);
		return {};
	}
	if (modelNameToHandle_.contains(desc.name)) {
		Logger::Output("同名のModelが既に存在するため生成に失敗しました: " + desc.name, Logger::Level::Warning);
		return {};
	}

	const std::string resolvedModelPath = ResolveModelPath(desc.modelName);
	const ModelSharedData* sharedData = FindSharedData(resolvedModelPath);
	if (!sharedData) {
		Logger::Output("Modelアセットが見つからないため生成に失敗しました: " + desc.modelName, Logger::Level::Warning);
		return {};
	}

	auto model = std::make_unique<Model>(desc.name);
	model->Initialize(device_, commandList_, *sharedData);
	model->SetPSORegistry(psoRegistry_);
	model->SetSceneType(desc.sceneType);

	// 破棄済みSlotを再利用してHandle Indexの増加を抑制
	uint32_t index = 0;
	if (freeModelSlots_.empty()) {
		index = static_cast<uint32_t>(modelSlots_.size());
		modelSlots_.emplace_back();
	} else {
		index = freeModelSlots_.back();
		freeModelSlots_.pop_back();
	}

	ModelSlot& slot = modelSlots_[index];
	slot.model = std::move(model);
	slot.name = desc.name;
	slot.assetName = resolvedModelPath;
	slot.managementMode = desc.managementMode;
	slot.active = true;
	const ModelHandle handle{ index, slot.generation };
	modelNameToHandle_.emplace(slot.name, handle);
	Logger::Output("Modelを生成しました: " + desc.name + " アセット: " + resolvedModelPath, Logger::Level::Application);
	return handle;
}

ModelHandle ModelManager::FindOrCreate(const ModelCreateDesc& desc) {
	const ModelHandle existing = Find(desc.name);
	if (!existing.IsValid()) {
		return Create(desc);
	}

	const ModelSlot& slot = modelSlots_[existing.index];
	if (slot.assetName != ResolveModelPath(desc.modelName) ||
		slot.model->GetSceneType() != desc.sceneType ||
		slot.managementMode != desc.managementMode) {
		Logger::Output("既存Modelの生成条件が一致しません: " + desc.name, Logger::Level::Warning);
		return {};
	}
	return existing;
}

ModelHandle ModelManager::CreateFromJson(const nlohmann::json& json) {
	if (!json.is_object()) {
		Logger::Output("ModelのJSON要素がオブジェクトではありません", Logger::Level::Warning);
		return {};
	}

	const std::string name = json.value("name", "Model");
	const std::string modelName = json.value("model", "");
	if (modelName.empty()) {
		Logger::Output("ModelのJSONにmodelが設定されていません: " + name, Logger::Level::Warning);
		return {};
	}

	ModelHandle handle = Find(name);
	if (handle.IsValid()) {
		ModelSlot& slot = modelSlots_[handle.index];

		// JSON復元で実行時専用Objectを上書きしない管理境界
		if (slot.managementMode == EditorManagementMode::RuntimeOnly) {
			Logger::Output("実行時専用Modelと同名のためJSON読み込みをスキップしました: " + name, Logger::Level::Warning);
			return {};
		}
		if (slot.assetName != ResolveModelPath(modelName)) {

			// Asset差し替えでは共有DataとGPU Resourceの再初期化が必要なためInstanceを再生成
			Destroy(handle);
			handle = {};
		}
	}
	if (!handle.IsValid()) {
		handle = Create(name, modelName, SceneType::None, EditorManagementMode::EditorManaged);
	}

	Model* model = TryGet(handle);
	if (!model) {
		return {};
	}
	model->FromJson(json);
	return handle;
}

Model* ModelManager::TryGet(ModelHandle handle) {
	return const_cast<Model*>(std::as_const(*this).TryGet(handle));
}

const Model* ModelManager::TryGet(ModelHandle handle) const {
	if (!handle.IsValid() || handle.index >= modelSlots_.size()) {
		return nullptr;
	}
	const ModelSlot& slot = modelSlots_[handle.index];
	if (!slot.active || slot.generation != handle.generation) {
		return nullptr;
	}
	return slot.model.get();
}

ModelHandle ModelManager::Find(const std::string& name) const {
	const auto it = modelNameToHandle_.find(name);
	return it == modelNameToHandle_.end() ? ModelHandle{} : it->second;
}

bool ModelManager::IsValid(ModelHandle handle) const {
	return TryGet(handle) != nullptr;
}

Model* ModelManager::Get(const std::string& name) {
	return TryGet(Find(name));
}

const Model* ModelManager::Get(const std::string& name) const {
	return TryGet(Find(name));
}

bool ModelManager::Rename(ModelHandle handle, const std::string& newName) {
	Model* model = TryGet(handle);
	if (!model) {
		Logger::Output("名前を変更するModelのHandleが無効です", Logger::Level::Warning);
		return false;
	}
	if (newName.empty()) {
		Logger::Output("Model名を空文字へ変更できません", Logger::Level::Warning);
		return false;
	}

	ModelSlot& slot = modelSlots_[handle.index];
	if (slot.name == newName) {
		return true;
	}
	if (modelNameToHandle_.contains(newName)) {
		Logger::Output("同名のModelが既に存在します: " + newName, Logger::Level::Warning);
		return false;
	}

	const std::string oldName = slot.name;

	// Object本体と名前索引を同時に更新してHandle検索の一貫性を維持
	modelNameToHandle_.erase(oldName);
	slot.name = newName;
	modelNameToHandle_.emplace(newName, handle);
	model->SetObjectName(newName);
	Logger::Output("Model名を変更しました: " + oldName + " -> " + newName, Logger::Level::Application);
	return true;
}

bool ModelManager::Rename(const std::string& currentName, const std::string& newName) {
	return Rename(Find(currentName), newName);
}

bool ModelManager::Destroy(ModelHandle handle) {
	if (!IsValid(handle)) {
		return false;
	}

	ModelSlot& slot = modelSlots_[handle.index];
	const std::string name = slot.name;

	// 遅延削除Queueと名前索引を先に掃除して破棄中の再参照を防止
	pendingDestroyModelHandles_.erase(
		std::remove(pendingDestroyModelHandles_.begin(), pendingDestroyModelHandles_.end(), handle),
		pendingDestroyModelHandles_.end());
	const auto nameIt = modelNameToHandle_.find(name);
	if (nameIt != modelNameToHandle_.end() && nameIt->second == handle) {
		modelNameToHandle_.erase(nameIt);
	}
	slot.model.reset();
	slot.name.clear();
	slot.assetName.clear();
	slot.managementMode = EditorManagementMode::RuntimeOnly;
	slot.active = false;

	// 旧Handleを無効化してからSlotを再利用候補へ返却
	slot.generation = NextObjectGeneration(slot.generation);
	freeModelSlots_.push_back(handle.index);
	Logger::Output("Modelを削除しました: " + name, Logger::Level::Application);
	return true;
}

bool ModelManager::Destroy(const std::string& name) {
	return Destroy(Find(name));
}

void ModelManager::RequestDestroy(ModelHandle handle) {
	if (!IsValid(handle)) {
		return;
	}
	if (std::find(pendingDestroyModelHandles_.begin(), pendingDestroyModelHandles_.end(), handle) == pendingDestroyModelHandles_.end()) {
		pendingDestroyModelHandles_.push_back(handle);
	}
}

void ModelManager::RequestDestroy(const std::string& name) {
	RequestDestroy(Find(name));
}

InstancedModelHandle ModelManager::CreateInstanced(const std::string& name, const std::string& modelName, SceneType sceneType) {
	return CreateInstanced(InstancedModelCreateDesc{ name, modelName, sceneType, EditorManagementMode::RuntimeOnly });
}

InstancedModelHandle ModelManager::CreateInstanced(const InstancedModelCreateDesc& desc) {
	if (desc.name.empty()) {
		Logger::Output("名前が空のInstancedModelは生成できません", Logger::Level::Warning);
		return {};
	}
	if (instancedModelNameToHandle_.contains(desc.name)) {
		Logger::Output("同名のInstancedModelが既に存在するため生成に失敗しました: " + desc.name, Logger::Level::Warning);
		return {};
	}

	const std::string resolvedModelPath = ResolveModelPath(desc.modelName);
	const ModelSharedData* sharedData = FindSharedData(resolvedModelPath);
	if (!sharedData) {
		Logger::Output("Modelアセットが見つからないためInstancedModelの生成に失敗しました: " + desc.modelName, Logger::Level::Warning);
		return {};
	}
	if (sharedData->type != ModelType::Static) {
		Logger::Output("InstancedModelはStatic Modelだけ生成できます: " + desc.modelName, Logger::Level::Warning);
		return {};
	}

	auto model = std::make_unique<InstancedModel>(desc.name);
	model->Initialize(device_, commandList_, *sharedData);
	model->SetPSORegistry(psoRegistry_);
	model->SetSceneType(desc.sceneType);

	// 通常Modelとは独立したSlot PoolでInstance描画用Handleを管理
	uint32_t index = 0;
	if (freeInstancedModelSlots_.empty()) {
		index = static_cast<uint32_t>(instancedModelSlots_.size());
		instancedModelSlots_.emplace_back();
	} else {
		index = freeInstancedModelSlots_.back();
		freeInstancedModelSlots_.pop_back();
	}

	InstancedModelSlot& slot = instancedModelSlots_[index];
	slot.model = std::move(model);
	slot.name = desc.name;
	slot.assetName = resolvedModelPath;
	slot.managementMode = desc.managementMode;
	slot.active = true;
	const InstancedModelHandle handle{ index, slot.generation };
	instancedModelNameToHandle_.emplace(slot.name, handle);
	Logger::Output("InstancedModelを生成しました: " + desc.name + " アセット: " + resolvedModelPath, Logger::Level::Application);
	return handle;
}

InstancedModelHandle ModelManager::FindOrCreateInstanced(const InstancedModelCreateDesc& desc) {
	const InstancedModelHandle existing = FindInstanced(desc.name);
	if (!existing.IsValid()) {
		return CreateInstanced(desc);
	}

	const InstancedModelSlot& slot = instancedModelSlots_[existing.index];
	if (slot.assetName != ResolveModelPath(desc.modelName) ||
		slot.model->GetSceneType() != desc.sceneType ||
		slot.managementMode != desc.managementMode) {
		Logger::Output("既存InstancedModelの生成条件が一致しません: " + desc.name, Logger::Level::Warning);
		return {};
	}
	return existing;
}

InstancedModelHandle ModelManager::GetOrCreateInstanced(const std::string& name, const std::string& modelName, SceneType sceneType) {
	return FindOrCreateInstanced(InstancedModelCreateDesc{ name, modelName, sceneType, EditorManagementMode::RuntimeOnly });
}

InstancedModel* ModelManager::TryGet(InstancedModelHandle handle) {
	return const_cast<InstancedModel*>(std::as_const(*this).TryGet(handle));
}

const InstancedModel* ModelManager::TryGet(InstancedModelHandle handle) const {
	if (!handle.IsValid() || handle.index >= instancedModelSlots_.size()) {
		return nullptr;
	}
	const InstancedModelSlot& slot = instancedModelSlots_[handle.index];
	if (!slot.active || slot.generation != handle.generation) {
		return nullptr;
	}
	return slot.model.get();
}

InstancedModelHandle ModelManager::FindInstanced(const std::string& name) const {
	const auto it = instancedModelNameToHandle_.find(name);
	return it == instancedModelNameToHandle_.end() ? InstancedModelHandle{} : it->second;
}

bool ModelManager::IsValid(InstancedModelHandle handle) const {
	return TryGet(handle) != nullptr;
}

InstancedModel* ModelManager::GetInstanced(const std::string& name) {
	return TryGet(FindInstanced(name));
}

const InstancedModel* ModelManager::GetInstanced(const std::string& name) const {
	return TryGet(FindInstanced(name));
}

bool ModelManager::RenameInstanced(InstancedModelHandle handle, const std::string& newName) {
	if (!IsValid(handle)) {
		Logger::Output("名前を変更するInstancedModelのHandleが無効です", Logger::Level::Warning);
		return false;
	}
	if (newName.empty()) {
		Logger::Output("InstancedModel名を空文字へ変更できません", Logger::Level::Warning);
		return false;
	}

	InstancedModelSlot& slot = instancedModelSlots_[handle.index];
	if (slot.name == newName) {
		return true;
	}
	if (instancedModelNameToHandle_.contains(newName)) {
		Logger::Output("同名のInstancedModelが既に存在します: " + newName, Logger::Level::Warning);
		return false;
	}

	const std::string oldName = slot.name;
	instancedModelNameToHandle_.erase(oldName);
	slot.name = newName;
	instancedModelNameToHandle_.emplace(newName, handle);
	Logger::Output("InstancedModel名を変更しました: " + oldName + " -> " + newName, Logger::Level::Application);
	return true;
}

bool ModelManager::Destroy(InstancedModelHandle handle) {
	if (!IsValid(handle)) {
		return false;
	}

	InstancedModelSlot& slot = instancedModelSlots_[handle.index];
	const std::string name = slot.name;
	pendingDestroyInstancedModelHandles_.erase(
		std::remove(pendingDestroyInstancedModelHandles_.begin(), pendingDestroyInstancedModelHandles_.end(), handle),
		pendingDestroyInstancedModelHandles_.end());
	const auto nameIt = instancedModelNameToHandle_.find(name);
	if (nameIt != instancedModelNameToHandle_.end() && nameIt->second == handle) {
		instancedModelNameToHandle_.erase(nameIt);
	}
	slot.model.reset();
	slot.name.clear();
	slot.assetName.clear();
	slot.managementMode = EditorManagementMode::RuntimeOnly;
	slot.active = false;
	slot.generation = NextObjectGeneration(slot.generation);
	freeInstancedModelSlots_.push_back(handle.index);
	Logger::Output("InstancedModelを削除しました: " + name, Logger::Level::Application);
	return true;
}

bool ModelManager::DestroyInstanced(const std::string& name) {
	return Destroy(FindInstanced(name));
}

void ModelManager::RequestDestroy(InstancedModelHandle handle) {
	if (!IsValid(handle)) {
		return;
	}
	if (std::find(pendingDestroyInstancedModelHandles_.begin(), pendingDestroyInstancedModelHandles_.end(), handle) == pendingDestroyInstancedModelHandles_.end()) {
		pendingDestroyInstancedModelHandles_.push_back(handle);
	}
}

void ModelManager::FlushPendingDestroys() {
	std::vector<ModelHandle> modelHandles = std::move(pendingDestroyModelHandles_);
	pendingDestroyModelHandles_.clear();
	for (ModelHandle handle : modelHandles) {
		Destroy(handle);
	}

	std::vector<InstancedModelHandle> instancedHandles = std::move(pendingDestroyInstancedModelHandles_);
	pendingDestroyInstancedModelHandles_.clear();
	for (InstancedModelHandle handle : instancedHandles) {
		Destroy(handle);
	}
}

void ModelManager::DestroyByScene(SceneType sceneType) {
	if (sceneType == SceneType::None) {
		Logger::Output("SceneType::NoneのModelはScene遷移で削除しません", Logger::Level::Warning);
		return;
	}

	std::vector<ModelHandle> modelHandles;
	for (const auto& [name, handle] : modelNameToHandle_) {
		(void)name;
		const Model* model = TryGet(handle);
		if (model && model->GetSceneType() == sceneType) {
			modelHandles.push_back(handle);
		}
	}
	std::vector<InstancedModelHandle> instancedHandles;
	for (const auto& [name, handle] : instancedModelNameToHandle_) {
		(void)name;
		const InstancedModel* model = TryGet(handle);
		if (model && model->GetSceneType() == sceneType) {
			instancedHandles.push_back(handle);
		}
	}

	for (ModelHandle handle : modelHandles) {
		Destroy(handle);
	}
	for (InstancedModelHandle handle : instancedHandles) {
		Destroy(handle);
	}
	Logger::Output("Scene内のModelを削除しました: " + SceneTypeToString(sceneType) + " 通常: " + std::to_string(modelHandles.size()) + " Instanced: " + std::to_string(instancedHandles.size()), Logger::Level::Application);
}

nlohmann::json ModelManager::ToJson() const {
	nlohmann::json models = nlohmann::json::array();
	for (const auto& [name, handle] : modelNameToHandle_) {
		const Model* model = TryGet(handle);
		if (!model || modelSlots_[handle.index].managementMode != EditorManagementMode::EditorManaged) {
			continue;
		}
		nlohmann::json modelJson = model->ToJson();
		modelJson["model"] = modelSlots_[handle.index].assetName;
		models.push_back(std::move(modelJson));
	}
	return { { "models", models } };
}

void ModelManager::FromJson(const nlohmann::json& json) {
	FromJsonInternal(json, std::nullopt);
}

void ModelManager::FromJson(const nlohmann::json& json, SceneType sceneType) {
	FromJsonInternal(json, sceneType);
}

void ModelManager::FromJsonInternal(
	const nlohmann::json& json,
	std::optional<SceneType> sceneType) {
	const nlohmann::json* modelArray = nullptr;
	if (json.is_array()) {
		modelArray = &json;
	} else if (json.contains("models") && json.at("models").is_array()) {
		modelArray = &json.at("models");
	}
	if (!modelArray) {
		Logger::Output("ModelのJSONにmodels配列がありません", Logger::Level::Warning);
		return;
	}

	for (const nlohmann::json& modelJson : *modelArray) {
		try {
			const SceneType modelSceneType = SceneTypeFromString(
				modelJson.value("scene", SceneTypeToString(SceneType::None)));
			if (sceneType &&
				modelSceneType != SceneType::None &&
				modelSceneType != *sceneType) {
				continue;
			}
			const ModelHandle handle = CreateFromJson(modelJson);
			(void)handle;
		}
		catch (const nlohmann::json::exception& exception) {
			Logger::Output("ModelのJSON要素を読み込めませんでした: " + std::string(exception.what()), Logger::Level::Error);
		}
	}
}

bool ModelManager::SaveToFile(const std::filesystem::path& filePath) const {
	return Json::JsonFile::Save(filePath, ToJson(), 4, true);
}

bool ModelManager::SaveToFile(
	const std::filesystem::path& filePath,
	SceneType sceneType) const {
	nlohmann::json outputJson = nlohmann::json::object();
	nlohmann::json mergedModels = nlohmann::json::array();

	if (Json::JsonFile::Exists(filePath)) {
		nlohmann::json existingJson;
		if (!Json::JsonFile::Load(filePath, existingJson)) {
			return false;
		}

		const nlohmann::json* existingModels = nullptr;
		if (existingJson.is_array()) {
			existingModels = &existingJson;
		} else if (existingJson.contains("models") && existingJson.at("models").is_array()) {
			existingModels = &existingJson.at("models");
		}
		if (!existingModels) {
			Logger::Output(
				"ModelのJSONにmodels配列がないため、シーン単位の保存を中止しました",
				Logger::Level::Warning);
			return false;
		}

		if (existingJson.is_object()) {
			outputJson = existingJson;
		}

		for (const nlohmann::json& modelJson : *existingModels) {
			try {
				const SceneType modelSceneType = SceneTypeFromString(
					modelJson.value("scene", SceneTypeToString(SceneType::None)));
				if (modelSceneType != SceneType::None && modelSceneType != sceneType) {
					mergedModels.push_back(modelJson);
				}
			}
			catch (const nlohmann::json::exception& exception) {
				Logger::Output(
					"Modelのシーン設定を判定できないため保存を中止しました: " +
					std::string(exception.what()),
					Logger::Level::Error);
				return false;
			}
		}
	}

	const nlohmann::json activeEditorJson = ToJson();
	for (const nlohmann::json& modelJson : activeEditorJson.at("models")) {
		mergedModels.push_back(modelJson);
	}
	outputJson["models"] = std::move(mergedModels);
	return Json::JsonFile::Save(filePath, outputJson, 4, true);
}

bool ModelManager::LoadFromFile(const std::filesystem::path& filePath) {
	nlohmann::json json;
	if (!Json::JsonFile::Load(filePath, json)) {
		return false;
	}
	FromJson(json);
	return true;
}

bool ModelManager::LoadFromFile(
	const std::filesystem::path& filePath,
	SceneType sceneType) {
	nlohmann::json json;
	if (!Json::JsonFile::Load(filePath, json)) {
		return false;
	}
	FromJson(json, sceneType);
	return true;
}

std::vector<std::string> ModelManager::GetNames() const {
	std::vector<std::string> names;
	names.reserve(modelNameToHandle_.size());
	for (const auto& [name, handle] : modelNameToHandle_) {
		(void)handle;
		names.push_back(name);
	}
	std::sort(names.begin(), names.end());
	return names;
}

std::size_t ModelManager::GetModelCount() const {
	return modelNameToHandle_.size();
}

std::vector<std::string> ModelManager::GetEditorManagedNames() const {
	std::vector<std::string> names;
	names.reserve(modelNameToHandle_.size());
	for (const auto& [name, handle] : modelNameToHandle_) {
		if (IsValid(handle) && modelSlots_[handle.index].managementMode == EditorManagementMode::EditorManaged) {
			names.push_back(name);
		}
	}
	std::sort(names.begin(), names.end());
	return names;
}

std::vector<std::string> ModelManager::GetAvailableModelNames() const {
	std::vector<std::string> names;
	names.reserve(sharedData_.size());
	for (const auto& [name, sharedData] : sharedData_) {
		(void)sharedData;
		names.push_back(name);
	}
	std::sort(names.begin(), names.end());
	return names;
}

std::string ModelManager::GetModelAssetName(ModelHandle handle) const {
	return IsValid(handle) ? modelSlots_[handle.index].assetName : std::string{};
}

std::string ModelManager::GetModelAssetName(const std::string& name) const {
	return GetModelAssetName(Find(name));
}

const ModelSharedData* ModelManager::GetSharedData(const std::string& modelName) const {
	return FindSharedData(modelName);
}

ModelHandle ModelManager::PickByRay(
	SceneType currentSceneType,
	const Vector3& rayOrigin,
	const Vector3& rayDirection,
	float maxDistance,
	float* outDistance) const {
	ModelHandle pickedHandle{};
	float nearestDistance = maxDistance;
	for (const auto& [name, handle] : modelNameToHandle_) {
		(void)name;
		const Model* model = TryGet(handle);
		if (!model || !model->IsVisible()) {
			continue;
		}
		const SceneType modelScene = model->GetSceneType();
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}

		float hitDistance = 0.0f;
		if (model->Raycast(rayOrigin, rayDirection, maxDistance, hitDistance) && hitDistance < nearestDistance) {
			nearestDistance = hitDistance;
			pickedHandle = handle;
		}
	}
	if (outDistance) {
		*outDistance = nearestDistance;
	}
	return pickedHandle;
}

const ModelSharedData* ModelManager::FindSharedData(const std::string& modelName) const {
	const auto aliasIt = aliases_.find(modelName);
	if (aliasIt != aliases_.end()) {
		const auto dataIt = sharedData_.find(aliasIt->second);
		if (dataIt != sharedData_.end()) {
			return dataIt->second.get();
		}
	}

	const std::string normalized = NormalizePath(modelName);
	const auto dataIt = sharedData_.find(normalized);
	return dataIt == sharedData_.end() ? nullptr : dataIt->second.get();
}

std::string ModelManager::ResolveModelPath(const std::string& modelName) const {
	const auto aliasIt = aliases_.find(modelName);
	return aliasIt == aliases_.end() ? NormalizePath(modelName) : aliasIt->second;
}

void ModelManager::UpdateAll(SceneType currentSceneType, float deltaTime) {
	for (const auto& [name, handle] : modelNameToHandle_) {
		(void)name;
		Model* model = TryGet(handle);
		if (!model || !model->IsVisible()) {
			continue;
		}
		const SceneType modelScene = model->GetSceneType();
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}
		model->Update(deltaTime);
	}
	for (const auto& [name, handle] : instancedModelNameToHandle_) {
		(void)name;
		InstancedModel* model = TryGet(handle);
		if (!model || !model->IsVisible()) {
			continue;
		}
		const SceneType modelScene = model->GetSceneType();
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}
		model->Update(deltaTime);
	}
}

void ModelManager::DrawAll(SceneType currentSceneType) {
	DrawLayerMask(currentSceneType, activeCamera_, Render::kAllRenderLayers);
}

void ModelManager::DrawAll(SceneType currentSceneType, Camera& camera) {
	DrawLayerMask(currentSceneType, camera, Render::kAllRenderLayers);
}

void ModelManager::DrawShadowMap(SceneType currentSceneType, const Matrix4x4& lightViewProjection) {
	DrawShadowMapLayerMask(currentSceneType, lightViewProjection, Render::kAllRenderLayers);
}

void ModelManager::DrawShadowMapLayer(SceneType currentSceneType, const Matrix4x4& lightViewProjection, Render::RenderLayer layer) {
	DrawShadowMapLayerMask(currentSceneType, lightViewProjection, Render::ToRenderLayerMask(layer));
}

void ModelManager::SetShadowMap(
	SceneType currentSceneType,
	D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSrv,
	const Matrix4x4& lightViewProjection,
	uint32_t width,
	uint32_t height) {
	SetShadowMapLayerMask(currentSceneType, shadowMapSrv, lightViewProjection, width, height, Render::kAllRenderLayers);
}

void ModelManager::DrawLayer(SceneType currentSceneType, Render::RenderLayer layer) {
	DrawLayerMask(currentSceneType, activeCamera_, Render::ToRenderLayerMask(layer));
}

void ModelManager::DrawLayer(SceneType currentSceneType, Camera& camera, Render::RenderLayer layer) {
	DrawLayerMask(currentSceneType, camera, Render::ToRenderLayerMask(layer));
}

void ModelManager::DrawLayerMask(SceneType currentSceneType, Render::RenderLayerMask layerMask) {
	DrawLayerMask(currentSceneType, activeCamera_, layerMask);
}

void ModelManager::DrawLayerMask(SceneType currentSceneType, Camera& camera, Render::RenderLayerMask layerMask) {
	DrawOpaqueLayerMask(currentSceneType, camera, layerMask);
	DrawTransparentLayerMask(currentSceneType, camera, layerMask);
}

void ModelManager::DrawOpaqueLayerMask(SceneType currentSceneType, Camera& camera, Render::RenderLayerMask layerMask) {
	activeCamera_ = camera;
	for (const auto& [name, handle] : modelNameToHandle_) {
		(void)name;
		Model* model = TryGet(handle);
		if (!model || !IsDrawTarget(*model, currentSceneType, layerMask) || model->RequiresTransparentPass()) {
			continue;
		}
		model->Draw(camera);
	}
	for (const auto& [name, handle] : instancedModelNameToHandle_) {
		(void)name;
		InstancedModel* model = TryGet(handle);
		if (!model || !IsDrawTarget(*model, currentSceneType, layerMask) || model->RequiresTransparentPass()) {
			continue;
		}
		model->Draw(camera);
	}
}

void ModelManager::DrawTransparentLayerMask(SceneType currentSceneType, Camera& camera, Render::RenderLayerMask layerMask) {
	activeCamera_ = camera;
	std::vector<IRenderObject3d*> drawTargets;
	drawTargets.reserve(modelNameToHandle_.size() + instancedModelNameToHandle_.size());

	for (const auto& [name, handle] : modelNameToHandle_) {
		(void)name;
		Model* model = TryGet(handle);
		if (model && IsDrawTarget(*model, currentSceneType, layerMask) && model->RequiresTransparentPass()) {
			drawTargets.push_back(model);
		}
	}

	for (const auto& [name, handle] : instancedModelNameToHandle_) {
		(void)name;
		InstancedModel* model = TryGet(handle);
		if (model && IsDrawTarget(*model, currentSceneType, layerMask) && model->RequiresTransparentPass()) {
			drawTargets.push_back(model);
		}
	}

	const Vector3 cameraPosition = camera.GetPosition();

	// Alpha Blend結果を安定させるためカメラ距離の降順で透明Modelを描画
	std::sort(drawTargets.begin(), drawTargets.end(), [&cameraPosition](const IRenderObject3d* lhs, const IRenderObject3d* rhs) {
		return lhs->GetTransparentSortDistanceSq(cameraPosition) > rhs->GetTransparentSortDistanceSq(cameraPosition);
	});

	for (IRenderObject3d* model : drawTargets) {
		model->Draw(camera);
	}
}

void ModelManager::DrawShadowMapLayerMask(SceneType currentSceneType, const Matrix4x4& lightViewProjection, Render::RenderLayerMask layerMask) {
	for (const auto& [name, handle] : modelNameToHandle_) {
		(void)name;
		Model* model = TryGet(handle);
		if (!model || !model->IsVisible()) {
			continue;
		}
		const SceneType modelScene = model->GetSceneType();
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}
		if (!model->IsRenderLayerIncluded(layerMask) || !model->CanCastShadow()) {
			continue;
		}
		model->DrawShadow(lightViewProjection, activeCamera_);
	}
	for (const auto& [name, handle] : instancedModelNameToHandle_) {
		(void)name;
		InstancedModel* model = TryGet(handle);
		if (!model || !model->IsVisible()) {
			continue;
		}
		const SceneType modelScene = model->GetSceneType();
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}
		if (!model->IsRenderLayerIncluded(layerMask) || !model->CanCastShadow()) {
			continue;
		}
		model->DrawShadow(lightViewProjection, activeCamera_);
	}
}

void ModelManager::SetShadowMapLayerMask(
	SceneType currentSceneType,
	D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSrv,
	const Matrix4x4& lightViewProjection,
	uint32_t width,
	uint32_t height,
	Render::RenderLayerMask layerMask) {
	for (const auto& [name, handle] : modelNameToHandle_) {
		(void)name;
		Model* model = TryGet(handle);
		if (!model) {
			continue;
		}
		const SceneType modelScene = model->GetSceneType();
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}
		if (!model->IsRenderLayerIncluded(layerMask)) {
			continue;
		}
		model->SetShadowMap(shadowMapSrv, lightViewProjection, width, height);
	}
	for (const auto& [name, handle] : instancedModelNameToHandle_) {
		(void)name;
		InstancedModel* model = TryGet(handle);
		if (!model) {
			continue;
		}
		const SceneType modelScene = model->GetSceneType();
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}
		if (!model->IsRenderLayerIncluded(layerMask)) {
			continue;
		}
		model->SetShadowMap(shadowMapSrv, lightViewProjection, width, height);
	}
}

} // namespace MadoEngine
