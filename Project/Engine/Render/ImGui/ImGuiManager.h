#pragma once
#ifdef USE_IMGUI
#include <Windows.h>
#include <d3d12.h>
#include <cstdint>
#include <vector>
#include "ImGuiHeaders.h"

namespace MadoEngine::Core {
	class DxDevice;
	class CommandManager;
	class SRVManager;
}

namespace MadoEngine {

	/// @brief ImGuiの初期化・破棄・フレーム描画管理を行うクラス
	class ImGuiManager {
	public:
		/// @brief ImGuiを初期化する
		/// @param device DxDeviceのポインタ
		/// @param commandManager CommandManagerのポインタ（CommandQueueの取得に使用）
		/// @param srvManager SRVManagerのポインタ（SRVコールバック経由でフォントSRVを管理）
		/// @param hwnd 描画対象のウィンドウハンドル
		/// @param bufferCount スワップチェーンのフレームバッファ数（デフォルト: 2）
		void Initialize(Core::DxDevice* device, Core::CommandManager* commandManager,
						Core::SRVManager* srvManager, HWND hwnd, uint32_t bufferCount = 2);

		/// @brief フレーム開始処理（NewFrame）
		/// PreDraw の先頭で呼ぶこと
		void Begin();

		/// @brief フレーム終了処理（Render + RenderDrawData）
		/// ResourceBarrier で PRESENT に遷移させる直前に呼ぶこと
		/// @param commandList 描画コマンドを積むコマンドリスト
		void End(ID3D12GraphicsCommandList* commandList);

		/// @brief エディタ用レイアウト（DockSpace + Game View）を描画する
		/// @param gameViewSRV ゲーム画面オフスクリーンテクスチャのSRV GPUハンドル
		void DrawEditorLayout(D3D12_GPU_DESCRIPTOR_HANDLE gameViewSRV);

		/// @brief ImGuiのスタイルカラー編集ウィンドウを描画します。
		void DrawStyleColorEditorUI();

		/// @brief ImGuiのスタイルカラーをJsonへ保存します。
		/// @return 保存できた場合はtrueを返します。
		bool SaveStyleColors() const;

		/// @brief ImGuiのスタイルカラーをJsonから読み込みます。
		/// @return 読み込めた場合はtrueを返します。
		bool LoadStyleColors();

		/// @brief ImGuiを終了し全リソースを解放する
		void Finalize();

	private:
		/// @brief ImGui から呼ばれる SRV アロケーターコールバック
		static void SrvAllocCallback(ImGui_ImplDX12_InitInfo* info,
									D3D12_CPU_DESCRIPTOR_HANDLE* outCpu,
									D3D12_GPU_DESCRIPTOR_HANDLE* outGpu);

		/// @brief ImGui から呼ばれる SRV フリーコールバック
		static void SrvFreeCallback(ImGui_ImplDX12_InitInfo* info,
									D3D12_CPU_DESCRIPTOR_HANDLE cpu,
									D3D12_GPU_DESCRIPTOR_HANDLE gpu);

		/// @brief 既定のImGuiスタイルカラーを適用します。
		void ApplyDefaultStyleColors();

		Core::SRVManager* srvManager_ = nullptr;
		std::vector<uint32_t> allocatedSrvIndices_; // ImGuiが確保したSRVスロットの追跡用
	};

} // namespace MadoEngine

#endif // USE_IMGUI
