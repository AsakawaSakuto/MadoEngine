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

	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string modelPath) override;
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const ModelSharedData& sharedData);
	void Update() override;
	void Draw(Camera& useCamera) override;

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
	void UpdateLightGpuData();
	bool IsValidHandle(uint32_t handle) const;

	const ModelSharedData* sharedData_ = nullptr;
	std::unique_ptr<ModelSharedData> ownedSharedData_;
	std::vector<InstanceDesc> instances_;

	ModelMaterial* materialData_ = nullptr;
	CameraForGPU* cameraData_ = nullptr;
	LightGpuData* lightGpuData_ = nullptr;
	InstanceForGPU* instanceGpuData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource_;
	uint32_t instanceSrvIndex_ = UINT32_MAX;
	size_t instanceCapacity_ = 0;
	size_t drawInstanceCount_ = 0;

	SceneType sceneType_ = SceneType::None;
	LightLayerMask receiveLightMask_ = ToLightLayerMask(LightLayer::World);
	bool enableLighting_ = true;
	bool useEnvironmentMap_ = false;
	uint32_t environmentMapIndex_ = 0;
	Transform2D uvTransform_;
};
