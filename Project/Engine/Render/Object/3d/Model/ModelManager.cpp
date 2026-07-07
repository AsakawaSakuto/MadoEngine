#include "ModelManager.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <filesystem>

namespace {

std::string ToLower(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return value;
}

std::string NormalizePath(const std::filesystem::path& path) {
	return path.lexically_normal().generic_string();
}

bool IsModelExtension(const std::filesystem::path& path) {
	const std::string extension = ToLower(path.extension().string());
	return extension == ".obj" || extension == ".gltf" || extension == ".glb";
}

bool TryGetModelType(const std::filesystem::path& modelRoot, const std::filesystem::path& path, ModelType& outType) {
	std::error_code ec;
	std::filesystem::path relative = std::filesystem::relative(path.parent_path(), modelRoot, ec);
	if (ec || relative.empty()) {
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

} // namespace

namespace MadoEngine {

ModelManager& ModelManager::GetInstance() {
	static ModelManager instance;
	return instance;
}

void ModelManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, MadoEngine::Render::PSORegistry* psoRegistry) {
	assert(device);
	assert(commandList);
	assert(psoRegistry);

	device_ = device;
	commandList_ = commandList;
	psoRegistry_ = psoRegistry;

	LoadAllModels();

	Logger::Output(std::to_string(sharedData_.size()) + " 件のモデルアセットを読み込みました", Logger::Level::Engine);
}

void ModelManager::Finalize() {
	models_.clear();
	instancedModels_.clear();
	for (auto& [key, data] : sharedData_) {
		MadoEngine::ModelResource::Finalize(*data);
	}
	sharedData_.clear();
	aliases_.clear();
	activeCamera_ = {};

	Logger::Output("終了処理完了", Logger::Level::Engine);
}

void ModelManager::LoadAllModels() {
	const std::filesystem::path modelRoot = "Assets/Model";
	if (!std::filesystem::exists(modelRoot)) {
		Logger::Output("Assets/Model フォルダが見つかりません", Logger::Level::Warning);
		return;
	}

	for (const auto& entry : std::filesystem::recursive_directory_iterator(modelRoot)) {
		if (!entry.is_regular_file() || !IsModelExtension(entry.path())) {
			continue;
		}

		ModelType type = ModelType::Static;
		if (!TryGetModelType(modelRoot, entry.path(), type)) {
			continue;
		}

		LoadModelFile(NormalizePath(entry.path()), type);
	}
}

void ModelManager::LoadModelFile(const std::string& path, ModelType type) {
	if (sharedData_.contains(path)) {
		return;
	}

	auto data = std::make_unique<ModelSharedData>();
	MadoEngine::ModelResource::Initialize(*data, device_, path, type);

	const std::string stem = std::filesystem::path(path).stem().string();
	if (!aliases_.contains(stem)) {
		aliases_.emplace(stem, path);
	} else {
		Logger::Output("モデルエイリアスが重複しています。パスを使用してください : " + stem, Logger::Level::Warning);
	}

	aliases_.emplace(path, path);
	sharedData_.emplace(path, std::move(data));

	Logger::Output("読み込み完了 " + path + " [" + MadoEngine::ModelResource::ModelTypeToString(sharedData_.at(path)->type) + "]", Logger::Level::Assets);
}

Model* ModelManager::Create(const std::string& name, const std::string& modelName, SceneType sceneType) {
	if (models_.contains(name)) {
		Logger::Output("モデルインスタンスが既に存在します : " + name, Logger::Level::Warning);
		return models_.at(name).get();
	}

	const ModelSharedData* sharedData = FindSharedData(modelName);
	if (!sharedData) {
		Logger::Output("モデルアセットが見つかりません : " + modelName, Logger::Level::Warning);
		return nullptr;
	}

	auto model = std::make_unique<Model>(name);
	model->Initialize(device_, commandList_, *sharedData);
	model->SetPSORegistry(psoRegistry_);
	model->SetSceneType(sceneType);

	Model* ptr = model.get();
	models_.emplace(name, std::move(model));

	Logger::Output("モデルインスタンスを作成しました : " + name + " アセット : " + modelName + " シーン : " + (sceneType == SceneType::None ? "全て" : SceneTypeToString(sceneType)), Logger::Level::Application);
	return ptr;
}

Model* ModelManager::Get(const std::string& name) const {
	auto it = models_.find(name);
	if (it == models_.end()) {
		Logger::Output("モデルインスタンスが見つかりません : " + name, Logger::Level::Warning);
		return nullptr;
	}
	return it->second.get();
}

void ModelManager::Destroy(const std::string& name) {
	if (models_.erase(name) > 0) {
		Logger::Output("モデルインスタンスを削除しました : " + name, Logger::Level::Application);
	}
}

InstancedModel* ModelManager::CreateInstanced(const std::string& name, const std::string& modelName, SceneType sceneType) {
	if (instancedModels_.contains(name)) {
		Logger::Output("[Engine] インスタンス描画モデルは既に存在します : " + name, Logger::Level::Warning);
		return instancedModels_.at(name).get();
	}

	const ModelSharedData* sharedData = FindSharedData(modelName);
	if (!sharedData) {
		Logger::Output("[Engine] インスタンス描画モデルのアセットが見つかりません : " + modelName, Logger::Level::Warning);
		return nullptr;
	}

	if (sharedData->type != ModelType::Static) {
		Logger::Output("[Engine] インスタンス描画はStaticモデルのみ対応しています : " + modelName, Logger::Level::Warning);
		return nullptr;
	}

	auto model = std::make_unique<InstancedModel>(name);
	model->Initialize(device_, commandList_, *sharedData);
	model->SetPSORegistry(psoRegistry_);
	model->SetSceneType(sceneType);

	InstancedModel* ptr = model.get();
	instancedModels_.emplace(name, std::move(model));

	Logger::Output("[Engine] インスタンス描画モデルを作成しました : " + name + " アセット : " + modelName, Logger::Level::Application);
	return ptr;
}

InstancedModel* ModelManager::GetOrCreateInstanced(const std::string& name, const std::string& modelName, SceneType sceneType) {
	auto it = instancedModels_.find(name);
	if (it != instancedModels_.end()) {
		return it->second.get();
	}

	return CreateInstanced(name, modelName, sceneType);
}

InstancedModel* ModelManager::GetInstanced(const std::string& name) const {
	auto it = instancedModels_.find(name);
	if (it == instancedModels_.end()) {
		Logger::Output("[Engine] インスタンス描画モデルが見つかりません : " + name, Logger::Level::Warning);
		return nullptr;
	}
	return it->second.get();
}

void ModelManager::DestroyInstanced(const std::string& name) {
	if (instancedModels_.erase(name) > 0) {
		Logger::Output("[Engine] インスタンス描画モデルを破棄しました : " + name, Logger::Level::Application);
	}
}

void ModelManager::DestroyByScene(SceneType sceneType) {
	if (sceneType == SceneType::None) {
		Logger::Output("SceneType::Noneは全シーン共通のため、Modelのシーン単位破棄をスキップしました", Logger::Level::Warning);
		return;
	}

	size_t destroyCount = 0;
	size_t destroyInstancedCount = 0;
	for (auto it = instancedModels_.begin(); it != instancedModels_.end();) {
		if (it->second->GetSceneType() == sceneType) {
			it = instancedModels_.erase(it);
			++destroyInstancedCount;
		} else {
			++it;
		}
	}
	Logger::Output("[Engine] シーン内のインスタンス描画モデルを破棄しました : " + SceneTypeToString(sceneType) + " 件数 : " + std::to_string(destroyInstancedCount), Logger::Level::Application);

	for (auto it = models_.begin(); it != models_.end();) {
		if (it->second->GetSceneType() == sceneType) {
			it = models_.erase(it);
			++destroyCount;
		} else {
			++it;
		}
	}

	Logger::Output("シーン内のModelインスタンスを破棄しました : " + SceneTypeToString(sceneType) + " 件数 : " + std::to_string(destroyCount), Logger::Level::Application);
}

const ModelSharedData* ModelManager::GetSharedData(const std::string& modelName) const {
	return FindSharedData(modelName);
}

Model* ModelManager::PickByRay(
	SceneType currentSceneType,
	const Vector3& rayOrigin,
	const Vector3& rayDirection,
	float maxDistance,
	float* outDistance) const {
	Model* pickedModel = nullptr;
	float nearestDistance = maxDistance;

	for (const auto& [name, model] : models_) {
		SceneType modelScene = model->GetSceneType();
		if (!model->IsVisible()) {
			continue;
		}
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}

		float hitDistance = 0.0f;
		if (model->Raycast(rayOrigin, rayDirection, maxDistance, hitDistance) && hitDistance < nearestDistance) {
			nearestDistance = hitDistance;
			pickedModel = model.get();
		}
	}

	if (outDistance) {
		*outDistance = nearestDistance;
	}
	return pickedModel;
}

const ModelSharedData* ModelManager::FindSharedData(const std::string& modelName) const {
	auto aliasIt = aliases_.find(modelName);
	if (aliasIt != aliases_.end()) {
		auto dataIt = sharedData_.find(aliasIt->second);
		if (dataIt != sharedData_.end()) {
			return dataIt->second.get();
		}
	}

	const std::string normalized = NormalizePath(modelName);
	auto dataIt = sharedData_.find(normalized);
	if (dataIt != sharedData_.end()) {
		return dataIt->second.get();
	}

	return nullptr;
}

void ModelManager::UpdateAll(SceneType currentSceneType) {
	if (models_.empty() && instancedModels_.empty()) {
		return;
	}

	for (auto& [name, model] : models_) {
		SceneType modelScene = model->GetSceneType();
		if (!model->IsVisible()) {
			continue;
		}
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}

		model->Update();
	}

	for (auto& [name, model] : instancedModels_) {
		SceneType modelScene = model->GetSceneType();
		if (!model->IsVisible()) {
			continue;
		}
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}

		model->Update();
	}
}

void ModelManager::DrawAll(SceneType currentSceneType) {
	DrawLayerMask(currentSceneType, activeCamera_, MadoEngine::Render::kAllRenderLayers);
}

void ModelManager::DrawAll(SceneType currentSceneType, Camera& camera) {
	DrawLayerMask(currentSceneType, camera, MadoEngine::Render::kAllRenderLayers);
}

void ModelManager::DrawShadowMap(SceneType currentSceneType, const Matrix4x4& lightViewProjection) {
	DrawShadowMapLayerMask(currentSceneType, lightViewProjection, MadoEngine::Render::kAllRenderLayers);
}

void ModelManager::DrawShadowMapLayer(SceneType currentSceneType, const Matrix4x4& lightViewProjection, MadoEngine::Render::RenderLayer layer) {
	DrawShadowMapLayerMask(currentSceneType, lightViewProjection, MadoEngine::Render::ToRenderLayerMask(layer));
}

void ModelManager::SetShadowMap(
	SceneType currentSceneType,
	D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSrv,
	const Matrix4x4& lightViewProjection,
	uint32_t width,
	uint32_t height) {
	SetShadowMapLayerMask(
		currentSceneType,
		shadowMapSrv,
		lightViewProjection,
		width,
		height,
		MadoEngine::Render::kAllRenderLayers
	);
}

void ModelManager::DrawLayer(SceneType currentSceneType, MadoEngine::Render::RenderLayer layer) {
	DrawLayerMask(currentSceneType, activeCamera_, MadoEngine::Render::ToRenderLayerMask(layer));
}

void ModelManager::DrawLayer(SceneType currentSceneType, Camera& camera, MadoEngine::Render::RenderLayer layer) {
	DrawLayerMask(currentSceneType, camera, MadoEngine::Render::ToRenderLayerMask(layer));
}

void ModelManager::DrawLayerMask(SceneType currentSceneType, MadoEngine::Render::RenderLayerMask layerMask) {
	DrawLayerMask(currentSceneType, activeCamera_, layerMask);
}

void ModelManager::DrawLayerMask(SceneType currentSceneType, Camera& camera, MadoEngine::Render::RenderLayerMask layerMask) {
	if (models_.empty() && instancedModels_.empty()) {
		return;
	}

	for (auto& [name, model] : models_) {
		SceneType modelScene = model->GetSceneType();
		if (!model->IsVisible()) {
			continue;
		}
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}
		if (!model->IsRenderLayerIncluded(layerMask)) {
			continue;
		}

		model->Draw(camera);
	}

	for (auto& [name, model] : instancedModels_) {
		SceneType modelScene = model->GetSceneType();
		if (!model->IsVisible()) {
			continue;
		}
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}
		if (!model->IsRenderLayerIncluded(layerMask)) {
			continue;
		}

		model->Draw(camera);
	}
}

void ModelManager::DrawShadowMapLayerMask(SceneType currentSceneType, const Matrix4x4& lightViewProjection, MadoEngine::Render::RenderLayerMask layerMask) {
	if (models_.empty() && instancedModels_.empty()) {
		return;
	}

	for (auto& [name, model] : models_) {
		SceneType modelScene = model->GetSceneType();
		if (!model->IsVisible()) {
			continue;
		}
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}
		if (!model->IsRenderLayerIncluded(layerMask)) {
			continue;
		}
		if (!model->CanCastShadow()) {
			continue;
		}

		model->DrawShadow(lightViewProjection);
	}

	for (auto& [name, model] : instancedModels_) {
		SceneType modelScene = model->GetSceneType();
		if (!model->IsVisible()) {
			continue;
		}
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}
		if (!model->IsRenderLayerIncluded(layerMask)) {
			continue;
		}
		if (!model->CanCastShadow()) {
			continue;
		}

		model->DrawShadow(lightViewProjection);
	}
}

void ModelManager::SetShadowMapLayerMask(
	SceneType currentSceneType,
	D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSrv,
	const Matrix4x4& lightViewProjection,
	uint32_t width,
	uint32_t height,
	MadoEngine::Render::RenderLayerMask layerMask) {
	if (models_.empty() && instancedModels_.empty()) {
		return;
	}

	for (auto& [name, model] : models_) {
		SceneType modelScene = model->GetSceneType();
		if (modelScene != SceneType::None && modelScene != currentSceneType) {
			continue;
		}
		if (!model->IsRenderLayerIncluded(layerMask)) {
			continue;
		}

		model->SetShadowMap(shadowMapSrv, lightViewProjection, width, height);
	}

	for (auto& [name, model] : instancedModels_) {
		SceneType modelScene = model->GetSceneType();
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
