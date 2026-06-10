#pragma once
#include <Windows.h>
#include <cstdint>
#include <functional>
#include <string>

namespace MadoEngine::Screen {

	class WindowsAPI {

	public:

		~WindowsAPI() = default;

		struct WindowDesc {
			std::string title = "MadoEngine";     // ウィンドウのタイトル
			std::string iconPath = "";            // アイコンファイルのパス (空の場合はデフォルトアイコン)
			int width = 1280;                     // ウィンドウの幅
			int height = 720;                     // ウィンドウの高さ
			bool isResizable = true;              // ウィンドウサイズを変更可能にするかどうか
			bool isShowMouseCursor = true;        // マウスカーソルを表示するかどうか
			std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> wndProc; // ウィンドウプロシージャのコールバック関数
		};

		/// @brief ウィンドウを初期化する
		/// @param desc ウィンドウの設定
		/// @param hInstance アプリケーションインスタンスハンドル
		void Initialize(WindowDesc& desc, HINSTANCE hInstance);

		/// @brief メッセージを処理する。アプリを継続する場合はtrue、終了する場合はfalseを返す
		bool ProcessMessage();

		/// @brief ウィンドウのサイズを取得する
		std::pair<int, int> GetWindowSize() const { return { desc_.width, desc_.height }; }

		/// @brief 現在のクライアント領域サイズを取得する
		/// @return クライアント領域の幅と高さ
		std::pair<int, int> GetClientSize() const;

		/// @brief 未処理のリサイズ要求を取得する
		/// @param width リサイズ後のクライアント領域幅
		/// @param height リサイズ後のクライアント領域高さ
		/// @return リサイズ要求がある場合はtrue
		bool ConsumeResize(uint32_t& width, uint32_t& height);

		/// @brief ウィンドウハンドルを取得する
		HWND GetHWnd() const { return hWnd_; }

		/// @brief 右上の×ボタンが押されたかどうかを取得する
		bool IsPushCloseButton() const { return isPushCloseBottom_; }

		/// @brief フルスクリーンモードかどうかを取得する
		bool IsFullscreen() const { return isFullscreen_; }

		/// @brief フルスクリーンモードを切り替える
		void ToggleFullscreen();

		/// @brief 入力処理（フルスクリーン切り替えなど）
		void ProcessInput();

		private:

		/// @brief ウィンドウプロシージャ
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		/// @brief アイコンをファイルから読み込む
		/// @param filePath アイコンファイルのパス
		HICON LoadIconFromFile(const std::string& filePath);

		HWND hWnd_ = nullptr;    // ウィンドウハンドル
		WNDCLASS wndClass_ = {}; // ウィンドウクラス

		WindowDesc desc_;        // ウィンドウの設定

		HICON hIcon_ = nullptr;                  // アイコンハンドル
		bool isFullscreen_ = false;              // フルスクリーンモードにするかどうか
		bool isPushCloseBottom_ = false;         // 右上の×ボタンが押されたかどうか
		bool hasResizeRequest_ = false;          // 未処理のリサイズ要求があるかどうか
		int pendingResizeWidth_ = 0;             // 次に反映するクライアント領域の幅
		int pendingResizeHeight_ = 0;            // 次に反映するクライアント領域の高さ

		// ウィンドウモード時の情報を保存（フルスクリーンから戻るために使用）
		RECT windowedRect_ = {};                 // ウィンドウモード時の位置とサイズ
		DWORD windowedStyle_ = 0;                // ウィンドウモード時のスタイル

		// アスペクト比の維持
		float aspectRatio_ = 0.0f;               // 初期ウィンドウのアスペクト比（幅/高さ）

	};
}
