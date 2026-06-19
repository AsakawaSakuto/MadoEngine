#include "Render/PostEffectManager.h"
#include <cassert>

namespace MadoEngine::Render {

	void PostEffectManager::Initialize(const PSODesc& basePostEffectDesc, ID3D12Device* device) {
		assert(device && "PostEffectManagerのD3D12Deviceが空です");

		basePostEffectDesc_ = basePostEffectDesc;
		device_ = device;
	}

	LayerEffectPass* PostEffectManager::AddLayerPass(const LayerEffectPass::Desc& desc) {
		assert(device_ && "PostEffectManagerが初期化されていません");

		layerPasses_.emplace_back();
		layerPasses_.back().Initialize(desc, basePostEffectDesc_, device_);
		return &layerPasses_.back();
	}

	LayerEffectPass* PostEffectManager::AddScreenPass(const LayerEffectPass::Desc& desc) {
		assert(device_ && "PostEffectManagerが初期化されていません");

		screenPasses_.emplace_back();
		screenPasses_.back().Initialize(desc, basePostEffectDesc_, device_);
		return &screenPasses_.back();
	}

	void PostEffectManager::ClearLayerPasses() {
		layerPasses_.clear();
	}

	void PostEffectManager::ClearScreenPasses() {
		screenPasses_.clear();
	}

	std::vector<LayerEffectPass>& PostEffectManager::GetLayerPasses() {
		return layerPasses_;
	}

	const std::vector<LayerEffectPass>& PostEffectManager::GetLayerPasses() const {
		return layerPasses_;
	}

	std::vector<LayerEffectPass>& PostEffectManager::GetScreenPasses() {
		return screenPasses_;
	}

	const std::vector<LayerEffectPass>& PostEffectManager::GetScreenPasses() const {
		return screenPasses_;
	}

	LayerEffectPass* PostEffectManager::FindPass(const std::string& name) {
		if (LayerEffectPass* pass = FindPassIn(layerPasses_, name)) {
			return pass;
		}

		return FindPassIn(screenPasses_, name);
	}

	const LayerEffectPass* PostEffectManager::FindPass(const std::string& name) const {
		if (const LayerEffectPass* pass = FindPassIn(layerPasses_, name)) {
			return pass;
		}

		return FindPassIn(screenPasses_, name);
	}

	bool PostEffectManager::SetEnabled(const std::string& name, bool enabled) {
		LayerEffectPass* pass = FindPass(name);
		if (!pass) {
			return false;
		}

		pass->SetEnabled(enabled);
		return true;
	}

	bool PostEffectManager::TryGetEnabled(const std::string& name, bool& outEnabled) const {
		const LayerEffectPass* pass = FindPass(name);
		if (!pass) {
			return false;
		}

		outEnabled = pass->IsEnabled();
		return true;
	}

	bool PostEffectManager::SetFloatParameter(const std::string& passName, const std::string& parameterKey, float value) {
		LayerEffectPass* pass = FindPass(passName);
		if (!pass) {
			return false;
		}

		return pass->SetFloatParameter(parameterKey, value);
	}

	bool PostEffectManager::TryGetFloatParameter(const std::string& passName, const std::string& parameterKey, float& outValue) const {
		const LayerEffectPass* pass = FindPass(passName);
		if (!pass) {
			return false;
		}

		return pass->TryGetFloatParameter(parameterKey, outValue);
	}

	const LayerEffectPass* PostEffectManager::GetFirstEnabledLayerPass() const {
		for (const LayerEffectPass& pass : layerPasses_) {
			if (pass.IsEnabled() && pass.GetTargetLayerMask() != 0) {
				return &pass;
			}
		}

		return nullptr;
	}

	RenderLayerMask PostEffectManager::GetEnabledLayerTargetMask() const {
		RenderLayerMask layerMask = 0;
		for (const LayerEffectPass& pass : layerPasses_) {
			if (!pass.IsEnabled()) {
				continue;
			}

			layerMask |= pass.GetTargetLayerMask();
		}

		return layerMask;
	}

	bool PostEffectManager::NeedsIgnoreDepthMask(RenderLayerMask layerMask) const {
		for (const LayerEffectPass& pass : layerPasses_) {
			if (!pass.IsEnabled()) {
				continue;
			}

			if (pass.GetTargetLayerMask() != layerMask) {
				continue;
			}

			if (pass.IsIgnoreDepthForMask()) {
				return true;
			}
		}

		return false;
	}

	LayerEffectPass* PostEffectManager::FindPassIn(std::vector<LayerEffectPass>& passes, const std::string& name) {
		for (LayerEffectPass& pass : passes) {
			if (pass.GetKey() == name || pass.GetName() == name) {
				return &pass;
			}
		}

		return nullptr;
	}

	const LayerEffectPass* PostEffectManager::FindPassIn(const std::vector<LayerEffectPass>& passes, const std::string& name) {
		for (const LayerEffectPass& pass : passes) {
			if (pass.GetKey() == name || pass.GetName() == name) {
				return &pass;
			}
		}

		return nullptr;
	}

} // namespace MadoEngine::Render
