#include "WindowsAPI.h"

namespace MadoEngine {

	LRESULT CALLBACK WindowsAPI::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		WindowsAPI* api = reinterpret_cast<WindowsAPI*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		switch (msg) {
		case WM_DESTROY:
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

	void WindowsAPI::Initialize(WindowDesc& desc, HINSTANCE hInstance) {
		desc_ = desc;

		wndClass_ = {};
		wndClass_.lpfnWndProc = WindowProc;
		wndClass_.lpszClassName = L"MadoEngineWindowClass";
		wndClass_.hInstance = hInstance;
		wndClass_.hCursor = LoadCursor(nullptr, IDC_ARROW);
		RegisterClass(&wndClass_);

		RECT wrc = { 0, 0, desc_.width, desc_.height };
		AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

		std::wstring wTitle(desc_.title.begin(), desc_.title.end());

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

		ShowWindow(hWnd_, SW_SHOW);
	}

	bool WindowsAPI::ProcessMessage() {
		MSG msg{};
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				return false;
			}
		}
		return true;
	}

}