#ifdef USE_IMGUI
#include "ImGuiManager.h"
#include "Core/DxDevice/DxDevice.h"
#include "Core/Command/Command.h"
#include "Core/View/SRVManager.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include<filesystem>

namespace {

	struct ImGuiStyleColorItem {
		ImGuiCol index;
		const char* jsonKey;
		const char* label;
		ImVec4 defaultColor;
	};

	constexpr const char* kStyleColorJsonPath = "Assets/Json/ImGuiStyleColors.json";

	const ImGuiStyleColorItem kEditableStyleColors[] = {
		{ ImGuiCol_WindowBg, "WindowBg", "背景 WindowBg", ImVec4(0.08f, 0.09f, 0.11f, 1.0f) },
		{ ImGuiCol_TitleBg, "TitleBg", "非アクティブTitleBar", ImVec4(0.10f, 0.12f, 0.16f, 1.0f) },
		{ ImGuiCol_TitleBgActive, "TitleBgActive", "アクティブTitleBar", ImVec4(0.18f, 0.22f, 0.30f, 1.0f) },
		{ ImGuiCol_MenuBarBg, "MenuBarBg", "MenuBar", ImVec4(0.12f, 0.14f, 0.18f, 1.0f) },
		{ ImGuiCol_Tab, "Tab", "Tab", ImVec4(0.12f, 0.15f, 0.20f, 1.0f) },
		{ ImGuiCol_TabHovered, "TabHovered", "TabHovered", ImVec4(0.25f, 0.32f, 0.42f, 1.0f) },
		{ ImGuiCol_TabActive, "TabActive", "TabActive", ImVec4(0.18f, 0.24f, 0.34f, 1.0f) },
		{ ImGuiCol_Header, "Header", "Header Selectable等", ImVec4(0.16f, 0.20f, 0.28f, 1.0f) },
		{ ImGuiCol_FrameBg, "FrameBg", "FrameBg Input/Slider背景", ImVec4(0.10f, 0.12f, 0.16f, 1.0f) },
		{ ImGuiCol_Button, "Button", "Button", ImVec4(0.18f, 0.22f, 0.30f, 1.0f) },
		{ ImGuiCol_ButtonHovered, "ButtonHovered", "ButtonHovered", ImVec4(0.25f, 0.32f, 0.42f, 1.0f) },
	};

	/// @brief ImGuiカラーをJson配列へ変換します。
	/// @param color 変換するImGuiカラー
	/// @return RGBA順のJson配列
	nlohmann::json ToJsonColor(const ImVec4& color) {
		return nlohmann::json::array({ color.x, color.y, color.z, color.w });
	}

	/// @brief Json配列からImGuiカラーを読み取ります。
	/// @param json 読み取り元のJson値
	/// @param outColor 読み取ったカラーの出力先
	/// @return 読み取りに成功した場合はtrue
	bool TryReadJsonColor(const nlohmann::json& json, ImVec4& outColor) {
		if (!json.is_array() || json.size() < 4) {
			return false;
		}

		for (size_t i = 0; i < 4; ++i) {
			if (!json[i].is_number()) {
				return false;
			}
		}

		outColor = ImVec4(
			json[0].get<float>(),
			json[1].get<float>(),
			json[2].get<float>(),
			json[3].get<float>()
		);
		return true;
	}

} // namespace

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

		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;

		colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.0f); // 背景
		colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.12f, 0.16f, 1.0f); // 非アクティブTitleBar
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.22f, 0.30f, 1.0f); // アクティブTitleBar
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.14f, 0.18f, 1.0f); // MenuBar
		colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.15f, 0.20f, 1.0f); // Tab
		colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.32f, 0.42f, 1.0f);
		colors[ImGuiCol_TabActive] = ImVec4(0.18f, 0.24f, 0.34f, 1.0f);
		colors[ImGuiCol_Header] = ImVec4(0.16f, 0.20f, 0.28f, 1.0f); // Selectable等
		colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.12f, 0.16f, 1.0f); // Input/Slider背景
		colors[ImGuiCol_Button] = ImVec4(0.18f, 0.22f, 0.30f, 1.0f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.32f, 0.42f, 1.0f);
		LoadStyleColors();

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
		initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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

	/// @brief 既定のImGuiスタイルカラーを適用します。
	void ImGuiManager::ApplyDefaultStyleColors() {
		ImVec4* colors = ImGui::GetStyle().Colors;

		for (const ImGuiStyleColorItem& item : kEditableStyleColors) {
			colors[item.index] = item.defaultColor;
		}
	}

	/// @brief ImGuiのスタイルカラーをJsonへ保存します。
	/// @return 保存できた場合はtrueを返します。
	bool ImGuiManager::SaveStyleColors() const {
		nlohmann::json root;
		nlohmann::json colorsJson = nlohmann::json::object();
		const ImVec4* colors = ImGui::GetStyle().Colors;

		for (const ImGuiStyleColorItem& item : kEditableStyleColors) {
			colorsJson[item.jsonKey] = ToJsonColor(colors[item.index]);
		}

		root["colors"] = colorsJson;
		return Json::JsonFile::Save(kStyleColorJsonPath, root, 4, true);
	}

	/// @brief ImGuiのスタイルカラーをJsonから読み込みます。
	/// @return 読み込めた場合はtrueを返します。
	bool ImGuiManager::LoadStyleColors() {
		if (!Json::JsonFile::Exists(kStyleColorJsonPath)) {
			return false;
		}

		nlohmann::json root;
		if (!Json::JsonFile::Load(kStyleColorJsonPath, root)) {
			return false;
		}

		const nlohmann::json* colorsJson = &root;
		if (root.contains("colors") && root["colors"].is_object()) {
			colorsJson = &root["colors"];
		}

		bool loaded = false;
		ImVec4* colors = ImGui::GetStyle().Colors;
		for (const ImGuiStyleColorItem& item : kEditableStyleColors) {
			const auto it = colorsJson->find(item.jsonKey);
			if (it == colorsJson->end()) {
				continue;
			}

			ImVec4 color;
			if (TryReadJsonColor(*it, color)) {
				colors[item.index] = color;
				loaded = true;
			}
		}

		return loaded;
	}

	/// @brief ImGuiのスタイルカラー編集ウィンドウを描画します。
	void ImGuiManager::DrawStyleColorEditorUI() {
		ImGui::Begin("ImGui Style Color");

		if (ImGui::Button("保存")) {
			SaveStyleColors();
		}
		ImGui::SameLine();
		if (ImGui::Button("読み込み")) {
			LoadStyleColors();
		}
		ImGui::SameLine();
		if (ImGui::Button("初期色に戻す")) {
			ApplyDefaultStyleColors();
		}

		ImGui::Text("保存先: %s", kStyleColorJsonPath);
		ImGui::Separator();
		ImVec4* colors = ImGui::GetStyle().Colors;
		for (const ImGuiStyleColorItem& item : kEditableStyleColors) {
			ImGui::ColorEdit4(item.label, &colors[item.index].x, ImGuiColorEditFlags_AlphaBar);
		}

		ImGui::End();
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
