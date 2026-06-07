#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>
#include <queue>
#include <algorithm>

namespace MadoEngine::Core {
	class DxDevice;

	/// @brief Render Target View (RTV) を管理するクラス
	class RTVManager {
	public:
		static RTVManager& GetInstance();

		/// @brief RTVManagerを初期化する
		/// @param device DxDeviceのポインタ
		/// @param maxDescriptors 最大デスクリプタ数（デフォルト: 256）
		void Initialize(DxDevice* device, uint32_t maxDescriptors = 256);

		/// @brief 新しいRTV用のデスクリプタインデックスを割り当てる
		/// @return 割り当てられたデスクリプタインデックス
		uint32_t Allocate();

		/// @brief デスクリプタインデックスを解放して再利用可能にする
		/// @param index 解放するデスクリプタインデックス
		void Free(uint32_t index);

		/// @brief 指定インデックスのCPUデスクリプタハンドルを取得
		/// @param index デスクリプタインデックス
		/// @return CPUデスクリプタハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const;

		/// @brief Render Target Viewを作成
		/// @param resource リソースのポインタ
		/// @param index デスクリプタインデックス
		/// @param format フォーマット（DXGI_FORMAT_UNKNOWN の場合はリソースのフォーマットを使用）
		void CreateRenderTargetView(
			ID3D12Resource* resource,
			uint32_t index,
			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN
		);

		/// @brief デスクリプタヒープを取得
		/// @return デスクリプタヒープのポインタ
		ID3D12DescriptorHeap* GetDescriptorHeap() const { return descriptorHeap_.Get(); }

		/// @brief 使用中のデスクリプタ数を取得
		/// @return 使用中のデスクリプタ数
		uint32_t GetUsedDescriptorCount() const { 
			return static_cast<uint32_t>(std::count(allocated_.begin(), allocated_.end(), true));
		}

	private:
		DxDevice* device_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_ = nullptr;
		uint32_t descriptorSize_ = 0;
		uint32_t maxDescriptors_ = 0;
		uint32_t nextIndex_ = 0;
		RTVManager() = default;
		~RTVManager() = default;

		std::queue<uint32_t> freeIndices_; // 解放されたインデックスを再利用
		std::vector<bool> allocated_; // デスクリプタの割り当て状態を追跡

		// コピー・ムーブ禁止
		RTVManager(const RTVManager&) = delete;
		RTVManager& operator=(const RTVManager&) = delete;
		RTVManager(RTVManager&&) = delete;
		RTVManager& operator=(RTVManager&&) = delete;
	};
}
