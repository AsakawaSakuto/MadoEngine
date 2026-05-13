#include "Render/PSO/ShaderReflector.h"
#include "Utility/Logger/Logger.h"
#include <d3dcompiler.h>
#include <cassert>
#include <wrl/client.h>

#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace MadoEngine::Render {

	std::vector<ShaderBindingInfo> ShaderReflector::Reflect(
		const void* shaderBytecode,
		SIZE_T bytecodeLength)
	{
		std::vector<ShaderBindingInfo> result;

		ComPtr<ID3D12ShaderReflection> reflection;
		D3DReflect(shaderBytecode, bytecodeLength,
			IID_PPV_ARGS(&reflection));

		if (!reflection) {
			Logger::Output("[ShaderReflector] シェーダーリフレクションの取得に失敗しました", Logger::Level::Error);
			return result;
		}

		D3D12_SHADER_DESC shaderDesc{};
		reflection->GetDesc(&shaderDesc);

		for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
			D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
			reflection->GetResourceBindingDesc(i, &bindDesc);

			ShaderBindingInfo info{};
			info.name      = bindDesc.Name;
			info.bindPoint = bindDesc.BindPoint;
			info.space     = bindDesc.Space;
			info.bindCount = bindDesc.BindCount;

			switch (bindDesc.Type) {
				case D3D_SIT_CBUFFER:
					info.type = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
					break;
				case D3D_SIT_TEXTURE:
				case D3D_SIT_STRUCTURED:
				case D3D_SIT_BYTEADDRESS:
					info.type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
					break;
				case D3D_SIT_UAV_RWTYPED:
				case D3D_SIT_UAV_RWSTRUCTURED:
				case D3D_SIT_UAV_RWBYTEADDRESS:
					info.type = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
					break;
				case D3D_SIT_SAMPLER:
					info.type = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
					break;
				default:
					info.type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
					break;
			}

			result.push_back(info);
		}

		return result;
	}

	void ShaderReflector::Validate(
		const std::vector<ShaderBindingInfo>& bindings,
		const D3D12_ROOT_SIGNATURE_DESC& rootSigDesc)
	{
#ifdef _DEBUG
		for (const auto& binding : bindings) {
			bool found = false;

			for (UINT i = 0; i < rootSigDesc.NumParameters; ++i) {
				const auto& param = rootSigDesc.pParameters[i];

				if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
					for (UINT r = 0; r < param.DescriptorTable.NumDescriptorRanges; ++r) {
						const auto& range = param.DescriptorTable.pDescriptorRanges[r];
						if (range.RangeType == binding.type &&
							range.BaseShaderRegister <= binding.bindPoint &&
							(range.NumDescriptors == UINT_MAX ||
							 range.BaseShaderRegister + range.NumDescriptors > binding.bindPoint) &&
							range.RegisterSpace == binding.space)
						{
							found = true;
							break;
						}
					}
				} else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV &&
						   binding.type == D3D12_DESCRIPTOR_RANGE_TYPE_CBV) {
					if (param.Descriptor.ShaderRegister == binding.bindPoint &&
						param.Descriptor.RegisterSpace  == binding.space) {
						found = true;
					}
				} else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV &&
						   binding.type == D3D12_DESCRIPTOR_RANGE_TYPE_SRV) {
					if (param.Descriptor.ShaderRegister == binding.bindPoint &&
						param.Descriptor.RegisterSpace  == binding.space) {
						found = true;
					}
				} else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV &&
						   binding.type == D3D12_DESCRIPTOR_RANGE_TYPE_UAV) {
					if (param.Descriptor.ShaderRegister == binding.bindPoint &&
						param.Descriptor.RegisterSpace  == binding.space) {
						found = true;
					}
				}

				if (found) break;
			}

			// スタティックサンプラーをチェック
			if (!found && binding.type == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
				for (UINT i = 0; i < rootSigDesc.NumStaticSamplers; ++i) {
					if (rootSigDesc.pStaticSamplers[i].ShaderRegister == binding.bindPoint &&
						rootSigDesc.pStaticSamplers[i].RegisterSpace  == binding.space) {
						found = true;
						break;
					}
				}
			}

			if (!found) {
				Logger::Output(
					"[ShaderReflector] RootSignatureにバインディングが見つかりません: " + binding.name,
					Logger::Level::Error
				);
				assert(false && "ShaderReflector::Validate: RootSignatureとシェーダーのバインディングが一致しません");
			} else {
				Logger::Output(
					"[ShaderReflector] バインディング検証OK: " + binding.name,
					Logger::Level::Debug
				);
			}
		}
#endif
	}

} // namespace MadoEngine::Render
