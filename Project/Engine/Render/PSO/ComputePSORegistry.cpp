#include "Render/PSO/ComputePSORegistry.h"
#include "Shader/RootSignatureManager.h"
#include "Shader/ShaderManager.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include <cstdint>
#include <sstream>

namespace {

	/// @brief Compute PSO生成失敗の詳細をログへ出力
	/// @param reason 失敗理由
	/// @param desc 生成に使用したCompute PSO記述子
	void LogComputePipelineError(
		const std::string& reason,
		const MadoEngine::Render::ComputePSODesc& desc) {
		std::ostringstream message;
		message
			<< "Compute PSOの生成に失敗しました。理由: "
			<< reason
			<< "、CS: "
			<< desc.csKey
			<< "、RootSignature: "
			<< desc.rootSigKey;
		Logger::Output(message.str(), Logger::Level::Error);
	}

	/// @brief HRESULTを含むCompute PSO生成失敗の詳細をログへ出力
	/// @param result 失敗したHRESULT
	/// @param desc 生成に使用したCompute PSO記述子
	void LogComputePipelineError(
		HRESULT result,
		const MadoEngine::Render::ComputePSODesc& desc) {
		std::ostringstream message;
		message
			<< "Compute PSOの生成に失敗しました。HRESULT: 0x"
			<< std::hex
			<< static_cast<uint32_t>(result)
			<< std::dec
			<< "、CS: "
			<< desc.csKey
			<< "、RootSignature: "
			<< desc.rootSigKey;
		Logger::Output(message.str(), Logger::Level::Error);
	}

} // 無名名前空間

namespace MadoEngine::Render {

	void ComputePSORegistry::Initialize(ID3D12Device* device) {
		assert(device);

		std::lock_guard<std::mutex> lock(cacheMutex_);
		psoCache_.clear();
		failedDescs_.clear();
		device_ = device;
		Logger::Output("Compute PSO Registryを初期化しました。", Logger::Level::Engine);
	}

	void ComputePSORegistry::Finalize() {
		std::lock_guard<std::mutex> lock(cacheMutex_);
		psoCache_.clear();
		failedDescs_.clear();
		device_ = nullptr;
		Logger::Output("Compute PSO Registryを終了しました。", Logger::Level::Engine);
	}

	ID3D12PipelineState* ComputePSORegistry::Get(const ComputePSODesc& desc) {
		std::lock_guard<std::mutex> lock(cacheMutex_);
		if (!device_) {
			if (!failedDescs_.contains(desc)) {
				LogComputePipelineError("Registryが初期化されていません", desc);
				failedDescs_.insert(desc);
			}
			return nullptr;
		}

		const auto cached = psoCache_.find(desc);
		if (cached != psoCache_.end()) {
			return cached->second.Get();
		}
		if (failedDescs_.contains(desc)) {
			return nullptr;
		}

		return CreateAndCache(desc);
	}

	std::size_t ComputePSORegistry::GetCachedCount() const {
		std::lock_guard<std::mutex> lock(cacheMutex_);
		return psoCache_.size();
	}

	ID3D12PipelineState* ComputePSORegistry::CreateAndCache(const ComputePSODesc& desc) {
		if (desc.csKey.empty()) {
			LogComputePipelineError("Compute Shaderのキーが空です", desc);
			failedDescs_.insert(desc);
			return nullptr;
		}
		if (desc.rootSigKey.empty()) {
			LogComputePipelineError("RootSignatureのキーが空です", desc);
			failedDescs_.insert(desc);
			return nullptr;
		}

		const D3D12_SHADER_BYTECODE computeShader =
			MadoEngine::ShaderManager::GetInstance().Get(desc.csKey);
		if (!computeShader.pShaderBytecode || computeShader.BytecodeLength == 0) {
			LogComputePipelineError("Compute ShaderのBytecodeを取得できません", desc);
			failedDescs_.insert(desc);
			return nullptr;
		}

		ID3D12RootSignature* rootSignature =
			MadoEngine::RootSignatureManager::GetInstance().Get(desc.rootSigKey);
		if (!rootSignature) {
			LogComputePipelineError("RootSignatureを取得できません", desc);
			failedDescs_.insert(desc);
			return nullptr;
		}

		D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc{};
		pipelineDesc.pRootSignature = rootSignature;
		pipelineDesc.CS = computeShader;
		pipelineDesc.NodeMask = 0;
		pipelineDesc.CachedPSO = {};
		pipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
		const HRESULT result = device_->CreateComputePipelineState(
			&pipelineDesc,
			IID_PPV_ARGS(&pipelineState)
		);
		if (FAILED(result)) {
			LogComputePipelineError(result, desc);
			failedDescs_.insert(desc);
			return nullptr;
		}

		ID3D12PipelineState* resultPipelineState = pipelineState.Get();
		psoCache_.emplace(desc, std::move(pipelineState));
		return resultPipelineState;
	}

} // MadoEngine::Render名前空間
