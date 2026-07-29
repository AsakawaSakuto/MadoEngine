#pragma once
#include <cstddef>
#include <functional>
#include <string>

namespace MadoEngine::Render {

	/// @brief Compute PSOを一意に識別するための記述子
	struct ComputePSODesc {
		std::string csKey;
		std::string rootSigKey;

		/// @brief 2つのCompute PSO記述子が同一か比較する
		/// @param other 比較対象のCompute PSO記述子
		/// @return ShaderとRootSignatureのキーが同一の場合はtrue
		bool operator==(const ComputePSODesc& other) const = default;
	};

	/// @brief ComputePSODescのハッシュ値を生成する
	struct ComputePSODescHash {
		/// @brief Compute PSO記述子からハッシュ値を生成する
		/// @param desc ハッシュ値を生成するCompute PSO記述子
		/// @return 生成したハッシュ値
		std::size_t operator()(const ComputePSODesc& desc) const noexcept {
			std::size_t seed = std::hash<std::string>{}(desc.csKey);
			seed ^= std::hash<std::string>{}(desc.rootSigKey)
				+ static_cast<std::size_t>(0x9e3779b9u)
				+ (seed << 6)
				+ (seed >> 2);
			return seed;
		}
	};

} // MadoEngine::Render名前空間
