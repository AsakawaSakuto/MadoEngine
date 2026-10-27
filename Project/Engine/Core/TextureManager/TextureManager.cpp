#include "TextureManager.h"
#include "TextureCache.h"
#include <filesystem>
#include <cassert>
#include <algorithm>
#include <utility>
#include "Utility/Logger/Logger.h"

namespace MadoEngine {

    TextureManager& TextureManager::GetInstance() {
        static TextureManager instance;
        return instance;
    }

    void TextureManager::Initialize(ID3D12Device* device, MadoEngine::Core::SRVManager* srvManager) {
        assert(device);
        assert(srvManager);
        device_ = device;
        srvManager_ = srvManager;

        Logger::Output("TextureManager の初期化を開始します。", Logger::Level::Engine);

        const std::filesystem::path textureDir = "Assets/Texture";

        if (!std::filesystem::exists(textureDir)) {
            Logger::Output("テクスチャフォルダが見つかりません : Assets/Texture", Logger::Level::Warning);
            return;
        }

        const TextureCache cache(textureDir);
        uint32_t loadCount = 0;
        uint32_t ddsCount = 0;

        // 生成キャッシュ自体の二重登録を避け、元画像ごとに読み込み経路を選択
        for (auto it = std::filesystem::recursive_directory_iterator(textureDir);
            it != std::filesystem::recursive_directory_iterator(); ++it) {
            const auto& entry = *it;
            if (entry.is_directory() && entry.path() == cache.GetDirectory()) {
                it.disable_recursion_pending();
                continue;
            }
            if (!entry.is_regular_file()) {
                continue;
            }
            const auto extension = entry.path().extension();
            if (extension != ".png" && extension != ".dds") {
                continue;
            }
            std::filesystem::path sourcePath = entry.path();
            if (extension == ".dds") {

                // PNGと同じ階層のDDSはPNG側で選択し、同じキーでの二重登録を防止
                auto pngPath = sourcePath;
                pngPath.replace_extension(".png");
                if (std::filesystem::is_regular_file(pngPath)) {
                    continue;
                }
            } else {

                // 元画像が更新された場合は古いDDSを使わずPNG経路へ切り替え
                auto adjacentDdsPath = sourcePath;
                adjacentDdsPath.replace_extension(".dds");
                const auto cachedDdsPath = cache.GetPath(sourcePath);
                if (cache.IsCurrent(sourcePath, adjacentDdsPath)) {
                    sourcePath = adjacentDdsPath;
                } else if (cache.IsCurrent(sourcePath, cachedDdsPath)) {
                    sourcePath = cachedDdsPath;
                }
            }

            const std::string key = entry.path().stem().string();

            Logger::Output("テクスチャを読み込みます : " + sourcePath.string(), Logger::Level::Assets);

            DirectX::ScratchImage mipImage;
            bool loaded = LoadTexture(sourcePath.wstring(), mipImage);
            if (!loaded && sourcePath != entry.path()) {

                // DDSが破損していても編集用PNGから復旧し、画面の欠損を回避
                Logger::Output("DDSを読み込めないためPNGへ切り替えます : " + sourcePath.string(), Logger::Level::Warning);
                sourcePath = entry.path();
                loaded = LoadTexture(sourcePath.wstring(), mipImage);
            }
            if (!loaded) {
                Logger::Output("テクスチャの読み込みに失敗しました : " + entry.path().string(), Logger::Level::Error);
                continue;
            }
            if (sourcePath.extension() == ".dds") {
                ++ddsCount;
            }

            Microsoft::WRL::ComPtr<ID3D12Resource> resource =
                CreateTextureResource(device_, mipImage.GetMetadata());
            if (!resource) {
                Logger::Output("テクスチャリソースの生成に失敗しました : " + key, Logger::Level::Error);
                continue;
            }

            UploadTextureData(resource.Get(), mipImage);

            // SRVデスクリプタを確保してShader Resource Viewを作成
            uint32_t srvIndex = srvManager_->Allocate();
            srvManager_->CreateShaderResourceView(resource.Get(), srvIndex);

            TextureEntry textureEntry;
            textureEntry.resource = std::move(resource);
            textureEntry.index = srvIndex;
            textureEntry.width = static_cast<uint32_t>(mipImage.GetMetadata().width);
            textureEntry.height = static_cast<uint32_t>(mipImage.GetMetadata().height);
            textures_[key] = std::move(textureEntry);
            loadCount++;

            Logger::Output("テクスチャの読み込み完了 : " + key, Logger::Level::Assets);
        }

        Logger::Output(
            "TextureManager の初期化完了。ロード数 : " + std::to_string(loadCount) +
            " 枚、DDS : " + std::to_string(ddsCount) + " 枚、PNG : " + std::to_string(loadCount - ddsCount) + " 枚",
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
            Logger::Output("PixelSize の取得に失敗しました。テクスチャが見つかりません: " + fileName, Logger::Level::Warning);
            return { 0.0f, 0.0f };
        }
        return { static_cast<float>(it->second.width), static_cast<float>(it->second.height) };
    }

    uint32_t TextureManager::GetTextureIndex(const std::string& fileName) const {
        uint32_t textureIndex = UINT32_MAX;
        if (!TryGetTextureIndex(fileName, textureIndex)) {
            Logger::Output("TextureIndex の取得に失敗しました。テクスチャが見つかりません: " + fileName, Logger::Level::Warning);
            return UINT32_MAX;
        }
        return textureIndex;
    }

    bool TextureManager::TryGetTextureIndex(const std::string& fileName, uint32_t& outTextureIndex) const {
        const auto it = textures_.find(fileName);
        if (it == textures_.end()) {
            outTextureIndex = UINT32_MAX;
            return false;
        }

        outTextureIndex = it->second.index;
        return true;
    }

    std::vector<std::string> TextureManager::GetTextureNames() const {
        std::vector<std::string> names;
        names.reserve(textures_.size());
        for (const auto& [name, texture] : textures_) {
            (void)texture;
            names.push_back(name);
        }
        std::sort(names.begin(), names.end());
        return names;
    }

    uint32_t TextureManager::RegisterOrUpdateRGBA(
        const std::string& key,
        uint32_t width,
        uint32_t height,
        const uint8_t* rgbaPixels,
        uint32_t dataSize) {
        if (!device_ || !srvManager_) {
            Logger::Output("[Engine] TextureManagerが未初期化のため動的テクスチャを登録できません: " + key, Logger::Level::Error);
            return UINT32_MAX;
        }
        if (key.empty() || width == 0 || height == 0 || !rgbaPixels) {
            Logger::Output("[Engine] 動的テクスチャの登録パラメータが不正です: " + key, Logger::Level::Error);
            return UINT32_MAX;
        }

        const uint32_t requiredSize = width * height * 4;
        if (dataSize < requiredSize) {
            Logger::Output("[Engine] 動的テクスチャのピクセルデータサイズが不足しています: " + key, Logger::Level::Error);
            return UINT32_MAX;
        }

        auto it = textures_.find(key);
        if (it != textures_.end()) {
            TextureEntry& existing = it->second;
            if (existing.width != width || existing.height != height || !existing.resource) {

                // Size変更時も既存SRV Indexを再利用して外部Texture参照を維持
                existing.resource = CreateDynamicTextureResource(width, height);
                if (!existing.resource) {
                    Logger::Output("[Engine] 動的テクスチャの再作成に失敗しました: " + key, Logger::Level::Error);
                    return UINT32_MAX;
                }
                srvManager_->CreateShaderResourceView(existing.resource.Get(), existing.index, DXGI_FORMAT_R8G8B8A8_UNORM);
                existing.width = width;
                existing.height = height;
            }

            UploadRGBAData(existing.resource.Get(), width, height, rgbaPixels);
            return existing.index;
        }

        // 新規KeyだけResourceとSRV領域を追加確保
        Microsoft::WRL::ComPtr<ID3D12Resource> resource = CreateDynamicTextureResource(width, height);
        if (!resource) {
            Logger::Output("[Engine] 動的テクスチャの作成に失敗しました: " + key, Logger::Level::Error);
            return UINT32_MAX;
        }

        UploadRGBAData(resource.Get(), width, height, rgbaPixels);

        const uint32_t srvIndex = srvManager_->Allocate();
        srvManager_->CreateShaderResourceView(resource.Get(), srvIndex, DXGI_FORMAT_R8G8B8A8_UNORM);

        TextureEntry entry;
        entry.resource = std::move(resource);
        entry.index = srvIndex;
        entry.width = width;
        entry.height = height;
        textures_[key] = std::move(entry);

        Logger::Output("[Engine] 動的テクスチャを登録しました: " + key, Logger::Level::Assets);
        return srvIndex;
    }

    bool TextureManager::RenameTexture(const std::string& currentKey, const std::string& newKey) {
        if (currentKey.empty() || newKey.empty()) {
            Logger::Output("[Engine] テクスチャの管理キーを空文字へ変更できません", Logger::Level::Warning);
            return false;
        }
        if (currentKey == newKey) {
            return textures_.contains(currentKey);
        }

        auto currentIt = textures_.find(currentKey);
        if (currentIt == textures_.end()) {
            Logger::Output("[Engine] 名前変更対象のテクスチャが見つかりません: " + currentKey, Logger::Level::Warning);
            return false;
        }
        if (textures_.contains(newKey)) {
            Logger::Output("[Engine] 変更先のテクスチャ管理キーが既に存在します: " + newKey, Logger::Level::Warning);
            return false;
        }

        // ResourceとDescriptorを再生成せずMap NodeのKeyだけを置換
        auto node = textures_.extract(currentIt);
        node.key() = newKey;
        textures_.insert(std::move(node));

        Logger::Output("[Engine] テクスチャの管理キーを変更しました: " + currentKey + " -> " + newKey, Logger::Level::Assets);
        return true;
    }

    bool TextureManager::DestroyTexture(const std::string& key) {
        auto it = textures_.find(key);
        if (it == textures_.end()) {
            Logger::Output("[Engine] 解放対象のテクスチャが見つかりません: " + key, Logger::Level::Warning);
            return false;
        }

        const uint32_t srvIndex = it->second.index;

        // Texture所有権を破棄してから対応するSRV領域をAllocatorへ返却
        textures_.erase(it);

        if (srvManager_ && srvIndex != UINT32_MAX) {
            srvManager_->Free(srvIndex);
        }

        Logger::Output("[Engine] テクスチャを解放しました: " + key, Logger::Level::Assets);
        return true;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureIndex) {
        return srvManager_->GetGPUHandle(textureIndex);
    }

    void TextureManager::Finalize() {
        textures_.clear();
        nextIndex_ = 0;
        device_ = nullptr;
        Logger::Output("TextureManager を終了しました。", Logger::Level::Engine);
    }

    bool TextureManager::LoadTexture(const std::wstring& filePath, DirectX::ScratchImage& mipImage) const {
        if (std::filesystem::path(filePath).extension() == ".dds") {

            // 事前生成済みのMipをそのまま使用し、起動時のPNG展開と縮小処理を省略
            const HRESULT result = DirectX::LoadFromDDSFile(filePath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, mipImage);
            if (FAILED(result)) {
                return false;
            }
            const auto& metadata = mipImage.GetMetadata();
            return !DirectX::IsCompressed(metadata.format) &&
                metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE2D &&
                metadata.arraySize == 1 && !metadata.IsCubemap();
        }
        DirectX::ScratchImage image;
        HRESULT hr = DirectX::LoadFromWICFile(
            filePath.c_str(),

            // 色空間情報を持たない一般画像をsRGBとして解釈
            DirectX::WIC_FLAGS_DEFAULT_SRGB,
            nullptr,
            image);

        if (FAILED(hr)) {
            return false;
        }

        if (image.GetMetadata().width == 1 && image.GetMetadata().height == 1) {

            // 1x1画像は追加Mipを生成できないため、Decode済みの単一Mipをそのまま使用
            mipImage = std::move(image);
            return true;
        }

        // 1辺が1画素の画像でも同じ縮小結果になるようCPUの線形フィルターを明示
        DirectX::TEX_FILTER_FLAGS filter = DirectX::TEX_FILTER_SRGB;
        if (image.GetMetadata().width == 1 || image.GetMetadata().height == 1) {
            filter = static_cast<DirectX::TEX_FILTER_FLAGS>(
                DirectX::TEX_FILTER_SRGB | DirectX::TEX_FILTER_FORCE_NON_WIC | DirectX::TEX_FILTER_LINEAR);
        }

        // 縮小可能な画像ではMipを生成し、遠距離のTexture参照を安定化
        hr = DirectX::GenerateMipMaps(
            image.GetImages(),
            image.GetImageCount(),
            image.GetMetadata(),
            filter,
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

        // 読み込み画像のDimensionとMip構成を保持したResource Descを構築
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Width = static_cast<UINT>(metadata.width);
        resourceDesc.Height = static_cast<UINT>(metadata.height);
        resourceDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
        resourceDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);
        resourceDesc.Format = metadata.format;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);

        // CPUから直接書き込むTexture向けのWRITE_BACK Custom Heap
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_CUSTOM;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
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

    Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateDynamicTextureResource(
        uint32_t width,
        uint32_t height) const {

        // 頻繁な更新をCommand転送なしで反映できる単一MipのWRITE_BACK Texture
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Width = width;
        resourceDesc.Height = height;
        resourceDesc.MipLevels = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_CUSTOM;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        HRESULT hr = device_->CreateCommittedResource(
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

    void TextureManager::UploadRGBAData(
        ID3D12Resource* texture,
        uint32_t width,
        uint32_t height,
        const uint8_t* rgbaPixels) const {
        D3D12_BOX box{};
        box.left = 0;
        box.top = 0;
        box.front = 0;
        box.right = width;
        box.bottom = height;
        box.back = 1;

        // RGBA8のRow Pitchと全Slice Pitchを連続Pixel配列から算出
        texture->WriteToSubresource(
            0,
            &box,
            rgbaPixels,
            width * 4,
            width * height * 4);
    }

    void TextureManager::UploadTextureData(
        ID3D12Resource* texture,
        const DirectX::ScratchImage& mipImage) const {

        const DirectX::TexMetadata& metadata = mipImage.GetMetadata();

        // DirectXTexが保持する各Mip固有のPitchをそのままSubresourceへ転送
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
