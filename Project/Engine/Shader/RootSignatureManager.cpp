#include "Shader/RootSignatureManager.h"
#include "Core/DxDevice/DxDevice.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include <format>

namespace MadoEngine {

	RootSignatureManager* RootSignatureManager::GetInstance() {
		static RootSignatureManager instance;
		return &instance;
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

			MadoEngine::RootSignatureManager::GetInstance()->Register("Sprite.RootSig", rootSigDesc);
		}

	}

} // namespace MadoEngine
