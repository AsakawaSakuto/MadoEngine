#pragma once
#include "ModelSharedData.h"
#include "Render/Object/IRenderObject3d.h"
#include ".SceneManager/SceneType.h"
#include <memory>
#include <string>

class Model : public IRenderObject3d {
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

	/// @brief 視錐台カリングの有効状態を設定する
	/// @param enabled trueの場合はカメラ範囲外のDrawCallをスキップする
	void SetFrustumCullingEnabled(bool enabled) { enableFrustumCulling_ = enabled; }

	/// @brief 視錐台カリングが有効か取得する
	/// @return 有効な場合はtrue
	bool IsFrustumCullingEnabled() const { return enableFrustumCulling_; }

	const ModelSharedData* GetSharedData() const { return sharedData_; }

private:
	void InitializeInstanceResources();

	/// @brief モデルのワールド空間AABBを計算する
	/// @param outMin ワールド空間AABBの最小座標
	/// @param outMax ワールド空間AABBの最大座標
	/// @return 計算できた場合はtrue
	bool CalculateWorldAABB(Vector3& outMin, Vector3& outMax) const;

	/// @brief モデルがカメラの視錐台内にあるか判定する
	/// @param camera 判定に使用するカメラ
	/// @return 視錐台内、または視錐台と交差している場合はtrue
	bool IsInsideCameraFrustum(const Camera& camera) const;

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
	bool enableFrustumCulling_ = true;
};
