#pragma once
#include "Render/PSO/ComputePSODesc.h"
#include <cstddef>
#include <d3d12.h>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <wrl/client.h>

namespace MadoEngine::Render {

	/// @brief Compute PSOを生成して再利用するためのキャッシュ管理クラス
	class ComputePSORegistry final {
	public:
		ComputePSORegistry() = default;
		~ComputePSORegistry() = default;

		ComputePSORegistry(const ComputePSORegistry&) = delete;
		ComputePSORegistry& operator=(const ComputePSORegistry&) = delete;
		ComputePSORegistry(ComputePSORegistry&&) = delete;
		ComputePSORegistry& operator=(ComputePSORegistry&&) = delete;

		/// @brief Compute PSO Registryを初期化
		/// @param device Compute PSOの生成に使用するD3D12 Device
		void Initialize(ID3D12Device* device);

		/// @brief 保持しているCompute PSOを解放して終了
		void Finalize();

		/// @brief Compute PSOを取得し、未生成の場合は作成してキャッシュ
		/// @param desc Compute ShaderとRootSignatureの識別キー
		/// @return Compute PSO、生成できない場合はnullptr
		ID3D12PipelineState* Get(const ComputePSODesc& desc);

		/// @brief キャッシュ済みのCompute PSO数を取得
		/// @return キャッシュ済みのCompute PSO数
		std::size_t GetCachedCount() const;

	private:
		/// @brief Compute PSOを生成してキャッシュへ登録
		/// @param desc Compute ShaderとRootSignatureの識別キー
		/// @return 生成したCompute PSO、生成できない場合はnullptr
		ID3D12PipelineState* CreateAndCache(const ComputePSODesc& desc);

		ID3D12Device* device_ = nullptr;
		std::unordered_map<
			ComputePSODesc,
			Microsoft::WRL::ComPtr<ID3D12PipelineState>,
			ComputePSODescHash
		> psoCache_;
		std::unordered_set<ComputePSODesc, ComputePSODescHash> failedDescs_;
		mutable std::mutex cacheMutex_;
	};

} // MadoEngine::Render名前空間
