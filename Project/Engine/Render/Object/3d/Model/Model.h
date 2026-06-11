#pragma once
#include "ModelSharedData.h"
#include "Render/Object/RenderObject3d.h"
#include ".SceneManager/SceneType.h"
#include <memory>
#include <string>

class Model : public RenderObject3d {
public:
	Model(std::string objectName);

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string modelPath) override;
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const ModelSharedData& sharedData);

	void Update() override;
	void Draw(Camera& useCamera) override;

	/// @brief レイとModelのワールドAABBの交差判定を行う
	/// @param rayOrigin レイの始点
	/// @param rayDirection 正規化済みのレイ方向
	/// @param maxDistance 判定する最大距離
	/// @param outDistance ヒットした距離の出力先
	/// @return レイがModelにヒットした場合はtrue
	bool Raycast(const Vector3& rayOrigin, const Vector3& rayDirection, float maxDistance, float& outDistance) const;

	void SetSceneType(SceneType sceneType) { sceneType_ = sceneType; }
	SceneType GetSceneType() const { return sceneType_; }

	const ModelSharedData* GetSharedData() const { return sharedData_; }

private:
	void InitializeInstanceResources();

	ModelType type_ = ModelType::Static;
	const ModelSharedData* sharedData_ = nullptr;
	std::unique_ptr<ModelSharedData> ownedSharedData_;

	Matrix4x4 worldMatrix_;
	Transform2D uvTransform_;

	std::string environmentMapName_;
	uint32_t environmentMapIndex_ = 0;
	bool useEnvironmentMap_ = false;

	bool useAnimationTimer_ = false;
	float animationTime_ = 0.0f;
	Skeleton skeletonData_;
	SkinCluster skinClusterData_;
	uint32_t skinClusterIndex_ = 0;

	ModelMaterial* materialData_ = nullptr;
	ModelTransformationMatrix* transformationData_ = nullptr;
	DirectionalLight* directionalLightData_ = nullptr;
	PointLight* pointLightData_ = nullptr;
	SpotLight* spotLightData_ = nullptr;
	CameraForGPU* cameraData_ = nullptr;

	ModelNode rootNode_;
	SceneType sceneType_ = SceneType::None;
};
