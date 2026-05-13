#pragma once
#include "Render/PSO/PSODesc.h"
#include <functional>
#include <cstddef>

namespace MadoEngine::Render {

	/// @brief ハッシュ値を合成するユーティリティ関数
	/// @param seed 現在のハッシュ値（合成先）
	/// @param value 合成する値
	template <typename T>
	inline void HashCombine(std::size_t& seed, const T& value) {
		seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	/// @brief PSODesc のハッシュファンクタ
	struct PSODescHash {
		std::size_t operator()(const PSODesc& desc) const {
			std::size_t seed = 0;
			HashCombine(seed, static_cast<int>(desc.blendMode));
			HashCombine(seed, static_cast<int>(desc.depthMode));
			HashCombine(seed, static_cast<int>(desc.cullMode));
			HashCombine(seed, static_cast<int>(desc.fillMode));
			HashCombine(seed, static_cast<int>(desc.topology));
			HashCombine(seed, static_cast<int>(desc.inputLayout));
			HashCombine(seed, static_cast<int>(desc.rtvFormat));
			HashCombine(seed, static_cast<int>(desc.dsvFormat));
			HashCombine(seed, desc.vsKey);
			HashCombine(seed, desc.psKey);
			HashCombine(seed, desc.rootSigKey);
			return seed;
		}
	};

	/// @brief PSODesc の等値比較ファンクタ
	struct PSODescEqual {
		bool operator()(const PSODesc& lhs, const PSODesc& rhs) const {
			return lhs.blendMode   == rhs.blendMode   &&
				   lhs.depthMode   == rhs.depthMode   &&
				   lhs.cullMode    == rhs.cullMode     &&
				   lhs.fillMode    == rhs.fillMode     &&
				   lhs.topology    == rhs.topology     &&
				   lhs.inputLayout == rhs.inputLayout  &&
				   lhs.rtvFormat   == rhs.rtvFormat    &&
				   lhs.dsvFormat   == rhs.dsvFormat    &&
				   lhs.vsKey       == rhs.vsKey        &&
				   lhs.psKey       == rhs.psKey        &&
				   lhs.rootSigKey  == rhs.rootSigKey;
		}
	};

} // namespace MadoEngine::Render
