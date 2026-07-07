#pragma once
#include "Core/DepthStencilBuffer/DepthStencilBuffer.h"
#include "Math/Matrix4x4.h"
#include "Math/Vector3.h"
#include "Render/PSO/PSODesc.h"
#include "Render/Screen/ViewportScissor.h"
#include <cstdint>
#include <d3d12.h>

struct DirectionalLight;

namespace MadoEngine::Core {
	class DxDevice;
	class DSVManager;
	class SRVManager;
}

namespace MadoEngine::Render {

	/// @brief DirectionalLight用シャドウマップを管理するクラス
	/// @details 深度専用テクスチャをDSVとして書き込み、通常描画ではSRVとして参照できるようにする。
	class ShadowMap {
	public:
		static constexpr uint32_t kShadowMapWidth = 2048;          // シャドウマップの幅
		static constexpr uint32_t kShadowMapHeight = 2048;         // シャドウマップの高さ
		static constexpr float kShadowOrthoWidth =  200.0f;         // シャドウマップの直交投影幅
		static constexpr float kShadowOrthoHeight = 200.0f;         // シャドウマップの直交投影高さ
		static constexpr float kShadowNearClip = 0.1f;             // シャドウマップの近クリップ距離
		static constexpr float kShadowFarClip = 1000.0f;           // シャドウマップの遠クリップ距離
		static constexpr int kShadowDepthBias = 1000;              // シャドウマップの深度バイアス
		static constexpr float kShadowDepthBiasClamp = 0.0;        // シャドウマップの深度バイアスクランプ
		static constexpr float kShadowSlopeScaledDepthBias = 1.5f; // シャドウマップの傾斜スケール深度バイアス

		ShadowMap();
		~ShadowMap() = default;

		ShadowMap(const ShadowMap&) = delete;
		ShadowMap& operator=(const ShadowMap&) = delete;
		ShadowMap(ShadowMap&&) = default;
		ShadowMap& operator=(ShadowMap&&) = default;

		/// @brief シャドウマップ用の深度バッファを初期化する
		/// @param device DxDeviceのポインタ
		/// @param dsvManager DSVManagerのポインタ
		/// @param srvManager SRVManagerのポインタ
		void Initialize(
			MadoEngine::Core::DxDevice* device,
			MadoEngine::Core::DSVManager* dsvManager,
			MadoEngine::Core::SRVManager* srvManager
		);

		/// @brief シャドウマップへの深度描画を開始する
		/// @param commandList 描画コマンドリスト
		void Begin(ID3D12GraphicsCommandList* commandList);

		/// @brief シャドウマップへの深度描画を終了し、SRV参照可能な状態にする
		/// @param commandList 描画コマンドリスト
		void End(ID3D12GraphicsCommandList* commandList);

		/// @brief シャドウマップ生成用PSODescを作成する
		/// @return RTVなし、DSVのみ、VSのみのPSODesc
		static PSODesc CreatePSODesc();

		/// @brief DirectionalLightの方向からライト視点行列を更新する
		/// @param directionalLight 影生成に使う平行光
		/// @param focusPosition ライトが注視するワールド座標
		void UpdateLightViewProjection(const DirectionalLight& directionalLight, Vector3 focusPosition = { 0.0f, 0.0f, 0.0f });

		/// @brief ライトビュー行列を取得する
		/// @return ライトビュー行列
		const Matrix4x4& GetLightViewMatrix() const { return lightViewMatrix_; }

		/// @brief ライト射影行列を取得する
		/// @return ライト射影行列
		const Matrix4x4& GetLightProjectionMatrix() const { return lightProjectionMatrix_; }

		/// @brief ライトビュー射影行列を取得する
		/// @return ライトビュー射影行列
		const Matrix4x4& GetLightViewProjectionMatrix() const { return lightViewProjectionMatrix_; }

		/// @brief シャドウマップSRVのGPUディスクリプタハンドルを取得する
		/// @return GPUディスクリプタハンドル
		D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const;

		/// @brief シャドウマップDSVのCPUディスクリプタハンドルを取得する
		/// @return CPUディスクリプタハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUHandle() const;

		/// @brief シャドウマップの幅を取得する
		/// @return シャドウマップの幅
		uint32_t GetWidth() const { return kShadowMapWidth; }

		/// @brief シャドウマップの高さを取得する
		/// @return シャドウマップの高さ
		uint32_t GetHeight() const { return kShadowMapHeight; }

	private:
		MadoEngine::Core::DepthStencilBuffer depthBuffer_;
		ViewportScissor viewportScissor_;
		Matrix4x4 lightViewMatrix_ = {};
		Matrix4x4 lightProjectionMatrix_ = {};
		Matrix4x4 lightViewProjectionMatrix_ = {};
		bool isInitialized_ = false;
	};

} // namespace MadoEngine::Render
