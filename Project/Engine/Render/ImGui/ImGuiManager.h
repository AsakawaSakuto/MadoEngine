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
		/// @brief ImGuiを初期化
		/// @param device DxDeviceのポインタ
		/// @param commandManager CommandManagerのポインタ（CommandQueueの取得に使用）
		/// @param srvManager SRVManagerのポインタ（SRVコールバック経由でフォントSRVを管理）
		/// @param hwnd 描画対象のウィンドウハンドル
		/// @param bufferCount スワップチェーンのフレームバッファ数（デフォルト: 2）
		void Initialize(Core::DxDevice* device, Core::CommandManager* commandManager,
						Core::SRVManager* srvManager, HWND hwnd, uint32_t bufferCount = 2);

		/// @brief フレーム開始処理（NewFrame）
	/// 呼び出しタイミングはPreDrawの先頭
		void Begin();

		/// @brief フレーム終了処理（Render + RenderDrawData）
	/// 呼び出しタイミングはResourceBarrierでPRESENTへ遷移させる直前
		/// @param commandList 描画コマンドを積むコマンドリスト
		void End(ID3D12GraphicsCommandList* commandList);

		/// @brief エディタ用レイアウト（DockSpace + Game View）を描画
		/// @param gameViewSRV ゲーム画面オフスクリーンテクスチャのSRV GPUハンドル
		/// @param topOffset Main Menu下に確保する共通ツールバーの高さ
		/// @param showGameView 可変サイズGame Viewを表示する場合はtrue
		/// @param showFixedGameView 固定サイズGame Viewを表示する場合はtrue
		void DrawEditorLayout(
			D3D12_GPU_DESCRIPTOR_HANDLE gameViewSRV,
			float topOffset,
			bool showGameView,
			bool showFixedGameView
		);

		/// @brief 現在のEditorレイアウトを保存
		/// @return 保存に成功した場合はtrue
		bool SaveEditorLayout() const;

		/// @brief Editorレイアウトを既定配置へ初期化
		void ResetEditorLayout();

		/// @brief ImGuiのスタイルカラー編集ウィンドウを描画
		void DrawStyleColorEditorUI();

		/// @brief ImGuiのスタイルカラーをJsonへ保存
		/// @return 保存できた場合はtrue
		bool SaveStyleColors() const;

		/// @brief ImGuiのスタイルカラーをJsonから読み込み
		/// @return 読み込めた場合はtrue
		bool LoadStyleColors();

		/// @brief ImGuiを終了し全リソースを解放
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

		/// @brief 既定のImGuiスタイルカラーを適用
		void ApplyDefaultStyleColors();

		/// @brief 既定のDockSpace配置を構築
		/// @param dockSpaceId 構築対象のDockSpace ID
		/// @param dockSpaceSize DockSpace全体のサイズ
		void BuildDefaultEditorLayout(ImGuiID dockSpaceId, const ImVec2& dockSpaceSize);

		Core::SRVManager* srvManager_ = nullptr;
		std::vector<uint32_t> allocatedSrvIndices_; // ImGuiが確保したSRVスロットの追跡用
		bool buildDefaultLayoutRequested_ = false;
	};

} // namespace MadoEngine

#endif // USE_IMGUI
