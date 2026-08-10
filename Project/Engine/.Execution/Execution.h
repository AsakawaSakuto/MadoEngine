#pragma once
#include <Windows.h>
#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <d3d12.h>
#include <wrl/client.h>
#include "CoreHeaders.h"
#include "RenderHeaders.h"
#include "Render/PostEffectManager.h"
#include "Render/PSO/ComputePSORegistry.h"
#include "UtilityHeaders.h"
#include ".SceneManager/SceneType.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#include "Render/ImGui/ImGuiManager.h"
#endif

namespace MadoEngine
{
	/// @brief ゲームエンジンの実行制御を管理するクラス
	class EngineExecution
	{
	public:
		~EngineExecution();

		/// @brief 初期化処理
		void Initialize(HINSTANCE hInstance);

		/// @brief 更新処理
		void Update();

		/// @brief 描画前処理
		/// @param currentSceneType 現在のシーン種別
		/// @param shadowFocusPosition シャドウマップの中心へ置くワールド座標
		void PreDraw(
			SceneType currentSceneType,
			const Vector3& shadowFocusPosition
		);

		/// @brief Layer向けPostEffectPassを登録する
		/// @param desc 個別描画レイヤー向けPassの生成設定
		/// @return 登録したPassのHandle
		[[nodiscard]] MadoEngine::Render::PostEffectPassHandle AddLayerEffectPass(
			const MadoEngine::Render::LayerPostEffectPassCreateDesc& desc
		);

		/// @brief 画面全体に適用するポストエフェクトPassを登録する
		/// @param desc フルスクリーン向けPassの生成設定
		/// @return 登録したPassのHandle
		[[nodiscard]] MadoEngine::Render::PostEffectPassHandle AddScreenEffectPass(
			const MadoEngine::Render::ScreenPostEffectPassCreateDesc& desc
		);

		/// @brief 登録済みのLayer向けPostEffectPassをすべて削除する
		void ClearLayerEffectPasses();

		/// @brief 登録済みの画面全体ポストエフェクトPassをすべて削除する
		void ClearScreenEffectPasses();

		/// @brief 登録済みのLayer向けPostEffectPassの実行順Handle一覧を取得する
		/// @return 登録済みのLayer向けPostEffectPassの実行順Handle一覧
		const std::vector<MadoEngine::Render::PostEffectPassHandle>& GetLayerEffectPassHandles() const;

		/// @brief HandleからPassを描画処理中だけ使用する一時参照として取得する
		/// @param handle 取得対象のHandle
		/// @return 有効な場合はPass、無効な場合はnullptr
		const MadoEngine::Render::PostEffectPass* TryGetPostEffectPass(
			MadoEngine::Render::PostEffectPassHandle handle
		) const;

		/// @brief 有効なLayer向けPostEffectPassの対象Layerをまとめたマスクを取得する
		/// @return 有効なLayer向けPostEffectPassの対象Layerマスク
		MadoEngine::Render::RenderLayerMask GetEnabledLayerEffectTargetMask() const;

		/// @brief 指定段階で有効なLayer向けPostEffectPassの対象Layerをまとめたマスクを取得する
		/// @param stage 対象の適用段階
		/// @return 指定段階で有効なLayer向けPostEffectPassの対象Layerマスク
		MadoEngine::Render::RenderLayerMask GetEnabledLayerEffectTargetMask(
			MadoEngine::Render::LayerEffectStage stage
		) const;

		/// @brief ポストエフェクト管理クラスを取得する
		/// @return ポストエフェクト管理クラス
		MadoEngine::Render::PostEffectManager& GetPostEffectManager();

		/// @brief ポストエフェクト管理クラスを取得する
		/// @return ポストエフェクト管理クラス
		const MadoEngine::Render::PostEffectManager& GetPostEffectManager() const;

		/// @brief シーンカラーRenderTargetへの描画を終了する
		void EndSceneColorRender();

		/// @brief ポストエフェクト対象Layer用RenderTargetへの描画を開始する
		/// @param pass 実行するLayer向けPostEffectPass
		void BeginLayerEffectRender(const MadoEngine::Render::PostEffectPass& pass);

		/// @brief ポストエフェクト対象Layer用RenderTargetへの描画を終了する
		void EndLayerEffectRender();

		/// @brief 対象Layerのポストエフェクト結果をシーンへ合成する
		/// @param pass 実行するLayer向けPostEffectPass
		void ApplyLayerEffectAndComposite(const MadoEngine::Render::PostEffectPass& pass);

		/// @brief 対象Layerの現在のチェーン結果へポストエフェクトを適用する
		/// @param pass 実行するLayer向けPostEffectPass
		void ApplyLayerEffectToChain(const MadoEngine::Render::PostEffectPass& pass);

		/// @brief 対象Layerのエフェクトチェーン結果を現在の合成済み画像へ合成する
		void CompositeLayerEffectChain();

		/// @brief 指定段階の画面全体ポストエフェクトを合成済み画像へ適用する
		/// @param stage 適用する画面全体ポストエフェクトの段階
		void ApplyScreenEffectPasses(MadoEngine::Render::ScreenEffectStage stage);

		/// @brief Scene段階のポストエフェクト結果へ透明オブジェクトを描画する準備を行う
		void BeginTransparentRender();

		/// @brief 透明オブジェクトの描画を終了する
		void EndTransparentRender();

		/// @brief Scene段階のポストエフェクト結果へSpriteとTextを描画する準備を行う
		void BeginOverlayRender();

		/// @brief SpriteとTextの描画を終了してFinal段階の入力を確定する
		void EndOverlayRender();

		/// @brief ImGuiレイアウト開始（DockSpace・GameView生成）
		/// @brief シーンの DrawImGui() より前に呼ぶこと
		void BeginImGuiLayout();

		/// @brief 描画後処理（ImGui確定・Present）
		void PostDraw();

		/// @brief 終了処理
		void Finalize();

		/// @brief ゲームループを継続するかどうかを取得
		bool IsRunning();

		/// @brief アプリケーションを停止するフラグを取得
		bool IsStopApplication() const { return isStopApplication_; }

		/// @brief 1フレームの経過時間を取得する
		/// @return 経過時間（秒）
		float GetDeltaTime() const { return static_cast<float>(deltaTime_->GetDeltaTime()); }

	private:

		//D3DResourceLeakChecker leakChecker;

		/// @brief ウィンドウリサイズ要求を描画リソースへ反映する
		void HandleResize();

		/// @brief 通常3D描画前にシャドウマップを生成してModelへ設定する
		/// @param currentSceneType 現在のシーン種別
		/// @param shadowFocusPosition シャドウマップの中心へ置くワールド座標
		void RenderShadowMap(
			SceneType currentSceneType,
			const Vector3& shadowFocusPosition
		);

		/// @brief 指定したSRVを現在の描画先へポストエフェクト描画する
		/// @param inputSrv 入力テクスチャのGPU SRVハンドル
		/// @param desc 使用するPSO設定
		/// @param parameterBufferAddress パラメータ用ConstantBufferのGPU仮想アドレス
		void DrawPostEffect(
			D3D12_GPU_DESCRIPTOR_HANDLE inputSrv,
			const MadoEngine::Render::PSODesc& desc,
			D3D12_GPU_VIRTUAL_ADDRESS parameterBufferAddress = 0,
			MadoEngine::Core::DepthStencilBuffer* maskDepthStencilBuffer = nullptr
		);

		/// @brief 指定段階とLayerMaskのチェーンにDepth無視マスクが必要か判定する
		/// @param layerMask 判定するLayerMask
		/// @param stage 対象の適用段階
		/// @return Depth無視マスクが必要な場合はtrue
		bool NeedsIgnoreDepthMask(
			Render::RenderLayerMask layerMask,
			Render::LayerEffectStage stage
		) const;

		/// @brief Scene段階のFog設定をParticle描画へ同期する
		void UpdateParticleFogParameters();

		/// @brief シーンとLayerエフェクト結果を現在の描画先へ合成する
		/// @param sceneSrv シーンカラーのGPU SRVハンドル
		/// @param effectSrv エフェクト結果のGPU SRVハンドル
		void DrawComposite(D3D12_GPU_DESCRIPTOR_HANDLE sceneSrv, D3D12_GPU_DESCRIPTOR_HANDLE effectSrv);

		/// @brief 現在のシーンカラーをポストエフェクト入力として使用可能な状態へ確定する
		void ResolveCompositeSource();

		/// @brief 次のポストエフェクト合成先RenderTarget名を取得する
		/// @return 次の合成先RenderTarget名
		const std::string& GetNextPostEffectOutputName() const;

		/// @brief 次のLayerエフェクトチェーン出力先RenderTarget名を取得する
		/// @return 次のLayerエフェクトチェーン出力先RenderTarget名
		const std::string& GetNextLayerEffectOutputName() const;

		bool isStopApplication_ = false;
		bool isInitialized_ = false;
		uint32_t renderWidth_ = 0;
		uint32_t renderHeight_ = 0;

		std::unique_ptr<MadoEngine::Screen::WindowsAPI> windowsAPI_;
		MadoEngine::Screen::WindowsAPI::WindowDesc winDesc_;

		std::unique_ptr<MadoEngine::DeltaTime> deltaTime_;

		std::unique_ptr<MadoEngine::Core::DxDevice> dxDevice_;

		std::unique_ptr<MadoEngine::Core::CommandManager> commandManager_;

		std::unique_ptr<MadoEngine::Screen::SwapChain> swapChain_;

		MadoEngine::Core::RTVManager* rtvManager_ = nullptr;
		MadoEngine::Core::SRVManager* srvManager_ = nullptr;
		MadoEngine::Core::DSVManager* dsvManager_ = nullptr;

		std::unique_ptr<MadoEngine::Render::PSOFactory> psoFactory_;
		std::unique_ptr<MadoEngine::Render::PSORegistry> psoRegistry_;
		std::unique_ptr<MadoEngine::Render::ComputePSORegistry> computePsoRegistry_;

		std::unique_ptr<MadoEngine::Core::DepthStencilBuffer> depthStencilBuffer_;
		std::unique_ptr<MadoEngine::Core::DepthStencilBuffer> layerDepthStencilBuffer_;
		MadoEngine::Core::DepthStencilBuffer* currentLayerMaskDepthStencilBuffer_ = nullptr;

		std::unique_ptr<MadoEngine::Render::ViewportScissor> viewportScissor_; // ビューポート＆シザー矩形

		std::unique_ptr<MadoEngine::Render::RenderTargetManager> renderTargetManager_;
		std::unique_ptr<MadoEngine::Render::GameViewCapture> gameViewCapture_;
		std::unique_ptr<MadoEngine::Render::ShadowMap> shadowMap_;
		MadoEngine::Render::PSODesc postEffectCopyDesc_;
		MadoEngine::Render::PSODesc compositeDesc_;
		Microsoft::WRL::ComPtr<ID3D12Resource> postEffectDefaultParameterResource_;
		std::string currentCompositeSourceName_ = "SceneColor";
		std::string resolvedPostEffectTargetName_ = "PostEffectResult";
		std::string currentLayerEffectSourceName_ = "LayerColor";
		std::string currentTransparentTargetName_;
		std::string currentOverlayTargetName_;
		SceneType currentSceneType_ = SceneType::None;
		bool isSceneColorEnded_ = false;
		bool isLayerEffectChainResolved_ = false;
		bool isLayerEffectResolved_ = false;
		bool isTransparentRenderActive_ = false;
		bool isOverlayRenderActive_ = false;
		bool isSceneScreenEffectStageApplied_ = false;
		bool isFinalScreenEffectStageApplied_ = false;
		bool isGameViewCaptureRequested_ = false;

#ifdef USE_IMGUI
		std::unique_ptr<MadoEngine::ImGuiManager> imguiManager_;

#endif

	};
}
