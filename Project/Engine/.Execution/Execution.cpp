#include "Execution.h"
#include "Render/ImGui/EditorUI.h"
#include <cassert>

namespace {

	const std::string kSceneColorTarget = "SceneColor";
	const std::string kPostEffectResultTarget = "PostEffectResult";
	const std::string kPostEffectWorkTarget = "PostEffectWork";
	const std::string kLayerColorTarget = "LayerColor";
	const std::string kLayerEffectResultTarget = "LayerEffectResult";
	const std::string kLayerEffectWorkTarget = "LayerEffectWork";

} // namespace

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

		renderTargetManager_ = std::make_unique<MadoEngine::Render::RenderTargetManager>();
		renderTargetManager_->Initialize(dxDevice_.get(), rtvManager_, srvManager_);

		MadoEngine::Render::RenderTargetManager::Desc sceneColorDesc{};
		sceneColorDesc.width = renderWidth_;
		sceneColorDesc.height = renderHeight_;
		sceneColorDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		sceneColorDesc.clearColor = { 0.1f, 0.25f, 0.5f, 1.0f };
		renderTargetManager_->Create(kSceneColorTarget, sceneColorDesc);

		MadoEngine::Render::RenderTargetManager::Desc postEffectDesc{};
		postEffectDesc.width = renderWidth_;
		postEffectDesc.height = renderHeight_;
		postEffectDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		postEffectDesc.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderTargetManager_->Create(kPostEffectResultTarget, postEffectDesc);
		renderTargetManager_->Create(kPostEffectWorkTarget, postEffectDesc);

		MadoEngine::Render::RenderTargetManager::Desc layerColorDesc{};
		layerColorDesc.width = renderWidth_;
		layerColorDesc.height = renderHeight_;
		layerColorDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		layerColorDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		renderTargetManager_->Create(kLayerColorTarget, layerColorDesc);

		MadoEngine::Render::RenderTargetManager::Desc layerEffectDesc{};
		layerEffectDesc.width = renderWidth_;
		layerEffectDesc.height = renderHeight_;
		layerEffectDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		layerEffectDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		renderTargetManager_->Create(kLayerEffectResultTarget, layerEffectDesc);
		renderTargetManager_->Create(kLayerEffectWorkTarget, layerEffectDesc);

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

		compositeDesc_ = postEffectCopyDesc_;
		compositeDesc_.psKey = "PostEffect/Composite.PS";
		compositeDesc_.rootSigKey = "PostEffect.Composite.RootSig";

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
		renderTargetManager_->ResizeAll(width, height);

		renderWidth_ = width;
		renderHeight_ = height;
		Logger::Output(
			"描画サイズを更新しました: " +
			std::to_string(renderWidth_) + "x" + std::to_string(renderHeight_),
			Logger::Level::Engine
		);
	}

	MadoEngine::Render::LayerEffectPass* Execution::AddLayerEffectPass(const MadoEngine::Render::LayerEffectPass::Desc& desc) {
		layerEffectPasses_.emplace_back();
		layerEffectPasses_.back().Initialize(desc, postEffectCopyDesc_);
		return &layerEffectPasses_.back();
	}

	void Execution::ClearLayerEffectPasses() {
		layerEffectPasses_.clear();
	}

	const std::vector<MadoEngine::Render::LayerEffectPass>& Execution::GetLayerEffectPasses() const {
		return layerEffectPasses_;
	}

	const MadoEngine::Render::LayerEffectPass* Execution::GetFirstEnabledLayerEffectPass() const {
		for (const MadoEngine::Render::LayerEffectPass& pass : layerEffectPasses_) {
			if (pass.IsEnabled() && pass.GetTargetLayerMask() != 0) {
				return &pass;
			}
		}

		return nullptr;
	}

	MadoEngine::Render::RenderLayerMask Execution::GetEnabledLayerEffectTargetMask() const {
		MadoEngine::Render::RenderLayerMask layerMask = 0;
		for (const MadoEngine::Render::LayerEffectPass& pass : layerEffectPasses_) {
			if (!pass.IsEnabled()) {
				continue;
			}

			layerMask |= pass.GetTargetLayerMask();
		}

		return layerMask;
	}

	void Execution::PreDraw()
	{
		isSceneColorEnded_ = false;
		isLayerEffectChainResolved_ = false;
		isLayerEffectResolved_ = false;
		currentCompositeSourceName_ = kSceneColorTarget;
		resolvedPostEffectTargetName_ = kPostEffectResultTarget;
		currentLayerEffectSourceName_ = kLayerColorTarget;

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
		renderTargetManager_->Begin(kSceneColorTarget, commandManager_->GetCommandList(), dsvHandle);
		commandManager_->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		viewportScissor_->Apply(commandManager_->GetCommandList());
	}

	void Execution::EndSceneColorRender() {
		if (isSceneColorEnded_) {
			return;
		}

		renderTargetManager_->End(kSceneColorTarget, commandManager_->GetCommandList());
		isSceneColorEnded_ = true;
	}

	void Execution::BeginLayerEffectRender(const MadoEngine::Render::LayerEffectPass& pass) {
		assert(pass.IsEnabled() && "無効なLayerEffectPassは実行できません");
		assert(pass.GetTargetLayerMask() != 0 && "LayerEffectPassの対象LayerMaskが0です");

		EndSceneColorRender();
		isLayerEffectChainResolved_ = false;
		currentLayerEffectSourceName_ = kLayerColorTarget;

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencilBuffer_->GetDSVCPUHandle();
		renderTargetManager_->Begin(kLayerColorTarget, commandManager_->GetCommandList(), dsvHandle);
		viewportScissor_->Apply(commandManager_->GetCommandList());
	}

	void Execution::EndLayerEffectRender() {
		renderTargetManager_->End(kLayerColorTarget, commandManager_->GetCommandList());
	}

	void Execution::ApplyLayerEffectAndComposite(const MadoEngine::Render::LayerEffectPass& pass) {
		ApplyLayerEffectToChain(pass);
		CompositeLayerEffectChain();
	}

	void Execution::ApplyLayerEffectToChain(const MadoEngine::Render::LayerEffectPass& pass) {
		assert(pass.IsEnabled() && "無効なLayerEffectPassは実行できません");
		assert(pass.GetTargetLayerMask() != 0 && "LayerEffectPassの対象LayerMaskが0です");

		const std::string& outputTargetName = GetNextLayerEffectOutputName();
		renderTargetManager_->Begin(outputTargetName, commandManager_->GetCommandList());
		viewportScissor_->Apply(commandManager_->GetCommandList());
		DrawPostEffect(renderTargetManager_->GetSRVGPUHandle(currentLayerEffectSourceName_), pass.GetEffectPSODesc());
		renderTargetManager_->End(outputTargetName, commandManager_->GetCommandList());

		currentLayerEffectSourceName_ = outputTargetName;
		isLayerEffectChainResolved_ = true;
	}

	void Execution::CompositeLayerEffectChain() {
		assert(isLayerEffectChainResolved_ && "LayerEffectChainが解決されていません");

		const std::string& outputTargetName = GetNextPostEffectOutputName();
		renderTargetManager_->Begin(outputTargetName, commandManager_->GetCommandList());
		viewportScissor_->Apply(commandManager_->GetCommandList());
		DrawComposite(
			renderTargetManager_->GetSRVGPUHandle(currentCompositeSourceName_),
			renderTargetManager_->GetSRVGPUHandle(currentLayerEffectSourceName_)
		);
		renderTargetManager_->End(outputTargetName, commandManager_->GetCommandList());

		currentCompositeSourceName_ = outputTargetName;
		resolvedPostEffectTargetName_ = outputTargetName;
		isLayerEffectResolved_ = true;
	}

	void Execution::BeginImGuiLayout()
	{
#ifdef USE_IMGUI
		// オフスクリーンRT: RENDER_TARGET → PIXEL_SHADER_RESOURCE に遷移
		if (!isLayerEffectResolved_) {
			EndSceneColorRender();
			renderTargetManager_->Begin(kPostEffectResultTarget, commandManager_->GetCommandList());
			viewportScissor_->Apply(commandManager_->GetCommandList());
			DrawPostEffect(renderTargetManager_->GetSRVGPUHandle(kSceneColorTarget), postEffectCopyDesc_);
			renderTargetManager_->End(kPostEffectResultTarget, commandManager_->GetCommandList());
			currentCompositeSourceName_ = kPostEffectResultTarget;
			resolvedPostEffectTargetName_ = kPostEffectResultTarget;
			isLayerEffectResolved_ = true;
		}

		// バックバッファをRENDER_TARGETに遷移し、ImGui描画先に設定・クリア
		float bbClearColor[] = { 1.0f, 0.08f, 0.08f, 1.0f };
		swapChain_->BeginRender(commandManager_->GetCommandList(), nullptr, bbClearColor);

		// エディタレイアウト（DockSpace + Game View）を描画
		// ※必ずシーンの DrawImGui() より前に呼ぶこと（DockSpaceを先に生成する必要があるため）
		imguiManager_->DrawEditorLayout(renderTargetManager_->GetSRVGPUHandle(resolvedPostEffectTargetName_));

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
		if (!isLayerEffectResolved_) {
			EndSceneColorRender();
			renderTargetManager_->Begin(kPostEffectResultTarget, commandManager_->GetCommandList());
			viewportScissor_->Apply(commandManager_->GetCommandList());
			DrawPostEffect(renderTargetManager_->GetSRVGPUHandle(kSceneColorTarget), postEffectCopyDesc_);
			renderTargetManager_->End(kPostEffectResultTarget, commandManager_->GetCommandList());
			currentCompositeSourceName_ = kPostEffectResultTarget;
			resolvedPostEffectTargetName_ = kPostEffectResultTarget;
			isLayerEffectResolved_ = true;
		}

		float bbClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		swapChain_->BeginRender(commandManager_->GetCommandList(), nullptr, bbClearColor);
		viewportScissor_->Apply(commandManager_->GetCommandList());
		DrawPostEffect(renderTargetManager_->GetSRVGPUHandle(resolvedPostEffectTargetName_), postEffectCopyDesc_);
#endif // USE_IMGUI
	}

	void Execution::DrawPostEffect(D3D12_GPU_DESCRIPTOR_HANDLE inputSrv, const MadoEngine::Render::PSODesc& desc) {
		auto* commandList = commandManager_->GetCommandList();
		commandList->SetGraphicsRootSignature(
			MadoEngine::RootSignatureManager::GetInstance().Get(desc.rootSigKey));
		commandList->SetPipelineState(psoRegistry_->Get(desc));
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->SetGraphicsRootDescriptorTable(0, inputSrv);
		commandList->DrawInstanced(3, 1, 0, 0);
	}

	void Execution::DrawComposite(D3D12_GPU_DESCRIPTOR_HANDLE sceneSrv, D3D12_GPU_DESCRIPTOR_HANDLE effectSrv) {
		auto* commandList = commandManager_->GetCommandList();
		commandList->SetGraphicsRootSignature(
			MadoEngine::RootSignatureManager::GetInstance().Get(compositeDesc_.rootSigKey));
		commandList->SetPipelineState(psoRegistry_->Get(compositeDesc_));
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->SetGraphicsRootDescriptorTable(0, sceneSrv);
		commandList->SetGraphicsRootDescriptorTable(1, effectSrv);
		commandList->DrawInstanced(3, 1, 0, 0);
	}

	const std::string& Execution::GetNextPostEffectOutputName() const {
		if (currentCompositeSourceName_ == kPostEffectResultTarget) {
			return kPostEffectWorkTarget;
		}

		return kPostEffectResultTarget;
	}

	const std::string& Execution::GetNextLayerEffectOutputName() const {
		if (currentLayerEffectSourceName_ == kLayerEffectResultTarget) {
			return kLayerEffectWorkTarget;
		}

		return kLayerEffectResultTarget;
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
