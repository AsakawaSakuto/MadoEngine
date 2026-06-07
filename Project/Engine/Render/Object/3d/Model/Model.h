#include "ModelData.h"
#include "ModelType.h"
#include "Render/Object/RenderObject3d.h"
#include "../Animation/AnimationFunction.h"
#include "../Animation/AnimationStruct.h"

class Model : public RenderObject3d {
public:

	Model(std::string ObjectName);

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string modelPath) override;

	void Update() override;

	void Draw(Camera& useCamera) override;
private:

	ModelType type_ = ModelType::Static;
	ModelData modelData_;

	// ワールド変換行列情報
	Matrix4x4 worldMatrix_;
	Transform2D uvTransform_; // UV変換情報

	// 環境マップ（CubeMap）関連
	std::string environmentMapName_;   // 環境マップテクスチャファイルパス
	uint32_t environmentMapIndex_ = 0; // 環境マップテクスチャインデックス
	bool useEnvironmentMap_ = false;   // 環境マップ使用フラグ

	// アニメーション関連
	bool useAnimationTimer_ = false; // アニメーション使用フラグ
	float animationTime_ = 0.0f;     // アニメーション再生時間
	Animation animationData_;     // 共有アニメーションデータ
	Skeleton skeletonData_;       // 共有スケルトンデータ
	SkinCluster skinClusterData_; // 共有スキンクラスター
	uint32_t skinClusterIndex_;   // スキンクラスター用SRVインデックス

	ModelVertexData* vertexData_ = nullptr;
	uint32_t* indexData_ = nullptr;
	ModelMaterial* materialData_ = nullptr;
	ModelTransformationMatrix* transformationData_ = nullptr;
	DirectionalLight* directionalLightData_ = nullptr;
	PointLight* pointLightData_ = nullptr;
	SpotLight* spotLightData_ = nullptr;
	CameraForGPU* cameraData_ = nullptr;
};