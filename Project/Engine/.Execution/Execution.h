#pragma once
#include <Windows.h>
#include <vector>
#include <memory>
#include <d3d12.h>
#include <wrl/client.h>
#include "CoreHeaders.h"
#include "RenderHeaders.h"
#include "UtilityHeaders.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#include "Render/ImGui/ImGuiManager.h"
#endif

namespace MadoEngine
{
	/// @brief ゲームエンジンの実行制御を管理するクラス
	class Execution
	{
	public:
		/// @brief 初期化処理
		void Initialize(HINSTANCE hInstance);

		/// @brief 更新処理
		void Update();

		/// @brief 描画前処理
		void PreDraw();

		/// @brief 描画後処理
		void PostDraw();

		/// @brief 終了処理
		void Finalize();

		/// @brief ゲームループを継続するかどうかを取得
		bool IsRunning();

	private:

		//D3DResourceLeakChecker leakChecker;

		std::unique_ptr<MadoEngine::Screen::WindowsAPI> windowsAPI_;
		MadoEngine::Screen::WindowsAPI::WindowDesc winDesc_;

		std::unique_ptr<MadoEngine::DeltaTime> deltaTime_;

		std::unique_ptr<MadoEngine::Core::DxDevice> dxDevice_;

		std::unique_ptr<MadoEngine::Core::CommandManager> commandManager_;

		std::unique_ptr<MadoEngine::Screen::SwapChain> swapChain_;

		std::unique_ptr<MadoEngine::Core::RTVManager> rtvManager_;
		std::unique_ptr<MadoEngine::Core::SRVManager> srvManager_;
		std::unique_ptr<MadoEngine::Core::DSVManager> dsvManager_;

		std::unique_ptr<MadoEngine::Render::PSOFactory> psoFactory_;
		std::unique_ptr<MadoEngine::Render::PSORegistry> psoRegistry_;

		std::unique_ptr<MadoEngine::Core::DepthStencilBuffer> depthStencilBuffer_;

		std::unique_ptr<MadoEngine::Render::ViewportScissor> viewportScissor_; // ビューポート＆シザー矩形

#ifdef USE_IMGUI
		std::unique_ptr<MadoEngine::ImGuiManager> imguiManager_;
		std::unique_ptr<MadoEngine::Render::RenderTexture> offscreenRT_;
#endif
	};
}