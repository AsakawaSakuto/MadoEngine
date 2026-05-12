#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include "DirectXTex/DirectXTex.h"

namespace MadoEngine {

    /// @brief テクスチャの読み込みと管理を行うシングルトンクラス
    class TextureManager {
    public:
        /// @brief シングルトンインスタンスを取得する
        /// @return TextureManagerの唯一のインスタンス
        static TextureManager* GetInstance();

        // コピー・ムーブ禁止
        TextureManager(const TextureManager&) = delete;
        TextureManager& operator=(const TextureManager&) = delete;
        TextureManager(TextureManager&&) = delete;
        TextureManager& operator=(TextureManager&&) = delete;

        /// @brief 初期化。"Assets/Texture" 内の全 .png を自動ロードする
        /// @param device Direct3D 12 デバイスポインタ
        void Initialize(ID3D12Device* device);

        /// @brief ファイル名をキーにテクスチャリソースを取得する
        /// @param fileName キーとなるファイル名（拡張子なし）
        /// @return テクスチャリソースの ComPtr（見つからない場合は nullptr）
        Microsoft::WRL::ComPtr<ID3D12Resource> GetTexture(const std::string& fileName) const;

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
        std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12Resource>> textures_;
    };

} // namespace MadoEngine::Core
