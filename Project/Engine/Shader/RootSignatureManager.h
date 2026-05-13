#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>

namespace MadoEngine::Core {
	class DxDevice;
}

namespace MadoEngine {

	/// @brief RootSignatureをキーで管理するシングルトンクラス
	class RootSignatureManager {
	public:
		/// @brief シングルトンインスタンスを取得する
		/// @return RootSignatureManagerの唯一のインスタンス
		static RootSignatureManager* GetInstance();

		// コピー・ムーブ禁止
		RootSignatureManager(const RootSignatureManager&) = delete;
		RootSignatureManager& operator=(const RootSignatureManager&) = delete;
		RootSignatureManager(RootSignatureManager&&) = delete;
		RootSignatureManager& operator=(RootSignatureManager&&) = delete;

		/// @brief 初期化
		/// @param device DxDeviceポインタ
		void Initialize(Core::DxDevice* device);

		/// @brief RootSignatureDescからRootSignatureを生成して登録する
		/// @param key 識別キー
		/// @param desc RootSignatureDesc
		void Register(const std::string& key, const D3D12_ROOT_SIGNATURE_DESC& desc);

		/// @brief 既存のRootSignatureを登録する
		/// @param key 識別キー
		/// @param rootSignature 登録するRootSignature
		void RegisterRaw(const std::string& key, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature);

		/// @brief キーに対応するRootSignatureを取得する
		/// @param key 識別キー
		/// @return ID3D12RootSignature ポインタ（見つからない場合は nullptr）
		ID3D12RootSignature* Get(const std::string& key) const;

		/// @brief 全RootSignatureを解放する
		void Finalize();

	private:
		RootSignatureManager() = default;
		~RootSignatureManager() = default;

		Core::DxDevice* device_ = nullptr;
		std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootSigMap_;
	};

} // namespace MadoEngine
