#include "Execution.h"

namespace MadoEngine
{
	void Execution::Initialize(HINSTANCE hInstance){

		CoInitializeEx(0, COINIT_MULTITHREADED);

		Logger::Initialize();

		// ウィンドウの設定
		winDesc_.title = "MadoEngine";
		winDesc_.width = 1280;
		winDesc_.height = 720;
		winDesc_.iconPath = "Assets/.EngineResource/icon.png";
		winDesc_.isResizable = true;

		// ウィンドウの初期化
		windowsAPI_ = std::make_unique<MadoEngine::Screen::WindowsAPI>();
		windowsAPI_->Initialize(winDesc_, hInstance);

		// DxDeviceの初期化
		dxDevice_ = std::make_unique<MadoEngine::Core::DxDevice>();
		dxDevice_->Initialize();

		// CommandManagerの初期化
		commandManager_ = std::make_unique<MadoEngine::Core::CommandManager>();
		commandManager_->Initialize(dxDevice_.get());

		// SwapChainの初期化
		swapChain_ = std::make_unique<MadoEngine::Screen::SwapChain>();
		swapChain_->Initialize(dxDevice_.get(), commandManager_.get(), windowsAPI_->GetHWnd(), winDesc_.width, winDesc_.height);

		// RTVManagerの初期化
		rtvManager_ = std::make_unique<MadoEngine::Core::RTVManager>();
		rtvManager_->Initialize(dxDevice_.get());
		
		// DeltaTimeの初期化
		deltaTime_ = std::make_unique<MadoEngine::DeltaTime>();

		// AudioManagerの初期化（Assets/Audio内の全ファイルを自動ロード）
		MadoEngine::AudioManager::GetInstance()->Initialize();
		MadoEngine::InputManager::GetInstance()->Initialize();
		MadoEngine::TextureManager::GetInstance()->Initialize(dxDevice_.get()->GetDevice());

		// バックバッファ用のRTVを作成
		backBufferRTVIndices_.resize(swapChain_->GetBufferCount());
		for (uint32_t i = 0; i < swapChain_->GetBufferCount(); ++i) {
			backBufferRTVIndices_[i] = rtvManager_->Allocate();
			rtvManager_->CreateRenderTargetView(swapChain_->GetBackBuffer(i), backBufferRTVIndices_[i]);
		}
	}

	void Execution::Update() {
		// デルタタイムを計算
		deltaTime_->Update();
		float dt = static_cast<float>(deltaTime_->GetDeltaTime());

		// AudioManagerの更新（終了した音声のクリーンアップなど）
		MadoEngine::AudioManager::GetInstance()->Update();

		// InputManagerの更新（キーボード、マウス、ゲームパッドの状態を更新）
		MadoEngine::InputManager::GetInstance()->Update(windowsAPI_->GetHWnd(), dt);

		// WindowsAPIの入力処理（フルスクリーン切り替えなど）
		windowsAPI_->ProcessInput();
	}

	void Execution::PreDraw()
	{
		// 1. BackBufferを決定する
		currentBackBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();

		// 2. CommandListを開く（記録開始）
		commandManager_->BeginFrame();

		// 3. BackBufferをRenderTarget状態に遷移
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = swapChain_->GetBackBuffer(currentBackBufferIndex_);
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandManager_->GetCommandList()->ResourceBarrier(1, &barrier);

		// 4. RTVを設定
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvManager_->GetCPUHandle(backBufferRTVIndices_[currentBackBufferIndex_]);
		commandManager_->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		// 5. 画面のクリアを行う
		float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f }; // RGBA
		commandManager_->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	}

	void Execution::PostDraw()
	{
		// 6. BackBufferをPresent状態に遷移
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = swapChain_->GetBackBuffer(currentBackBufferIndex_);
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandManager_->GetCommandList()->ResourceBarrier(1, &barrier);

		// 7. CommandListを閉じる
		// 8. CommandListの実行（キック）
		commandManager_->EndFrame();

		// 9. 画面のスワップ（BackBufferとFrontBufferを入れ替える）
		swapChain_->Present();
	}

	void Execution::Finalize()
	{
		// 終了処理
		MadoEngine::AudioManager::GetInstance()->Finalize();
		MadoEngine::InputManager::GetInstance()->Finalize();
		MadoEngine::TextureManager::GetInstance()->Finalize();
		Logger::Finalize();

		CoUninitialize();
	}

	bool Execution::IsRunning() {
		return windowsAPI_->ProcessMessage();
	}
}