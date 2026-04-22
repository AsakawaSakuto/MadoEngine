#include "DxDevice.h"
#include "Utility/Logger/Logger.h"

#include <cassert>
#include <format>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

namespace MadoEngine::Core {

    void DxDevice::Initialize() {
        // DXGIファクトリーの生成
        HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
        assert(SUCCEEDED(hr));

        // 使用するアダプタを選択
        for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter_)) !=
            DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC3 adapterDesc{};
            hr = useAdapter_->GetDesc3(&adapterDesc);
            assert(SUCCEEDED(hr));
            if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
                std::wstring wideDesc = adapterDesc.Description;
                int size = WideCharToMultiByte(CP_UTF8, 0, wideDesc.c_str(), -1, nullptr, 0, nullptr, nullptr);
                std::string desc(size - 1, '\0');
                WideCharToMultiByte(CP_UTF8, 0, wideDesc.c_str(), -1, &desc[0], size, nullptr, nullptr);
                Logger::Info(std::format("使用しているアダプタ : {}", desc));
                break;
            }
            useAdapter_.Reset();
        }
        assert(useAdapter_.Get() != nullptr);

        // D3D12デバイスの生成
        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0
        };
        const char* featureLevelStrings[] = { "12.2", "12.1", "12.0" };
        for (size_t i = 0; i < _countof(featureLevels); ++i) {
            hr = D3D12CreateDevice(useAdapter_.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
            if (SUCCEEDED(hr)) {
                Logger::Info(std::format("使用している FeatureLevel : {}", featureLevelStrings[i]));
                break;
            }
        }
        assert(device_.Get() != nullptr);
        Logger::Info("D3D12Deviceの生成が完了しました");
    }

}