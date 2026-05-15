#include "TextureManager.h"
#include <filesystem>
#include <cassert>
#include "Utility/Logger/Logger.h"

namespace MadoEngine {

    TextureManager* TextureManager::GetInstance() {
        static TextureManager instance;
        return &instance;
    }

    void TextureManager::Initialize(ID3D12Device* device) {
        assert(device);
        device_ = device;

        Logger::Output("TextureManager の初期化を開始します。", Logger::Level::Engine);

        const std::filesystem::path textureDir = "Assets/Texture";

        if (!std::filesystem::exists(textureDir)) {
            Logger::Output("テクスチャフォルダが見つかりません : Assets/Texture", Logger::Level::Warning);
            return;
        }

        uint32_t loadCount = 0;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(textureDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            if (entry.path().extension() != ".png") {
                continue;
            }

            const std::wstring widePath = entry.path().wstring();
            const std::string key = entry.path().stem().string();

            Logger::Output("テクスチャを読み込みます : " + entry.path().string(), Logger::Level::Assets);

            DirectX::ScratchImage mipImage;
            if (!LoadTexture(widePath, mipImage)) {
                Logger::Output("テクスチャの読み込みに失敗しました : " + entry.path().string(), Logger::Level::Error);
                continue;
            }

            Microsoft::WRL::ComPtr<ID3D12Resource> resource =
                CreateTextureResource(device_, mipImage.GetMetadata());
            if (!resource) {
                Logger::Output("テクスチャリソースの生成に失敗しました : " + key, Logger::Level::Error);
                continue;
            }

            UploadTextureData(resource.Get(), mipImage);

            TextureEntry entry;
            entry.resource = std::move(resource);
            entry.index    = nextIndex_++;
            entry.width    = static_cast<uint32_t>(mipImage.GetMetadata().width);
            entry.height   = static_cast<uint32_t>(mipImage.GetMetadata().height);
            textures_[key] = std::move(entry);
            loadCount++;

            Logger::Output("テクスチャの読み込み完了 : " + key, Logger::Level::Assets);
        }

        Logger::Output(
            "TextureManager の初期化完了。ロード数 : " + std::to_string(loadCount) + " 枚",
            Logger::Level::Engine);
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::GetTexture(const std::string& fileName) const {
        auto it = textures_.find(fileName);
        if (it == textures_.end()) {
            Logger::Output("テクスチャが見つかりません: " + fileName, Logger::Level::Warning);
            return nullptr;
        }
        return it->second.resource;
    }

    Vector2 TextureManager::GetPixelSize(const std::string& fileName) const {
        auto it = textures_.find(fileName);
        if (it == textures_.end()) {
            Logger::Output("[TextureManager] PixelSize の取得に失敗しました。テクスチャが見つかりません: " + fileName, Logger::Level::Warning);
            return { 0.0f, 0.0f };
        }
        return { static_cast<float>(it->second.width), static_cast<float>(it->second.height) };
    }

    uint32_t TextureManager::GetTextureIndex(const std::string& fileName) const {
        auto it = textures_.find(fileName);
        if (it == textures_.end()) {
            Logger::Output("[TextureManager] TextureIndex の取得に失敗しました。テクスチャが見つかりません: " + fileName, Logger::Level::Warning);
            return UINT32_MAX;
        }
        return it->second.index;
    }

    void TextureManager::Finalize() {
        textures_.clear();
        nextIndex_ = 0;
        device_ = nullptr;
        Logger::Output("TextureManager を終了しました。", Logger::Level::Engine);
    }

    bool TextureManager::LoadTexture(const std::wstring& filePath, DirectX::ScratchImage& mipImage) const {
        DirectX::ScratchImage image;
        HRESULT hr = DirectX::LoadFromWICFile(
            filePath.c_str(),
            DirectX::WIC_FLAGS_FORCE_SRGB,
            nullptr,
            image);

        if (FAILED(hr)) {
            return false;
        }

        hr = DirectX::GenerateMipMaps(
            image.GetImages(),
            image.GetImageCount(),
            image.GetMetadata(),
            DirectX::TEX_FILTER_SRGB,
            0,
            mipImage);

        if (FAILED(hr)) {
            return false;
        }

        return true;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateTextureResource(
        ID3D12Device* device,
        const DirectX::TexMetadata& metadata) const {

        // リソースデスクを TexMetadata から設定
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Width            = static_cast<UINT>(metadata.width);
        resourceDesc.Height           = static_cast<UINT>(metadata.height);
        resourceDesc.MipLevels        = static_cast<UINT16>(metadata.mipLevels);
        resourceDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
        resourceDesc.Format           = metadata.format;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Dimension        = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);

        // CUSTOM ヒープ（WRITE_BACK / L0）
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type                 = D3D12_HEAP_TYPE_CUSTOM;
        heapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&resource));

        if (FAILED(hr)) {
            return nullptr;
        }

        return resource;
    }

    void TextureManager::UploadTextureData(
        ID3D12Resource* texture,
        const DirectX::ScratchImage& mipImage) const {

        const DirectX::TexMetadata& metadata = mipImage.GetMetadata();

        for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
            const DirectX::Image* img = mipImage.GetImage(mipLevel, 0, 0);
            if (!img) {
                continue;
            }

            texture->WriteToSubresource(
                static_cast<UINT>(mipLevel),
                nullptr,
                img->pixels,
                static_cast<UINT>(img->rowPitch),
                static_cast<UINT>(img->slicePitch));
        }
    }

    std::wstring TextureManager::ConvertString(const std::string& str) {
        if (str.empty()) {
            return {};
        }
        const int size = MultiByteToWideChar(
            CP_UTF8, 0,
            str.c_str(), static_cast<int>(str.size()),
            nullptr, 0);
        std::wstring result(size, L'\0');
        MultiByteToWideChar(
            CP_UTF8, 0,
            str.c_str(), static_cast<int>(str.size()),
            result.data(), size);
        return result;
    }

} // namespace MadoEngine::Core
