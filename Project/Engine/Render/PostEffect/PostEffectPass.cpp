#include "Render/PostEffect/PostEffectPass.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include <cstring>

namespace MadoEngine::Render {

	void PostEffectPass::Initialize(const Desc& desc, const PSODesc& basePostEffectDesc, ID3D12Device* device) {
		assert(!desc.name.empty() && "PostEffectPass名が空です");
		assert(!desc.effectShaderKey.empty() && "PostEffectPassのPixelShaderキーが空です");
		assert(IsValidLayerEffectStage(desc.layerEffectStage) && "LayerEffectStageが範囲外です");
		assert(IsValidScreenEffectStage(desc.screenEffectStage) && "ScreenEffectStageが範囲外です");
		assert(device && "D3D12Deviceが空です");

		desc_ = desc;
		if (desc_.key.empty()) {
			desc_.key = desc_.name;
		}

		// 共通PSO記述子からPixel Shaderだけを差し替えてPass固有設定を構築
		effectDesc_ = basePostEffectDesc;
		effectDesc_.psKey = desc_.effectShaderKey;
		device_ = device;
		if (const PostEffectDefinition* definition =
			PostEffectDefinitionRegistry::FindByShaderKey(desc_.effectShaderKey)) {
			ApplyEffectDefinition(*definition);
		} else {
			effectType_.reset();
			Logger::Output(
				"未登録のポストエフェクトShaderキーです: " + desc_.effectShaderKey,
				Logger::Level::Warning
			);
		}

		Logger::Output("PostEffectPassを初期化しました: " + desc_.name, Logger::Level::Engine);
	}

	void PostEffectPass::SetEnabled(bool enabled) {
		desc_.enabled = enabled;
	}

	bool PostEffectPass::IsEnabled() const {
		return desc_.enabled;
	}

	void PostEffectPass::SetIgnoreDepthForMask(bool ignoreDepth) {
		desc_.ignoreDepthForMask = ignoreDepth;
	}

	bool PostEffectPass::IsIgnoreDepthForMask() const {
		return desc_.ignoreDepthForMask;
	}

	void PostEffectPass::SetScreenEffectStage(ScreenEffectStage stage) {
		assert(IsValidScreenEffectStage(stage) && "ScreenEffectStageが範囲外です");
		desc_.screenEffectStage = stage;
	}

	ScreenEffectStage PostEffectPass::GetScreenEffectStage() const {
		return desc_.screenEffectStage;
	}

	void PostEffectPass::SetName(const std::string& name) {
		assert(!name.empty() && "PostEffectPass名が空です");
		desc_.name = name;
	}

	const std::string& PostEffectPass::GetName() const {
		return desc_.name;
	}

	const std::string& PostEffectPass::GetKey() const {
		return desc_.key;
	}

	void PostEffectPass::SetTargetLayer(RenderLayer layer) {
		SetTargetLayerMask(ToRenderLayerMask(layer));
	}

	void PostEffectPass::SetTargetLayerMask(RenderLayerMask layerMask) {
		desc_.targetLayerMask = layerMask;
	}

	RenderLayerMask PostEffectPass::GetTargetLayerMask() const {
		return desc_.targetLayerMask;
	}

	void PostEffectPass::SetLayerEffectStage(LayerEffectStage stage) {
		assert(IsValidLayerEffectStage(stage) && "LayerEffectStageが範囲外です");
		desc_.layerEffectStage = stage;
	}

	LayerEffectStage PostEffectPass::GetLayerEffectStage() const {
		return desc_.layerEffectStage;
	}

	RenderLayerMask PostEffectPass::GetBaseLayerMask(RenderLayerMask sourceLayerMask) const {
		return RemoveRenderLayerMask(sourceLayerMask, desc_.targetLayerMask);
	}

	void PostEffectPass::SetEffectShaderKey(const std::string& shaderKey) {
		assert(!shaderKey.empty() && "PostEffectPassのPixelShaderキーが空です");
		if (const PostEffectDefinition* definition = PostEffectDefinitionRegistry::FindByShaderKey(shaderKey)) {
			ApplyEffectDefinition(*definition);
			return;
		}

		// 未登録Shaderでは古いParameter Layoutを再利用せずParameterなしのPassへ退避
		desc_.effectShaderKey = shaderKey;
		effectDesc_.psKey = desc_.effectShaderKey;
		effectType_.reset();
		ClearFloatParameterControls();
		ClearParameterData();
		Logger::Output("未登録のポストエフェクトShaderキーです: " + shaderKey, Logger::Level::Warning);
	}

	void PostEffectPass::ApplyEffectDefinition(const PostEffectDefinition& definition) {
		desc_.effectShaderKey = std::string(definition.shaderKey);
		effectDesc_.psKey = desc_.effectShaderKey;
		effectType_ = definition.type;
		ClearFloatParameterControls();
		ClearParameterData();

		// Definitionが保持する既定値とEditor用Offsetを同じLayoutから再構築
		if (definition.defaultParameterData && definition.parameterSize > 0) {
			SetParameterData(definition.defaultParameterData, definition.parameterSize);
		}

		for (const PostEffectFloatParameterDefinition& parameter : definition.GetParameters()) {
			AddFloatParameterControl(
				std::string(parameter.key),
				std::string(parameter.label),
				parameter.offset,
				parameter.minValue,
				parameter.maxValue,
				parameter.speed
			);
		}
	}

	std::optional<PostEffectType> PostEffectPass::GetPostEffectType() const {
		return effectType_;
	}

	const std::string& PostEffectPass::GetEffectShaderKey() const {
		return desc_.effectShaderKey;
	}

	const PSODesc& PostEffectPass::GetEffectPSODesc() const {
		return effectDesc_;
	}

	void PostEffectPass::SetParameterData(const void* data, std::size_t sizeInBytes) {
		assert(data && "ConstantBufferへ書き込むデータが空です");
		assert(sizeInBytes > 0 && "ConstantBufferへ書き込むサイズが0です");

		EnsureParameterBuffer(sizeInBytes);
		parameterData_.resize(sizeInBytes);
		std::memcpy(parameterData_.data(), data, sizeInBytes);
		parameterSizeInBytes_ = sizeInBytes;
		UploadParameterData();
	}

	bool PostEffectPass::TryCopyParameterData(void* outData, std::size_t sizeInBytes) const {
		if (!outData || sizeInBytes != parameterData_.size()) {
			return false;
		}

		if (sizeInBytes > 0) {
			std::memcpy(outData, parameterData_.data(), sizeInBytes);
		}
		return true;
	}

	std::size_t PostEffectPass::GetParameterDataSize() const {
		return parameterData_.size();
	}

	void PostEffectPass::AddFloatParameterControl(
		const std::string& label,
		std::size_t offset,
		float minValue,
		float maxValue,
		float speed)
	{
		AddFloatParameterControl(label, label, offset, minValue, maxValue, speed);
	}

	void PostEffectPass::AddFloatParameterControl(
		const std::string& key,
		const std::string& label,
		std::size_t offset,
		float minValue,
		float maxValue,
		float speed)
	{
		assert(!key.empty() && "floatパラメータキーが空です");
		assert(!label.empty() && "floatパラメータ名が空です");
		assert(offset + sizeof(float) <= parameterSizeInBytes_ && "floatパラメータのoffsetがConstantBufferの範囲外です");

		FloatParameterControl control{};
		control.key = key;
		control.label = label;
		control.offset = offset;
		control.minValue = minValue;
		control.maxValue = maxValue;
		control.speed = speed;
		floatParameterControls_.push_back(control);
	}

	const std::vector<PostEffectPass::FloatParameterControl>& PostEffectPass::GetFloatParameterControls() const {
		return floatParameterControls_;
	}

	void PostEffectPass::ClearFloatParameterControls() {
		floatParameterControls_.clear();
	}

	void PostEffectPass::ClearParameterData() {

		// 永続Mapを解除してCPU側CopyとGPU Resourceの寿命を同時に終了
		if (parameterResource_ && mappedParameter_) {
			parameterResource_->Unmap(0, nullptr);
		}

		parameterResource_.Reset();
		mappedParameter_ = nullptr;
		parameterData_.clear();
		parameterSizeInBytes_ = 0;
		parameterBufferSizeInBytes_ = 0;
	}

	bool PostEffectPass::TryGetFloatParameter(std::size_t offset, float& outValue) const {
		if (offset + sizeof(float) > parameterData_.size()) {
			return false;
		}

		std::memcpy(&outValue, parameterData_.data() + offset, sizeof(float));
		return true;
	}

	bool PostEffectPass::TryGetFloatParameter(const std::string& key, float& outValue) const {
		for (const FloatParameterControl& control : floatParameterControls_) {
			if (control.key == key) {
				return TryGetFloatParameter(control.offset, outValue);
			}
		}

		return false;
	}

	void PostEffectPass::SetFloatParameter(std::size_t offset, float value) {
		assert(offset + sizeof(float) <= parameterData_.size() && "floatパラメータのoffsetがConstantBufferの範囲外です");

		std::memcpy(parameterData_.data() + offset, &value, sizeof(float));
		UploadParameterData();
	}

	bool PostEffectPass::SetFloatParameter(const std::string& key, float value) {
		for (const FloatParameterControl& control : floatParameterControls_) {
			if (control.key == key) {
				SetFloatParameter(control.offset, value);
				return true;
			}
		}

		return false;
	}

	bool PostEffectPass::HasParameterBuffer() const {
		return parameterResource_.Get() != nullptr;
	}

	D3D12_GPU_VIRTUAL_ADDRESS PostEffectPass::GetParameterGPUVirtualAddress() const {
		if (!parameterResource_) {
			return 0;
		}

		return parameterResource_->GetGPUVirtualAddress();
	}

	std::size_t PostEffectPass::AlignConstantBufferSize(std::size_t sizeInBytes) {
		constexpr std::size_t kConstantBufferAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		return (sizeInBytes + kConstantBufferAlignment - 1) & ~(kConstantBufferAlignment - 1);
	}

	void PostEffectPass::EnsureParameterBuffer(std::size_t sizeInBytes) {
		assert(device_ && "D3D12Deviceが空です");

		const std::size_t alignedSize = AlignConstantBufferSize(sizeInBytes);

		// 既存容量に収まる更新ではResource再生成と再Mapを回避
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
			"[Engine] PostEffectPassのパラメータ用ConstantBufferを作成しました: " +
			desc_.name + " " + std::to_string(parameterBufferSizeInBytes_) + " bytes",
			Logger::Level::Engine
		);
	}

	void PostEffectPass::UploadParameterData() {
		assert(mappedParameter_ && "ConstantBufferがMapされていません");

		// Alignment領域を0で埋めて未使用Byteに前回値を残さない転送
		std::memset(mappedParameter_, 0, parameterBufferSizeInBytes_);
		if (!parameterData_.empty()) {
			std::memcpy(mappedParameter_, parameterData_.data(), parameterData_.size());
		}
	}

} // namespace MadoEngine::Render
