#pragma once
#include "Render/Screen/RenderTexture.h"
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace MadoEngine::Core {
	class DxDevice;
	class RTVManager;
	class SRVManager;
}

namespace MadoEngine::Render {

	/// @brief RenderTextureの生成・取得・描画先切り替えを名前付きで管理するクラス
	class RenderTargetManager {
	public:
		/// @brief RenderTargetManagerを初期化する
		/// @param device DxDeviceのポインタ
		/// @param rtvManager RTVManagerのポインタ
		/// @param srvManager SRVManagerのポインタ
		void Initialize(
			MadoEngine::Core::DxDevice* device,
			MadoEngine::Core::RTVManager* rtvManager,
			MadoEngine::Core::SRVManager* srvManager
		);

		/// @brief レンダーターゲット生成時の設定
		struct Desc {
			uint32_t width = 0;
			uint32_t height = 0;
			DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
			std::array<float, 4> clearColor = { 0.1f, 0.25f, 0.5f, 1.0f };
			bool resizeWithWindow = true;
		};

		/// @brief 名前付きレンダーターゲットを作成する
		/// @param name レンダーターゲット名
		/// @param desc 作成設定
		/// @return 作成済み、または既存のRenderTextureポインタ
		RenderTexture* Create(const std::string& name, const Desc& desc);

		/// @brief 名前付きレンダーターゲットを作成する
		/// @param name レンダーターゲット名
		/// @param width 幅
		/// @param height 高さ
		/// @param format フォーマット
		/// @param resizeWithWindow ウィンドウリサイズ時に追従するか
		/// @return 作成済み、または既存のRenderTextureポインタ
		RenderTexture* Create(
			const std::string& name,
			uint32_t width,
			uint32_t height,
			DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM,
			bool resizeWithWindow = true
		);

		/// @brief 名前付きレンダーターゲットが存在するか確認する
		/// @param name レンダーターゲット名
		/// @return 存在する場合true
		bool Contains(const std::string& name) const;

		/// @brief 名前付きレンダーターゲットを取得する
		/// @param name レンダーターゲット名
		/// @return RenderTextureポインタ
		RenderTexture* Get(const std::string& name);

		/// @brief 名前付きレンダーターゲットを取得する
		/// @param name レンダーターゲット名
		/// @return RenderTextureポインタ
		const RenderTexture* Get(const std::string& name) const;

		/// @brief 指定したレンダーターゲットへの描画を開始する
		/// @param name レンダーターゲット名
		/// @param commandList コマンドリスト
		/// @param depthStencilHandle 深度ステンシルビューのCPUハンドル。不要な場合はnullptr
		void Begin(
			const std::string& name,
			ID3D12GraphicsCommandList* commandList,
			const D3D12_CPU_DESCRIPTOR_HANDLE* depthStencilHandle = nullptr
		);

		/// @brief 指定したレンダーターゲットへの描画を開始する
		/// @param name レンダーターゲット名
		/// @param commandList コマンドリスト
		/// @param depthStencilHandle 深度ステンシルビューのCPUハンドル
		void Begin(
			const std::string& name,
			ID3D12GraphicsCommandList* commandList,
			D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle
		);

		/// @brief 指定したレンダーターゲットへの描画を終了する
		/// @param name レンダーターゲット名
		/// @param commandList コマンドリスト
		void End(const std::string& name, ID3D12GraphicsCommandList* commandList);

		/// @brief 指定したレンダーターゲットのGPU SRVハンドルを取得する
		/// @param name レンダーターゲット名
		/// @return GPU SRVハンドル
		D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle(const std::string& name) const;

		/// @brief 指定したレンダーターゲットのCPU SRVハンドルを取得する
		/// @param name レンダーターゲット名
		/// @return CPU SRVハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle(const std::string& name) const;

		/// @brief 指定したレンダーターゲットのCPU RTVハンドルを取得する
		/// @param name レンダーターゲット名
		/// @return CPU RTVハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUHandle(const std::string& name) const;

		/// @brief 指定したレンダーターゲットをリサイズする
		/// @param name レンダーターゲット名
		/// @param width 幅
		/// @param height 高さ
		void Resize(const std::string& name, uint32_t width, uint32_t height);

		/// @brief リサイズ追従設定が有効な全レンダーターゲットをリサイズする
		/// @param width 幅
		/// @param height 高さ
		void ResizeAll(uint32_t width, uint32_t height);

		/// @brief 管理中のレンダーターゲットをすべて解放する
		void Clear();

	private:
		struct Entry {
			Desc desc;
			std::unique_ptr<RenderTexture> texture;
		};

		Entry& GetEntry(const std::string& name);
		const Entry& GetEntry(const std::string& name) const;

		MadoEngine::Core::DxDevice* device_ = nullptr;
		MadoEngine::Core::RTVManager* rtvManager_ = nullptr;
		MadoEngine::Core::SRVManager* srvManager_ = nullptr;
		std::unordered_map<std::string, Entry> renderTargets_;
	};

} // namespace MadoEngine::Render
