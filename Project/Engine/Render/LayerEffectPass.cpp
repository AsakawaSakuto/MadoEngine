#include "Render/LayerEffectPass.h"
#include "Utility/Logger/Logger.h"
#include <cassert>

namespace MadoEngine::Render {

	void LayerEffectPass::Initialize(const Desc& desc, const PSODesc& basePostEffectDesc) {
		assert(!desc.name.empty() && "LayerEffectPass名が空です");
		assert(!desc.effectShaderKey.empty() && "LayerEffectPassのPixelShaderキーが空です");

		desc_ = desc;
		effectDesc_ = basePostEffectDesc;
		effectDesc_.psKey = desc_.effectShaderKey;

		Logger::Output("LayerEffectPassを初期化しました: " + desc_.name, Logger::Level::Engine);
	}

	void LayerEffectPass::SetEnabled(bool enabled) {
		desc_.enabled = enabled;
	}

	bool LayerEffectPass::IsEnabled() const {
		return desc_.enabled;
	}

	void LayerEffectPass::SetName(const std::string& name) {
		assert(!name.empty() && "LayerEffectPass名が空です");
		desc_.name = name;
	}

	const std::string& LayerEffectPass::GetName() const {
		return desc_.name;
	}

	void LayerEffectPass::SetTargetLayer(RenderLayer layer) {
		SetTargetLayerMask(ToRenderLayerMask(layer));
	}

	void LayerEffectPass::SetTargetLayerMask(RenderLayerMask layerMask) {
		desc_.targetLayerMask = layerMask;
	}

	RenderLayerMask LayerEffectPass::GetTargetLayerMask() const {
		return desc_.targetLayerMask;
	}

	RenderLayerMask LayerEffectPass::GetBaseLayerMask(RenderLayerMask sourceLayerMask) const {
		return RemoveRenderLayerMask(sourceLayerMask, desc_.targetLayerMask);
	}

	void LayerEffectPass::SetEffectShaderKey(const std::string& shaderKey) {
		assert(!shaderKey.empty() && "LayerEffectPassのPixelShaderキーが空です");
		desc_.effectShaderKey = shaderKey;
		effectDesc_.psKey = desc_.effectShaderKey;
	}

	const std::string& LayerEffectPass::GetEffectShaderKey() const {
		return desc_.effectShaderKey;
	}

	const PSODesc& LayerEffectPass::GetEffectPSODesc() const {
		return effectDesc_;
	}

} // namespace MadoEngine::Render
