#include "Execution.h"
#include "EditorUIHeaders.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <filesystem>

namespace {

	// レンダーターゲットの識別用キー
	const std::string kSceneColorTarget = "SceneColor";               // 通常のシーン全体を最初に描く
	const std::string kPostEffectResultTarget = "PostEffectResult";   // ポストエフェクト後の結果を置く場所
	const std::string kPostEffectWorkTarget = "PostEffectWork";       // 複数回ポストエフェクトするための作業用バッファ
	const std::string kDisplayResultTarget = "DisplayResult";         // Tone Mapping後の表示用結果
	const std::string kDisplayWorkTarget = "DisplayWork";             // 表示用ポストエフェクトの作業用バッファ
	const std::string kLayerColorTarget = "LayerColor";               // 特定レイヤーだけを描く
	const std::string kLayerEffectResultTarget = "LayerEffectResult"; // レイヤー用ポストエフェクト結果
	const std::string kLayerEffectWorkTarget = "LayerEffectWork";     // レイヤー用ポストエフェクトの作業用バッファ
	const std::string kOverlayLayerColorTarget = "OverlayLayerColor";               // 表示色空間の特定レイヤーだけを描く
	const std::string kOverlayLayerEffectResultTarget = "OverlayLayerEffectResult"; // 表示色空間のレイヤー用ポストエフェクト結果
	const std::string kOverlayLayerEffectWorkTarget = "OverlayLayerEffectWork";     // 表示色空間のレイヤー用作業バッファ

	const std::filesystem::path kScreenshotOutputDirectory = "Assets/Screenshot"; // スクリーンショットの出力先ディレクトリ
	constexpr int kGameViewCaptureKey = DIK_F11;                                  // スクリーンショットを撮るキー
} // namespace

namespace MadoEngine
{
	EngineExecution::~EngineExecution() {
		Finalize();
	}

	void EngineExecution::Initialize(HINSTANCE hInstance) {

		CoInitializeEx(0, COINIT_MULTITHREADED);

		Logger::Initialize();

		// ウィンドウの設定
		winDesc_.title = "MadoEngine";
		winDesc_.width = 1280;
		winDesc_.height = 720;
		winDesc_.iconPath = "Assets/Texture/.Engine/icon.png";
		winDesc_.isResizable = false;
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
		computePsoRegistry_ = std::make_unique<MadoEngine::Render::ComputePSORegistry>();
		computePsoRegistry_->Initialize(dxDevice_->GetDevice());

		// DeltaTimeの初期化
		deltaTime_ = std::make_unique<MadoEngine::DeltaTime>();

		// InputManagerの初期化
		MadoEngine::InputManager::GetInstance().Initialize();

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
			psoRegistry_.get(),
			computePsoRegistry_.get()
		);
		MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().Initialize(
			dxDevice_->GetDevice(),
			commandManager_->GetCommandList(),
			psoRegistry_.get()
		);
		MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().Initialize(
			dxDevice_->GetDevice(),
			commandManager_->GetCommandList(),
			psoRegistry_.get()
		);
		MadoEngine::Beam::BeamEffectSystem3d::GetInstance().Initialize(
			dxDevice_->GetDevice(),
			commandManager_->GetCommandList(),
			psoRegistry_.get()
		);
		MadoEngine::EffectSequence::EffectSequenceSystem::GetInstance().Initialize();

		// SpriteとTextの座標系を実Windowサイズではなく基準解像度に固定
		MadoEngine::SpriteManager::GetInstance().SetScreenSize(static_cast<float>(winDesc_.width), static_cast<float>(winDesc_.height));
		MadoEngine::TextManager::GetInstance().SetScreenSize(static_cast<float>(winDesc_.width), static_cast<float>(winDesc_.height));

		DebugLineManager::GetInstance().Initialize(dxDevice_->GetDevice(), commandManager_->GetCommandList(), 200000);
		DebugLineManager::GetInstance().SetPSORegistry(psoRegistry_.get());

		renderTargetManager_ = std::make_unique<MadoEngine::Render::RenderTargetManager>();
		renderTargetManager_->Initialize(dxDevice_.get(), rtvManager_, srvManager_);
		gameViewCapture_ = std::make_unique<MadoEngine::Render::GameViewCapture>();
		gameViewCapture_->Initialize(
			commandManager_->GetCommandQueue(),
			kScreenshotOutputDirectory
		);

		MadoEngine::Render::RenderTargetManager::Desc sceneColorDesc{};
		sceneColorDesc.width = renderWidth_;
		sceneColorDesc.height = renderHeight_;
		sceneColorDesc.format = MadoEngine::Render::kHdrRenderTargetFormat;
		sceneColorDesc.clearColor = { 0.1f, 0.25f, 0.5f, 1.0f };
		renderTargetManager_->Create(kSceneColorTarget, sceneColorDesc);

		MadoEngine::Render::RenderTargetManager::Desc postEffectDesc{};
		postEffectDesc.width = renderWidth_;
		postEffectDesc.height = renderHeight_;
		postEffectDesc.format = MadoEngine::Render::kHdrRenderTargetFormat;
		postEffectDesc.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderTargetManager_->Create(kPostEffectResultTarget, postEffectDesc);
		renderTargetManager_->Create(kPostEffectWorkTarget, postEffectDesc);

		MadoEngine::Render::RenderTargetManager::Desc displayDesc{};
		displayDesc.width = renderWidth_;
		displayDesc.height = renderHeight_;
		displayDesc.format = MadoEngine::Render::kDisplayRenderTargetFormat;
		displayDesc.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderTargetManager_->Create(kDisplayResultTarget, displayDesc);
		renderTargetManager_->Create(kDisplayWorkTarget, displayDesc);

		MadoEngine::Render::RenderTargetManager::Desc overlayLayerDesc = displayDesc;
		overlayLayerDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		renderTargetManager_->Create(kOverlayLayerColorTarget, overlayLayerDesc);
		renderTargetManager_->Create(kOverlayLayerEffectResultTarget, overlayLayerDesc);
		renderTargetManager_->Create(kOverlayLayerEffectWorkTarget, overlayLayerDesc);

		MadoEngine::Render::RenderTargetManager::Desc layerColorDesc{};
		layerColorDesc.width = renderWidth_;
		layerColorDesc.height = renderHeight_;
		layerColorDesc.format = MadoEngine::Render::kHdrRenderTargetFormat;
		layerColorDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		renderTargetManager_->Create(kLayerColorTarget, layerColorDesc);

		MadoEngine::Render::RenderTargetManager::Desc layerEffectDesc{};
		layerEffectDesc.width = renderWidth_;
		layerEffectDesc.height = renderHeight_;
		layerEffectDesc.format = MadoEngine::Render::kHdrRenderTargetFormat;
		layerEffectDesc.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		renderTargetManager_->Create(kLayerEffectResultTarget, layerEffectDesc);
		renderTargetManager_->Create(kLayerEffectWorkTarget, layerEffectDesc);

		postEffectCopyDesc_.blendMode = MadoEngine::Render::BlendMode::None;
		postEffectCopyDesc_.depthMode = MadoEngine::Render::DepthMode::Disable;
		postEffectCopyDesc_.cullMode = MadoEngine::Render::CullMode::None;
		postEffectCopyDesc_.fillMode = MadoEngine::Render::FillMode::Solid;
		postEffectCopyDesc_.topology = MadoEngine::Render::TopologyType::Triangle;
		postEffectCopyDesc_.inputLayout = MadoEngine::Render::InputLayoutType::None;
		postEffectCopyDesc_.rtvFormat = MadoEngine::Render::kHdrRenderTargetFormat;
		postEffectCopyDesc_.dsvFormat = DXGI_FORMAT_UNKNOWN;
		postEffectCopyDesc_.vsKey = "PostEffect/CopyImage.VS";
		postEffectCopyDesc_.psKey = "PostEffect/CopyImage.PS";
		postEffectCopyDesc_.rootSigKey = "PostEffect.RootSig";

		displayCopyDesc_ = postEffectCopyDesc_;
		displayCopyDesc_.rtvFormat = MadoEngine::Render::kDisplayRenderTargetFormat;

		compositeDesc_ = postEffectCopyDesc_;
		compositeDesc_.psKey = "PostEffect/Composite.PS";
		compositeDesc_.rootSigKey = "PostEffect.Composite.RootSig";

		fallbackToneMappingDesc_ = displayCopyDesc_;
		fallbackToneMappingDesc_.psKey = "PostEffect/ToneMapping.PS";

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

		MadoEngine::Render::PostEffectManager& postEffectManager =
			MadoEngine::Render::PostEffectManager::GetInstance();
		postEffectManager.Initialize(postEffectCopyDesc_, dxDevice_->GetDevice());

		MadoEngine::Editor::LoadAudioEditorJson();
		MadoEngine::Editor::LoadLightEditorJson();
		MadoEngine::Editor::LoadPostEffectEditorJson(postEffectManager);
		
#ifdef USE_IMGUI

		// ImGuiManagerの初期化
		imguiManager_ = std::make_unique<MadoEngine::ImGuiManager>();
		imguiManager_->Initialize(dxDevice_.get(), commandManager_.get(), srvManager_, windowsAPI_->GetHWnd(), swapChain_->GetBufferCount());
#endif // USE_IMGUI
		isInitialized_ = true;
	}

	void EngineExecution::Update() {

		// デルタタイムを計算
		deltaTime_->Update();
		float dt = static_cast<float>(deltaTime_->GetDeltaTime());
		MadoEngine::Render::PostEffectManager::GetInstance().UpdateRuntimeParameters(dt);

		// ウィンドウリサイズ要求があれば描画リソースへ反映
		HandleResize();

		// AudioManagerの更新（終了した音声のクリーンアップなど）
		MadoEngine::AudioManager::GetInstance().Update();

		// InputManagerの更新（キーボード、マウス、ゲームパッドの状態を更新）
		MadoEngine::InputManager::GetInstance().Update(windowsAPI_->GetHWnd(), dt);
		if (MadoEngine::InputManager::GetInstance().GetKeybord()->IsTrigger(kGameViewCaptureKey)) {
			isGameViewCaptureRequested_ = true;
		}

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
		MadoEngine::Particle::ParticleSystem3d::GetInstance().OnGpuFrameCompleted(
			commandManager_->GetCompletedFenceValue()
		);
		MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().OnGpuFrameCompleted(
			commandManager_->GetCompletedFenceValue()
		);
		MadoEngine::Beam::BeamEffectSystem3d::GetInstance().OnGpuFrameCompleted(
			commandManager_->GetCompletedFenceValue()
		);
		swapChain_->Resize(width, height);
		depthStencilBuffer_->Resize(width, height);
		layerDepthStencilBuffer_->Resize(width, height);
		viewportScissor_->UpdateSize(width, height);
		renderTargetManager_->ResizeAll(width, height);

		renderWidth_ = width;
		renderHeight_ = height;

		// Resize後も2D座標系を基準解像度のまま維持
		MadoEngine::SpriteManager::GetInstance().SetScreenSize(static_cast<float>(winDesc_.width), static_cast<float>(winDesc_.height));
		MadoEngine::TextManager::GetInstance().SetScreenSize(static_cast<float>(winDesc_.width), static_cast<float>(winDesc_.height));
		Logger::Output(
			"描画サイズを更新しました: " +
			std::to_string(renderWidth_) + "x" + std::to_string(renderHeight_),
			Logger::Level::Engine
		);
	}

	MadoEngine::Render::PostEffectPassHandle EngineExecution::AddLayerEffectPass(
		const MadoEngine::Render::LayerPostEffectPassCreateDesc& desc)
	{
		return MadoEngine::Render::PostEffectManager::GetInstance().CreateLayerPass(desc);
	}

	MadoEngine::Render::PostEffectPassHandle EngineExecution::AddScreenEffectPass(
		const MadoEngine::Render::ScreenPostEffectPassCreateDesc& desc)
	{
		return MadoEngine::Render::PostEffectManager::GetInstance().CreateScreenPass(desc);
	}

	void EngineExecution::ClearLayerEffectPasses() {
		MadoEngine::Render::PostEffectManager::GetInstance().ClearLayerPasses();
	}

	void EngineExecution::ClearScreenEffectPasses() {
		MadoEngine::Render::PostEffectManager::GetInstance().ClearScreenPasses();
	}

	const std::vector<MadoEngine::Render::PostEffectPassHandle>& EngineExecution::GetLayerEffectPassHandles() const {
		return MadoEngine::Render::PostEffectManager::GetInstance().GetLayerPassHandles();
	}

	const MadoEngine::Render::PostEffectPass* EngineExecution::TryGetPostEffectPass(
		MadoEngine::Render::PostEffectPassHandle handle) const
	{
		return MadoEngine::Render::PostEffectManager::GetInstance().TryGet(handle);
	}

	MadoEngine::Render::RenderLayerMask EngineExecution::GetEnabledLayerEffectTargetMask() const {
		return MadoEngine::Render::PostEffectManager::GetInstance().GetEnabledLayerTargetMask();
	}

	MadoEngine::Render::RenderLayerMask EngineExecution::GetEnabledLayerEffectTargetMask(
		MadoEngine::Render::LayerEffectStage stage) const
	{
		return MadoEngine::Render::PostEffectManager::GetInstance().GetEnabledLayerTargetMask(stage);
	}

	MadoEngine::Render::PostEffectManager& EngineExecution::GetPostEffectManager() {
		return MadoEngine::Render::PostEffectManager::GetInstance();
	}

	const MadoEngine::Render::PostEffectManager& EngineExecution::GetPostEffectManager() const {
		return MadoEngine::Render::PostEffectManager::GetInstance();
	}

	void EngineExecution::PreDraw(
		SceneType currentSceneType,
		const Vector3& shadowFocusPosition)
	{
		currentSceneType_ = currentSceneType;
#ifdef USE_IMGUI
		MadoEngine::Editor::ApplyPendingPostEffectEditorOperations(
			MadoEngine::Render::PostEffectManager::GetInstance());
#endif // USE_IMGUI

		isSceneColorEnded_ = false;
		isLayerEffectChainResolved_ = false;
		isLayerEffectResolved_ = false;
		isTransparentRenderActive_ = false;
		isOverlayRenderActive_ = false;
		isSceneScreenEffectStageApplied_ = false;
		isFinalScreenEffectStageApplied_ = false;
		isToneMapped_ = false;
		currentCompositeSourceName_ = kSceneColorTarget;
		resolvedPostEffectTargetName_ = kPostEffectResultTarget;
		currentLayerEffectSourceName_ = kLayerColorTarget;
		currentLayerColorTargetName_ = kLayerColorTarget;
		currentTransparentTargetName_.clear();
		currentOverlayTargetName_.clear();
		isCurrentLayerEffectDisplay_ = false;

#ifdef USE_IMGUI

		// ImGuiフレーム開始
		imguiManager_->Begin();
#endif // USE_IMGUI

		// CommandListを開く（記録開始）
		commandManager_->BeginFrame();

		// SRV用DescriptorHeapをセット（テクスチャ参照に必須）
		ID3D12DescriptorHeap* heaps[] = { srvManager_->GetDescriptorHeap() };
		commandManager_->GetCommandList()->SetDescriptorHeaps(1, heaps);
		MadoEngine::Particle::ParticleSystem3d::GetInstance().RecordGpuSimulation(
			commandManager_->GetNextFenceValue()
		);
		MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().BeginFrame(
			commandManager_->GetNextFenceValue()
		);
		MadoEngine::Beam::BeamEffectSystem3d::GetInstance().BeginFrame(
			commandManager_->GetNextFenceValue()
		);

		RenderShadowMap(
			currentSceneType,
			shadowFocusPosition
		);

		// PostEffect適用前のSceneをOffscreen RenderTargetへ集約
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

		// World Layerの先頭Directional LightをShadow生成の代表Lightとして採用
		const std::vector<DirectionalLight> directionalLights =
			LightManager::GetInstance().GetFilteredDirectionalLights(
				currentSceneType,
				ToLightLayerMask(LightLayer::World)
			);

		if (directionalLights.empty() || directionalLights[0].useLight == 0) {

			// 有効Lightがない場合もModel側へ空Handleを通知して前FrameのShadow参照を解除
			MadoEngine::ModelManager::GetInstance().SetShadowMap(
				currentSceneType,
				{},
				Matrix::MakeIdentity(),
				shadowMap_->GetWidth(),
				shadowMap_->GetHeight()
			);
			return;
		}

		// Light空間のDepthを描画し、同じ行列とSRVを後続のModel描画へ公開
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

	void EngineExecution::BeginLayerEffectRender(const MadoEngine::Render::PostEffectPass& pass) {
		assert(pass.IsEnabled() && "無効なLayer向けPostEffectPassは実行できません");
		assert(pass.GetTargetLayerMask() != 0 && "Layer向けPostEffectPassの対象LayerMaskが0です");

		EndSceneColorRender();
		const MadoEngine::Render::LayerEffectStage stage = pass.GetLayerEffectStage();
		isLayerEffectChainResolved_ = false;
		isCurrentLayerEffectDisplay_ = stage == MadoEngine::Render::LayerEffectStage::Overlay;
		currentLayerColorTargetName_ = isCurrentLayerEffectDisplay_
			? kOverlayLayerColorTarget
			: kLayerColorTarget;
		currentLayerEffectSourceName_ = currentLayerColorTargetName_;
		assert((!isCurrentLayerEffectDisplay_ || isToneMapped_) &&
			"OverlayのLayer EffectはTone Mapping後に実行してください");

		// OverlayまたはDepth無視LayerではScene Depthと分離したMask用Depthを選択
		const bool ignoreDepthForMask = stage == MadoEngine::Render::LayerEffectStage::Overlay ||
			NeedsIgnoreDepthMask(pass.GetTargetLayerMask(), stage);
		currentLayerMaskDepthStencilBuffer_ = ignoreDepthForMask ? layerDepthStencilBuffer_.get() : depthStencilBuffer_.get();
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = currentLayerMaskDepthStencilBuffer_->GetDSVCPUHandle();
		currentLayerMaskDepthStencilBuffer_->Transition(commandManager_->GetCommandList(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
		renderTargetManager_->Begin(currentLayerColorTargetName_, commandManager_->GetCommandList(), dsvHandle);
		if (ignoreDepthForMask) {
			commandManager_->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		}
		viewportScissor_->Apply(commandManager_->GetCommandList());
	}

	void EngineExecution::EndLayerEffectRender() {
		renderTargetManager_->End(currentLayerColorTargetName_, commandManager_->GetCommandList());
	}

	void EngineExecution::ApplyLayerEffectAndComposite(const MadoEngine::Render::PostEffectPass& pass) {
		ApplyLayerEffectToChain(pass);
		CompositeLayerEffectChain();
	}

	void EngineExecution::ApplyLayerEffectToChain(const MadoEngine::Render::PostEffectPass& pass) {
		assert(pass.IsEnabled() && "無効なLayer向けPostEffectPassは実行できません");
		assert(pass.GetTargetLayerMask() != 0 && "Layer向けPostEffectPassの対象LayerMaskが0です");

		const std::string& outputTargetName = GetNextLayerEffectOutputName();
		renderTargetManager_->Begin(outputTargetName, commandManager_->GetCommandList());
		viewportScissor_->Apply(commandManager_->GetCommandList());
		MadoEngine::Render::PSODesc effectDesc = pass.GetEffectPSODesc();
		effectDesc.rtvFormat = isCurrentLayerEffectDisplay_
			? MadoEngine::Render::kDisplayRenderTargetFormat
			: MadoEngine::Render::kHdrRenderTargetFormat;
		DrawPostEffect(
			renderTargetManager_->GetSRVGPUHandle(currentLayerEffectSourceName_),
			effectDesc,
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

	void EngineExecution::ApplyScreenEffectPass(const MadoEngine::Render::PostEffectPass& pass) {
		const std::optional<MadoEngine::Render::PostEffectType> effectType = pass.GetPostEffectType();
		const bool isToneMappingPass = effectType == MadoEngine::Render::PostEffectType::ToneMapping;
		if (isToneMappingPass && isToneMapped_) {
			return;
		}

		// Tone Mapping Passから後続の出力先をHDR Targetから表示用Targetへ切り替え
		if (isToneMappingPass) {
			isToneMapped_ = true;
		}

		const std::string& outputTargetName = GetNextPostEffectOutputName();
		renderTargetManager_->Begin(outputTargetName, commandManager_->GetCommandList());
		viewportScissor_->Apply(commandManager_->GetCommandList());
		MadoEngine::Render::PSODesc effectDesc = pass.GetEffectPSODesc();
		effectDesc.rtvFormat = isToneMapped_
			? MadoEngine::Render::kDisplayRenderTargetFormat
			: MadoEngine::Render::kHdrRenderTargetFormat;
		DrawPostEffect(
			renderTargetManager_->GetSRVGPUHandle(currentCompositeSourceName_),
			effectDesc,
			pass.GetParameterGPUVirtualAddress()
		);
		renderTargetManager_->End(outputTargetName, commandManager_->GetCommandList());

		currentCompositeSourceName_ = outputTargetName;
		resolvedPostEffectTargetName_ = outputTargetName;
		isLayerEffectResolved_ = true;
	}

	void EngineExecution::ApplyScreenEffectPasses(MadoEngine::Render::ScreenEffectStage stage) {
		assert(MadoEngine::Render::IsValidScreenEffectStage(stage) && "ScreenEffectStageが範囲外です");
		if (stage == MadoEngine::Render::ScreenEffectStage::Scene && isSceneScreenEffectStageApplied_) {
			return;
		}
		if (stage == MadoEngine::Render::ScreenEffectStage::Final && isFinalScreenEffectStageApplied_) {
			return;
		}

		// 各Stageを一Frame一度だけ適用して重複呼び出しによるEffectの多重化を防止
		EndSceneColorRender();
		MadoEngine::Render::PostEffectManager& manager =
			MadoEngine::Render::PostEffectManager::GetInstance();
		std::vector<const MadoEngine::Render::PostEffectPass*> deferredFXAAPasses;
		std::vector<const MadoEngine::Render::PostEffectPass*> deferredCRTPasses;

		// 出力先を交互に切り替えて前Passの結果を次Passの入力へ接続
		for (MadoEngine::Render::PostEffectPassHandle handle : manager.GetScreenPassHandles()) {
			const MadoEngine::Render::PostEffectPass* pass = manager.TryGet(handle);
			if (!pass || !pass->IsEnabled() || pass->GetScreenEffectStage() != stage) {
				continue;
			}
			const std::optional<MadoEngine::Render::PostEffectType> effectType = pass->GetPostEffectType();
			const bool isFXAAPass = effectType == MadoEngine::Render::PostEffectType::FXAA;
			const bool isCRTPass = effectType == MadoEngine::Render::PostEffectType::CRT;
			if (stage == MadoEngine::Render::ScreenEffectStage::Final && isCRTPass) {

				// CRTのピクセルパターンを後続Effectで平滑化しないようFinal Chain末尾まで延期
				deferredCRTPasses.push_back(pass);
				continue;
			}
			if (stage == MadoEngine::Render::ScreenEffectStage::Final && isFXAAPass && !isToneMapped_) {

				// FXAAをHDR色へ適用しないようTone Mappingの完了まで登録順を保持して延期
				deferredFXAAPasses.push_back(pass);
				continue;
			}

			ApplyScreenEffectPass(*pass);
			if (isToneMapped_ && !deferredFXAAPasses.empty()) {

				// 明示Tone Mapping直後へ前方のFXAAを移動して表示色空間での輪郭判定を保証
				for (const MadoEngine::Render::PostEffectPass* deferredPass : deferredFXAAPasses) {
					ApplyScreenEffectPass(*deferredPass);
				}
				deferredFXAAPasses.clear();
			}
		}

		if (stage == MadoEngine::Render::ScreenEffectStage::Final) {

			// Final EffectのHDR結果をUI合成前に表示色空間へ必ず変換
			EnsureToneMappedCompositeSource();
			for (const MadoEngine::Render::PostEffectPass* deferredPass : deferredFXAAPasses) {
				ApplyScreenEffectPass(*deferredPass);
			}

			// CRTをTone MappingとFXAAの完了後へ固定して表示ピクセル基準の模様を維持
			for (const MadoEngine::Render::PostEffectPass* deferredPass : deferredCRTPasses) {
				ApplyScreenEffectPass(*deferredPass);
			}
		}

		if (stage == MadoEngine::Render::ScreenEffectStage::Scene) {
			isSceneScreenEffectStageApplied_ = true;
		} else {
			isFinalScreenEffectStageApplied_ = true;
		}
	}

	void EngineExecution::BeginTransparentRender() {
		assert(!isTransparentRenderActive_ && "透明オブジェクト描画が既に開始されています");
		assert(!isOverlayRenderActive_ && "Overlay描画中に透明オブジェクト描画は開始できません");
		assert(!isFinalScreenEffectStageApplied_ && "Final段階の後に透明オブジェクト描画は開始できません");

		ApplyScreenEffectPasses(MadoEngine::Render::ScreenEffectStage::Scene);
		EndSceneColorRender();

		// Composite済みSceneを新しいTargetへ複製して透明描画のBlend先を準備
		currentTransparentTargetName_ = GetNextPostEffectOutputName();
		ID3D12GraphicsCommandList* commandList = commandManager_->GetCommandList();
		renderTargetManager_->Begin(currentTransparentTargetName_, commandList);
		viewportScissor_->Apply(commandList);
		DrawPostEffect(
			renderTargetManager_->GetSRVGPUHandle(currentCompositeSourceName_),
			postEffectCopyDesc_
		);

		depthStencilBuffer_->Transition(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
			renderTargetManager_->GetRTVCPUHandle(currentTransparentTargetName_);
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthStencilBuffer_->GetDSVCPUHandle();
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		viewportScissor_->Apply(commandList);

		UpdateParticleFogParameters();
		isTransparentRenderActive_ = true;
	}

	void EngineExecution::EndTransparentRender() {
		assert(isTransparentRenderActive_ && "透明オブジェクト描画が開始されていません");

		renderTargetManager_->End(currentTransparentTargetName_, commandManager_->GetCommandList());
		currentCompositeSourceName_ = currentTransparentTargetName_;
		resolvedPostEffectTargetName_ = currentTransparentTargetName_;
		isLayerEffectResolved_ = true;
		isTransparentRenderActive_ = false;
	}

	void EngineExecution::BeginOverlayRender() {
		assert(!isTransparentRenderActive_ && "透明オブジェクト描画を終了してからOverlay描画を開始してください");
		assert(!isOverlayRenderActive_ && "Overlay描画が既に開始されています");

		ApplyScreenEffectPasses(MadoEngine::Render::ScreenEffectStage::Scene);
		ApplyScreenEffectPasses(MadoEngine::Render::ScreenEffectStage::Final);
		EnsureToneMappedCompositeSource();
		EndSceneColorRender();
		currentOverlayTargetName_ = GetNextPostEffectOutputName();
		renderTargetManager_->Begin(currentOverlayTargetName_, commandManager_->GetCommandList());
		viewportScissor_->Apply(commandManager_->GetCommandList());
		DrawPostEffect(
			renderTargetManager_->GetSRVGPUHandle(currentCompositeSourceName_),
			displayCopyDesc_
		);
		isOverlayRenderActive_ = true;
	}

	void EngineExecution::EndOverlayRender() {
		assert(isOverlayRenderActive_ && "Overlay描画が開始されていません");

		renderTargetManager_->End(currentOverlayTargetName_, commandManager_->GetCommandList());
		currentCompositeSourceName_ = currentOverlayTargetName_;
		resolvedPostEffectTargetName_ = currentOverlayTargetName_;
		isLayerEffectResolved_ = true;
		isOverlayRenderActive_ = false;
	}

	void EngineExecution::ResolveCompositeSource() {
		if (isLayerEffectResolved_) {
			return;
		}

		EndSceneColorRender();
		const std::string& outputTargetName = GetNextPostEffectOutputName();
		renderTargetManager_->Begin(outputTargetName, commandManager_->GetCommandList());
		viewportScissor_->Apply(commandManager_->GetCommandList());
		DrawPostEffect(
			renderTargetManager_->GetSRVGPUHandle(currentCompositeSourceName_),
			isToneMapped_ ? displayCopyDesc_ : postEffectCopyDesc_
		);
		renderTargetManager_->End(outputTargetName, commandManager_->GetCommandList());
		currentCompositeSourceName_ = outputTargetName;
		resolvedPostEffectTargetName_ = outputTargetName;
		isLayerEffectResolved_ = true;
	}

	void EngineExecution::EnsureToneMappedCompositeSource() {
		if (isToneMapped_) {
			return;
		}

		EndSceneColorRender();
		isToneMapped_ = true;
		const std::string& outputTargetName = GetNextPostEffectOutputName();
		renderTargetManager_->Begin(outputTargetName, commandManager_->GetCommandList());
		viewportScissor_->Apply(commandManager_->GetCommandList());

		// 明示的なTone Mapping Passがない場合も同じShader既定値で表示可能範囲へ変換
		DrawPostEffect(
			renderTargetManager_->GetSRVGPUHandle(currentCompositeSourceName_),
			fallbackToneMappingDesc_
		);
		renderTargetManager_->End(outputTargetName, commandManager_->GetCommandList());
		currentCompositeSourceName_ = outputTargetName;
		resolvedPostEffectTargetName_ = outputTargetName;
		isLayerEffectResolved_ = true;
	}

	void EngineExecution::BeginImGuiLayout() {
		assert(!isTransparentRenderActive_ && "透明オブジェクト描画を終了してからImGuiレイアウトを開始してください");
		assert(!isOverlayRenderActive_ && "Overlay描画を終了してからImGuiレイアウトを開始してください");

		ApplyScreenEffectPasses(MadoEngine::Render::ScreenEffectStage::Scene);
		ApplyScreenEffectPasses(MadoEngine::Render::ScreenEffectStage::Final);
		EnsureToneMappedCompositeSource();
		ResolveCompositeSource();

#ifdef USE_IMGUI

		// バックバッファをRENDER_TARGETに遷移し、ImGui描画先に設定・クリア
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

		MadoEngine::Editor::DrawPostEffectEditorUI(
			MadoEngine::Render::PostEffectManager::GetInstance());
		MadoEngine::Editor::DrawAudioManagerUI();
		MadoEngine::Editor::DrawLightManagerEditorUI();
		MadoEngine::Editor::DrawModelManagerEditorUI(currentSceneType_);
		MadoEngine::Editor::DrawSpriteManagerEditorUI(currentSceneType_);
		MadoEngine::Editor::DrawTextManagerEditorUI(currentSceneType_);
		MadoEngine::Editor::DrawParticleSystemEditorUI();
		MadoEngine::Editor::DrawCylinderEffectEditorUI();
		MadoEngine::Editor::DrawRibbonEffectEditorUI();
		MadoEngine::Editor::DrawBeamEffectEditorUI();
		MadoEngine::Editor::DrawEffectSequenceEditorUI();

		imguiManager_->DrawStyleColorEditorUI();

#else
		float bbClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		swapChain_->BeginRender(commandManager_->GetCommandList(), nullptr, bbClearColor);
		viewportScissor_->Apply(commandManager_->GetCommandList());
		DrawPostEffect(renderTargetManager_->GetSRVGPUHandle(resolvedPostEffectTargetName_), displayCopyDesc_);
#endif // USE_IMGUI
	}

	void EngineExecution::DrawPostEffect(
		D3D12_GPU_DESCRIPTOR_HANDLE inputSrv,
		const MadoEngine::Render::PSODesc& desc,
		D3D12_GPU_VIRTUAL_ADDRESS parameterBufferAddress,
		MadoEngine::Core::DepthStencilBuffer* maskDepthStencilBuffer)
	{
		auto* commandList = commandManager_->GetCommandList();

		// Pass定義のRootSignatureとPSOへFullscreen Triangle用の共通入力を設定
		commandList->SetGraphicsRootSignature(
			MadoEngine::RootSignatureManager::GetInstance().Get(desc.rootSigKey));
		commandList->SetPipelineState(psoRegistry_->Get(desc));
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->SetGraphicsRootDescriptorTable(0, inputSrv);
		if (desc.rootSigKey == "PostEffect.RootSig") {

			// PostEffect共通RootSignatureだけDepth、Mask、Parameter、補助Textureを追加Binding
			depthStencilBuffer_->Transition(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->SetGraphicsRootDescriptorTable(1, depthStencilBuffer_->GetSRVGPUHandle());
			MadoEngine::Core::DepthStencilBuffer* maskDepth =
				maskDepthStencilBuffer != nullptr ? maskDepthStencilBuffer : depthStencilBuffer_.get();
			maskDepth->Transition(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->SetGraphicsRootDescriptorTable(2, maskDepth->GetSRVGPUHandle());
			if (parameterBufferAddress == 0) {

				// Parameter未指定Passでも有効なConstant BufferをBindingする既定値Fallback
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

	bool EngineExecution::NeedsIgnoreDepthMask(
		Render::RenderLayerMask layerMask,
		Render::LayerEffectStage stage) const
	{
		return MadoEngine::Render::PostEffectManager::GetInstance().NeedsIgnoreDepthMask(layerMask, stage);
	}

	void EngineExecution::UpdateParticleFogParameters() {
		MadoEngine::Particle::ParticleFogParameters parameters{};
		MadoEngine::Render::PostEffectManager& manager =
			MadoEngine::Render::PostEffectManager::GetInstance();

		// Scene Stageで有効なFog Passの設定をParticle描画用Parameterへ同期
		for (MadoEngine::Render::PostEffectPassHandle handle : manager.GetScreenPassHandles()) {
			const MadoEngine::Render::PostEffectPass* pass = manager.TryGet(handle);
			if (!pass || !pass->IsEnabled() ||
				pass->GetScreenEffectStage() != MadoEngine::Render::ScreenEffectStage::Scene ||
				pass->GetPostEffectType() != MadoEngine::Render::PostEffectType::Fog) {
				continue;
			}

			MadoEngine::Render::FogParameters fogParameters{};
			if (!manager.TryGetParameters(handle, fogParameters)) {
				continue;
			}

			parameters.color = fogParameters.color;
			if (parameters.color == Vector4{}) {

				// 旧設定や未初期化値を描画可能なFog既定値へ補完
				parameters.color = { 0.58f, 0.68f, 0.74f, 1.0f };
			}

			parameters.distanceParams = {
				fogParameters.startDistance,
				fogParameters.endDistance,
				fogParameters.density,
				fogParameters.heightStrength,
			};
			if (parameters.distanceParams == Vector4{}) {
				parameters.distanceParams = { 850.0f, 1000.0f, 1.0f, 0.0f };
			}

			parameters.cameraParams = {
				fogParameters.nearClip,
				fogParameters.farClip,
				0.0f,
				0.0f,
			};
			if (parameters.cameraParams == Vector4{}) {
				parameters.cameraParams = { 0.1f, 1000.0f, 0.0f, 0.0f };
			}
			parameters.cameraParams.z = 1.0f;
		}

		MadoEngine::Particle::ParticleSystem3d::GetInstance().SetFogParameters(parameters);
	}

	void EngineExecution::DrawComposite(D3D12_GPU_DESCRIPTOR_HANDLE sceneSrv, D3D12_GPU_DESCRIPTOR_HANDLE effectSrv) {
		auto* commandList = commandManager_->GetCommandList();
		MadoEngine::Render::PSODesc compositeDesc = compositeDesc_;
		compositeDesc.rtvFormat = isToneMapped_
			? MadoEngine::Render::kDisplayRenderTargetFormat
			: MadoEngine::Render::kHdrRenderTargetFormat;
		commandList->SetGraphicsRootSignature(
			MadoEngine::RootSignatureManager::GetInstance().Get(compositeDesc.rootSigKey));
		commandList->SetPipelineState(psoRegistry_->Get(compositeDesc));
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->SetGraphicsRootDescriptorTable(0, sceneSrv);
		commandList->SetGraphicsRootDescriptorTable(1, effectSrv);
		commandList->DrawInstanced(3, 1, 0, 0);
	}

	const std::string& EngineExecution::GetNextPostEffectOutputName() const {
		if (isToneMapped_) {
			if (currentCompositeSourceName_ == kDisplayResultTarget) {
				return kDisplayWorkTarget;
			}

			return kDisplayResultTarget;
		}

		if (currentCompositeSourceName_ == kPostEffectResultTarget) {
			return kPostEffectWorkTarget;
		}

		return kPostEffectResultTarget;
	}

	const std::string& EngineExecution::GetNextLayerEffectOutputName() const {
		if (isCurrentLayerEffectDisplay_) {
			if (currentLayerEffectSourceName_ == kOverlayLayerEffectResultTarget) {
				return kOverlayLayerEffectWorkTarget;
			}

			return kOverlayLayerEffectResultTarget;
		}

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
		MadoEngine::Particle::ParticleSystem3d::GetInstance().OnGpuFrameCompleted(
			commandManager_->GetCompletedFenceValue()
		);
		MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().OnGpuFrameCompleted(
			commandManager_->GetCompletedFenceValue()
		);
		MadoEngine::Beam::BeamEffectSystem3d::GetInstance().OnGpuFrameCompleted(
			commandManager_->GetCompletedFenceValue()
		);

		// 描画で参照したResourceをGPU処理完了後に解放
		MadoEngine::SpriteManager::GetInstance().FlushPendingDestroys();
		MadoEngine::TextManager::GetInstance().FlushPendingDestroys();
		MadoEngine::ModelManager::GetInstance().FlushPendingDestroys();
		MadoEngine::Render::PostEffectManager::GetInstance().FlushPendingDestroys();

		if (isGameViewCaptureRequested_) {
			const MadoEngine::Render::RenderTexture* gameViewTexture =
				renderTargetManager_->Get(resolvedPostEffectTargetName_);
			(void)gameViewCapture_->Capture(*gameViewTexture);
			isGameViewCaptureRequested_ = false;
		}
	}

	void EngineExecution::Finalize()
	{
		if (!isInitialized_) {
			return;
		}
		isInitialized_ = false;
		commandManager_->WaitForGPU();
		MadoEngine::Particle::ParticleSystem3d::GetInstance().OnGpuFrameCompleted(
			commandManager_->GetCompletedFenceValue()
		);
		MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().OnGpuFrameCompleted(
			commandManager_->GetCompletedFenceValue()
		);
		MadoEngine::Beam::BeamEffectSystem3d::GetInstance().OnGpuFrameCompleted(
			commandManager_->GetCompletedFenceValue()
		);

		// 終了処理
		MadoEngine::AudioManager::GetInstance().Finalize();
		MadoEngine::InputManager::GetInstance().Finalize();
		MadoEngine::SpriteManager::GetInstance().Finalize();
		MadoEngine::TextManager::GetInstance().Finalize();
		MadoEngine::ModelManager::GetInstance().Finalize();
		MadoEngine::EffectSequence::EffectSequenceSystem::GetInstance().Finalize();
		MadoEngine::Particle::ParticleSystem3d::GetInstance().Finalize();
		MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().Finalize();
		MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().Finalize();
		MadoEngine::Beam::BeamEffectSystem3d::GetInstance().Finalize();
		MadoEngine::Render::PostEffectManager::GetInstance().Finalize();
		computePsoRegistry_->Finalize();
		psoRegistry_->Finalize();
		MadoEngine::TextureManager::GetInstance().Finalize();
		MadoEngine::ShaderManager::GetInstance().Finalize();
		MadoEngine::RootSignatureManager::GetInstance().Finalize();

#ifdef USE_IMGUI
		imguiManager_->Finalize();
#endif // USE_IMGUI

		Logger::Finalize();
		CoUninitialize();
	}

	bool EngineExecution::IsRunning() {
		return windowsAPI_->ProcessMessage();
	}
}
