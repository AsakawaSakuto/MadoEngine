#pragma once
#include "ModelSharedData.h"
#include "Render/Object/IRenderObject3d.h"
#include "Utility/Light/LightManager.h"
#include ".SceneManager/SceneType.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class InstancedModel : public IRenderObject3d {
public:
	struct InstanceDesc {
		Transform3D transform;
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		bool isVisible = true;
	};

	InstancedModel(std::string objectName);
	~InstancedModel() override;

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const ModelSharedData& sharedData);

	void Update() override;

	void Draw(Camera& useCamera) override;

	/// @brief シャドウマップへインスタンスモデルの深度を書き込みます。
	/// @param lightViewProjection ライト視点のビュー射影行列です。
	void DrawShadow(const Matrix4x4& lightViewProjection);

	/// @brief 他の3DObjectへ影を書き込むかを設定します。
	/// @param enabled 影を書き込む場合はtrueです。
	void SetCastShadow(bool enabled) { castShadow_ = enabled; }

	/// @brief 他の3DObjectへ影を書き込むかを取得します。
	/// @return 影を書き込む場合はtrueです。
	bool CanCastShadow() const { return castShadow_; }

	/// @brief 他の3DObjectから影を書き込まれるかを設定します。
	/// @param enabled 影を書き込まれる場合はtrueです。
	void SetReceiveShadow(bool enabled) { receiveShadow_ = enabled; }

	/// @brief 他の3DObjectから影を書き込まれるかを取得します。
	/// @return 影を書き込まれる場合はtrueです。
	bool CanReceiveShadow() const { return receiveShadow_; }

	/// @brief 通常描画で参照するシャドウマップ情報を設定します。
	/// @param shadowMapSrv シャドウマップSRVのGPUディスクリプタハンドルです。
	/// @param lightViewProjection ライト視点のビュー射影行列です。
	/// @param width シャドウマップの幅です。
	/// @param height シャドウマップの高さです。
	void SetShadowMap(
		D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSrv,
		const Matrix4x4& lightViewProjection,
		uint32_t width,
		uint32_t height
	);

	/// @brief インスタンスを追加します。
	/// @param desc 追加するインスタンスの設定です。
	/// @return 追加したインスタンスのハンドルです。
	uint32_t AddInstance(const InstanceDesc& desc);

	/// @brief すべてのインスタンスを削除します。
	void ClearInstances();

	/// @brief インスタンスのTransformを設定します。
	/// @param handle 対象インスタンスのハンドルです。
	/// @param transform 設定するTransformです。
	void SetInstanceTransform(uint32_t handle, const Transform3D& transform);

	/// @brief インスタンスの色を設定します。
	/// @param handle 対象インスタンスのハンドルです。
	/// @param color 設定する色です。
	void SetInstanceColor(uint32_t handle, const Vector4& color);

	/// @brief インスタンスの表示状態を設定します。
	/// @param handle 対象インスタンスのハンドルです。
	/// @param isVisible 表示する場合はtrueです。
	void SetInstanceVisible(uint32_t handle, bool isVisible);

	/// @brief インスタンス数を取得します。
	/// @return 登録済みインスタンス数です。
	size_t GetInstanceCount() const { return instances_.size(); }

	void SetSceneType(SceneType sceneType);
	SceneType GetSceneType() const { return sceneType_; }
	void SetReceiveLightMask(LightLayerMask receiveLightMask);
	LightLayerMask GetReceiveLightMask() const { return receiveLightMask_; }
	const ModelSharedData* GetSharedData() const { return sharedData_; }

private:
	struct InstanceForGPU {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Matrix4x4 WorldInverseTranspose;
		Vector4 color;
	};

	void InitializeInstanceResources();
	void EnsureInstanceResource(size_t requiredCount);
	void EnsureShadowInstanceResource(size_t requiredCount);
	size_t BuildInstanceGpuData(const Matrix4x4& viewProjectionMatrix, InstanceForGPU* outputData);
	void UpdateLightGpuData();
	void UpdateReceiveShadowGpuData();
	bool IsValidHandle(uint32_t handle) const;

	const ModelSharedData* sharedData_ = nullptr;
	std::unique_ptr<ModelSharedData> ownedSharedData_;
	std::vector<InstanceDesc> instances_;

	ModelMaterial* materialData_ = nullptr;
	CameraForGPU* cameraData_ = nullptr;
	LightGpuData* lightGpuData_ = nullptr;
	ModelShadowGpuData* shadowGpuData_ = nullptr;
	InstanceForGPU* instanceGpuData_ = nullptr;
	InstanceForGPU* shadowInstanceGpuData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> shadowInstanceResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> shadowGpuDataResource_;
	uint32_t instanceSrvIndex_ = UINT32_MAX;
	uint32_t shadowInstanceSrvIndex_ = UINT32_MAX;
	size_t instanceCapacity_ = 0;
	size_t shadowInstanceCapacity_ = 0;
	size_t drawInstanceCount_ = 0;
	size_t shadowDrawInstanceCount_ = 0;

	SceneType sceneType_ = SceneType::None;
	LightLayerMask receiveLightMask_ = ToLightLayerMask(LightLayer::World);
	D3D12_GPU_DESCRIPTOR_HANDLE shadowMapSrvHandle_{};
	Matrix4x4 shadowLightViewProjection_ = Matrix::MakeIdentity();
	uint32_t shadowMapWidth_ = 2048;
	uint32_t shadowMapHeight_ = 2048;
	bool enableLighting_ = true;
	bool useEnvironmentMap_ = false;
	bool castShadow_ = true;
	bool receiveShadow_ = true;
	uint32_t environmentMapIndex_ = 0;
	Transform2D uvTransform_;
};
