#include "Execution.h"
#include "Render/ImGui/EditorUI.h"

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
		winDesc_.isShowMouseCursor = true;

		// ウィンドウの初期化
		windowsAPI_ = std::make_unique<MadoEngine::Screen::WindowsAPI>();
		windowsAPI_->Initialize(winDesc_, hInstance);
		auto [clientWidth, clientHeight] = windowsAPI_->GetClientSize();
		renderWidth_ = static_cast<uint32_t>(clientWidth > 0 ? clientWidth : winDesc_.width);
		renderHeight_ = static_cast<uint32_t>(clientHeight > 0 ? clientHeight : winDesc_.height);

		// DxDeviceの初期化
		dxDevice_ = std::make_unique<MadoEngine::Core::DxDevice>();
		dxDevice_->Initialize();

		// CommandManagerの初期化
		commandManager_ = std::make_unique<MadoEngine::Core::CommandManager>();
		commandManager_->Initialize(dxDevice_.get());

		// RTVManagerの初期化
		rtvManager_ = &MadoEngine::Core::RTVManager::GetInstance();
		rtvManager_->Initialize(dxDevice_.get());

		// SwapChainの初期化
		swapChain_ = std::make_unique<MadoEngine::Screen::SwapChain>();
		swapChain_->Initialize(dxDevice_.get(), commandManager_.get(), windowsAPI_->GetHWnd(), renderWidth_, renderHeight_, 2, rtvManager_);

		// SRVManagerの初期化
		srvManager_ = &MadoEngine::Core::SRVManager::GetInstance();
		srvManager_->Initialize(dxDevice_.get());

		// DSVManagerの初期化
		dsvManager_ = &MadoEngine::Core::DSVManager::GetInstance();
		dsvManager_->Initialize(dxDevice_.get());
		
		// ShaderManagerの初期化（Assets/Shader 内の全HLSLをコンパイル・キャッシュ）
		MadoEngine::ShaderManager::GetInstance().Initialize();

		// RootSignatureManagerの初期化 デフォルトのRootSignatureを生成・登録
		MadoEngine::RootSignatureManager::GetInstance().Initialize(dxDevice_.get());
		MadoEngine::RootSignatureManager::GetInstance().Make();

		// PSOFactoryの初期化
		psoFactory_ = std::make_unique<MadoEngine::Render::PSOFactory>();
		psoFactory_->Initialize(dxDevice_.get());

		// PSORegistryの初期化
		psoRegistry_ = std::make_unique<MadoEngine::Render::PSORegistry>();
		psoRegistry_->Initialize(dxDevice_.get(), psoFactory_.get());

		// DeltaTimeの初期化
		deltaTime_ = std::make_unique<MadoEngine::DeltaTime>();

		// InputManagerの初期化
		MadoEngine::InputManager::GetInstance().Initialize();
		MyInput::SetInput("Up", { DIK_UP,DIK_W }, { GAMEPAD_UP });
		MyInput::SetInput("Down", { DIK_DOWN,DIK_S }, { GAMEPAD_DOWN });
		MyInput::SetInput("Left", { DIK_LEFT,DIK_A }, { GAMEPAD_LEFT });
		MyInput::SetInput("Right", { DIK_RIGHT,DIK_D }, { GAMEPAD_RIGHT });
		MyInput::SetInput("Q", { DIK_Q }, { });
		MyInput::SetInput("E", { DIK_E }, { });
		MyInput::SetInput("Jump", { DIK_SPACE,DIK_Z }, { GAMEPAD_A });
		MyInput::SetInput("Dash", { DIK_LSHIFT,DIK_X }, { GAMEPAD_STICK_L });

		// AudioManagerの初期化（Assets/Audio内の全ファイルを自動ロード）
		MadoEngine::AudioManager::GetInstance().Initialize();

		// TextureManagerの初期化（Assets/Texture内の全.pngを自動ロード）
		MadoEngine::TextureManager::GetInstance().Initialize(dxDevice_.get()->GetDevice(), srvManager_);

		// DepthStencilBuffer の生成
		depthStencilBuffer_ = std::make_unique<MadoEngine::Core::DepthStencilBuffer>();
		depthStencilBuffer_->Initialize(dxDevice_.get(), dsvManager_, renderWidth_, renderHeight_);

		// ViewportScissor の初期化
		viewportScissor_ = std::make_unique<MadoEngine::Render::ViewportScissor>();
		viewportScissor_->UpdateSize(renderWidth_, renderHeight_);

		MadoEngine::SpriteManager::GetInstance().Initialize(dxDevice_->GetDevice(), commandManager_->GetCommandList(), psoRegistry_.get());
		MadoEngine::ModelManager::GetInstance().Initialize(dxDevice_->GetDevice(), commandManager_->GetCommandList(), psoRegistry_.get());

		DebugLineManager::GetInstance().Initialize(dxDevice_->GetDevice(), commandManager_->GetCommandList(), 20000);
		DebugLineManager::GetInstance().SetPSORegistry(psoRegistry_.get());

		offscreenRT_ = std::make_unique<MadoEngine::Render::RenderTexture>();
		offscreenRT_->Initialize(dxDevice_.get(), rtvManager_, srvManager_, renderWidth_, renderHeight_, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

		postEffectRT_ = std::make_unique<MadoEngine::Render::RenderTexture>();
		postEffectRT_->Initialize(dxDevice_.get(), rtvManager_, srvManager_, renderWidth_, renderHeight_, DXGI_FORMAT_R8G8B8A8_UNORM);

		postEffectCopyDesc_.blendMode = MadoEngine::Render::BlendMode::None;
		postEffectCopyDesc_.depthMode = MadoEngine::Render::DepthMode::Disable;
		postEffectCopyDesc_.cullMode = MadoEngine::Render::CullMode::None;
		postEffectCopyDesc_.fillMode = MadoEngine::Render::FillMode::Solid;
		postEffectCopyDesc_.topology = MadoEngine::Render::TopologyType::Triangle;
		postEffectCopyDesc_.inputLayout = MadoEngine::Render::InputLayoutType::None;
		postEffectCopyDesc_.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		postEffectCopyDesc_.dsvFormat = DXGI_FORMAT_UNKNOWN;
		postEffectCopyDesc_.vsKey = "PostEffect/CopyImage.VS";
		postEffectCopyDesc_.psKey = "PostEffect/CopyImage.PS";
		postEffectCopyDesc_.rootSigKey = "PostEffect.RootSig";
#ifdef USE_IMGUI
		// ImGuiManagerの初期化
		imguiManager_ = std::make_unique<MadoEngine::ImGuiManager>();
		imguiManager_->Initialize(dxDevice_.get(), commandManager_.get(), srvManager_, windowsAPI_->GetHWnd(), swapChain_->GetBufferCount());
#endif // USE_IMGUI
	}

	void Execution::Update() {
		// デルタタイムを計算
		deltaTime_->Update();
		float dt = static_cast<float>(deltaTime_->GetDeltaTime());

		// ウィンドウリサイズ要求があれば描画リソースへ反映
		HandleResize();

		// AudioManagerの更新（終了した音声のクリーンアップなど）
		MadoEngine::AudioManager::GetInstance().Update();

		// InputManagerの更新（キーボード、マウス、ゲームパッドの状態を更新）
		MadoEngine::InputManager::GetInstance().Update(windowsAPI_->GetHWnd(), dt);

		// WindowsAPIの入力処理（フルスクリーン切り替えなど）
		windowsAPI_->ProcessInput();
	}

	void Execution::HandleResize() {
		uint32_t width = 0;
		uint32_t height = 0;
		if (!windowsAPI_->ConsumeResize(width, height)) {
			return;
		}

		if (width == renderWidth_ && height == renderHeight_) {
			return;
		}

		commandManager_->WaitForGPU();
		swapChain_->Resize(width, height);
		depthStencilBuffer_->Resize(width, height);
		viewportScissor_->UpdateSize(width, height);
		offscreenRT_->Resize(width, height);
		postEffectRT_->Resize(width, height);

		renderWidth_ = width;
		renderHeight_ = height;
		Logger::Output(
			"描画サイズを更新しました: " +
			std::to_string(renderWidth_) + "x" + std::to_string(renderHeight_),
			Logger::Level::Engine
		);
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

		// オフスクリーンRTへ描画する
		// オフスクリーンRTをPIXEL_SHADER_RESOURCE → RENDER_TARGET へ遷移し、RTV/DSVをセット・クリア
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencilBuffer_->GetDSVCPUHandle();
		offscreenRT_->BeginRender(commandManager_->GetCommandList(), dsvHandle);
		commandManager_->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		viewportScissor_->Apply(commandManager_->GetCommandList());
	}

	void Execution::BeginImGuiLayout()
	{
#ifdef USE_IMGUI
		// オフスクリーンRT: RENDER_TARGET → PIXEL_SHADER_RESOURCE に遷移
		offscreenRT_->EndRender(commandManager_->GetCommandList());

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencilBuffer_->GetDSVCPUHandle();
		postEffectRT_->BeginRender(commandManager_->GetCommandList(), dsvHandle);
		viewportScissor_->Apply(commandManager_->GetCommandList());
		DrawPostEffectCopy();
		postEffectRT_->EndRender(commandManager_->GetCommandList());

		// バックバッファをRENDER_TARGETに遷移し、ImGui描画先に設定・クリア
		float bbClearColor[] = { 1.0f, 0.08f, 0.08f, 1.0f };
		swapChain_->BeginRender(commandManager_->GetCommandList(), nullptr, bbClearColor);

		// エディタレイアウト（DockSpace + Game View）を描画
		// ※必ずシーンの DrawImGui() より前に呼ぶこと（DockSpaceを先に生成する必要があるため）
		imguiManager_->DrawEditorLayout(postEffectRT_->GetSRVGPUHandle());

		// デモウィンドウ（動作確認用、不要になったら削除してください）
		ImGui::ShowDemoWindow();

		// エンジン情報ウィンドウ（FPS表示）
		ImGui::Begin("Engine Info");
		ImGui::Text("FPS: %.1f", deltaTime_->GetFPS());
		ImGui::Text("DeltaTime: %.4f ms", deltaTime_->GetDeltaTime() * 1000.0);
		ImGui::Checkbox("FPS Limit", &isStopApplication_);
		ImGui::End();

		//if (ImGui::BeginMainMenuBar())
		//{
		//	
		//	// 必ず最後はEndで閉じる
		//	ImGui::EndMainMenuBar();
		//}

		MadoEngine::Editor::DrawAudioManagerUI();
#else
		offscreenRT_->EndRender(commandManager_->GetCommandList());

		float bbClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		swapChain_->BeginRender(commandManager_->GetCommandList(), nullptr, bbClearColor);
		viewportScissor_->Apply(commandManager_->GetCommandList());
		DrawPostEffectCopy();
#endif // USE_IMGUI
	}

	void Execution::DrawPostEffectCopy() {
		auto* commandList = commandManager_->GetCommandList();
		commandList->SetGraphicsRootSignature(
			MadoEngine::RootSignatureManager::GetInstance().Get(postEffectCopyDesc_.rootSigKey));
		commandList->SetPipelineState(psoRegistry_->Get(postEffectCopyDesc_));
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->SetGraphicsRootDescriptorTable(0, offscreenRT_->GetSRVGPUHandle());
		commandList->DrawInstanced(3, 1, 0, 0);
	}

	void Execution::PostDraw()
	{
#ifdef USE_IMGUI
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
		MadoEngine::AudioManager::GetInstance().Finalize();
		MadoEngine::InputManager::GetInstance().Finalize();
		MadoEngine::TextureManager::GetInstance().Finalize();
		MadoEngine::ShaderManager::GetInstance().Finalize();
		MadoEngine::RootSignatureManager::GetInstance().Finalize();
		MadoEngine::SpriteManager::GetInstance().Finalize();
		MadoEngine::ModelManager::GetInstance().Finalize();

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
