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
		winDesc_.isResizable = false;
		winDesc_.isShowMouseCursor = true;

		// ウィンドウの初期化
		windowsAPI_ = std::make_unique<MadoEngine::Screen::WindowsAPI>();
		windowsAPI_->Initialize(winDesc_, hInstance);

		// DxDeviceの初期化
		dxDevice_ = std::make_unique<MadoEngine::Core::DxDevice>();
		dxDevice_->Initialize();

		// CommandManagerの初期化
		commandManager_ = std::make_unique<MadoEngine::Core::CommandManager>();
		commandManager_->Initialize(dxDevice_.get());

		// RTVManagerの初期化
		rtvManager_ = std::make_unique<MadoEngine::Core::RTVManager>();
		rtvManager_->Initialize(dxDevice_.get());

		// SwapChainの初期化
		swapChain_ = std::make_unique<MadoEngine::Screen::SwapChain>();
		swapChain_->Initialize(dxDevice_.get(), commandManager_.get(), windowsAPI_->GetHWnd(), winDesc_.width, winDesc_.height, 2, rtvManager_.get());

		// SRVManagerの初期化
		srvManager_ = std::make_unique<MadoEngine::Core::SRVManager>();
		srvManager_->Initialize(dxDevice_.get());

		// DSVManagerの初期化
		dsvManager_ = std::make_unique<MadoEngine::Core::DSVManager>();
		dsvManager_->Initialize(dxDevice_.get());
		
		// ShaderManagerの初期化（Assets/Shader 内の全HLSLをコンパイル・キャッシュ）
		MadoEngine::ShaderManager::GetInstance()->Initialize();

		// RootSignatureManagerの初期化 デフォルトのRootSignatureを生成・登録
		MadoEngine::RootSignatureManager::GetInstance()->Initialize(dxDevice_.get());
		MadoEngine::RootSignatureManager::GetInstance()->Make();

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

		// DepthStencilBuffer の生成
		depthStencilBuffer_ = std::make_unique<MadoEngine::Core::DepthStencilBuffer>();
		depthStencilBuffer_->Initialize(dxDevice_.get(), dsvManager_.get(), winDesc_.width, winDesc_.height);

		// ViewportScissor の初期化
		viewportScissor_ = std::make_unique<MadoEngine::Render::ViewportScissor>();
		viewportScissor_->UpdateSize(winDesc_.width, winDesc_.height);

		MadoEngine::SpriteManager::GetInstance()->Initialize(dxDevice_->GetDevice(), commandManager_->GetCommandList(), psoRegistry_.get());

		DebugLineManager::GetInstance()->Initialize(dxDevice_->GetDevice(), commandManager_->GetCommandList(), 10000);
		DebugLineManager::GetInstance()->SetPSORegistry(psoRegistry_.get());
#ifdef USE_IMGUI
		// ImGuiManagerの初期化
		imguiManager_ = std::make_unique<MadoEngine::ImGuiManager>();
		imguiManager_->Initialize(dxDevice_.get(), commandManager_.get(), srvManager_.get(), windowsAPI_->GetHWnd(), swapChain_->GetBufferCount());

		// オフスクリーンレンダーターゲットの生成（ゲーム描画をImGui内に表示するため）
		offscreenRT_ = std::make_unique<MadoEngine::Render::RenderTexture>();
		offscreenRT_->Initialize(dxDevice_.get(), rtvManager_.get(), srvManager_.get(), winDesc_.width, winDesc_.height);
#endif // USE_IMGUI
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
#ifdef USE_IMGUI
		// ImGuiフレーム開始
		imguiManager_->Begin();
#endif // USE_IMGUI

		// CommandListを開く（記録開始）
		commandManager_->BeginFrame();

		// SRV用DescriptorHeapをセット（テクスチャ参照に必須）
		ID3D12DescriptorHeap* heaps[] = { srvManager_->GetDescriptorHeap() };
		commandManager_->GetCommandList()->SetDescriptorHeaps(1, heaps);

#ifdef USE_IMGUI
		// USE_IMGUI時: オフスクリーンRTへ描画する
		// オフスクリーンRTをPIXEL_SHADER_RESOURCE → RENDER_TARGET へ遷移し、RTV/DSVをセット・クリア
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencilBuffer_->GetDSVCPUHandle();
		offscreenRT_->BeginRender(commandManager_->GetCommandList(), dsvHandle);
		commandManager_->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
#else
		// バックバッファをレンダーターゲットに遷移し、RTV/DSVをセット・クリア
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencilBuffer_->GetDSVCPUHandle();
		float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
		swapChain_->BeginRender(commandManager_->GetCommandList(), &dsvHandle, clearColor);
		commandManager_->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
#endif // USE_IMGUI

		viewportScissor_->Apply(commandManager_->GetCommandList());
	}

	void Execution::PostDraw()
	{
#ifdef USE_IMGUI
		// オフスクリーンRT: RENDER_TARGET → PIXEL_SHADER_RESOURCE に遷移
		offscreenRT_->EndRender(commandManager_->GetCommandList());

		// バックバッファをRENDER_TARGETに遷移し、ImGui描画先に設定・クリア
		float bbClearColor[] = { 1.0f, 0.08f, 0.08f, 1.0f };
		swapChain_->BeginRender(commandManager_->GetCommandList(), nullptr, bbClearColor);

		// エディタレイアウト（DockSpace + Game View）を描画
		imguiManager_->DrawEditorLayout(offscreenRT_->GetSRVGPUHandle());

		// デモウィンドウ（動作確認用、不要になったら削除してください）
		ImGui::ShowDemoWindow();

		// ImGui描画コマンドをコマンドリストに積む
		imguiManager_->End(commandManager_->GetCommandList());
#endif // USE_IMGUI

		// バックバッファ: RENDER_TARGET → PRESENT に遷移
		swapChain_->EndRender(commandManager_->GetCommandList());

		// CommandListを閉じてGPUに送信
		commandManager_->EndFrame();

		// 画面のスワップ（BackBufferとFrontBufferを入れ替える）
		swapChain_->Present();

		// GPU処理完了を待機
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
		MadoEngine::SpriteManager::GetInstance()->Finalize();

		psoRegistry_->Finalize();

		Logger::Finalize();

#ifdef USE_IMGUI
		imguiManager_->Finalize();
#endif // USE_IMGUI

		CoUninitialize();
	}

	bool Execution::IsRunning() {
		return windowsAPI_->ProcessMessage();
	}
}