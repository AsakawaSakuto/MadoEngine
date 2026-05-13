#pragma once
#include <d3d12.h>
#include <vector>
#include <string>

namespace MadoEngine::Render {

	/// @brief シェーダーバインディング情報
	struct ShaderBindingInfo {
		std::string                 name;       ///< リソース名
		D3D12_DESCRIPTOR_RANGE_TYPE type;       ///< リソースタイプ（CBV/SRV/UAV/Sampler）
		UINT                        bindPoint;  ///< レジスタ番号
		UINT                        space;      ///< レジスタスペース
		UINT                        bindCount;  ///< バインド数
	};

	/// @brief シェーダーリフレクションユーティリティ
	class ShaderReflector {
	public:
		/// @brief シェーダーバイナリからバインディング情報を取得する
		/// @param shaderBytecode シェーダーバイナリ
		/// @param bytecodeLength バイナリのサイズ
		/// @return バインディング情報のリスト
		static std::vector<ShaderBindingInfo> Reflect(
			const void* shaderBytecode,
			SIZE_T bytecodeLength
		);

		/// @brief バインディング情報とRootSignatureDescの整合性を検証する
		/// @param bindings シェーダーから取得したバインディング情報
		/// @param rootSigDesc 検証対象のRootSignatureDesc
		static void Validate(
			const std::vector<ShaderBindingInfo>& bindings,
			const D3D12_ROOT_SIGNATURE_DESC& rootSigDesc
		);
	};

} // namespace MadoEngine::Render
