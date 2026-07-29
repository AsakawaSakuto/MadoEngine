#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace MadoEngine::Core {

    class DxDevice;

    /// @brief CommandQueueとCommandListを管理するクラス
    class CommandManager {
    public:
        /// @brief 初期化処理
        /// @param device DxDeviceのポインタ
        void Initialize(DxDevice* device);

        /// @brief コマンドリストの記録開始
        void BeginFrame();

        /// @brief コマンドリストを閉じてGPUに送信
        void EndFrame();

        /// @brief 最後に提出したCommandのGPU処理完了を待機
        void WaitForGPU();

        ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
        ID3D12CommandQueue* GetCommandQueue() const { return commandQueue_.Get(); }

        /// @brief 次にSignalされるFence値を取得する
        /// @return 次回提出処理へ紐付けるFence値
        uint64_t GetNextFenceValue() const { return fenceValue_ + 1; }

        /// @brief GPUが完了済みのFence値を取得する
        /// @return 完了済みFence値
        uint64_t GetCompletedFenceValue() const {
            return fence_ ? fence_->GetCompletedValue() : 0;
        }

    private:
        /// @brief コマンドキューの生成
        void CreateCommandQueue();

        /// @brief コマンドアロケータの生成
        void CreateCommandAllocator();

        /// @brief コマンドリストの生成
        void CreateCommandList();

        /// @brief フェンスの生成
        void CreateFence();

        DxDevice* device_ = nullptr;

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
        Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
        uint64_t fenceValue_ = 0;
        HANDLE fenceEvent_ = nullptr;
    };

}
