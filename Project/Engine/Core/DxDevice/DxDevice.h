#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

namespace MadoEngine {

    class DxDevice {
    public:
        void Initialize();

        ID3D12Device* GetDevice() const { return device_.Get(); }
        IDXGIFactory7* GetDxgiFactory() const { return dxgiFactory_.Get(); }

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
        Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;
        Microsoft::WRL::ComPtr<ID3D12Device> device_;
    };

}