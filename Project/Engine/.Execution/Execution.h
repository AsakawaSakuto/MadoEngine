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

		/// @brief LayerEffectPassを登録する
		/// @param desc LayerEffectPassの生成設定
		/// @return 登録したLayerEffectPassのポインタ
		MadoEngine::Render::LayerEffectPass* AddLayerEffectPass(const MadoEngine::Render::LayerEffectPass::Desc& desc);

		/// @brief 画面全体に適用するポストエフェクトPassを登録する
		/// @param desc ポストエフェクトPassの生成設定
		/// @return 登録したポストエフェクトPassのポインタ
		MadoEngine::Render::LayerEffectPass* AddScreenEffectPass(const MadoEngine::Render::LayerEffectPass::Desc& desc);

		/// @brief 登録済みLayerEffectPassをすべて削除する
		void ClearLayerEffectPasses();

		/// @brief 登録済みの画面全体ポストエフェクトPassをすべて削除する
		void ClearScreenEffectPasses();

		/// @brief 登録済みLayerEffectPassを取得する
		/// @return 登録済みLayerEffectPass配列
		const std::vector<MadoEngine::Render::LayerEffectPass>& GetLayerEffectPasses() const;

		/// @brief 最初に有効なLayerEffectPassを取得する
		/// @return 有効なLayerEffectPass。存在しない場合はnullptr
		const MadoEngine::Render::LayerEffectPass* GetFirstEnabledLayerEffectPass() const;

		/// @brief 有効なLayerEffectPassの対象Layerをまとめたマスクを取得する
		/// @return 有効なLayerEffectPassの対象Layerマスク
		MadoEngine::Render::RenderLayerMask GetEnabledLayerEffectTargetMask() const;

		/// @brief シーンカラーRenderTargetへの描画を終了する
		void EndSceneColorRender();

		/// @brief ポストエフェクト対象Layer用RenderTargetへの描画を開始する
		/// @param pass 実行するLayerEffectPass
		void BeginLayerEffectRender(const MadoEngine::Render::LayerEffectPass& pass);

		/// @brief ポストエフェクト対象Layer用RenderTargetへの描画を終了する
		void EndLayerEffectRender();

		/// @brief 対象Layerのポストエフェクト結果をシーンへ合成する
		/// @param pass 実行するLayerEffectPass
		void ApplyLayerEffectAndComposite(const MadoEngine::Render::LayerEffectPass& pass);

		/// @brief 対象Layerの現在のチェーン結果へポストエフェクトを適用する
		/// @param pass 実行するLayerEffectPass
		void ApplyLayerEffectToChain(const MadoEngine::Render::LayerEffectPass& pass);

		/// @brief 対象Layerのエフェクトチェーン結果を現在の合成済み画像へ合成する
		void CompositeLayerEffectChain();

		/// @brief 画面全体ポストエフェクトを合成済み画像へ適用する
		void ApplyScreenEffectPasses();

		/// @brief ImGuiレイアウト開始（DockSpace・GameView生成）
		/// @brief シーンの DrawImGui() より前に呼ぶこと
		void BeginImGuiLayout();

		/// @brief 描画後処理（ImGui確定・Present）
		void PostDraw();

		/// @brief 終了処理
		void Finalize();

		/// @brief ゲームループを継続するかどうかを取得
		bool IsRunning();

		/// @brief アプリケーションを終了するフラグを設定
		bool IsStopApplication() const { return isStopApplication_; }

		/// @brief 1フレームの経過時間を取得する
		/// @return 経過時間（秒）
		float GetDeltaTime() const { return static_cast<float>(deltaTime_->GetDeltaTime()); }

	private:

		//D3DResourceLeakChecker leakChecker;

		/// @brief ウィンドウリサイズ要求を描画リソースへ反映する
		void HandleResize();

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

		/// @brief 指定LayerMaskのチェーンにDepth無視マスクが必要か判定する
		/// @param layerMask 判定するLayerMask
		/// @return Depth無視マスクが必要な場合はtrue
		bool NeedsIgnoreDepthMask(Render::RenderLayerMask layerMask) const;

		/// @brief シーンとLayerエフェクト結果を現在の描画先へ合成する
		/// @param sceneSrv シーンカラーのGPU SRVハンドル
		/// @param effectSrv エフェクト結果のGPU SRVハンドル
		void DrawComposite(D3D12_GPU_DESCRIPTOR_HANDLE sceneSrv, D3D12_GPU_DESCRIPTOR_HANDLE effectSrv);

		/// @brief 次のポストエフェクト合成先RenderTarget名を取得する
		/// @return 次の合成先RenderTarget名
		const std::string& GetNextPostEffectOutputName() const;

		/// @brief 次のLayerエフェクトチェーン出力先RenderTarget名を取得する
		/// @return 次のLayerエフェクトチェーン出力先RenderTarget名
		const std::string& GetNextLayerEffectOutputName() const;

#ifdef USE_IMGUI
		/// @brief LayerEffectPassのデバッグ操作UIを描画する
		void DrawLayerEffectPassDebugUI();
#endif // USE_IMGUI

		bool isStopApplication_ = false;
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

		std::unique_ptr<MadoEngine::Core::DepthStencilBuffer> depthStencilBuffer_;
		std::unique_ptr<MadoEngine::Core::DepthStencilBuffer> layerDepthStencilBuffer_;
		MadoEngine::Core::DepthStencilBuffer* currentLayerMaskDepthStencilBuffer_ = nullptr;

		std::unique_ptr<MadoEngine::Render::ViewportScissor> viewportScissor_; // ビューポート＆シザー矩形

		std::unique_ptr<MadoEngine::Render::RenderTargetManager> renderTargetManager_;
		MadoEngine::Render::PSODesc postEffectCopyDesc_;
		MadoEngine::Render::PSODesc compositeDesc_;
		Microsoft::WRL::ComPtr<ID3D12Resource> postEffectDefaultParameterResource_;
		std::vector<MadoEngine::Render::LayerEffectPass> layerEffectPasses_;
		std::vector<MadoEngine::Render::LayerEffectPass> screenEffectPasses_;
		std::string currentCompositeSourceName_ = "SceneColor";
		std::string resolvedPostEffectTargetName_ = "PostEffectResult";
		std::string currentLayerEffectSourceName_ = "LayerColor";
		bool isSceneColorEnded_ = false;
		bool isLayerEffectChainResolved_ = false;
		bool isLayerEffectResolved_ = false;

#ifdef USE_IMGUI
		std::unique_ptr<MadoEngine::ImGuiManager> imguiManager_;
#endif
	};
}
