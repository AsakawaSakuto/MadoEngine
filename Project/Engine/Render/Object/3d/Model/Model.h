#pragma once
#include "ModelSharedData.h"
#include "Render/Object/IRenderObject3d.h"
#include "Utility/Light/LightManager.h"
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

	/// @brief モデルのライティング有効状態を設定する
	/// @param enabled trueの場合はライト計算を行う
	void SetLightingEnabled(bool enabled);

	/// @brief 平行光源を設定する
	/// @param light モデル描画に使用する平行光源
	void SetDirectionalLight(const DirectionalLight& light);

	/// @brief 平行光源を有効化する
	/// @param enabled trueの場合は平行光源を使用する
	void SetDirectionalLightEnabled(bool enabled);

	/// @brief 平行光源の方向を設定する
	/// @param direction 光が進む方向
	void SetDirectionalLightDirection(const Vector3& direction);

	/// @brief 平行光源の色を設定する
	/// @param color ライトカラー
	void SetDirectionalLightColor(const Vector4& color);

	/// @brief 平行光源の強度を設定する
	/// @param intensity ライト強度
	void SetDirectionalLightIntensity(float intensity);

	/// @brief ハーフランバートを使用するか設定する
	/// @param enabled trueの場合はハーフランバートで拡散反射を計算する
	void SetUseHalfLambert(bool enabled);

	/// @brief レイとModelのワールドAABBの交差判定を行う
	/// @param rayOrigin レイの始点
	/// @param rayDirection 正規化済みのレイ方向
	/// @param maxDistance 判定する最大距離
	/// @param outDistance ヒットした距離の出力先
	/// @return レイがModelにヒットした場合はtrue
	bool Raycast(const Vector3& rayOrigin, const Vector3& rayDirection, float maxDistance, float& outDistance) const;

	void SetSceneType(SceneType sceneType);
	SceneType GetSceneType() const { return sceneType_; }

	/// @brief モデルが受け取るライトレイヤーマスクを設定する
	/// @param receiveLightMask 受け取るライトレイヤーマスク
	void SetReceiveLightMask(LightLayerMask receiveLightMask);

	/// @brief モデルが受け取るライトレイヤーマスクを取得する
	/// @return 受け取るライトレイヤーマスク
	LightLayerMask GetReceiveLightMask() const { return receiveLightMask_; }

	/// @brief 視錐台カリングの有効状態を設定する
	/// @param enabled trueの場合はカメラ範囲外のDrawCallをスキップする
	void SetFrustumCullingEnabled(bool enabled) { enableFrustumCulling_ = enabled; }

	/// @brief 視錐台カリングが有効か取得する
	/// @return 有効な場合はtrue
	bool IsFrustumCullingEnabled() const { return enableFrustumCulling_; }

	const ModelSharedData* GetSharedData() const { return sharedData_; }

private:
	void InitializeInstanceResources();

	/// @brief LightManagerからGPU送信用ライトデータを作成して定数バッファへ反映する
	void UpdateLightGpuData();

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
	bool enableLighting_ = true;
	DirectionalLight directionalLight_;

	bool useAnimationTimer_ = false;
	float animationTime_ = 0.0f;
	Skeleton skeletonData_;
	SkinCluster skinClusterData_;
	uint32_t skinClusterIndex_ = 0;

	ModelMaterial* materialData_ = nullptr;
	ModelTransformationMatrix* transformationData_ = nullptr;
	LightGpuData* lightGpuData_ = nullptr;
	CameraForGPU* cameraData_ = nullptr;

	ModelNode rootNode_;
	SceneType sceneType_ = SceneType::None;
	LightLayerMask receiveLightMask_ = ToLightLayerMask(LightLayer::World);
	bool enableFrustumCulling_ = true;
};
