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

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const ModelSharedData& sharedData);

	void Update() override;

	void Draw(Camera& useCamera) override;

	/// @brief シャドウマップ生成用にモデルを描画する
	/// @param lightViewProjection ライト視点のビュー射影行列
	void DrawShadow(const Matrix4x4& lightViewProjection);

	/// @brief シャドウマップ生成用にモデルを描画する
	/// @param lightViewProjection ライト視点のビュープロジェクション行列
	/// @param billboardCamera ビルボードの向きに使用するカメラ
	void DrawShadow(const Matrix4x4& lightViewProjection, const Camera& billboardCamera);

	/// @brief ビルボード行列を使用するかを設定する
	/// @param enabled trueの場合はカメラ向きのビルボード行列を使用する
	void SetUseBillboard(bool enabled) { usebillbord_ = enabled; }

	/// @brief ビルボード行列を使用するかを取得する
	/// @return 使用する場合はtrue
	bool IsUseBillboard() const { return usebillbord_; }

	/// @brief 他の3Dオブジェクトに影を落とすかを設定する
	/// @param enabled trueの場合はシャドウマップへ深度を書き込む
	void SetCastShadow(bool enabled) { castShadow_ = enabled; }

	/// @brief 他の3Dオブジェクトに影を落とすかを取得する
	/// @return 影を落とす場合はtrue
	bool CanCastShadow() const { return castShadow_; }

	/// @brief 他の3Dオブジェクトから影を受けるかを設定する
	/// @param enabled trueの場合は通常描画時にシャドウマップを参照する
	void SetReceiveShadow(bool enabled) { receiveShadow_ = enabled; }

	/// @brief 他の3Dオブジェクトから影を受けるかを取得する
	/// @return 影を受ける場合はtrue
	bool CanReceiveShadow() const { return receiveShadow_; }

	/// @brief 通常描画で参照するシャドウマップ情報を設定する
	/// @param shadowMapSrv シャドウマップSRVのGPUディスクリプタハンドル
	/// @param lightViewProjection ライト視点のビュー射影行列
	/// @param width シャドウマップの幅
	/// @param height シャドウマップの高さ
	void SetShadowMap(
		D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSrv,
		const Matrix4x4& lightViewProjection,
		uint32_t width,
		uint32_t height
	);

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

	/// @brief 現在のTransformからワールド行列を作成する
	/// @param billboardCamera ビルボードの向きに使用するカメラ。nullptrの場合は通常の回転を使用する
	/// @return 作成したワールド行列
	Matrix4x4 MakeWorldMatrix(const Camera* billboardCamera) const;

	/// @brief シャドウマップ生成用の共通描画処理を行う
	/// @param lightViewProjection ライト視点のビュープロジェクション行列
	/// @param billboardCamera ビルボードの向きに使用するカメラ。nullptrの場合は通常の回転を使用する
	void DrawShadowInternal(const Matrix4x4& lightViewProjection, const Camera* billboardCamera);

	/// @brief LightManagerからGPU送信用ライトデータを作成して定数バッファへ反映する
	void UpdateLightGpuData();

	/// @brief カメラを反映した変換行列をGPUデータへ更新する
	/// @param camera 描画に使用するカメラ
	void UpdateTransformGpuData(const Camera& camera);

	/// @brief モデルのワールド空間AABBを計算する
	/// @param outMin ワールド空間AABBの最小座標
	/// @param outMax ワールド空間AABBの最大座標
	/// @return 計算できた場合はtrue
	bool CalculateWorldAABB(Vector3& outMin, Vector3& outMax, const Camera* billboardCamera = nullptr) const;

	/// @brief モデルがカメラの視錐台内にあるか判定する
	/// @param camera 判定に使用するカメラ
	/// @return 視錐台内、または視錐台と交差している場合はtrue
	bool IsInsideCameraFrustum(const Camera& camera) const;

	/// @brief シャドウ描画用の変換行列をGPUバッファへ更新する
	/// @param lightViewProjection ライト視点のビュー射影行列
	void UpdateShadowTransformGpuData(const Matrix4x4& lightViewProjection, const Camera* billboardCamera = nullptr);

	/// @brief 通常描画用のシャドウ情報をGPUバッファへ反映する
	void UpdateReceiveShadowGpuData();

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
	ModelTransformationMatrix* shadowTransformationData_ = nullptr;
	ModelShadowGpuData* shadowGpuData_ = nullptr;
	LightGpuData* lightGpuData_ = nullptr;
	CameraForGPU* cameraData_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> shadowTransformationResource_;

	ModelNode rootNode_;
	SceneType sceneType_ = SceneType::None;
	LightLayerMask receiveLightMask_ = ToLightLayerMask(LightLayer::World);
	bool enableFrustumCulling_ = true;
	bool usebillbord_ = false;
	bool castShadow_ = true;
	bool receiveShadow_ = true;
	D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSrvHandle_ = {};
	Matrix4x4 shadowLightViewProjection_ = Matrix::MakeIdentity();
	uint32_t shadowMapWidth_ = 2048;
	uint32_t shadowMapHeight_ = 2048;
};
