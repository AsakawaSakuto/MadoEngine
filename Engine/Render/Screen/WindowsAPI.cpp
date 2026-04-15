#include "WindowsAPI.h"
#include "WindowsAPI.h"
#include <gdiplus.h>
#include "../../Utility/Logger/Logger.h"
#pragma comment(lib, "gdiplus.lib")

namespace MadoEngine {

	// ウィンドウプロシージャ
	LRESULT CALLBACK WindowsAPI::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

		// ウィンドウプロシージャ内でWindowsAPIインスタンスにアクセスするための方法
		WindowsAPI* api = reinterpret_cast<WindowsAPI*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		// ウィンドウが閉じられたときの処理
		switch (msg) {
		case WM_DESTROY:
			Logger::Info("ウィンドウの閉じるボタンが押されました");
			if (api) {
				api->isPushCloseBottom_ = true;
			}
			PostQuitMessage(0);
			return 0;
		}

		if (api && api->desc_.wndProc) {
			return api->desc_.wndProc(hwnd, msg, wparam, lparam);
		}

			return DefWindowProc(hwnd, msg, wparam, lparam);
		}

		// アイコンをファイルから読み込む
		HICON WindowsAPI::LoadIconFromFile(const std::string& filePath) {
			if (filePath.empty()) {
				Logger::Info("アイコンファイルが指定されていません。デフォルトアイコンを使用します");
				return LoadIcon(nullptr, IDI_APPLICATION);
			}

			Logger::Info("アイコンファイルを読み込んでいます: " + filePath);

			std::wstring wPath(filePath.begin(), filePath.end());

			Gdiplus::GdiplusStartupInput gdiplusStartupInput;
			ULONG_PTR gdiplusToken;
			Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

			Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(wPath.c_str());
			if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok) {

				Logger::Warning("アイコンファイルの読み込みに失敗しました: " + filePath + "。デフォルトアイコンを使用します");

				if (bitmap) delete bitmap;
				Gdiplus::GdiplusShutdown(gdiplusToken);
				return LoadIcon(nullptr, IDI_APPLICATION);
			}

			HICON hIcon = nullptr;
			bitmap->GetHICON(&hIcon);

			delete bitmap;
			Gdiplus::GdiplusShutdown(gdiplusToken);

			if (hIcon) {
				Logger::Info("アイコンの読み込みに成功しました");
			} else {
				Logger::Warning("ビットマップからHICONの取得に失敗しました。デフォルトアイコンを使用します");
			}

			return hIcon ? hIcon : LoadIcon(nullptr, IDI_APPLICATION);
		}

		// ウィンドウを初期化する
		void WindowsAPI::Initialize(WindowDesc& desc, HINSTANCE hInstance) {

		Logger::Info("WindowsAPIを初期化しています");
		Logger::Info("ウィンドウタイトル: " + desc.title);
		Logger::Info("ウィンドウサイズ: " + std::to_string(desc.width) + "x" + std::to_string(desc.height));

		desc_ = desc;

		wndClass_ = {};
		wndClass_.lpfnWndProc = WindowProc;
		wndClass_.lpszClassName = L"MadoEngineWindowClass";
		wndClass_.hInstance = hInstance;
		wndClass_.hCursor = LoadCursor(nullptr, IDC_ARROW);

		hIcon_ = LoadIconFromFile(desc_.iconPath);
		wndClass_.hIcon = hIcon_;

		Logger::Info("ウィンドウクラスを登録しています");

		RegisterClass(&wndClass_);

		RECT wrc = { 0, 0, desc_.width, desc_.height };
		AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

		std::wstring wTitle(desc_.title.begin(), desc_.title.end());

		// CreateWindowの呼び出し
		hWnd_ = CreateWindow(
			wndClass_.lpszClassName,
			wTitle.c_str(),
			WS_OVERLAPPEDWINDOW,
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
			Logger::Info("ウィンドウの作成に成功しました");
		} else {
			Logger::Error("ウィンドウの作成に失敗しました");
		}

		ShowWindow(hWnd_, SW_SHOW);

		Logger::Info("WindowsAPIの初期化が完了しました");
	}

	// メッセージを処理する。アプリを継続する場合はtrue、終了する場合はfalseを返す
	bool WindowsAPI::ProcessMessage() {
		MSG msg{};
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				Logger::Info("WM_QUITメッセージを受信しました。アプリケーションを終了します");
				return false;
			}
		}
		return true;
	}

}