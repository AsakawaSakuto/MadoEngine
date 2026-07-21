#include "Execution.h"
#include "EditorUIHeaders.h"
#include <algorithm>
#include <cassert>
#include <cstring>

namespace {

	// レンダーターゲットの識別用キー
	const std::string kSceneColorTarget = "SceneColor";               // 通常のシーン全体を最初に描く
	const std::string kPostEffectResultTarget = "PostEffectResult";   // ポストエフェクト後の結果を置く場所
	const std::string kPostEffectWorkTarget = "PostEffectWork";       // 複数回ポストエフェクトするための作業用バッファ
	const std::string kLayerColorTarget = "LayerColor";               // 特定レイヤーだけを描く
	const std::string kLayerEffectResultTarget = "LayerEffectResult"; // レイヤー用ポストエフェクト結果
	const std::string kLayerEffectWorkTarget = "LayerEffectWork";     // レイヤー用ポストエフェクトの作業用バッファ

} // namespace

namespace MadoEngine
{
	void EngineExecution::Initialize(HINSTANCE hInstance) {

		CoInitializeEx(0, COINIT_MULTITHREADED);

		Logger::Initialize();

		// ウィンドウの設定
		winDesc_.title = "MadoEngine";
		winDesc_.width = 1280;
		winDesc_.height = 720;
		winDesc_.iconPath = "Assets/Texture/.Engine/icon.png";
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
		MyInput::RegisterInput("Up", { DIK_UP,DIK_W }, { GAMEPAD_UP });
		MyInput::RegisterInput("Down", { DIK_DOWN,DIK_S }, { GAMEPAD_DOWN });
		MyInput::RegisterInput("Left", { DIK_LEFT,DIK_A }, { GAMEPAD_LEFT });
		MyInput::RegisterInput("Right", { DIK_RIGHT,DIK_D }, { GAMEPAD_RIGHT });

		MyInput::RegisterInput("Jump", { DIK_SPACE,DIK_Z }, { GAMEPAD_A, GAMEPAD_STICK_L });
		MyInput::RegisterInput("Crouching", { DIK_LSHIFT }, { GAMEPAD_R });

		MyInput::RegisterInput("Interact", { DIK_E }, { GAMEPAD_X });
		MyInput::RegisterInput("Decision", { DIK_SPACE }, { GAMEPAD_A });

		MyInput::RegisterInput("Pause", { DIK_ESCAPE }, { GAMEPAD_START });

		// AudioManagerの初期化（Assets/Audio内の全ファイルを自動ロード）
		MadoEngine::AudioManager::GetInstance().Initialize();

		// TextureManagerの初期化（Assets/Texture内の全.pngを自動ロード）
		MadoEngine::TextureManager::GetInstance().Initialize(dxDevice_.get()->GetDevice(), srvManager_);

		// DepthStencilBuffer の生成
		depthStencilBuffer_ = std::make_unique<MadoEngine::Core::DepthStencilBuffer>();
		depthStencilBuffer_->Initialize(dxDevice_.get(), dsvManager_, srvManager_, renderWidth_, renderHeight_);
		layerDepthStencilBuffer_ = std::make_unique<MadoEngine::Core::DepthStencilBuffer>();
		layerDepthStencilBuffer_->Initialize(dxDevice_.get(), dsvManager_, srvManager_, renderWidth_, renderHeight_);

		shadowMap_ = std::make_unique<MadoEngine::Render::ShadowMap>();
		shadowMap_->Initialize(dxDevice_.get(), dsvManager_, srvManager_);

		// ViewportScissor の初期化
		viewportScissor_ = std::make_unique<MadoEngine::Render::ViewportScissor>();
		viewportScissor_->UpdateSize(renderWidth_, renderHeight_);

		MadoEngine::SpriteManager::GetInstance().Initialize(dxDevice_->GetDevice(), commandManager_->GetCommandList(), psoRegistry_.get());
		MadoEngine::TextManager::GetInstance().Initialize(dxDevice_->GetDevice(), commandManager_->GetCommandList(), psoRegistry_.get());
		MadoEngine::ModelManager::GetInstance().Initialize(dxDevice_->GetDevice(), commandManager_->GetCommandList(), psoRegistry_.get());
		MadoEngine::Particle::ParticleSystem3d::GetInstance().Initialize(
			dxDevice_->GetDevice(),
			commandManager_->GetCommandList(),
			psoRegistry_.get()
		);
		MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().Initialize(
			dxDevice_->GetDevice(),
			commandManager_->GetCommandList(),
			psoRegistry_.get()
		);

		// Sprite/Textの座標系は実ウィンドウサイズではなく基準解像度に固定する
		MadoEngine::SpriteManager::GetInstance().SetScreenSize(static_cast<float>(winDesc_.width), static_cast<float>(winDesc_.height));
		MadoEngine::TextManager::GetInstance().SetScreenSize(static_cast<float>(winDesc_.width), static_cast<float>(winDesc_.height));

		DebugLineManager::GetInstance().Initialize(dxDevice_->GetDevice(), commandManager_->GetCommandList(), 200000);
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
		postEffectDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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
		layerEffectDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		layerEffectDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		renderTargetManager_->Create(kLayerEffectResultTarget, layerEffectDesc);
		renderTargetManager_->Create(kLayerEffectWorkTarget, layerEffectDesc);

		postEffectCopyDesc_.blendMode = MadoEngine::Render::BlendMode::None;
		postEffectCopyDesc_.depthMode = MadoEngine::Render::DepthMode::Disable;
		postEffectCopyDesc_.cullMode = MadoEngine::Render::CullMode::None;
		postEffectCopyDesc_.fillMode = MadoEngine::Render::FillMode::Solid;
		postEffectCopyDesc_.topology = MadoEngine::Render::TopologyType::Triangle;
		postEffectCopyDesc_.inputLayout = MadoEngine::Render::InputLayoutType::None;
		postEffectCopyDesc_.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		postEffectCopyDesc_.dsvFormat = DXGI_FORMAT_UNKNOWN;
		postEffectCopyDesc_.vsKey = "PostEffect/CopyImage.VS";
		postEffectCopyDesc_.psKey = "PostEffect/CopyImage.PS";
		postEffectCopyDesc_.rootSigKey = "PostEffect.RootSig";

		compositeDesc_ = postEffectCopyDesc_;
		compositeDesc_.psKey = "PostEffect/Composite.PS";
		compositeDesc_.rootSigKey = "PostEffect.Composite.RootSig";

		postEffectDefaultParameterResource_ = CreateBufferResource(
			dxDevice_->GetDevice(),
			D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT,
			false
		);
		void* defaultParameter = nullptr;
		HRESULT defaultParameterResult = postEffectDefaultParameterResource_->Map(0, nullptr, &defaultParameter);
		assert(SUCCEEDED(defaultParameterResult));
		std::memset(defaultParameter, 0, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		postEffectDefaultParameterResource_->Unmap(0, nullptr);

		postEffectManager_.Initialize(postEffectCopyDesc_, dxDevice_->GetDevice());

		MadoEngine::Editor::LoadAudioEditorJson();
		MadoEngine::Editor::LoadLightEditorJson();
		MadoEngine::Editor::LoadModelEditorJson();
		MadoEngine::Editor::LoadSpriteEditorJson();
		MadoEngine::Editor::LoadTextEditorJson();
		MadoEngine::Editor::LoadLayerEffectPassEditorJson(postEffectManager_);
		
#ifdef USE_IMGUI
		// ImGuiManagerの初期化
		imguiManager_ = std::make_unique<MadoEngine::ImGuiManager>();
		imguiManager_->Initialize(dxDevice_.get(), commandManager_.get(), srvManager_, windowsAPI_->GetHWnd(), swapChain_->GetBufferCount());
#endif // USE_IMGUI
	}

	void EngineExecution::Update() {
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

	void EngineExecution::HandleResize() {
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
		layerDepthStencilBuffer_->Resize(width, height);
		viewportScissor_->UpdateSize(width, height);
		renderTargetManager_->ResizeAll(width, height);

		renderWidth_ = width;
		renderHeight_ = height;

		// リサイズ時も2D座標系は基準解像度のまま維持する
		MadoEngine::SpriteManager::GetInstance().SetScreenSize(static_cast<float>(winDesc_.width), static_cast<float>(winDesc_.height));
		MadoEngine::TextManager::GetInstance().SetScreenSize(static_cast<float>(winDesc_.width), static_cast<float>(winDesc_.height));
		Logger::Output(
			"描画サイズを更新しました: " +
			std::to_string(renderWidth_) + "x" + std::to_string(renderHeight_),
			Logger::Level::Engine
		);
	}

	MadoEngine::Render::LayerEffectPass* EngineExecution::AddLayerEffectPass(const MadoEngine::Render::LayerEffectPass::Desc& desc) {
		return postEffectManager_.AddLayerPass(desc);
	}

	MadoEngine::Render::LayerEffectPass* EngineExecution::AddScreenEffectPass(const MadoEngine::Render::LayerEffectPass::Desc& desc) {
		return postEffectManager_.AddScreenPass(desc);
	}

	void EngineExecution::ClearLayerEffectPasses() {
		postEffectManager_.ClearLayerPasses();
	}

	void EngineExecution::ClearScreenEffectPasses() {
		postEffectManager_.ClearScreenPasses();
	}

	const std::vector<MadoEngine::Render::LayerEffectPass>& EngineExecution::GetLayerEffectPasses() const {
		return postEffectManager_.GetLayerPasses();
	}

	const MadoEngine::Render::LayerEffectPass* EngineExecution::GetFirstEnabledLayerEffectPass() const {
		return postEffectManager_.GetFirstEnabledLayerPass();
	}

	MadoEngine::Render::RenderLayerMask EngineExecution::GetEnabledLayerEffectTargetMask() const {
		return postEffectManager_.GetEnabledLayerTargetMask();
	}

	MadoEngine::Render::PostEffectManager& EngineExecution::GetPostEffectManager() {
		return postEffectManager_;
	}

	const MadoEngine::Render::PostEffectManager& EngineExecution::GetPostEffectManager() const {
		return postEffectManager_;
	}

	void EngineExecution::PreDraw(
		SceneType currentSceneType,
		const Vector3& shadowFocusPosition)
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

		RenderShadowMap(
			currentSceneType,
			shadowFocusPosition
		);

		// オフスクリーンRTへ描画する
		// オフスクリーンRTをPIXEL_SHADER_RESOURCE → RENDER_TARGET へ遷移し、RTV/DSVをセット・クリア
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencilBuffer_->GetDSVCPUHandle();
		depthStencilBuffer_->Transition(commandManager_->GetCommandList(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
		renderTargetManager_->Begin(kSceneColorTarget, commandManager_->GetCommandList(), dsvHandle);
		commandManager_->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		viewportScissor_->Apply(commandManager_->GetCommandList());
	}

	void EngineExecution::RenderShadowMap(
		SceneType currentSceneType,
		const Vector3& shadowFocusPosition) {
		assert(shadowMap_ && "ShadowMapが未初期化です");


		const std::vector<DirectionalLight> directionalLights =
			LightManager::GetInstance().GetFilteredDirectionalLights(
				currentSceneType,
				ToLightLayerMask(LightLayer::World)
			);

		if (directionalLights.empty() || directionalLights[0].useLight == 0) {
			MadoEngine::ModelManager::GetInstance().SetShadowMap(
				currentSceneType,
				{},
				Matrix::MakeIdentity(),
				shadowMap_->GetWidth(),
				shadowMap_->GetHeight()
			);
			return;
		}

		auto* commandList = commandManager_->GetCommandList();
		shadowMap_->UpdateLightViewProjection(directionalLights[0], shadowFocusPosition);
		shadowMap_->Begin(commandList);
		MadoEngine::ModelManager::GetInstance().DrawShadowMap(
			currentSceneType,
			shadowMap_->GetLightViewProjectionMatrix()
		);
		shadowMap_->End(commandList);

		MadoEngine::ModelManager::GetInstance().SetShadowMap(
			currentSceneType,
			shadowMap_->GetSRVGPUHandle(),
			shadowMap_->GetLightViewProjectionMatrix(),
			shadowMap_->GetWidth(),
			shadowMap_->GetHeight()
		);
	}

	void EngineExecution::EndSceneColorRender() {
		if (isSceneColorEnded_) {
			return;
		}

		renderTargetManager_->End(kSceneColorTarget, commandManager_->GetCommandList());
		isSceneColorEnded_ = true;
	}

	void EngineExecution::BeginLayerEffectRender(const MadoEngine::Render::LayerEffectPass& pass) {
		assert(pass.IsEnabled() && "無効なLayerEffectPassは実行できません");
		assert(pass.GetTargetLayerMask() != 0 && "LayerEffectPassの対象LayerMaskが0です");

		EndSceneColorRender();
		isLayerEffectChainResolved_ = false;
		currentLayerEffectSourceName_ = kLayerColorTarget;

		const bool ignoreDepthForMask = NeedsIgnoreDepthMask(pass.GetTargetLayerMask());
		currentLayerMaskDepthStencilBuffer_ = ignoreDepthForMask ? layerDepthStencilBuffer_.get() : depthStencilBuffer_.get();
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = currentLayerMaskDepthStencilBuffer_->GetDSVCPUHandle();
		currentLayerMaskDepthStencilBuffer_->Transition(commandManager_->GetCommandList(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
		renderTargetManager_->Begin(kLayerColorTarget, commandManager_->GetCommandList(), dsvHandle);
		if (ignoreDepthForMask) {
			commandManager_->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		}
		viewportScissor_->Apply(commandManager_->GetCommandList());
	}

	void EngineExecution::EndLayerEffectRender() {
		renderTargetManager_->End(kLayerColorTarget, commandManager_->GetCommandList());
	}

	void EngineExecution::ApplyLayerEffectAndComposite(const MadoEngine::Render::LayerEffectPass& pass) {
		ApplyLayerEffectToChain(pass);
		CompositeLayerEffectChain();
	}

	void EngineExecution::ApplyLayerEffectToChain(const MadoEngine::Render::LayerEffectPass& pass) {
		assert(pass.IsEnabled() && "無効なLayerEffectPassは実行できません");
		assert(pass.GetTargetLayerMask() != 0 && "LayerEffectPassの対象LayerMaskが0です");

		const std::string& outputTargetName = GetNextLayerEffectOutputName();
		renderTargetManager_->Begin(outputTargetName, commandManager_->GetCommandList());
		viewportScissor_->Apply(commandManager_->GetCommandList());
		DrawPostEffect(
			renderTargetManager_->GetSRVGPUHandle(currentLayerEffectSourceName_),
			pass.GetEffectPSODesc(),
			pass.GetParameterGPUVirtualAddress(),
			currentLayerMaskDepthStencilBuffer_
		);
		renderTargetManager_->End(outputTargetName, commandManager_->GetCommandList());

		currentLayerEffectSourceName_ = outputTargetName;
		isLayerEffectChainResolved_ = true;
	}

	void EngineExecution::CompositeLayerEffectChain() {
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

	void EngineExecution::ApplyScreenEffectPasses() {
		for (MadoEngine::Render::LayerEffectPass& pass : postEffectManager_.GetScreenPasses()) {
			if (!pass.IsEnabled()) {
				continue;
			}

			const std::string& outputTargetName = GetNextPostEffectOutputName();
			renderTargetManager_->Begin(outputTargetName, commandManager_->GetCommandList());
			viewportScissor_->Apply(commandManager_->GetCommandList());
			DrawPostEffect(
				renderTargetManager_->GetSRVGPUHandle(currentCompositeSourceName_),
				pass.GetEffectPSODesc(),
				pass.GetParameterGPUVirtualAddress()
			);
			renderTargetManager_->End(outputTargetName, commandManager_->GetCommandList());

			currentCompositeSourceName_ = outputTargetName;
			resolvedPostEffectTargetName_ = outputTargetName;
			isLayerEffectResolved_ = true;
		}
	}

	void EngineExecution::BeginImGuiLayout()
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
		ApplyScreenEffectPasses();

		float bbClearColor[] = { 1.0f, 0.08f, 0.08f, 1.0f };
		swapChain_->BeginRender(commandManager_->GetCommandList(), nullptr, bbClearColor);

		// エディタレイアウト（DockSpace + Game View）を描画
		// ※必ずシーンの DrawImGui() より前に呼ぶこと（DockSpaceを先に生成する必要があるため）
		imguiManager_->DrawEditorLayout(renderTargetManager_->GetSRVGPUHandle(resolvedPostEffectTargetName_));

		// エンジン情報ウィンドウ（FPS表示）
		ImGui::Begin("Engine Info");
		ImGui::Text("FPS: %.1f", deltaTime_->GetFPS());
		ImGui::Text("DeltaTime: %.4f ms", deltaTime_->GetDeltaTime() * 1000.0);
		ImGui::Checkbox("FPS Limit", &isStopApplication_);
		ImGui::End();

		//ImGui::ShowDemoWindow();

		MadoEngine::Editor::DrawLayerEffectPassEditorUI(postEffectManager_);
		MadoEngine::Editor::DrawAudioManagerUI();
		MadoEngine::Editor::DrawLightManagerEditorUI();
		MadoEngine::Editor::DrawModelManagerEditorUI();
		MadoEngine::Editor::DrawSpriteManagerEditorUI();
		MadoEngine::Editor::DrawTextManagerEditorUI();
		MadoEngine::Editor::DrawParticleSystemEditorUI();
		MadoEngine::Editor::DrawCylinderEffectEditorUI();
		imguiManager_->DrawStyleColorEditorUI();
		MadoEngine::Editor::DrawLoggerEditorUI();

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

		ApplyScreenEffectPasses();

		float bbClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		swapChain_->BeginRender(commandManager_->GetCommandList(), nullptr, bbClearColor);
		viewportScissor_->Apply(commandManager_->GetCommandList());
		DrawPostEffect(renderTargetManager_->GetSRVGPUHandle(resolvedPostEffectTargetName_), postEffectCopyDesc_);
#endif // USE_IMGUI
	}

	void EngineExecution::DrawPostEffect(
		D3D12_GPU_DESCRIPTOR_HANDLE inputSrv,
		const MadoEngine::Render::PSODesc& desc,
		D3D12_GPU_VIRTUAL_ADDRESS parameterBufferAddress,
		MadoEngine::Core::DepthStencilBuffer* maskDepthStencilBuffer)
	{
		auto* commandList = commandManager_->GetCommandList();
		commandList->SetGraphicsRootSignature(
			MadoEngine::RootSignatureManager::GetInstance().Get(desc.rootSigKey));
		commandList->SetPipelineState(psoRegistry_->Get(desc));
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->SetGraphicsRootDescriptorTable(0, inputSrv);
		if (desc.rootSigKey == "PostEffect.RootSig") {
			depthStencilBuffer_->Transition(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->SetGraphicsRootDescriptorTable(1, depthStencilBuffer_->GetSRVGPUHandle());
			MadoEngine::Core::DepthStencilBuffer* maskDepth =
				maskDepthStencilBuffer != nullptr ? maskDepthStencilBuffer : depthStencilBuffer_.get();
			maskDepth->Transition(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->SetGraphicsRootDescriptorTable(2, maskDepth->GetSRVGPUHandle());
			if (parameterBufferAddress == 0) {
				assert(postEffectDefaultParameterResource_ && "ポストエフェクト用の既定ConstantBufferが未作成です");
				parameterBufferAddress = postEffectDefaultParameterResource_->GetGPUVirtualAddress();
			}
			commandList->SetGraphicsRootConstantBufferView(3, parameterBufferAddress);

			static const uint32_t noiseTextureIndex =
				MadoEngine::TextureManager::GetInstance().GetTextureIndex("noise1");
			D3D12_GPU_DESCRIPTOR_HANDLE effectTextureSrv = inputSrv;
			if (noiseTextureIndex != UINT32_MAX) {
				effectTextureSrv = MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(noiseTextureIndex);
			}
			commandList->SetGraphicsRootDescriptorTable(4, effectTextureSrv);
		}
		commandList->DrawInstanced(3, 1, 0, 0);
	}

	bool EngineExecution::NeedsIgnoreDepthMask(Render::RenderLayerMask layerMask) const {
		return postEffectManager_.NeedsIgnoreDepthMask(layerMask);
	}

	void EngineExecution::DrawComposite(D3D12_GPU_DESCRIPTOR_HANDLE sceneSrv, D3D12_GPU_DESCRIPTOR_HANDLE effectSrv) {
		auto* commandList = commandManager_->GetCommandList();
		commandList->SetGraphicsRootSignature(
			MadoEngine::RootSignatureManager::GetInstance().Get(compositeDesc_.rootSigKey));
		commandList->SetPipelineState(psoRegistry_->Get(compositeDesc_));
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->SetGraphicsRootDescriptorTable(0, sceneSrv);
		commandList->SetGraphicsRootDescriptorTable(1, effectSrv);
		commandList->DrawInstanced(3, 1, 0, 0);
	}

	const std::string& EngineExecution::GetNextPostEffectOutputName() const {
		if (currentCompositeSourceName_ == kPostEffectResultTarget) {
			return kPostEffectWorkTarget;
		}

		return kPostEffectResultTarget;
	}

	const std::string& EngineExecution::GetNextLayerEffectOutputName() const {
		if (currentLayerEffectSourceName_ == kLayerEffectResultTarget) {
			return kLayerEffectWorkTarget;
		}

		return kLayerEffectResultTarget;
	}

	void EngineExecution::PostDraw()
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

		// 描画で参照したModelリソースはGPU処理完了後に解放する
		MadoEngine::ModelManager::GetInstance().FlushPendingDestroys();
	}

	void EngineExecution::Finalize()
	{
		// 終了処理
		MadoEngine::AudioManager::GetInstance().Finalize();
		MadoEngine::InputManager::GetInstance().Finalize();
		MadoEngine::SpriteManager::GetInstance().Finalize();
		MadoEngine::TextManager::GetInstance().Finalize();
		MadoEngine::ModelManager::GetInstance().Finalize();
		MadoEngine::Particle::ParticleSystem3d::GetInstance().Finalize();
		MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().Finalize();
		MadoEngine::TextureManager::GetInstance().Finalize();
		MadoEngine::ShaderManager::GetInstance().Finalize();
		MadoEngine::RootSignatureManager::GetInstance().Finalize();

		psoRegistry_->Finalize();
		Logger::Finalize();

#ifdef USE_IMGUI
		imguiManager_->Finalize();
#endif // USE_IMGUI

		CoUninitialize();
	}

	bool EngineExecution::IsRunning() {
		return windowsAPI_->ProcessMessage();
	}
}
