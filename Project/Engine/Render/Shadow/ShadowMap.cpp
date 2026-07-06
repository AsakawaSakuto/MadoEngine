#include "Render/Shadow/ShadowMap.h"
#include "Core/DxDevice/DxDevice.h"
#include "Core/View/DSVManager.h"
#include "Core/View/SRVManager.h"
#include "Math/Function/MathFunction.h"
#include "Math/Function/MatrixFunction.h"
#include "Utility/Light/DirectionalLight.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include <cmath>
#include <string>

namespace {

	/// @brief シャドウ用ライト方向を正規化する
	/// @param direction DirectionalLightの方向
	/// @return 正規化済み方向
	Vector3 NormalizeShadowLightDirection(const Vector3& direction) {
		Vector3 normalized = direction.Normalized();
		if (normalized.LengthSq() == 0.0f) {
			normalized = { 0.0f, -1.0f, 0.0f };
		}
		return normalized;
	}

	/// @brief 左手系のLookAtビュー行列を作成する
	/// @param eye 視点位置
	/// @param target 注視点
	/// @param up 上方向
	/// @return ビュー行列
	Matrix4x4 MakeLookAtLH(const Vector3& eye, const Vector3& target, const Vector3& up) {
		Vector3 zAxis = (target - eye).Normalized();
		if (zAxis.LengthSq() == 0.0f) {
			zAxis = { 0.0f, 0.0f, 1.0f };
		}

		Vector3 upAxis = up.Normalized();
		if (upAxis.LengthSq() == 0.0f || std::abs(Math::Dot(upAxis, zAxis)) > 0.99f) {
			upAxis = { 0.0f, 0.0f, 1.0f };
		}
		if (std::abs(Math::Dot(upAxis, zAxis)) > 0.99f) {
			upAxis = { 1.0f, 0.0f, 0.0f };
		}

		Vector3 xAxis = Math::Cross(upAxis, zAxis).Normalized();
		Vector3 yAxis = Math::Cross(zAxis, xAxis);

		Matrix4x4 result = Matrix::MakeIdentity();
		result.m[0][0] = xAxis.x;
		result.m[1][0] = xAxis.y;
		result.m[2][0] = xAxis.z;
		result.m[3][0] = -Math::Dot(xAxis, eye);

		result.m[0][1] = yAxis.x;
		result.m[1][1] = yAxis.y;
		result.m[2][1] = yAxis.z;
		result.m[3][1] = -Math::Dot(yAxis, eye);

		result.m[0][2] = zAxis.x;
		result.m[1][2] = zAxis.y;
		result.m[2][2] = zAxis.z;
		result.m[3][2] = -Math::Dot(zAxis, eye);

		return result;
	}

} // namespace

namespace MadoEngine::Render {

	ShadowMap::ShadowMap() {
		lightViewMatrix_ = Matrix::MakeIdentity();
		lightProjectionMatrix_ = Matrix::MakeIdentity();
		lightViewProjectionMatrix_ = Matrix::MakeIdentity();
	}

	/// @brief シャドウマップ用の深度バッファを初期化する
	/// @param device DxDeviceのポインタ
	/// @param dsvManager DSVManagerのポインタ
	/// @param srvManager SRVManagerのポインタ
	void ShadowMap::Initialize(
		MadoEngine::Core::DxDevice* device,
		MadoEngine::Core::DSVManager* dsvManager,
		MadoEngine::Core::SRVManager* srvManager
	) {
		assert(device != nullptr && "DxDeviceがnullptrです");
		assert(dsvManager != nullptr && "DSVManagerがnullptrです");
		assert(srvManager != nullptr && "SRVManagerがnullptrです");

		depthBuffer_.Initialize(
			device,
			dsvManager,
			srvManager,
			kShadowMapWidth,
			kShadowMapHeight,
			DXGI_FORMAT_D32_FLOAT
		);
		viewportScissor_.UpdateSize(kShadowMapWidth, kShadowMapHeight);
		lightProjectionMatrix_ = Matrix::MakeOrthographic(
			-kShadowOrthoWidth * 0.5f,
			kShadowOrthoHeight * 0.5f,
			kShadowOrthoWidth * 0.5f,
			-kShadowOrthoHeight * 0.5f,
			kShadowNearClip,
			kShadowFarClip
		);
		lightViewMatrix_ = Matrix::MakeIdentity();
		lightViewProjectionMatrix_ = Matrix::Multiply(lightViewMatrix_, lightProjectionMatrix_);
		isInitialized_ = true;

		Logger::Output(
			"[Engine] シャドウマップ用深度バッファを初期化しました: " +
			std::to_string(kShadowMapWidth) + "x" + std::to_string(kShadowMapHeight),
			Logger::Level::Engine
		);
	}

	/// @brief シャドウマップへの深度描画を開始する
	/// @param commandList 描画コマンドリスト
	void ShadowMap::Begin(ID3D12GraphicsCommandList* commandList) {
		assert(isInitialized_ && "ShadowMapが未初期化です");
		assert(commandList != nullptr && "commandListがnullptrです");

		depthBuffer_.Transition(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		viewportScissor_.Apply(commandList);

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthBuffer_.GetDSVCPUHandle();
		commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
		commandList->ClearDepthStencilView(
			dsvHandle,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f,
			0,
			0,
			nullptr
		);
	}

	/// @brief シャドウマップへの深度描画を終了し、SRV参照可能な状態にする
	/// @param commandList 描画コマンドリスト
	void ShadowMap::End(ID3D12GraphicsCommandList* commandList) {
		assert(isInitialized_ && "ShadowMapが未初期化です");
		assert(commandList != nullptr && "commandListがnullptrです");

		depthBuffer_.Transition(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	/// @brief シャドウマップ生成用PSODescを作成する
	/// @return RTVなし、DSVのみ、VSのみのPSODesc
	PSODesc ShadowMap::CreatePSODesc() {
		PSODesc desc{};
		desc.blendMode = BlendMode::None;
		desc.depthMode = DepthMode::ReadWrite;
		desc.cullMode = CullMode::Back;
		desc.fillMode = FillMode::Solid;
		desc.topology = TopologyType::Triangle;
		desc.inputLayout = InputLayoutType::StaticModel;
		desc.renderTargetCount = 0;
		desc.rtvFormat = DXGI_FORMAT_UNKNOWN;
		desc.dsvFormat = DXGI_FORMAT_D32_FLOAT;
		desc.depthBias = kShadowDepthBias;
		desc.depthBiasClamp = kShadowDepthBiasClamp;
		desc.slopeScaledDepthBias = kShadowSlopeScaledDepthBias;
		desc.vsKey = "Object3d/Shadow/ShadowMap.VS";
		desc.psKey.clear();
		desc.rootSigKey = "ShadowMap.RootSig";
		return desc;
	}

	/// @brief DirectionalLightの方向からライト視点行列を更新する
	/// @param directionalLight 影生成に使う平行光
	/// @param focusPosition ライトが注視するワールド座標
	void ShadowMap::UpdateLightViewProjection(const DirectionalLight& directionalLight, Vector3 focusPosition) {
		Vector3 lightDirection = NormalizeShadowLightDirection(directionalLight.direction);
		Vector3 lightPosition = focusPosition - lightDirection * ((kShadowNearClip + kShadowFarClip) * 0.5f);

		lightViewMatrix_ = MakeLookAtLH(lightPosition, focusPosition, { 0.0f, 1.0f, 0.0f });
		lightProjectionMatrix_ = Matrix::MakeOrthographic(
			-kShadowOrthoWidth * 0.5f,
			kShadowOrthoHeight * 0.5f,
			kShadowOrthoWidth * 0.5f,
			-kShadowOrthoHeight * 0.5f,
			kShadowNearClip,
			kShadowFarClip
		);
		lightViewProjectionMatrix_ = Matrix::Multiply(lightViewMatrix_, lightProjectionMatrix_);
	}

	/// @brief シャドウマップSRVのGPUディスクリプタハンドルを取得する
	/// @return GPUディスクリプタハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE ShadowMap::GetSRVGPUHandle() const {
		assert(isInitialized_ && "ShadowMapが未初期化です");
		return depthBuffer_.GetSRVGPUHandle();
	}

	/// @brief シャドウマップDSVのCPUディスクリプタハンドルを取得する
	/// @return CPUディスクリプタハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE ShadowMap::GetDSVCPUHandle() const {
		assert(isInitialized_ && "ShadowMapが未初期化です");
		return depthBuffer_.GetDSVCPUHandle();
	}

} // namespace MadoEngine::Render
