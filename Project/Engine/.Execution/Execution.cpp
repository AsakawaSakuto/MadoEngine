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

		// SRVManagerの初期化
		srvManager_ = std::make_unique<MadoEngine::Core::SRVManager>();
		srvManager_->Initialize(dxDevice_.get());

		// DSVManagerの初期化
		dsvManager_ = std::make_unique<MadoEngine::Core::DSVManager>();
		dsvManager_->Initialize(dxDevice_.get());
		
		// ShaderManagerの初期化（Assets/Shader 内の全HLSLをコンパイル・キャッシュ）
		MadoEngine::ShaderManager::GetInstance()->Initialize();

		// RootSignatureManagerの初期化
		MadoEngine::RootSignatureManager::GetInstance()->Initialize(dxDevice_.get());

		// PSOFactoryの初期化
		psoFactory_ = std::make_unique<MadoEngine::Render::PSOFactory>();
		psoFactory_->Initialize(dxDevice_.get());

		// PSORegistryの初期化
		psoRegistry_ = std::make_unique<MadoEngine::Render::PSORegistry>();
		psoRegistry_->Initialize(dxDevice_.get(), psoFactory_.get());

		// DeltaTimeの初期化
		deltaTime_ = std::make_unique<MadoEngine::DeltaTime>();

		// InputManagerの初期化
		MadoEngine::InputManager::GetInstance()->Initialize();

		// AudioManagerの初期化（Assets/Audio内の全ファイルを自動ロード）
		MadoEngine::AudioManager::GetInstance()->Initialize();

		// TextureManagerの初期化（Assets/Texture内の全.pngを自動ロード）
		MadoEngine::TextureManager::GetInstance()->Initialize(dxDevice_.get()->GetDevice(), srvManager_.get());

		// バックバッファ用のRTVを作成
		backBufferRTVIndices_.resize(swapChain_->GetBufferCount());
		for (uint32_t i = 0; i < swapChain_->GetBufferCount(); ++i) {
			backBufferRTVIndices_[i] = rtvManager_->Allocate();
			rtvManager_->CreateRenderTargetView(swapChain_->GetBackBuffer(i), backBufferRTVIndices_[i]);
		}

		// デプスバッファリソースの生成
		D3D12_RESOURCE_DESC depthDesc{};
		depthDesc.Width            = winDesc_.width;
		depthDesc.Height           = winDesc_.height;
		depthDesc.MipLevels        = 1;
		depthDesc.DepthOrArraySize = 1;
		depthDesc.Format           = DXGI_FORMAT_D32_FLOAT;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthDesc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_HEAP_PROPERTIES depthHeapProps{};
		depthHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_CLEAR_VALUE depthClearValue{};
		depthClearValue.Format               = DXGI_FORMAT_D32_FLOAT;
		depthClearValue.DepthStencil.Depth   = 1.0f;
		depthClearValue.DepthStencil.Stencil = 0;

		HRESULT hr = dxDevice_->GetDevice()->CreateCommittedResource(
			&depthHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthClearValue,
			IID_PPV_ARGS(&depthBuffer_)
		);
		assert(SUCCEEDED(hr));
		Logger::Output("[Engine] デプスバッファの生成が完了しました", Logger::Level::Engine);

		// デプスバッファ用のDSVを作成
		depthDSVIndex_ = dsvManager_->Allocate();
		dsvManager_->CreateDepthStencilView(depthBuffer_.Get(), depthDSVIndex_);

		// クライアント領域のサイズと一緒にして画面全体に表示
		viewport_.Width = FLOAT(windowsAPI_->GetWindowSize().first);
		viewport_.Height = FLOAT(windowsAPI_->GetWindowSize().second);
		viewport_.TopLeftX = 0;
		viewport_.TopLeftY = 0;
		viewport_.MinDepth = 0.0f;
		viewport_.MaxDepth = 1.0f;

		// 基本的にビューポートと同じ矩形が構成されるようにする
		scissorRect_.left = 0;
		scissorRect_.right = windowsAPI_->GetWindowSize().first;
		scissorRect_.top = 0;
		scissorRect_.bottom = windowsAPI_->GetWindowSize().second;

		testSprite_.Initialize(dxDevice_->GetDevice(), commandManager_->GetCommandList(), "uvChecker");
		testSprite_.SetPSORegistry(psoRegistry_.get());
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

		//testSprite_.SetRotation(testSprite_.GetRotation() + dt); // 毎フレーム回転させる
		testSprite_.SetScreenSize(static_cast<float>(winDesc_.width), static_cast<float>(winDesc_.height));
		testSprite_.Update();
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

		// 4. SRV用DescriptorHeapをセット（テクスチャ参照に必須）
		ID3D12DescriptorHeap* heaps[] = { srvManager_->GetDescriptorHeap() };
		commandManager_->GetCommandList()->SetDescriptorHeaps(1, heaps);

		// 5. RTVとDSVを設定
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvManager_->GetCPUHandle(backBufferRTVIndices_[currentBackBufferIndex_]);
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvManager_->GetCPUHandle(depthDSVIndex_);
		commandManager_->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		// 6. 画面のクリアを行う
		float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f }; // RGBA
		commandManager_->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		commandManager_->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		commandManager_->GetCommandList()->RSSetViewports(1, &viewport_);
		commandManager_->GetCommandList()->RSSetScissorRects(1, &scissorRect_);
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

		// 7. CommandListを閉じてGPUに送信
		commandManager_->EndFrame();

		// 8. 画面のスワップ（BackBufferとFrontBufferを入れ替える）
		// PresentをWaitForGPUより先に呼ぶことで、GPU処理と並行してPresent処理を行う
		swapChain_->Present();

		// 9. GPU処理完了を待機
		commandManager_->WaitForGPU();
	}

	void Execution::Finalize()
	{
		// 終了処理
		MadoEngine::AudioManager::GetInstance()->Finalize();
		MadoEngine::InputManager::GetInstance()->Finalize();
		MadoEngine::TextureManager::GetInstance()->Finalize();
		MadoEngine::ShaderManager::GetInstance()->Finalize();
		MadoEngine::RootSignatureManager::GetInstance()->Finalize();

		psoRegistry_->Finalize();

		Logger::Finalize();

		CoUninitialize();
	}

	bool Execution::IsRunning() {
		return windowsAPI_->ProcessMessage();
	}

	void Execution::TestDraw() {
		testSprite_.Update();
		testSprite_.Draw();
	}
}