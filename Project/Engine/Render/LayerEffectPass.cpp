#include "Render/LayerEffectPass.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include <cstring>

namespace MadoEngine::Render {

	void LayerEffectPass::Initialize(const Desc& desc, const PSODesc& basePostEffectDesc, ID3D12Device* device) {
		assert(!desc.name.empty() && "LayerEffectPass名が空です");
		assert(!desc.effectShaderKey.empty() && "LayerEffectPassのPixelShaderキーが空です");
		assert(device && "D3D12Deviceが空です");

		desc_ = desc;
		effectDesc_ = basePostEffectDesc;
		effectDesc_.psKey = desc_.effectShaderKey;
		device_ = device;

		Logger::Output("LayerEffectPassを初期化しました: " + desc_.name, Logger::Level::Engine);
	}

	void LayerEffectPass::SetEnabled(bool enabled) {
		desc_.enabled = enabled;
	}

	bool LayerEffectPass::IsEnabled() const {
		return desc_.enabled;
	}

	void LayerEffectPass::SetIgnoreDepthForMask(bool ignoreDepth) {
		desc_.ignoreDepthForMask = ignoreDepth;
	}

	bool LayerEffectPass::IsIgnoreDepthForMask() const {
		return desc_.ignoreDepthForMask;
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

	void LayerEffectPass::SetParameterData(const void* data, std::size_t sizeInBytes) {
		assert(data && "ConstantBufferへ書き込むデータが空です");
		assert(sizeInBytes > 0 && "ConstantBufferへ書き込むサイズが0です");

		EnsureParameterBuffer(sizeInBytes);
		parameterData_.resize(sizeInBytes);
		std::memcpy(parameterData_.data(), data, sizeInBytes);
		parameterSizeInBytes_ = sizeInBytes;
		UploadParameterData();
	}

	void LayerEffectPass::AddFloatParameterControl(
		const std::string& label,
		std::size_t offset,
		float minValue,
		float maxValue,
		float speed)
	{
		assert(!label.empty() && "floatパラメータ名が空です");
		assert(offset + sizeof(float) <= parameterSizeInBytes_ && "floatパラメータのoffsetがConstantBufferの範囲外です");

		FloatParameterControl control{};
		control.label = label;
		control.offset = offset;
		control.minValue = minValue;
		control.maxValue = maxValue;
		control.speed = speed;
		floatParameterControls_.push_back(control);
	}

	const std::vector<LayerEffectPass::FloatParameterControl>& LayerEffectPass::GetFloatParameterControls() const {
		return floatParameterControls_;
	}

	bool LayerEffectPass::TryGetFloatParameter(std::size_t offset, float& outValue) const {
		if (offset + sizeof(float) > parameterData_.size()) {
			return false;
		}

		std::memcpy(&outValue, parameterData_.data() + offset, sizeof(float));
		return true;
	}

	void LayerEffectPass::SetFloatParameter(std::size_t offset, float value) {
		assert(offset + sizeof(float) <= parameterData_.size() && "floatパラメータのoffsetがConstantBufferの範囲外です");

		std::memcpy(parameterData_.data() + offset, &value, sizeof(float));
		UploadParameterData();
	}

	bool LayerEffectPass::HasParameterBuffer() const {
		return parameterResource_.Get() != nullptr;
	}

	D3D12_GPU_VIRTUAL_ADDRESS LayerEffectPass::GetParameterGPUVirtualAddress() const {
		if (!parameterResource_) {
			return 0;
		}

		return parameterResource_->GetGPUVirtualAddress();
	}

	std::size_t LayerEffectPass::AlignConstantBufferSize(std::size_t sizeInBytes) {
		constexpr std::size_t kConstantBufferAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		return (sizeInBytes + kConstantBufferAlignment - 1) & ~(kConstantBufferAlignment - 1);
	}

	void LayerEffectPass::EnsureParameterBuffer(std::size_t sizeInBytes) {
		assert(device_ && "D3D12Deviceが空です");

		const std::size_t alignedSize = AlignConstantBufferSize(sizeInBytes);
		if (parameterResource_ && parameterBufferSizeInBytes_ >= alignedSize) {
			return;
		}

		if (parameterResource_ && mappedParameter_) {
			parameterResource_->Unmap(0, nullptr);
			mappedParameter_ = nullptr;
		}

		parameterResource_ = CreateBufferResource(device_, alignedSize, false);
		parameterBufferSizeInBytes_ = alignedSize;

		HRESULT hr = parameterResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedParameter_));
		assert(SUCCEEDED(hr));
		std::memset(mappedParameter_, 0, parameterBufferSizeInBytes_);

		Logger::Output(
			"[Engine] LayerEffectPassのパラメータ用ConstantBufferを作成しました: " +
			desc_.name + " " + std::to_string(parameterBufferSizeInBytes_) + " bytes",
			Logger::Level::Engine
		);
	}

	void LayerEffectPass::UploadParameterData() {
		assert(mappedParameter_ && "ConstantBufferがMapされていません");

		std::memset(mappedParameter_, 0, parameterBufferSizeInBytes_);
		if (!parameterData_.empty()) {
			std::memcpy(mappedParameter_, parameterData_.data(), parameterData_.size());
		}
	}

} // namespace MadoEngine::Render
