#ifdef USE_IMGUI
#include "ImGuiManager.h"
#include "Core/DxDevice/DxDevice.h"
#include "Core/Command/Command.h"
#include "Core/View/SRVManager.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include<filesystem>

namespace MadoEngine {

	void ImGuiManager::SrvAllocCallback(ImGui_ImplDX12_InitInfo* info,
		D3D12_CPU_DESCRIPTOR_HANDLE* outCpu,
		D3D12_GPU_DESCRIPTOR_HANDLE* outGpu) {
		auto* mgr = static_cast<ImGuiManager*>(info->UserData);
		uint32_t index = mgr->srvManager_->Allocate();
		*outCpu = mgr->srvManager_->GetCPUHandle(index);
		*outGpu = mgr->srvManager_->GetGPUHandle(index);
		mgr->allocatedSrvIndices_.push_back(index);
	}

	void ImGuiManager::SrvFreeCallback(ImGui_ImplDX12_InitInfo* info,
		D3D12_CPU_DESCRIPTOR_HANDLE cpu,
		D3D12_GPU_DESCRIPTOR_HANDLE) {
		auto* mgr = static_cast<ImGuiManager*>(info->UserData);
		for (size_t i = 0; i < mgr->allocatedSrvIndices_.size(); ++i) {
			if (mgr->srvManager_->GetCPUHandle(mgr->allocatedSrvIndices_[i]).ptr == cpu.ptr) {
				mgr->srvManager_->Free(mgr->allocatedSrvIndices_[i]);
				mgr->allocatedSrvIndices_.erase(mgr->allocatedSrvIndices_.begin() + i);
				return;
			}
		}
	}

	void ImGuiManager::Initialize(Core::DxDevice* device, Core::CommandManager* commandManager,
		Core::SRVManager* srvManager, HWND hwnd, uint32_t bufferCount) {
		assert(device && commandManager && srvManager && hwnd);
		srvManager_ = srvManager;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGui_ImplWin32_Init(hwnd);

		// ドッキング機能・マルチビューポート（ウィンドウ外ドラッグ）を有効化
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		// ドッキング自由度の向上
		io.ConfigDockingWithShift        = false; // Shiftキーなしで任意の場所にドッキング可能
		io.ConfigDockingAlwaysTabBar     = true;  // ドックノードに常にタブバーを表示
		io.ConfigWindowsMoveFromTitleBarOnly = false; // タイトルバー以外の領域からもウィンドウを移動可能

		ImGui_ImplDX12_InitInfo initInfo;
		initInfo.Device = device->GetDevice();
		initInfo.CommandQueue = commandManager->GetCommandQueue();
		initInfo.NumFramesInFlight = static_cast<int>(bufferCount);
		initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
		initInfo.SrvDescriptorHeap = srvManager_->GetDescriptorHeap();
		initInfo.SrvDescriptorAllocFn = &ImGuiManager::SrvAllocCallback;
		initInfo.SrvDescriptorFreeFn = &ImGuiManager::SrvFreeCallback;
		initInfo.UserData = this;
		ImGui_ImplDX12_Init(&initInfo);

		// 日本語フォントの設定
		const char* fontPath = "C:/Windows/Fonts/YuGothB.ttc";

		if (std::filesystem::exists(fontPath)) {
			ImFontConfig config;
			config.SizePixels = 14.0f;

			ImFont* font = io.Fonts->AddFontFromFileTTF(
				fontPath,
				config.SizePixels,
				&config,
				io.Fonts->GetGlyphRangesJapanese());

			if (font) {
				io.FontDefault = font;
				io.FontGlobalScale = 1.0f;
				io.Fonts->Build();
			}
		}

		Logger::Output("[Engine] ImGuiManager: 初期化完了", Logger::Level::Engine);
	}

	void ImGuiManager::Begin() {
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiManager::DrawEditorLayout(D3D12_GPU_DESCRIPTOR_HANDLE gameViewSRV) {
		// 全画面 DockSpace ウィンドウの設定
		ImGuiWindowFlags dockFlags =
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoBackground;

		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(vp->Pos);
		ImGui::SetNextWindowSize(vp->Size);
		ImGui::SetNextWindowViewport(vp->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpaceWindow", nullptr, dockFlags);
		ImGui::PopStyleVar(3);
		ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
		ImGui::End();

		// Game View ウィンドウにオフスクリーンテクスチャを表示（16:9 固定）
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Game View");
		ImVec2 avail = ImGui::GetContentRegionAvail();
		constexpr float kAspect = 16.0f / 9.0f;
		ImVec2 imageSize;
		if (avail.x / avail.y > kAspect) {
			// 横が余る → 高さ基準
			imageSize.y = avail.y;
			imageSize.x = avail.y * kAspect;
		} else {
			// 縦が余る → 幅基準
			imageSize.x = avail.x;
			imageSize.y = avail.x / kAspect;
		}
		// 余白をセンタリング
		ImVec2 offset((avail.x - imageSize.x) * 0.5f, (avail.y - imageSize.y) * 0.5f);
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + offset.x, ImGui::GetCursorPosY() + offset.y));
		ImGui::Image(static_cast<ImTextureID>(gameViewSRV.ptr), imageSize);
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void ImGuiManager::End(ID3D12GraphicsCommandList* commandList) {
		assert(commandList);
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

		// マルチビューポート: EXE外のウィンドウを更新・描画
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault(nullptr, commandList);
		}
	}

	void ImGuiManager::Finalize() {
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		allocatedSrvIndices_.clear();
		Logger::Output("[Engine] ImGuiManager: 終了処理完了", Logger::Level::Engine);
	}

} // namespace MadoEngine

#endif // USE_IMGUI
