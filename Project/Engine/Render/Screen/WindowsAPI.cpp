#include "WindowsAPI.h"
#include <gdiplus.h>
#include "Utility/Logger/Logger.h"
#include "Input/InputManager.h"
#pragma comment(lib, "gdiplus.lib")

#ifdef USE_IMGUI
// imgui_impl_win32.cpp が定義するウィンドウメッセージハンドラの前方宣言
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

namespace MadoEngine::Screen {

	// ウィンドウプロシージャ
	LRESULT CALLBACK WindowsAPI::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

#ifdef USE_IMGUI
		// ImGuiにメッセージを転送し、ImGuiが処理した場合はここで終了する
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
			return true;
		}
#endif // USE_IMGUI

		// ウィンドウプロシージャ内でWindowsAPIインスタンスにアクセスするための方法
		WindowsAPI* api = reinterpret_cast<WindowsAPI*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		// ウィンドウが閉じられたときの処理
		switch (msg) {
		case WM_DESTROY:
			Logger::Output("ウィンドウの閉じるボタンが押されました", Logger::Level::Engine);
			if (api) {
				api->isPushCloseBottom_ = true;
			}
			PostQuitMessage(0);
			return 0;

		case WM_SIZING:
			// ウィンドウのリサイズ時にアスペクト比を維持
			if (api && api->desc_.isResizable && !api->isFullscreen_ && api->aspectRatio_ > 0.0f) {
				RECT* rect = reinterpret_cast<RECT*>(lparam);
				int width = rect->right - rect->left;
				int height = rect->bottom - rect->top;

				// クライアント領域のサイズを取得するため、ウィンドウ枠のサイズを計算
				RECT clientRect = {};
				RECT windowRect = {};
				GetClientRect(hwnd, &clientRect);
				GetWindowRect(hwnd, &windowRect);
				int frameWidth = (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
				int frameHeight = (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);

				// クライアント領域のサイズ
				int clientWidth = width - frameWidth;
				int clientHeight = height - frameHeight;

				// リサイズの方向に応じてアスペクト比を維持
				switch (wparam) {
				case WMSZ_LEFT:
				case WMSZ_RIGHT:
					// 幅ベースで高さを調整
					clientHeight = static_cast<int>(clientWidth / api->aspectRatio_);
					rect->bottom = rect->top + clientHeight + frameHeight;
					break;

				case WMSZ_TOP:
				case WMSZ_BOTTOM:
					// 高さベースで幅を調整
					clientWidth = static_cast<int>(clientHeight * api->aspectRatio_);
					rect->right = rect->left + clientWidth + frameWidth;
					break;

				case WMSZ_TOPLEFT:
				case WMSZ_TOPRIGHT:
				case WMSZ_BOTTOMLEFT:
				case WMSZ_BOTTOMRIGHT:
					// 角のドラッグの場合、幅ベースで高さを調整
					clientHeight = static_cast<int>(clientWidth / api->aspectRatio_);
					if (wparam == WMSZ_TOPLEFT || wparam == WMSZ_TOPRIGHT) {
						rect->top = rect->bottom - clientHeight - frameHeight;
					} else {
						rect->bottom = rect->top + clientHeight + frameHeight;
					}
					break;
				}

				return TRUE;
			}
			break;
		}

		if (api && api->desc_.wndProc) {
			return api->desc_.wndProc(hwnd, msg, wparam, lparam);
		}

			return DefWindowProc(hwnd, msg, wparam, lparam);
		}

		// アイコンをファイルから読み込む
		HICON WindowsAPI::LoadIconFromFile(const std::string& filePath) {
			if (filePath.empty()) {
				Logger::Output("アイコンファイルが指定されていません。デフォルトアイコンを使用します", Logger::Level::Engine);
				return LoadIcon(nullptr, IDI_APPLICATION);
			}

			Logger::Output("アイコンファイルを読み込んでいます: " + filePath, Logger::Level::Engine	);

			std::wstring wPath(filePath.begin(), filePath.end());

			Gdiplus::GdiplusStartupInput gdiplusStartupInput;
			ULONG_PTR gdiplusToken;
			Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

			Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(wPath.c_str());
			if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok) {

				Logger::Output("アイコンファイルの読み込みに失敗しました: " + filePath + "。デフォルトアイコンを使用します", Logger::Level::Warning);

				if (bitmap) delete bitmap;
				Gdiplus::GdiplusShutdown(gdiplusToken);
				return LoadIcon(nullptr, IDI_APPLICATION);
			}

			HICON hIcon = nullptr;
			bitmap->GetHICON(&hIcon);

			delete bitmap;
			Gdiplus::GdiplusShutdown(gdiplusToken);

			if (hIcon) {
				Logger::Output("アイコンの読み込みに成功しました", Logger::Level::Engine);
			} else {
				Logger::Output("ビットマップからHICONの取得に失敗しました。デフォルトアイコンを使用します", Logger::Level::Warning);
			}

			return hIcon ? hIcon : LoadIcon(nullptr, IDI_APPLICATION);
		}

		// ウィンドウを初期化する
		void WindowsAPI::Initialize(WindowDesc& desc, HINSTANCE hInstance) {

		Logger::Output("WindowsAPIを初期化しています", Logger::Level::Engine);
		Logger::Output("ウィンドウタイトル: " + desc.title, Logger::Level::Engine);
		Logger::Output("ウィンドウサイズ: " + std::to_string(desc.width) + "x" + std::to_string(desc.height), Logger::Level::Engine);

		desc_ = desc;

		// アスペクト比を計算して保存（リサイズ時に使用）
		if (desc_.height > 0) {
			aspectRatio_ = static_cast<float>(desc_.width) / static_cast<float>(desc_.height);
			Logger::Output("アスペクト比: " + std::to_string(aspectRatio_), Logger::Level::Engine);
		}

		wndClass_ = {};
		wndClass_.lpfnWndProc = WindowProc;
		wndClass_.lpszClassName = L"MadoEngineWindowClass";
		wndClass_.hInstance = hInstance;
		wndClass_.hCursor = LoadCursor(nullptr, IDC_ARROW);

		hIcon_ = LoadIconFromFile(desc_.iconPath);
		wndClass_.hIcon = hIcon_;

		Logger::Output("ウィンドウクラスを登録しています", Logger::Level::Engine);

		RegisterClass(&wndClass_);

		// ウィンドウスタイルの設定（サイズ変更可否に基づく）
		DWORD windowStyle = WS_OVERLAPPEDWINDOW;
		if (!desc_.isResizable) {
			// サイズ変更不可の場合、WS_THICKFRAMEとWS_MAXIMIZEBOXを除外
			windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
			Logger::Output("ウィンドウサイズ変更を無効化しました", Logger::Level::Engine);
		} else {
			Logger::Output("ウィンドウサイズ変更を有効化しました", Logger::Level::Engine);
		}

		RECT wrc = { 0, 0, desc_.width, desc_.height };
		AdjustWindowRect(&wrc, windowStyle, false);

		std::wstring wTitle(desc_.title.begin(), desc_.title.end());

		// CreateWindowの呼び出し
		hWnd_ = CreateWindow(
			wndClass_.lpszClassName,
			wTitle.c_str(),
			windowStyle,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			wrc.right - wrc.left,
			wrc.bottom - wrc.top,
			nullptr,
			nullptr,
			hInstance,
			nullptr);

		SetWindowLongPtr(hWnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

		if (hWnd_) {
			Logger::Output("ウィンドウの作成に成功しました", Logger::Level::Engine);
		} else {
			Logger::Output("ウィンドウの作成に失敗しました", Logger::Level::Error);
		}

		ShowWindow(hWnd_, SW_SHOW);

		Logger::Output("WindowsAPIの初期化が完了しました", Logger::Level::Engine);
	}

	// メッセージを処理する。アプリを継続する場合はtrue、終了する場合はfalseを返す
	bool WindowsAPI::ProcessMessage() {
		MSG msg{};
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				Logger::Output("WM_QUITメッセージを受信しました。アプリケーションを終了します", Logger::Level::Engine);
				return false;
			}
		}
		return true;
	}

	// フルスクリーンモードを切り替える
	void WindowsAPI::ToggleFullscreen() {
		if (!desc_.isResizable) {
			Logger::Output("ウィンドウサイズ変更が無効のため、フルスクリーンに切り替えられません", Logger::Level::Warning);
			return;
		}

		if (isFullscreen_) {
			// フルスクリーンからウィンドウモードに戻す
			Logger::Output("ウィンドウモードに切り替えています", Logger::Level::Engine);

			// ウィンドウスタイルを元に戻す
			SetWindowLong(hWnd_, GWL_STYLE, windowedStyle_);

			// ウィンドウの位置とサイズを復元
			SetWindowPos(
				hWnd_,
				HWND_NOTOPMOST,
				windowedRect_.left,
				windowedRect_.top,
				windowedRect_.right - windowedRect_.left,
				windowedRect_.bottom - windowedRect_.top,
				SWP_FRAMECHANGED | SWP_SHOWWINDOW
			);

			isFullscreen_ = false;
			Logger::Output("ウィンドウモードに切り替えました", Logger::Level::Engine);
		} else {
			// ウィンドウモードからフルスクリーンにする
			Logger::Output("フルスクリーンモードに切り替えています", Logger::Level::Engine);

			// 現在のウィンドウスタイルと位置を保存
			windowedStyle_ = GetWindowLong(hWnd_, GWL_STYLE);
			GetWindowRect(hWnd_, &windowedRect_);

			// フルスクリーン用のウィンドウスタイルに変更（枠なし）
			SetWindowLong(hWnd_, GWL_STYLE, WS_POPUP | WS_VISIBLE);

			// モニターのサイズを取得
			HMONITOR hMonitor = MonitorFromWindow(hWnd_, MONITOR_DEFAULTTOPRIMARY);
			MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
			GetMonitorInfo(hMonitor, &monitorInfo);

			// ウィンドウをモニター全体に広げる
			SetWindowPos(
				hWnd_,
				HWND_TOPMOST,
				monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_SHOWWINDOW
			);

			isFullscreen_ = true;
			Logger::Output("フルスクリーンモードに切り替えました", Logger::Level::Engine);
		}
	}

	// 入力処理（フルスクリーン切り替えなど）
	void WindowsAPI::ProcessInput() {
		auto* keyboard = MadoEngine::InputManager::GetInstance()->GetKeybord();
		if (keyboard) {
			// ALT+Enterでフルスクリーン切り替え
			bool altPressed = keyboard->IsPress(DIK_LMENU) || keyboard->IsPress(DIK_RMENU);
			bool enterTriggered = keyboard->IsTrigger(DIK_RETURN);

			// F11キーでフルスクリーン切り替え
			bool f11Triggered = keyboard->IsTrigger(DIK_F11);

			if ((altPressed && enterTriggered) || f11Triggered) {
				ToggleFullscreen();
			}
		}
	}
}
