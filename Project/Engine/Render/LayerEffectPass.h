#pragma once
#include "Render/Object/RenderLayer.h"
#include "Render/PSO/PSODesc.h"
#include <string>

namespace MadoEngine::Render {

	/// @brief 特定の描画Layerに適用するポストエフェクト設定を管理するクラス
	class LayerEffectPass {
	public:
		/// @brief LayerEffectPassの生成設定
		struct Desc {
			std::string name = "LayerEffect";
			RenderLayerMask targetLayerMask = ToRenderLayerMask(RenderLayer::Default);
			std::string effectShaderKey = "PostEffect/CopyImage.PS";
			bool enabled = true;
		};

		/// @brief LayerEffectPassを初期化する
		/// @param desc LayerEffectPassの生成設定
		/// @param basePostEffectDesc ポストエフェクト用の基本PSO設定
		void Initialize(const Desc& desc, const PSODesc& basePostEffectDesc);

		/// @brief 有効状態を設定する
		/// @param enabled 有効にする場合はtrue
		void SetEnabled(bool enabled);

		/// @brief 有効状態を取得する
		/// @return 有効な場合はtrue
		bool IsEnabled() const;

		/// @brief パス名を設定する
		/// @param name 設定するパス名
		void SetName(const std::string& name);

		/// @brief パス名を取得する
		/// @return パス名
		const std::string& GetName() const;

		/// @brief 対象Layerを単体で設定する
		/// @param layer 対象Layer
		void SetTargetLayer(RenderLayer layer);

		/// @brief 対象Layerマスクを設定する
		/// @param layerMask 対象Layerマスク
		void SetTargetLayerMask(RenderLayerMask layerMask);

		/// @brief 対象Layerマスクを取得する
		/// @return 対象Layerマスク
		RenderLayerMask GetTargetLayerMask() const;

		/// @brief 対象Layerを除外した描画Layerマスクを取得する
		/// @param sourceLayerMask 元になるLayerマスク
		/// @return 対象Layerを除外したLayerマスク
		RenderLayerMask GetBaseLayerMask(RenderLayerMask sourceLayerMask) const;

		/// @brief 適用するPixelShaderキーを設定する
		/// @param shaderKey PixelShaderキー
		void SetEffectShaderKey(const std::string& shaderKey);

		/// @brief 適用するPixelShaderキーを取得する
		/// @return PixelShaderキー
		const std::string& GetEffectShaderKey() const;

		/// @brief ポストエフェクト描画に使用するPSO設定を取得する
		/// @return PSO設定
		const PSODesc& GetEffectPSODesc() const;

	private:
		Desc desc_;
		PSODesc effectDesc_;
	};

} // namespace MadoEngine::Render
