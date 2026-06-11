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

	/// @brief 指定したシーンに属するModelインスタンスをすべて破棄する
	/// @param sceneType 破棄対象のシーン種別
	void DestroyByScene(SceneType sceneType);

	const ModelSharedData* GetSharedData(const std::string& modelName) const;

	/// @brief レイにヒットする最前面のModelを取得する
	/// @param currentSceneType 選択対象のシーン種別
	/// @param rayOrigin レイの始点
	/// @param rayDirection 正規化済みのレイ方向
	/// @param maxDistance 判定する最大距離
	/// @param outDistance ヒット距離の出力先
	/// @return ヒットしたModel。ヒットしない場合はnullptr
	Model* PickByRay(
		SceneType currentSceneType,
		const Vector3& rayOrigin,
		const Vector3& rayDirection,
		float maxDistance,
		float* outDistance = nullptr) const;

	void SetCamera(const Camera& camera) { activeCamera_ = camera; }
	Camera GetCamera() const { return activeCamera_; }

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
	Camera activeCamera_;

	std::unordered_map<std::string, std::unique_ptr<ModelSharedData>> sharedData_;
	std::unordered_map<std::string, std::string> aliases_;
	std::unordered_map<std::string, std::unique_ptr<Model>> models_;
};

} // namespace MadoEngine
