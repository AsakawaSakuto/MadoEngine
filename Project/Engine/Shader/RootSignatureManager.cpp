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
		Logger::Output("[RootSignatureManager] 初期化完了", Logger::Level::Engine);
	}

	void RootSignatureManager::Register(const std::string& key, const D3D12_ROOT_SIGNATURE_DESC& desc) {
		assert(device_);

		if (rootSigMap_.contains(key)) {
			Logger::Output(
				std::format("[RootSignatureManager] 既に登録済みのキーです（スキップ） : {}", key),
				Logger::Level::Warning
			);
			return;
		}

		Microsoft::WRL::ComPtr<ID3DBlob> sigBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errBlob;
		HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
		if (FAILED(hr)) {
			Logger::Output(
				std::format("[RootSignatureManager] RootSignatureのシリアライズに失敗しました : {}", key),
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
				std::format("[RootSignatureManager] RootSignatureの生成に失敗しました : {}", key),
				Logger::Level::Error
			);
			assert(false);
			return;
		}

		rootSigMap_[key] = std::move(rootSig);
		Logger::Output(
			std::format("[RootSignatureManager] RootSignatureを登録しました : {}", key),
			Logger::Level::Engine
		);
	}

	void RootSignatureManager::RegisterRaw(
		const std::string& key,
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature)
	{
		if (rootSigMap_.contains(key)) {
			Logger::Output(
				std::format("[RootSignatureManager] 既に登録済みのキーです（スキップ） : {}", key),
				Logger::Level::Warning
			);
			return;
		}
		rootSigMap_[key] = std::move(rootSignature);
		Logger::Output(
			std::format("[RootSignatureManager] RootSignatureを登録しました（raw） : {}", key),
			Logger::Level::Engine
		);
	}

	ID3D12RootSignature* RootSignatureManager::Get(const std::string& key) const {
		const auto it = rootSigMap_.find(key);
		if (it == rootSigMap_.end()) {
			Logger::Output(
				std::format("[RootSignatureManager] キーが見つかりません : {}", key),
				Logger::Level::Warning
			);
			return nullptr;
		}
		return it->second.Get();
	}

	void RootSignatureManager::Finalize() {
		rootSigMap_.clear();
		device_ = nullptr;
		Logger::Output("[RootSignatureManager] 終了しました", Logger::Level::Engine);
	}

} // namespace MadoEngine
