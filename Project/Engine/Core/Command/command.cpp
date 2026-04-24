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

        Logger::Output("CommandManagerの初期化が完了しました", Logger::Level::Info);
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

        ID3D12CommandList* commandLists[] = { commandList_.Get() };
        commandQueue_->ExecuteCommandLists(1, commandLists);

        WaitForGPU();
    }

    void CommandManager::WaitForGPU() {
        fenceValue_++;
        commandQueue_->Signal(fence_.Get(), fenceValue_);

        if (fence_->GetCompletedValue() < fenceValue_) {
            fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
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
        Logger::Output("CommandQueueの生成が完了しました", Logger::Level::Info);
    }

    void CommandManager::CreateCommandAllocator() {
        HRESULT hr = device_->GetDevice()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&commandAllocator_)
        );
        assert(SUCCEEDED(hr));
        Logger::Output("CommandAllocatorの生成が完了しました", Logger::Level::Info);
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

        Logger::Output("CommandListの生成が完了しました", Logger::Level::Info);

        // コマンドリストは初期状態で開いているので閉じる
        commandList_->Close();
    }

    void CommandManager::CreateFence() {
        HRESULT hr = device_->GetDevice()->CreateFence(
            fenceValue_,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&fence_)
        );
        assert(SUCCEEDED(hr));

        Logger::Output("Fenceの生成が完了しました", Logger::Level::Info);

        fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        assert(fenceEvent_ != nullptr);
        Logger::Output("FenceEventの生成が完了しました", Logger::Level::Info);
    }

}