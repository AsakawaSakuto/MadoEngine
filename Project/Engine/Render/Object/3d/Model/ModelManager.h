#pragma once
#include "Model.h"
#include ".SceneManager/SceneType.h"
#include <d3d12.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace MadoEngine {

class ModelManager {
public:
	static ModelManager& GetInstance();

	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;
	ModelManager(ModelManager&&) = delete;
	ModelManager& operator=(ModelManager&&) = delete;

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, MadoEngine::Render::PSORegistry* psoRegistry);
	void Finalize();

	Model* Create(const std::string& name, const std::string& modelName, SceneType sceneType = SceneType::None);
	Model* Get(const std::string& name) const;
	void Destroy(const std::string& name);

	const ModelSharedData* GetSharedData(const std::string& modelName) const;

	void SetCamera(Camera* camera) { activeCamera_ = camera; }
	void UpdateAll(SceneType currentSceneType);
	void DrawAll(SceneType currentSceneType);
	void DrawAll(SceneType currentSceneType, Camera& camera);

private:
	ModelManager() = default;
	~ModelManager() = default;

	void LoadAllModels();
	void LoadModelFile(const std::string& path, ModelType type);
	const ModelSharedData* FindSharedData(const std::string& modelName) const;

	ID3D12Device* device_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
	MadoEngine::Render::PSORegistry* psoRegistry_ = nullptr;
	Camera* activeCamera_ = nullptr;

	std::unordered_map<std::string, std::unique_ptr<ModelSharedData>> sharedData_;
	std::unordered_map<std::string, std::string> aliases_;
	std::unordered_map<std::string, std::unique_ptr<Model>> models_;
};

} // namespace MadoEngine
