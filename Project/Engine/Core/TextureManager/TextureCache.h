#pragma once
#include <filesystem>
#include <fstream>
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include "nlohmann/json.hpp"

namespace MadoEngine {

    /// @brief 元画像に対応するDDSキャッシュのパスと更新状態の管理
    class TextureCache {
    public:
        /// @brief 元画像ディレクトリを基準とするキャッシュ管理を生成
        /// @param textureDirectory 元画像ディレクトリ
        explicit TextureCache(std::filesystem::path textureDirectory)
            : textureDirectory_(std::move(textureDirectory))
            , cacheDirectory_(textureDirectory_ / ".cache") {
            std::ifstream input(cacheDirectory_ / "manifest.json");
            if (!input) {
                return;
            }

            // 壊れた管理情報は未生成として扱い、実行側のPNG読み込みとビルド側の再生成へ接続
            auto manifest = nlohmann::json::parse(input, nullptr, false);
            if (!manifest.is_object() || manifest.value("version", nlohmann::json{}) != kVersion ||
                !manifest.contains("textures") || !manifest["textures"].is_object() ||
                !manifest.contains("converterTime") || !manifest["converterTime"].is_number_integer()) {
                return;
            }
            converterTime_ = manifest["converterTime"].get<std::int64_t>();
            textures_ = std::move(manifest["textures"]);
        }

        /// @brief キャッシュディレクトリを取得
        /// @return キャッシュディレクトリ
        const std::filesystem::path& GetDirectory() const { return cacheDirectory_; }

        /// @brief 元画像の相対階層を維持したDDSパスを取得
        /// @param sourcePath 元画像のパス
        /// @return DDSパス、基準ディレクトリ外の場合は空のパス
        std::filesystem::path GetPath(const std::filesystem::path& sourcePath) const {
            std::error_code error;
            std::filesystem::path relativePath = std::filesystem::relative(sourcePath, textureDirectory_, error);
            if (error || relativePath.empty() || *relativePath.begin() == "..") {
                return {};
            }
            relativePath.replace_extension(".dds");
            return cacheDirectory_ / relativePath;
        }

        /// @brief DDSが元画像の最終更新と保存時の情報を維持しているか確認
        /// @param sourcePath 元画像のパス
        /// @param ddsPath DDSのパス
        /// @return 画像とキャッシュの更新時刻およびサイズが保存時と一致する場合はtrue
        bool IsCurrent(const std::filesystem::path& sourcePath, const std::filesystem::path& ddsPath) const {
            std::error_code error;
            if (!std::filesystem::is_regular_file(ddsPath, error) || error) {
                return false;
            }
            const auto sourceTime = std::filesystem::last_write_time(sourcePath, error);
            if (error) {
                return false;
            }
            const auto ddsTime = std::filesystem::last_write_time(ddsPath, error);
            if (error) {
                return false;
            }
            if (ddsPath != GetPath(sourcePath)) {

                // 手動配置のDDSには管理情報がないため、同階層のPNGとの更新順だけを比較
                return ddsTime >= sourceTime;
            }
            const auto found = textures_.find(GetKey(sourcePath));
            if (found == textures_.end() || !found->is_object()) {
                return false;
            }
            const auto sourceSize = std::filesystem::file_size(sourcePath, error);
            if (error) {
                return false;
            }
            const auto ddsSize = std::filesystem::file_size(ddsPath, error);
            if (error) {
                return false;
            }

            if (found->value("sourceSize", nlohmann::json{}) != sourceSize ||
                found->value("ddsSize", nlohmann::json{}) != ddsSize) {
                return false;
            }

            // 配布用アーカイブの展開で時刻だけが変わった場合は、内容の一致でキャッシュを維持
            std::uint64_t fingerprint = 0;
            if (found->value("sourceTime", nlohmann::json{}) != sourceTime.time_since_epoch().count()) {
                if (!ComputeFingerprint(sourcePath, fingerprint) ||
                    found->value("sourceFingerprint", nlohmann::json{}) != fingerprint) {
                    return false;
                }
            }
            if (found->value("ddsTime", nlohmann::json{}) != ddsTime.time_since_epoch().count()) {
                if (!ComputeFingerprint(ddsPath, fingerprint) ||
                    found->value("ddsFingerprint", nlohmann::json{}) != fingerprint) {
                    return false;
                }
            }
            return true;
        }

        /// @brief 現在の変換ツールで管理情報が生成されたか確認
        /// @param converterTime 変換ツールの更新時刻
        /// @return 同じ変換ツールで生成済みの場合はtrue
        bool IsConverterCurrent(std::filesystem::file_time_type converterTime) const {
            return converterTime_ == converterTime.time_since_epoch().count();
        }

        /// @brief 変換した元画像とDDSの更新情報を登録
        /// @param sourcePath 元画像のパス
        /// @param ddsPath DDSのパス
        /// @return 更新情報の取得に成功した場合はtrue
        bool Record(const std::filesystem::path& sourcePath, const std::filesystem::path& ddsPath) {
            std::uint64_t sourceFingerprint = 0;
            std::uint64_t ddsFingerprint = 0;
            if (!ComputeFingerprint(sourcePath, sourceFingerprint) || !ComputeFingerprint(ddsPath, ddsFingerprint)) {
                return false;
            }
            textures_[GetKey(sourcePath)] = {
                { "sourceTime", std::filesystem::last_write_time(sourcePath).time_since_epoch().count() },
                { "sourceSize", std::filesystem::file_size(sourcePath) },
                { "sourceFingerprint", sourceFingerprint },
                { "ddsTime", std::filesystem::last_write_time(ddsPath).time_since_epoch().count() },
                { "ddsSize", std::filesystem::file_size(ddsPath) },
                { "ddsFingerprint", ddsFingerprint }
            };
            return true;
        }

        /// @brief 変換ツールと各画像の更新情報を保存
        /// @param converterTime 変換ツールの更新時刻
        /// @return 保存に成功した場合はtrue
        bool Save(std::filesystem::file_time_type converterTime) const {
            std::filesystem::create_directories(cacheDirectory_);
            std::ofstream output(cacheDirectory_ / "manifest.json");
            if (!output) {
                return false;
            }
            const nlohmann::json manifest = {
                { "version", kVersion },
                { "converterTime", converterTime.time_since_epoch().count() },
                { "textures", textures_ }
            };
            output << manifest.dump(2);
            output.close();
            return !output.fail();
        }

    private:
        /// @brief ファイル内容の変更検出用FNV-1a値を算出
        /// @param filePath 対象ファイル
        /// @param fingerprint 出力先の指紋値
        /// @return 読み込みに成功した場合はtrue
        static bool ComputeFingerprint(const std::filesystem::path& filePath, std::uint64_t& fingerprint) {
            std::ifstream input(filePath, std::ios::binary);
            if (!input) {
                return false;
            }
            fingerprint = 14695981039346656037ull;
            std::array<char, 65536> buffer{};
            while (input) {
                input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
                for (std::streamsize index = 0; index < input.gcount(); ++index) {
                    fingerprint ^= static_cast<unsigned char>(buffer[static_cast<std::size_t>(index)]);
                    fingerprint *= 1099511628211ull;
                }
            }
            return input.eof();
        }

        /// @brief 元画像の相対パスをUTF-8の管理キーへ変換
        /// @param sourcePath 元画像のパス
        /// @return UTF-8の相対パス
        std::string GetKey(const std::filesystem::path& sourcePath) const {
            const auto key = std::filesystem::relative(sourcePath, textureDirectory_).generic_u8string();
            return std::string(key.begin(), key.end());
        }

        static constexpr std::uint32_t kVersion = 1;
        std::filesystem::path textureDirectory_;
        std::filesystem::path cacheDirectory_;
        std::int64_t converterTime_ = 0;
        nlohmann::json textures_ = nlohmann::json::object();
    };

} // namespace MadoEngine
