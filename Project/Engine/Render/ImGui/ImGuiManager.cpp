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

		// ドッキング機能を有効化
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

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

	void ImGuiManager::End(ID3D12GraphicsCommandList* commandList) {
		assert(commandList);
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
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
