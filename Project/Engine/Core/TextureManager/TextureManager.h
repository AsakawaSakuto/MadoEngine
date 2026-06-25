#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include <cstdint>
#include "DirectXTex/DirectXTex.h"
#include "Math/Vector2.h"
#include "Core/View/SRVManager.h"

namespace MadoEngine {

    /// @brief テクスチャ1件分の情報
    struct TextureEntry {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        uint32_t index = 0; // 登録順インデックス
        uint32_t width = 0;
        uint32_t height = 0;
    };

    /// @brief テクスチャの読み込みと管理を行うシングルトンクラス
    class TextureManager {
    public:
        /// @brief シングルトンインスタンスを取得する
        /// @return TextureManagerの唯一のインスタンス
        static TextureManager& GetInstance();

        // コピー・ムーブ禁止
        TextureManager(const TextureManager&) = delete;
        TextureManager& operator=(const TextureManager&) = delete;
        TextureManager(TextureManager&&) = delete;
        TextureManager& operator=(TextureManager&&) = delete;

        /// @brief 初期化。"Assets/Texture" 内の全 .png を自動ロードする
        /// @param device Direct3D 12 デバイスポインタ
        /// @param srvManager SRVManagerのポインタ
        void Initialize(ID3D12Device* device, MadoEngine::Core::SRVManager* srvManager);

        /// @brief ファイル名をキーにテクスチャリソースを取得する
        /// @param fileName キーとなるファイル名（拡張子なし）
        /// @return テクスチャリソースの ComPtr（見つからない場合は nullptr）
        Microsoft::WRL::ComPtr<ID3D12Resource> GetTexture(const std::string& fileName) const;

        /// @brief テクスチャのピクセルサイズを取得する
        /// @param fileName キーとなるファイル名（拡張子なし）
        /// @return ピクセル単位の幅・高さ（見つからない場合は {0,0}）
        Vector2 GetPixelSize(const std::string& fileName) const;

        /// @brief テクスチャの登録順インデックスを取得する
        /// @param fileName キーとなるファイル名（拡張子なし）
        /// @return テクスチャインデックス（見つからない場合は UINT32_MAX）
        uint32_t GetTextureIndex(const std::string& fileName) const;

        /// @brief RGBAピクセルを動的テクスチャとして登録または更新します。
        /// @param key 登録キー。
        /// @param width テクスチャ幅。
        /// @param height テクスチャ高さ。
        /// @param rgbaPixels RGBA8ピクセルデータ。
        /// @param dataSize ピクセルデータサイズ。
        /// @return SRVインデックス。失敗時はUINT32_MAX。
        uint32_t RegisterOrUpdateRGBA(
            const std::string& key,
            uint32_t width,
            uint32_t height,
            const uint8_t* rgbaPixels,
            uint32_t dataSize);

        /// @brief 指定キーのテクスチャを登録解除し、SRVを解放します。
        /// @param key 登録キー。
        /// @return 解放できた場合はtrue。
        bool DestroyTexture(const std::string& key);

        /// @brief テクスチャインデックスからSRVハンドルを取得する
        /// @param textureIndex テクスチャインデックス
        /// @return GPU用のSRVハンドル
        D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);

        /// @brief 全テクスチャリソースを解放する
        void Finalize();

    private:
        TextureManager() = default;
        ~TextureManager() = default;

        /// @brief WICファイルを読み込みミップマップを生成する
        /// @param filePath 読み込む画像ファイルのパス（ワイド文字列）
        /// @param mipImage 出力先 ScratchImage
        /// @return 成功した場合 true
        bool LoadTexture(const std::wstring& filePath, DirectX::ScratchImage& mipImage) const;

        /// @brief テクスチャ用 GPU リソースを生成する
        /// @param device Direct3D 12 デバイスポインタ
        /// @param metadata テクスチャメタデータ
        /// @return 生成した ID3D12Resource の ComPtr
        Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(
            ID3D12Device* device,
            const DirectX::TexMetadata& metadata) const;

        /// @brief RGBA8用の動的テクスチャリソースを作成します。
        /// @param width テクスチャ幅。
        /// @param height テクスチャ高さ。
        /// @return 作成されたテクスチャリソース。
        Microsoft::WRL::ComPtr<ID3D12Resource> CreateDynamicTextureResource(
            uint32_t width,
            uint32_t height) const;

        /// @brief RGBA8ピクセルをテクスチャリソースへ書き込みます。
        /// @param texture 書き込み先テクスチャ。
        /// @param width テクスチャ幅。
        /// @param height テクスチャ高さ。
        /// @param rgbaPixels RGBA8ピクセルデータ。
        void UploadRGBAData(
            ID3D12Resource* texture,
            uint32_t width,
            uint32_t height,
            const uint8_t* rgbaPixels) const;

        /// @brief GPU リソースへミップマップデータを転送する
        /// @param texture 転送先リソース
        /// @param mipImage 転送元 ScratchImage
        void UploadTextureData(
            ID3D12Resource* texture,
            const DirectX::ScratchImage& mipImage) const;

        /// @brief std::string を std::wstring に変換する
        /// @param str 変換元の文字列
        /// @return 変換後のワイド文字列
        static std::wstring ConvertString(const std::string& str);

        ID3D12Device* device_ = nullptr;
        MadoEngine::Core::SRVManager* srvManager_ = nullptr;
        std::unordered_map<std::string, TextureEntry> textures_;
        uint32_t nextIndex_ = 0; // 次に割り当てるインデックス
    };

} // namespace MadoEngine::Core
