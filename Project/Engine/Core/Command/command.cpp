#include "command.h"
#include "Core/DxDevice/DxDevice.h"
#include "Utility/Logger/Logger.h"

#include <cassert>

namespace MadoEngine::Core {

    void CommandManager::Initialize(DxDevice* device) {
        device_ = device;

        CreateCommandQueue();
        CreateCommandAllocator();
        CreateCommandList();
        CreateFence();

        Logger::Output("CommandManagerの初期化が完了しました", Logger::Level::Engine);
    }

    void CommandManager::BeginFrame() {
        HRESULT hr = commandAllocator_->Reset();
        assert(SUCCEEDED(hr));

        hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
        assert(SUCCEEDED(hr));
    }

    void CommandManager::EndFrame() {
        HRESULT hr = commandList_->Close();
        assert(SUCCEEDED(hr));

        // 当FrameのCommandをQueueへ投入して完了確認用Fence値を発行
        ID3D12CommandList* commandLists[] = { commandList_.Get() };
        commandQueue_->ExecuteCommandLists(1, commandLists);

        ++fenceValue_;
        hr = commandQueue_->Signal(fence_.Get(), fenceValue_);
        assert(SUCCEEDED(hr));
    }

    void CommandManager::WaitForGPU() {
        if (!fence_ || !fenceEvent_ || fenceValue_ == 0) {
            return;
        }

        if (fence_->GetCompletedValue() < fenceValue_) {

            // 最新の投入済みCommandが完了するまでEvent待機してCPU側の先行を防止
            const HRESULT hr = fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
            assert(SUCCEEDED(hr));
            WaitForSingleObject(fenceEvent_, INFINITE);
        }
    }

    void CommandManager::CreateCommandQueue() {
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;

        HRESULT hr = device_->GetDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_));
        assert(SUCCEEDED(hr));
        Logger::Output("CommandQueueの生成が完了しました", Logger::Level::Engine);
    }

    void CommandManager::CreateCommandAllocator() {
        HRESULT hr = device_->GetDevice()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&commandAllocator_)
        );
        assert(SUCCEEDED(hr));
        Logger::Output("CommandAllocatorの生成が完了しました", Logger::Level::Engine);
    }

    void CommandManager::CreateCommandList() {
        HRESULT hr = device_->GetDevice()->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            commandAllocator_.Get(),
            nullptr,
            IID_PPV_ARGS(&commandList_)
        );
        assert(SUCCEEDED(hr));

        Logger::Output("CommandListの生成が完了しました", Logger::Level::Engine);

        // 初回Resetと記録開始に備え、生成直後の開いた状態を終了
        commandList_->Close();
    }

    void CommandManager::CreateFence() {
        HRESULT hr = device_->GetDevice()->CreateFence(
            fenceValue_,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&fence_)
        );
        assert(SUCCEEDED(hr));

        Logger::Output("Fenceの生成が完了しました", Logger::Level::Engine);

        fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        assert(fenceEvent_ != nullptr);
        Logger::Output("FenceEventの生成が完了しました", Logger::Level::Engine);
    }

}
