#include <Windows.h>
#include <vector>
#include "Engine/CoreHeaders.h"
#include "Engine/RenderHeaders.h"
#include "Engine/UtilityHeaders.h"
#include "Engine/MathHeaders.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	Logger::Initialize();

	MadoEngine::Screen::WindowsAPI windowsAPI;
	MadoEngine::DeltaTime deltaTime;

	MadoEngine::Screen::WindowsAPI::WindowDesc desc;
	desc.title = "MadoEngine";
	desc.width = 1280;
	desc.height = 720;
	desc.iconPath = "Assets/.EngineResource/icon.png";
	desc.isResizable = true;

	// ウィンドウの初期化
	windowsAPI.Initialize(desc, hInstance);

	// DxDeviceの初期化
	MadoEngine::Core::DxDevice dxDevice;
	dxDevice.Initialize();

	// CommandManagerの初期化
	MadoEngine::Core::CommandManager commandManager;
	commandManager.Initialize(&dxDevice);

	// SwapChainの初期化
	MadoEngine::Screen::SwapChain swapChain;
	swapChain.Initialize(&dxDevice, &commandManager, windowsAPI.GetHWnd(), desc.width, desc.height);

	// RTVManagerの初期化
	MadoEngine::Core::RTVManager rtvManager;
	rtvManager.Initialize(&dxDevice);

	// バックバッファ用のRTVを作成
	std::vector<uint32_t> backBufferRTVIndices(swapChain.GetBufferCount());
	for (uint32_t i = 0; i < swapChain.GetBufferCount(); ++i) {
		backBufferRTVIndices[i] = rtvManager.Allocate();
		rtvManager.CreateRenderTargetView(swapChain.GetBackBuffer(i), backBufferRTVIndices[i]);
	}

	// AudioManagerの初期化（Assets/Audio内の全ファイルを自動ロード）
	MadoEngine::AudioManager::GetInstance()->Initialize();

	MadoEngine::InputManager::GetInstance()->Initialize();

	Input::SetInputKeys("Jump", DIK_SPACE, GAMEPAD_STICK_R, GAMEPAD_STICK_L, GAMEPAD_L, GAMEPAD_R, GAMEPAD_A);

	// メインループ
	while (windowsAPI.ProcessMessage()) {
		// デルタタイムを計算
		deltaTime.Update();
		float dt = static_cast<float>(deltaTime.GetDeltaTime());

		// ゲームの処理
		MadoEngine::AudioManager::GetInstance()->Update();

		MadoEngine::InputManager::GetInstance()->Update(windowsAPI.GetHWnd(), dt);

		// フルスクリーン切り替え（ALT+EnterまたはF11キー）
		auto* keyboard = MadoEngine::InputManager::GetInstance()->GetKeybord();
		if (keyboard) {
			// ALT+Enterでフルスクリーン切り替え
			bool altPressed = keyboard->IsPress(DIK_LMENU) || keyboard->IsPress(DIK_RMENU);
			bool enterTriggered = keyboard->IsTrigger(DIK_RETURN);

			// F11キーでフルスクリーン切り替え
			bool f11Triggered = keyboard->IsTrigger(DIK_F11);

			if ((altPressed && enterTriggered) || f11Triggered) {
				windowsAPI.ToggleFullscreen();
			}
		}

		if (Input::Trigger("jump")) {}

		if (Input::Press("jump")) {}

		if (Input::Release("jump")) {}

		// ===== 画面クリア処理 =====

		// 1. BackBufferを決定する
		uint32_t backBufferIndex = swapChain.GetCurrentBackBufferIndex();

		// 2. CommandListを開く（記録開始）
		commandManager.BeginFrame();

		// 3. BackBufferをRenderTarget状態に遷移
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = swapChain.GetBackBuffer(backBufferIndex);
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandManager.GetCommandList()->ResourceBarrier(1, &barrier);

		// 4. RTVを設定
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvManager.GetCPUHandle(backBufferRTVIndices[backBufferIndex]);
		commandManager.GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		// 5. 画面のクリアを行う（青色でクリア）
		float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f }; // RGBA
		commandManager.GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		// 6. BackBufferをPresent状態に遷移
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		commandManager.GetCommandList()->ResourceBarrier(1, &barrier);

		// 7. CommandListを閉じる
		// 8. CommandListの実行（キック）
		commandManager.EndFrame();

		// 9. 画面のスワップ（BackBufferとFrontBufferを入れ替える）
		swapChain.Present();
	}

	MadoEngine::AudioManager::GetInstance()->Finalize();

	MadoEngine::InputManager::GetInstance()->Finalize();

	Logger::Finalize();

	return 0;
}