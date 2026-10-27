#include <d3d12.h>
#include "DirectXTex/DirectXTex.h"
#include "Core/TextureManager/TextureCache.h"
#include <algorithm>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <utility>

namespace {

    /// @brief PNGから非圧縮DDSを生成するビルド専用変換クラス
    class TextureConverter {
    public:
        /// @brief 元画像と変換ツールの更新時刻を保持
        /// @param textureDirectory 元画像ディレクトリ
        /// @param converterTime 変換ツールの更新時刻
        TextureConverter(std::filesystem::path textureDirectory, std::filesystem::file_time_type converterTime)
            : textureDirectory_(std::move(textureDirectory))
            , cache_(textureDirectory_)
            , converterTime_(converterTime) {
        }

        /// @brief 更新されたPNGをミップマップ付き非圧縮DDSへ変換
        /// @return すべての変換に成功した場合はtrue
        bool Run() {
            if (!std::filesystem::is_directory(textureDirectory_)) {
                std::cerr << "テクスチャディレクトリが見つかりません\n";
                return false;
            }
            std::uint32_t convertedCount = 0;
            std::uint32_t skippedCount = 0;
            bool succeeded = true;
            canReuse_ = cache_.IsConverterCurrent(converterTime_);

            // 生成先を走査から除外し、元PNGだけを変換対象として維持
            for (auto it = std::filesystem::recursive_directory_iterator(textureDirectory_);
                it != std::filesystem::recursive_directory_iterator(); ++it) {
                const auto& entry = *it;
                if (entry.is_directory() && entry.path() == cache_.GetDirectory()) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (!entry.is_regular_file() || entry.path().extension() != ".png") {
                    continue;
                }
                const auto ddsPath = cache_.GetPath(entry.path());
                if (IsReusable(entry.path(), ddsPath)) {
                    ++skippedCount;
                    continue;
                }
                if (!Convert(entry.path(), ddsPath)) {
                    std::cerr << "非圧縮DDSの生成に失敗しました: " << entry.path().string() << '\n';
                    succeeded = false;
                    continue;
                }
                if (!cache_.Record(entry.path(), ddsPath)) {
                    std::cerr << "DDSの更新情報を取得できませんでした: " << entry.path().string() << '\n';
                    succeeded = false;
                    continue;
                }
                ++convertedCount;
            }
            if (succeeded && (convertedCount > 0 || !canReuse_) && !cache_.Save(converterTime_)) {
                std::cerr << "DDSの更新管理情報を保存できませんでした\n";
                succeeded = false;
            }
            std::cout << "非圧縮DDS: 変換 " << convertedCount << " 件、更新なし " << skippedCount << " 件\n";
            return succeeded;
        }

    private:
        /// @brief 元画像と変換処理の変更を反映したDDSか確認
        /// @param sourcePath 元PNGのパス
        /// @param ddsPath DDSのパス
        /// @return 再生成せず使用可能な場合はtrue
        bool IsReusable(const std::filesystem::path& sourcePath, const std::filesystem::path& ddsPath) const {
            if (!canReuse_ || !cache_.IsCurrent(sourcePath, ddsPath)) {
                return false;
            }

            // 変換設定の変更はツールの再ビルドで無効化し、破損したDDSヘッダーも再生成
            DirectX::TexMetadata metadata{};
            const HRESULT result = DirectX::GetMetadataFromDDSFile(ddsPath.c_str(), DirectX::DDS_FLAGS_NONE, metadata);
            return SUCCEEDED(result) && !DirectX::IsCompressed(metadata.format);
        }

        /// @brief 現在のPNG読み込みと同じ設定でミップ付きDDSを保存
        /// @param sourcePath 元PNGのパス
        /// @param ddsPath 出力先DDSのパス
        /// @return 保存に成功した場合はtrue
        bool Convert(const std::filesystem::path& sourcePath, const std::filesystem::path& ddsPath) const {
            DirectX::ScratchImage image;
            HRESULT result = DirectX::LoadFromWICFile(sourcePath.c_str(), DirectX::WIC_FLAGS_DEFAULT_SRGB, nullptr, image);
            if (FAILED(result)) {
                return false;
            }
            DirectX::ScratchImage mipImage;
            if (image.GetMetadata().width == 1 && image.GetMetadata().height == 1) {

                // 1x1画像は単一Mipのまま保持し、既存のPNG経路と同じ形式を維持
                mipImage = std::move(image);
            } else {

                // 1辺が1画素の画像は実行側と同じCPUフィルターで安定した縮小結果を保持
                DirectX::TEX_FILTER_FLAGS filter = DirectX::TEX_FILTER_SRGB;
                if (image.GetMetadata().width == 1 || image.GetMetadata().height == 1) {
                    filter = static_cast<DirectX::TEX_FILTER_FLAGS>(
                        DirectX::TEX_FILTER_SRGB | DirectX::TEX_FILTER_FORCE_NON_WIC | DirectX::TEX_FILTER_LINEAR);
                }

                // 実行時フォールバックと同じ色空間とフィルターで縮小画像を生成
                result = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(),
                    filter, 0, mipImage);
                if (FAILED(result)) {
                    return false;
                }
            }
            std::filesystem::create_directories(ddsPath.parent_path());
            std::filesystem::path temporaryPath = ddsPath;
            temporaryPath += L".tmp." + std::to_wstring(GetCurrentProcessId());

            // 保存途中のDDSを実行側へ見せないよう、完成後に生成済みファイルを置換
            result = DirectX::SaveToDDSFile(mipImage.GetImages(), mipImage.GetImageCount(), mipImage.GetMetadata(),
                DirectX::DDS_FLAGS_NONE, temporaryPath.c_str());
            if (FAILED(result) || !MoveFileExW(temporaryPath.c_str(), ddsPath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
                std::error_code error;
                std::filesystem::remove(temporaryPath, error);
                return false;
            }

            // 未来時刻の元画像でも変換済みと判定できるよう、必要な更新時刻を下限として維持
            const auto cacheTime = (std::max)(std::filesystem::last_write_time(sourcePath), converterTime_);
            if (std::filesystem::last_write_time(ddsPath) < cacheTime) {
                std::filesystem::last_write_time(ddsPath, cacheTime);
            }
            return true;
        }

        std::filesystem::path textureDirectory_;
        MadoEngine::TextureCache cache_;
        std::filesystem::file_time_type converterTime_;
        bool canReuse_ = false;
    };

} // namespace

/// @brief ビルド前にPNGの非圧縮DDSキャッシュを生成
/// @param argc 引数の数
/// @param argv 実行ファイルと元画像ディレクトリのパス
/// @return 変換に成功した場合は0
int wmain(int argc, wchar_t* argv[]) {
    if (argc != 2) {
        std::cerr << "元画像ディレクトリを引数に指定してください\n";
        return 1;
    }
    const HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(result)) {
        std::cerr << "画像変換用COMの初期化に失敗しました\n";
        return 1;
    }
    int exitCode = 1;
    try {

        // ツールの更新時刻を基準に、変換コード変更後の古いDDS再利用を防止
        const auto converterTime = std::filesystem::last_write_time(std::filesystem::absolute(argv[0]));
        TextureConverter converter(std::filesystem::path(argv[1]).lexically_normal(), converterTime);
        if (converter.Run()) {
            exitCode = 0;
        }
    } catch (const std::exception& error) {
        std::cerr << "DDS変換処理でエラーが発生しました: " << error.what() << '\n';
    }
    CoUninitialize();
    return exitCode;
}
