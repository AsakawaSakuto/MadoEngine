#pragma once
#include "ModelSharedData.h"
#include "../Animation/Animator.h"
#include "Render/Object/IRenderObject3d.h"
#include "Utility/Light/LightManager.h"
#include "Utility/Json/Core/IJsonSerializable.h"
#include ".SceneManager/SceneType.h"
#include <memory>
#include <string>
#include <string_view>

namespace MadoEngine {
	class ModelManager;
}

class Model : public IRenderObject3d, public MadoEngine::Json::IJsonSerializable {
public:

	Model(std::string objectName);
	~Model() override;

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const ModelSharedData& sharedData);

	/// @brief Model状態とAnimation Poseを更新
	/// @param deltaTime 前フレームからの経過時間
	void Update(float deltaTime) override;

	void Draw(Camera& useCamera) override;

	/// @brief JsonからModelの状態を復元
	/// @param json 復元元のJson
	void FromJson(const nlohmann::json& json) override;

	/// @brief Modelの状態をJsonへ変換
	/// @return Modelの状態を格納したJson
	nlohmann::json ToJson() const override;

	/// @brief シャドウマップ生成用にモデルを描画
	/// @param lightViewProjection ライト視点のビュー射影行列
	void DrawShadow(const Matrix4x4& lightViewProjection);

	/// @brief シャドウマップ生成用にモデルを描画
	/// @param lightViewProjection ライト視点のビュープロジェクション行列
	/// @param billboardCamera ビルボードの向きに使用するカメラ
	void DrawShadow(const Matrix4x4& lightViewProjection, const Camera& billboardCamera);

	/// @brief ビルボード行列を使用するかを設定
	/// @param enabled trueの場合はカメラ向きのビルボード行列を使用
	void SetUseBillboard(bool enabled) { usebillbord_ = enabled; }

	/// @brief ビルボード行列を使用するかを取得
	/// @return 使用する場合はtrue
	bool IsUseBillboard() const { return usebillbord_; }

	/// @brief 他の3Dオブジェクトに影を落とすかを設定
	/// @param enabled trueの場合はシャドウマップへ深度を書き込み
	void SetCastShadow(bool enabled) { castShadow_ = enabled; }

	/// @brief 他の3Dオブジェクトに影を落とすかを取得
	/// @return 影を落とす場合はtrue
	bool CanCastShadow() const { return castShadow_; }

	/// @brief 他の3Dオブジェクトから影を受けるかを設定
	/// @param enabled trueの場合は通常描画時にシャドウマップを参照
	void SetReceiveShadow(bool enabled) { receiveShadow_ = enabled; }

	/// @brief 他の3Dオブジェクトから影を受けるかを取得
	/// @return 影を受ける場合はtrue
	bool CanReceiveShadow() const { return receiveShadow_; }

	/// @brief 通常描画で参照するシャドウマップ情報を設定
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

	/// @brief モデルのライティング有効状態を設定
	/// @param enabled trueの場合はライト計算
	void SetLightingEnabled(bool enabled);

	/// @brief ライティングが有効かを取得
	/// @return 有効な場合はtrue
	bool IsLightingEnabled() const { return enableLighting_; }

	/// @brief 平行光源を設定
	/// @param light モデル描画に使用する平行光源
	void SetDirectionalLight(const DirectionalLight& light);

	/// @brief 平行光源を有効化
	/// @param enabled trueの場合は平行光源を使用
	void SetDirectionalLightEnabled(bool enabled);

	/// @brief 平行光源の方向を設定
	/// @param direction 光が進む方向
	void SetDirectionalLightDirection(const Vector3& direction);

	/// @brief 平行光源の色を設定
	/// @param color ライトカラー
	void SetDirectionalLightColor(const Vector4& color);

	/// @brief 平行光源の強度を設定
	/// @param intensity ライト強度
	void SetDirectionalLightIntensity(float intensity);

	/// @brief ハーフランバートを使用するか設定
	/// @param enabled trueの場合はハーフランバートで拡散反射を計算
	void SetUseHalfLambert(bool enabled);

	/// @brief レイとModelのワールドAABBの交差判定
	/// @param rayOrigin レイの始点
	/// @param rayDirection 正規化済みのレイ方向
	/// @param maxDistance 判定する最大距離
	/// @param outDistance ヒットした距離の出力先
	/// @return レイがModelにヒットした場合はtrue
	bool Raycast(const Vector3& rayOrigin, const Vector3& rayDirection, float maxDistance, float& outDistance) const;

	void SetSceneType(SceneType sceneType);
	SceneType GetSceneType() const { return sceneType_; }

	/// @brief モデルが受け取るライトレイヤーマスクを設定
	/// @param receiveLightMask 受け取るライトレイヤーマスク
	void SetReceiveLightMask(LightLayerMask receiveLightMask);

	/// @brief モデルが受け取るライトレイヤーマスクを取得
	/// @return 受け取るライトレイヤーマスク
	LightLayerMask GetReceiveLightMask() const { return receiveLightMask_; }

	/// @brief 視錐台カリングの有効状態を設定
	/// @param enabled trueの場合はカメラ範囲外のDrawCallをスキップ
	void SetFrustumCullingEnabled(bool enabled) { enableFrustumCulling_ = enabled; }

	/// @brief 視錐台カリングが有効か取得
	/// @return 有効な場合はtrue
	bool IsFrustumCullingEnabled() const { return enableFrustumCulling_; }

	/// @brief Modelで使用するテクスチャを上書き
	/// @param textureName TextureManagerに登録されているテクスチャ名
	/// @return テクスチャを変更できた場合はtrue
	bool SetTexture(const std::string& textureName);

	/// @brief Modelのテクスチャ上書きを解除してアセット既定値へ復元
	/// @return アセット既定値へ戻せた場合はtrue
	bool ResetTexture();

	/// @brief Modelで現在使用している代表テクスチャ名を取得
	/// @return 現在使用しているテクスチャ名
	const std::string& GetTextureName() const { return textureName_; }

	/// @brief Modelのテクスチャが上書きされているかを取得
	/// @return 上書きされている場合はtrue
	bool HasTextureOverride() const { return !textureOverrideName_.empty(); }

	/// @brief 指定した頂点のローカル座標を取得
	/// @param vertexIndex 取得する頂点配列のインデックス
	/// @return 頂点のローカル座標、未初期化または範囲外の場合はゼロベクトル
	Vector3 GetVertexPosition(uint32_t vertexIndex) const;

	/// @brief モデルが保持する頂点数を取得
	/// @return 頂点数
	size_t GetVertexCount() const;

	/// @brief 指定した頂点の現在のワールド座標を取得
	/// @param vertexIndex 取得する頂点配列のインデックス
	/// @param outPosition 頂点のワールド座標の出力先
	/// @return 頂点を取得できた場合はtrue
	bool TryGetVertexWorldPosition(uint32_t vertexIndex, Vector3& outPosition) const;

	/// @brief 指定したAnimationClipへ遷移
	/// @param clipName 再生するAnimationClip名
	/// @param blendDuration 遷移時間、負数の場合はClipの標準値
	/// @param restart 同じClipでも先頭から再生する場合はtrue
	/// @return 再生を開始できた場合はtrue
	bool PlayAnimation(std::string_view clipName, float blendDuration = -1.0f, bool restart = false);

	/// @brief 指定したAnimationClipを再生可能か判定
	/// @param clipName 確認するAnimationClip名
	/// @return 再生可能な場合はtrue
	bool HasAnimationClip(std::string_view clipName) const;

	/// @brief 現在再生中のAnimationClip名を取得
	/// @return 現在再生中のAnimationClip名
	std::string_view GetCurrentAnimationName() const { return animator_.GetCurrentClipName(); }

	/// @brief 現在のAnimationClipが終端へ到達したか判定
	/// @return 終端へ到達した場合はtrue
	bool IsAnimationFinished() const { return animator_.IsFinished(); }

	/// @brief Animation再生速度倍率を設定
	/// @param playbackSpeed 再生速度倍率
	void SetAnimationPlaybackSpeed(float playbackSpeed) { animator_.SetPlaybackSpeed(playbackSpeed); }

	const ModelSharedData* GetSharedData() const { return sharedData_; }

private:
	friend class MadoEngine::ModelManager;

	/// @brief ModelManagerが管理する識別名を更新
	/// @param objectName 新しい識別名
	void SetObjectName(const std::string& objectName) { objectName_ = objectName; }

	void InitializeInstanceResources();

	/// @brief 現在のTransformからワールド行列を作成
	/// @param billboardCamera ビルボードの向きに使用するカメラ、nullptrの場合は通常の回転を使用
	/// @return 作成したワールド行列
	Matrix4x4 MakeWorldMatrix(const Camera* billboardCamera) const;

	/// @brief シャドウマップ生成用の共通描画処理
	/// @param lightViewProjection ライト視点のビュープロジェクション行列
	/// @param billboardCamera ビルボードの向きに使用するカメラ、nullptrの場合は通常の回転を使用
	void DrawShadowInternal(const Matrix4x4& lightViewProjection, const Camera* billboardCamera);

	/// @brief LightManagerからGPU送信用ライトデータを作成して定数バッファへ反映
	void UpdateLightGpuData();

	/// @brief カメラを反映した変換行列をGPUデータへ更新
	/// @param camera 描画に使用するカメラ
	void UpdateTransformGpuData(const Camera& camera);

	/// @brief モデルのワールド空間AABBを計算
	/// @param outMin ワールド空間AABBの最小座標
	/// @param outMax ワールド空間AABBの最大座標
	/// @return 計算できた場合はtrue
	bool CalculateWorldAABB(Vector3& outMin, Vector3& outMax, const Camera* billboardCamera = nullptr) const;

	/// @brief モデルがカメラの視錐台内にあるか判定
	/// @param camera 判定に使用するカメラ
	/// @return 視錐台内、または視錐台と交差している場合はtrue
	bool IsInsideCameraFrustum(const Camera& camera) const;

	/// @brief シャドウ描画用の変換行列をGPUバッファへ更新
	/// @param lightViewProjection ライト視点のビュー射影行列
	void UpdateShadowTransformGpuData(const Matrix4x4& lightViewProjection, const Camera* billboardCamera = nullptr);

	/// @brief 通常描画用のシャドウ情報をGPUバッファへ反映
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

	Animator animator_;
	Skeleton skeletonData_;
	SkinCluster skinClusterData_;
	uint32_t skinClusterIndex_ = UINT32_MAX;

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
	std::string textureOverrideName_;
	D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSrvHandle_ = {};
	Matrix4x4 shadowLightViewProjection_ = Matrix::MakeIdentity();
	uint32_t shadowMapWidth_ = 2048;
	uint32_t shadowMapHeight_ = 2048;
};
