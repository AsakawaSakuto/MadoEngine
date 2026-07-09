#ifdef USE_IMGUI
#include "ImGuiManager.h"
#include "Core/DxDevice/DxDevice.h"
#include "Core/Command/Command.h"
#include "Core/View/SRVManager.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include <cstring>
#include<filesystem>

namespace {

	struct ImGuiStyleColorItem {
		ImGuiCol index;
		const char* jsonKey;
		const char* category;
		const char* label;
	};

	constexpr const char* kStyleColorJsonPath = "Assets/Json/ImGuiStyleColors.json";

	const ImGuiStyleColorItem kEditableStyleColors[] = {
		{ ImGuiCol_Text, "Text", "基本", "通常文字" },
		{ ImGuiCol_TextDisabled, "TextDisabled", "基本", "無効文字" },
		{ ImGuiCol_TextLink, "TextLink", "基本", "リンク文字" },
		{ ImGuiCol_TextSelectedBg, "TextSelectedBg", "基本", "選択文字背景" },
		{ ImGuiCol_WindowBg, "WindowBg", "背景", "ウィンドウ背景" },
		{ ImGuiCol_ChildBg, "ChildBg", "背景", "子ウィンドウ背景" },
		{ ImGuiCol_PopupBg, "PopupBg", "背景", "ポップアップ背景" },
		{ ImGuiCol_Border, "Border", "枠線", "枠線" },
		{ ImGuiCol_BorderShadow, "BorderShadow", "枠線", "枠線の影" },
		{ ImGuiCol_Separator, "Separator", "枠線", "区切り線" },
		{ ImGuiCol_SeparatorHovered, "SeparatorHovered", "枠線", "区切り線ホバー" },
		{ ImGuiCol_SeparatorActive, "SeparatorActive", "枠線", "区切り線アクティブ" },
		{ ImGuiCol_FrameBg, "FrameBg", "入力", "入力背景" },
		{ ImGuiCol_FrameBgHovered, "FrameBgHovered", "入力", "入力背景ホバー" },
		{ ImGuiCol_FrameBgActive, "FrameBgActive", "入力", "入力背景アクティブ" },
		{ ImGuiCol_InputTextCursor, "InputTextCursor", "入力", "テキストカーソル" },
		{ ImGuiCol_CheckMark, "CheckMark", "入力", "チェックマーク" },
		{ ImGuiCol_SliderGrab, "SliderGrab", "入力", "スライダーつまみ" },
		{ ImGuiCol_SliderGrabActive, "SliderGrabActive", "入力", "スライダーつまみアクティブ" },
		{ ImGuiCol_Button, "Button", "ボタン", "ボタン" },
		{ ImGuiCol_ButtonHovered, "ButtonHovered", "ボタン", "ボタンホバー" },
		{ ImGuiCol_ButtonActive, "ButtonActive", "ボタン", "ボタンアクティブ" },
		{ ImGuiCol_Header, "Header", "ヘッダー", "ヘッダー" },
		{ ImGuiCol_HeaderHovered, "HeaderHovered", "ヘッダー", "ヘッダーホバー" },
		{ ImGuiCol_HeaderActive, "HeaderActive", "ヘッダー", "ヘッダーアクティブ" },
		{ ImGuiCol_TitleBg, "TitleBg", "タイトル", "タイトル背景" },
		{ ImGuiCol_TitleBgActive, "TitleBgActive", "タイトル", "タイトル背景アクティブ" },
		{ ImGuiCol_TitleBgCollapsed, "TitleBgCollapsed", "タイトル", "タイトル背景折りたたみ" },
		{ ImGuiCol_MenuBarBg, "MenuBarBg", "タイトル", "メニューバー背景" },
		{ ImGuiCol_Tab, "Tab", "タブ", "タブ" },
		{ ImGuiCol_TabHovered, "TabHovered", "タブ", "タブホバー" },
		{ ImGuiCol_TabSelected, "TabSelected", "タブ", "選択タブ" },
		{ ImGuiCol_TabSelectedOverline, "TabSelectedOverline", "タブ", "選択タブ上線" },
		{ ImGuiCol_TabDimmed, "TabDimmed", "タブ", "非フォーカスタブ" },
		{ ImGuiCol_TabDimmedSelected, "TabDimmedSelected", "タブ", "非フォーカス選択タブ" },
		{ ImGuiCol_TabDimmedSelectedOverline, "TabDimmedSelectedOverline", "タブ", "非フォーカス選択タブ上線" },
		{ ImGuiCol_ScrollbarBg, "ScrollbarBg", "スクロールバー", "スクロールバー背景" },
		{ ImGuiCol_ScrollbarGrab, "ScrollbarGrab", "スクロールバー", "スクロールバーつまみ" },
		{ ImGuiCol_ScrollbarGrabHovered, "ScrollbarGrabHovered", "スクロールバー", "スクロールバーつまみホバー" },
		{ ImGuiCol_ScrollbarGrabActive, "ScrollbarGrabActive", "スクロールバー", "スクロールバーつまみアクティブ" },
		{ ImGuiCol_ResizeGrip, "ResizeGrip", "リサイズ", "リサイズつまみ" },
		{ ImGuiCol_ResizeGripHovered, "ResizeGripHovered", "リサイズ", "リサイズつまみホバー" },
		{ ImGuiCol_ResizeGripActive, "ResizeGripActive", "リサイズ", "リサイズつまみアクティブ" },
		{ ImGuiCol_DockingPreview, "DockingPreview", "ドッキング", "ドッキングプレビュー" },
		{ ImGuiCol_DockingEmptyBg, "DockingEmptyBg", "ドッキング", "空ドック背景" },
		{ ImGuiCol_TableHeaderBg, "TableHeaderBg", "テーブル", "テーブルヘッダー背景" },
		{ ImGuiCol_TableBorderStrong, "TableBorderStrong", "テーブル", "テーブル外枠線" },
		{ ImGuiCol_TableBorderLight, "TableBorderLight", "テーブル", "テーブル内枠線" },
		{ ImGuiCol_TableRowBg, "TableRowBg", "テーブル", "テーブル行背景" },
		{ ImGuiCol_TableRowBgAlt, "TableRowBgAlt", "テーブル", "テーブル交互行背景" },
		{ ImGuiCol_PlotLines, "PlotLines", "プロット", "プロット線" },
		{ ImGuiCol_PlotLinesHovered, "PlotLinesHovered", "プロット", "プロット線ホバー" },
		{ ImGuiCol_PlotHistogram, "PlotHistogram", "プロット", "ヒストグラム" },
		{ ImGuiCol_PlotHistogramHovered, "PlotHistogramHovered", "プロット", "ヒストグラムホバー" },
		{ ImGuiCol_TreeLines, "TreeLines", "その他", "ツリー線" },
		{ ImGuiCol_DragDropTarget, "DragDropTarget", "その他", "ドラッグ先枠" },
		{ ImGuiCol_DragDropTargetBg, "DragDropTargetBg", "その他", "ドラッグ先背景" },
		{ ImGuiCol_UnsavedMarker, "UnsavedMarker", "その他", "未保存マーカー" },
		{ ImGuiCol_NavCursor, "NavCursor", "その他", "ナビカーソル" },
		{ ImGuiCol_NavWindowingHighlight, "NavWindowingHighlight", "その他", "ナビウィンドウ強調" },
		{ ImGuiCol_NavWindowingDimBg, "NavWindowingDimBg", "その他", "ナビウィンドウ暗転背景" },
		{ ImGuiCol_ModalWindowDimBg, "ModalWindowDimBg", "その他", "モーダル暗転背景" },
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
		ApplyDefaultStyleColors();
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
		ImGui::StyleColorsDark();
		ImVec4* colors = ImGui::GetStyle().Colors;

		colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.0f); // 背景
		colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.12f, 0.16f, 1.0f); // 非アクティブTitleBar
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.22f, 0.30f, 1.0f); // アクティブTitleBar
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.14f, 0.18f, 1.0f); // MenuBar
		colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.15f, 0.20f, 1.0f); // Tab
		colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.32f, 0.42f, 1.0f);
		colors[ImGuiCol_TabSelected] = ImVec4(0.18f, 0.24f, 0.34f, 1.0f);
		colors[ImGuiCol_Header] = ImVec4(0.16f, 0.20f, 0.28f, 1.0f); // Selectable等
		colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.12f, 0.16f, 1.0f); // Input/Slider背景
		colors[ImGuiCol_Button] = ImVec4(0.18f, 0.22f, 0.30f, 1.0f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.32f, 0.42f, 1.0f);
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
			auto oldTabActiveIt = colorsJson->end();
			if (std::strcmp(item.jsonKey, "TabSelected") == 0) {
				oldTabActiveIt = colorsJson->find("TabActive");
			}
			if (it == colorsJson->end() && oldTabActiveIt == colorsJson->end()) {
				continue;
			}

			const nlohmann::json& colorJson = it != colorsJson->end() ? *it : *oldTabActiveIt;
			ImVec4 color;
			if (TryReadJsonColor(colorJson, color)) {
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

		const char* currentCategory = nullptr;
		bool isCategoryOpen = false;
		for (const ImGuiStyleColorItem& item : kEditableStyleColors) {
			if (currentCategory == nullptr || std::strcmp(currentCategory, item.category) != 0) {
				if (isCategoryOpen) {
					ImGui::TreePop();
				}

				currentCategory = item.category;
				isCategoryOpen = ImGui::TreeNodeEx(currentCategory, ImGuiTreeNodeFlags_DefaultOpen);
			}

			if (isCategoryOpen) {
				ImGui::PushID(item.jsonKey);
				ImGui::ColorEdit4(item.label, &colors[item.index].x, ImGuiColorEditFlags_AlphaBar);
				ImGui::PopID();
			}
		}

		if (isCategoryOpen) {
			ImGui::TreePop();
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
