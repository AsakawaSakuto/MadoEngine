#include "Shader/RootSignatureManager.h"
#include "Core/DxDevice/DxDevice.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include <format>

namespace MadoEngine {

	RootSignatureManager& RootSignatureManager::GetInstance() {
		static RootSignatureManager instance;
		return instance;
	}

	void RootSignatureManager::Initialize(Core::DxDevice* device) {
		assert(device);
		device_ = device;
		Logger::Output("初期化完了", Logger::Level::Engine);
	}

	void RootSignatureManager::Register(const std::string& key, const D3D12_ROOT_SIGNATURE_DESC& desc) {
		assert(device_);

		if (rootSigMap_.contains(key)) {
			Logger::Output(
				std::format("既に登録済みのキーです（スキップ） : {}", key),
				Logger::Level::Warning
			);
			return;
		}

		Microsoft::WRL::ComPtr<ID3DBlob> sigBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errBlob;
		HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
		if (FAILED(hr)) {
			Logger::Output(
				std::format("RootSignatureのシリアライズに失敗しました : {}", key),
				Logger::Level::Error
			);
			assert(false);
			return;
		}

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
		hr = device_->GetDevice()->CreateRootSignature(
			0,
			sigBlob->GetBufferPointer(),
			sigBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootSig)
		);
		if (FAILED(hr)) {
			Logger::Output(
				std::format("RootSignatureの生成に失敗しました : {}", key),
				Logger::Level::Error
			);
			assert(false);
			return;
		}

		rootSigMap_[key] = std::move(rootSig);
		Logger::Output(
			std::format("RootSignatureを登録しました : {}", key),
			Logger::Level::Engine
		);
	}

	void RootSignatureManager::RegisterRaw(
		const std::string& key,
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature)
	{
		if (rootSigMap_.contains(key)) {
			Logger::Output(
				std::format("既に登録済みのキーです（スキップ） : {}", key),
				Logger::Level::Warning
			);
			return;
		}
		rootSigMap_[key] = std::move(rootSignature);
		Logger::Output(
			std::format("RootSignatureを登録しました（raw） : {}", key),
			Logger::Level::Engine
		);
	}

	ID3D12RootSignature* RootSignatureManager::Get(const std::string& key) const {
		const auto it = rootSigMap_.find(key);
		if (it == rootSigMap_.end()) {
			Logger::Output(
				std::format("キーが見つかりません : {}", key),
				Logger::Level::Warning
			);
			return nullptr;
		}
		return it->second.Get();
	}

	void RootSignatureManager::Finalize() {
		rootSigMap_.clear();
		device_ = nullptr;
		Logger::Output("終了しました", Logger::Level::Engine);
	}

	void RootSignatureManager::Make() {

		// RootSignatureの登録（未登録の場合のみ実行される）
		// b0: SpriteMaterial (VS/PS共通), b1: SpriteTransformationMatrix (VS), t0: Texture (PS), s0: Sampler
		{
			D3D12_DESCRIPTOR_RANGE srvRange{};
			srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			srvRange.NumDescriptors = 1;
			srvRange.BaseShaderRegister = 0; // t0
			srvRange.RegisterSpace = 0;
			srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_ROOT_PARAMETER rootParams[3]{};
			// b0: SpriteMaterial
			rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[0].Descriptor.ShaderRegister = 0;
			rootParams[0].Descriptor.RegisterSpace = 0;
			rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			// b1: SpriteTransformationMatrix
			rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[1].Descriptor.ShaderRegister = 1;
			rootParams[1].Descriptor.RegisterSpace = 0;
			rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
			// t0: Texture (DescriptorTable)
			rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;
			rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_STATIC_SAMPLER_DESC staticSampler{};
			staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			staticSampler.MipLODBias = 0.0f;
			staticSampler.MaxAnisotropy = 0;
			staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			staticSampler.MinLOD = 0.0f;
			staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
			staticSampler.ShaderRegister = 0; // s0
			staticSampler.RegisterSpace = 0;
			staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
			rootSigDesc.NumParameters = _countof(rootParams);
			rootSigDesc.pParameters = rootParams;
			rootSigDesc.NumStaticSamplers = 1;
			rootSigDesc.pStaticSamplers = &staticSampler;
			rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			MadoEngine::RootSignatureManager::GetInstance().Register("Sprite.RootSig", rootSigDesc);
		}

		// Model RootSignature
		{
			D3D12_DESCRIPTOR_RANGE textureRange{};
			textureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			textureRange.NumDescriptors = 1;
			textureRange.BaseShaderRegister = 0;
			textureRange.RegisterSpace = 0;
			textureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE environmentRange{};
			environmentRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			environmentRange.NumDescriptors = 1;
			environmentRange.BaseShaderRegister = 1;
			environmentRange.RegisterSpace = 0;
			environmentRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE shadowMapRange{};
			shadowMapRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			shadowMapRange.NumDescriptors = 1;
			shadowMapRange.BaseShaderRegister = 3;
			shadowMapRange.RegisterSpace = 0;
			shadowMapRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_ROOT_PARAMETER rootParams[8]{};
			rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[0].Descriptor.ShaderRegister = 0;
			rootParams[0].Descriptor.RegisterSpace = 0;
			rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[1].Descriptor.ShaderRegister = 1;
			rootParams[1].Descriptor.RegisterSpace = 0;
			rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

			rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[2].DescriptorTable.pDescriptorRanges = &textureRange;
			rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[3].Descriptor.ShaderRegister = 3;
			rootParams[3].Descriptor.RegisterSpace = 0;
			rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[4].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[4].DescriptorTable.pDescriptorRanges = &environmentRange;
			rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[5].Descriptor.ShaderRegister = 6;
			rootParams[5].Descriptor.RegisterSpace = 0;
			rootParams[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[6].Descriptor.ShaderRegister = 7;
			rootParams[6].Descriptor.RegisterSpace = 0;
			rootParams[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[7].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[7].DescriptorTable.pDescriptorRanges = &shadowMapRange;
			rootParams[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_STATIC_SAMPLER_DESC staticSamplers[2]{};
			staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			staticSamplers[0].MipLODBias = 0.0f;
			staticSamplers[0].MaxAnisotropy = 0;
			staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			staticSamplers[0].MinLOD = 0.0f;
			staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
			staticSamplers[0].ShaderRegister = 0;
			staticSamplers[0].RegisterSpace = 0;
			staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			staticSamplers[1] = staticSamplers[0];
			staticSamplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
			staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			staticSamplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
			staticSamplers[1].ShaderRegister = 1;

			D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
			rootSigDesc.NumParameters = _countof(rootParams);
			rootSigDesc.pParameters = rootParams;
			rootSigDesc.NumStaticSamplers = _countof(staticSamplers);
			rootSigDesc.pStaticSamplers = staticSamplers;
			rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			MadoEngine::RootSignatureManager::GetInstance().Register("Model.RootSig", rootSigDesc);
		}

		// InstancedModel RootSignature
		{
			D3D12_DESCRIPTOR_RANGE textureRange{};
			textureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			textureRange.NumDescriptors = 1;
			textureRange.BaseShaderRegister = 0;
			textureRange.RegisterSpace = 0;
			textureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE environmentRange{};
			environmentRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			environmentRange.NumDescriptors = 1;
			environmentRange.BaseShaderRegister = 1;
			environmentRange.RegisterSpace = 0;
			environmentRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE shadowMapRange{};
			shadowMapRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			shadowMapRange.NumDescriptors = 1;
			shadowMapRange.BaseShaderRegister = 3;
			shadowMapRange.RegisterSpace = 0;
			shadowMapRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE instanceRange{};
			instanceRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			instanceRange.NumDescriptors = 1;
			instanceRange.BaseShaderRegister = 2;
			instanceRange.RegisterSpace = 0;
			instanceRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_ROOT_PARAMETER rootParams[8]{};
			rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[0].Descriptor.ShaderRegister = 0;
			rootParams[0].Descriptor.RegisterSpace = 0;
			rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[1].DescriptorTable.pDescriptorRanges = &instanceRange;
			rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

			rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[2].DescriptorTable.pDescriptorRanges = &textureRange;
			rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[3].Descriptor.ShaderRegister = 3;
			rootParams[3].Descriptor.RegisterSpace = 0;
			rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[4].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[4].DescriptorTable.pDescriptorRanges = &environmentRange;
			rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[5].Descriptor.ShaderRegister = 6;
			rootParams[5].Descriptor.RegisterSpace = 0;
			rootParams[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[6].Descriptor.ShaderRegister = 7;
			rootParams[6].Descriptor.RegisterSpace = 0;
			rootParams[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[7].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[7].DescriptorTable.pDescriptorRanges = &shadowMapRange;
			rootParams[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_STATIC_SAMPLER_DESC staticSamplers[2]{};
			staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			staticSamplers[0].MipLODBias = 0.0f;
			staticSamplers[0].MaxAnisotropy = 0;
			staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			staticSamplers[0].MinLOD = 0.0f;
			staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
			staticSamplers[0].ShaderRegister = 0;
			staticSamplers[0].RegisterSpace = 0;
			staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			staticSamplers[1] = staticSamplers[0];
			staticSamplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
			staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			staticSamplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
			staticSamplers[1].ShaderRegister = 1;

			D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
			rootSigDesc.NumParameters = _countof(rootParams);
			rootSigDesc.pParameters = rootParams;
			rootSigDesc.NumStaticSamplers = _countof(staticSamplers);
			rootSigDesc.pStaticSamplers = staticSamplers;
			rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			MadoEngine::RootSignatureManager::GetInstance().Register("InstancedModel.RootSig", rootSigDesc);
		}

		// SkinningModel RootSignature
		{
			D3D12_DESCRIPTOR_RANGE textureRange{};
			textureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			textureRange.NumDescriptors = 1;
			textureRange.BaseShaderRegister = 0;
			textureRange.RegisterSpace = 0;
			textureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE paletteRange{};
			paletteRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			paletteRange.NumDescriptors = 1;
			paletteRange.BaseShaderRegister = 1;
			paletteRange.RegisterSpace = 0;
			paletteRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE environmentRange{};
			environmentRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			environmentRange.NumDescriptors = 1;
			environmentRange.BaseShaderRegister = 2;
			environmentRange.RegisterSpace = 0;
			environmentRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE shadowMapRange{};
			shadowMapRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			shadowMapRange.NumDescriptors = 1;
			shadowMapRange.BaseShaderRegister = 3;
			shadowMapRange.RegisterSpace = 0;
			shadowMapRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_ROOT_PARAMETER rootParams[9]{};
			rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[0].Descriptor.ShaderRegister = 0;
			rootParams[0].Descriptor.RegisterSpace = 0;
			rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[1].Descriptor.ShaderRegister = 1;
			rootParams[1].Descriptor.RegisterSpace = 0;
			rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

			rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[2].DescriptorTable.pDescriptorRanges = &textureRange;
			rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[3].Descriptor.ShaderRegister = 3;
			rootParams[3].Descriptor.RegisterSpace = 0;
			rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[4].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[4].DescriptorTable.pDescriptorRanges = &paletteRange;
			rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

			rootParams[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[5].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[5].DescriptorTable.pDescriptorRanges = &environmentRange;
			rootParams[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[6].Descriptor.ShaderRegister = 6;
			rootParams[6].Descriptor.RegisterSpace = 0;
			rootParams[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[7].Descriptor.ShaderRegister = 7;
			rootParams[7].Descriptor.RegisterSpace = 0;
			rootParams[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[8].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[8].DescriptorTable.pDescriptorRanges = &shadowMapRange;
			rootParams[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_STATIC_SAMPLER_DESC staticSamplers[2]{};
			staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			staticSamplers[0].MipLODBias = 0.0f;
			staticSamplers[0].MaxAnisotropy = 0;
			staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			staticSamplers[0].MinLOD = 0.0f;
			staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
			staticSamplers[0].ShaderRegister = 0;
			staticSamplers[0].RegisterSpace = 0;
			staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			staticSamplers[1] = staticSamplers[0];
			staticSamplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
			staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			staticSamplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
			staticSamplers[1].ShaderRegister = 1;

			D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
			rootSigDesc.NumParameters = _countof(rootParams);
			rootSigDesc.pParameters = rootParams;
			rootSigDesc.NumStaticSamplers = _countof(staticSamplers);
			rootSigDesc.pStaticSamplers = staticSamplers;
			rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			MadoEngine::RootSignatureManager::GetInstance().Register("SkinningModel.RootSig", rootSigDesc);
		}

		// Line3d 用 RootSignature
		// b0: LineTransform (viewProjection) VS のみ
		{
			D3D12_ROOT_PARAMETER rootParam{};
			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParam.Descriptor.ShaderRegister = 0; // b0
			rootParam.Descriptor.RegisterSpace = 0;
			rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

			D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
			rootSigDesc.NumParameters = 1;
			rootSigDesc.pParameters = &rootParam;
			rootSigDesc.NumStaticSamplers = 0;
			rootSigDesc.pStaticSamplers = nullptr;
			rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			MadoEngine::RootSignatureManager::GetInstance().Register("Line3d.RootSig", rootSigDesc);
		}

		// ShadowMap 用 RootSignature
		// b0: Shadow用WVP VSのみ
		{
			D3D12_ROOT_PARAMETER rootParam{};
			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParam.Descriptor.ShaderRegister = 0;
			rootParam.Descriptor.RegisterSpace = 0;
			rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

			D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
			rootSigDesc.NumParameters = 1;
			rootSigDesc.pParameters = &rootParam;
			rootSigDesc.NumStaticSamplers = 0;
			rootSigDesc.pStaticSamplers = nullptr;
			rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			MadoEngine::RootSignatureManager::GetInstance().Register("ShadowMap.RootSig", rootSigDesc);
		}

		// PostEffect 用 RootSignature
		// t0: 入力カラー, t1: シーン深度, t2: マスク深度, t3: エフェクト用テクスチャ, b0: パラメータ, s0: Linear, s1: Point
		{
			D3D12_DESCRIPTOR_RANGE colorRange{};
			colorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			colorRange.NumDescriptors = 1;
			colorRange.BaseShaderRegister = 0;
			colorRange.RegisterSpace = 0;
			colorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE sceneDepthRange{};
			sceneDepthRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			sceneDepthRange.NumDescriptors = 1;
			sceneDepthRange.BaseShaderRegister = 1;
			sceneDepthRange.RegisterSpace = 0;
			sceneDepthRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE maskDepthRange{};
			maskDepthRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			maskDepthRange.NumDescriptors = 1;
			maskDepthRange.BaseShaderRegister = 2;
			maskDepthRange.RegisterSpace = 0;
			maskDepthRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE effectTextureRange{};
			effectTextureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			effectTextureRange.NumDescriptors = 1;
			effectTextureRange.BaseShaderRegister = 3;
			effectTextureRange.RegisterSpace = 0;
			effectTextureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_ROOT_PARAMETER rootParams[5]{};
			rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[0].DescriptorTable.pDescriptorRanges = &colorRange;
			rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[1].DescriptorTable.pDescriptorRanges = &sceneDepthRange;
			rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[2].DescriptorTable.pDescriptorRanges = &maskDepthRange;
			rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParams[3].Descriptor.ShaderRegister = 0;
			rootParams[3].Descriptor.RegisterSpace = 0;
			rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[4].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[4].DescriptorTable.pDescriptorRanges = &effectTextureRange;
			rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_STATIC_SAMPLER_DESC staticSamplers[2]{};
			staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			staticSamplers[0].MipLODBias = 0.0f;
			staticSamplers[0].MaxAnisotropy = 0;
			staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			staticSamplers[0].MinLOD = 0.0f;
			staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
			staticSamplers[0].ShaderRegister = 0;
			staticSamplers[0].RegisterSpace = 0;
			staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			staticSamplers[1] = staticSamplers[0];
			staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			staticSamplers[1].ShaderRegister = 1;

			D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
			rootSigDesc.NumParameters = _countof(rootParams);
			rootSigDesc.pParameters = rootParams;
			rootSigDesc.NumStaticSamplers = _countof(staticSamplers);
			rootSigDesc.pStaticSamplers = staticSamplers;
			rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			MadoEngine::RootSignatureManager::GetInstance().Register("PostEffect.RootSig", rootSigDesc);
		}

		// Composite PostEffect用 RootSignature
		// t0: シーンテクスチャ, t1: エフェクトテクスチャ, s0: サンプラー
		{
			D3D12_DESCRIPTOR_RANGE sceneRange{};
			sceneRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			sceneRange.NumDescriptors = 1;
			sceneRange.BaseShaderRegister = 0;
			sceneRange.RegisterSpace = 0;
			sceneRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_DESCRIPTOR_RANGE effectRange{};
			effectRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			effectRange.NumDescriptors = 1;
			effectRange.BaseShaderRegister = 1;
			effectRange.RegisterSpace = 0;
			effectRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_ROOT_PARAMETER rootParams[2]{};
			rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[0].DescriptorTable.pDescriptorRanges = &sceneRange;
			rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
			rootParams[1].DescriptorTable.pDescriptorRanges = &effectRange;
			rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_STATIC_SAMPLER_DESC staticSampler{};
			staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			staticSampler.MipLODBias = 0.0f;
			staticSampler.MaxAnisotropy = 0;
			staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			staticSampler.MinLOD = 0.0f;
			staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
			staticSampler.ShaderRegister = 0;
			staticSampler.RegisterSpace = 0;
			staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
			rootSigDesc.NumParameters = _countof(rootParams);
			rootSigDesc.pParameters = rootParams;
			rootSigDesc.NumStaticSamplers = 1;
			rootSigDesc.pStaticSamplers = &staticSampler;
			rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			MadoEngine::RootSignatureManager::GetInstance().Register("PostEffect.Composite.RootSig", rootSigDesc);
		}

	}

} // namespace MadoEngine
