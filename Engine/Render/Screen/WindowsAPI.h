#pragma once
#include <Windows.h>
#include <functional>
#include <string>

namespace MadoEngine {

	class WindowsAPI {

	public:

		~WindowsAPI() = default;

		struct WindowDesc {
			std::string title = "MadoEngine"; // ウィンドウのタイトル
			int width = 1280;                 // ウィンドウの幅
			int height = 720;                 // ウィンドウの高さ
			std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> wndProc; // ウィンドウプロシージャのコールバック関数
		};

		void Initialize(WindowDesc& desc, HINSTANCE hInstance);

		// メッセージを処理する。アプリを継続する場合はtrue、終了する場合はfalseを返す
		bool ProcessMessage();

		std::pair<int, int> GetWindowSize() const { return { desc_.width, desc_.height }; }
		HWND GetHWnd() const { return hWnd_; }

		bool IsFullscreen() const { return isFullscreen_; }
		bool IsPushCloseButton() const { return isPushCloseBottom_; }

	private:

		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		HWND hWnd_ = nullptr;    // ウィンドウハンドル
		WNDCLASS wndClass_ = {}; // ウィンドウクラス

		WindowDesc desc_;        // ウィンドウの設定

		bool isFullscreen_ = false;      // フルスクリーンモードにするかどうか
		bool isPushCloseBottom_ = false; // 右上の×ボタンが押されたかどうか

	};

}