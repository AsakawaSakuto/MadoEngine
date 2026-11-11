#include "AudioSettings.h"
#include "Audio/AudioManager.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include "Utility/AssetKey.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace MadoEngine::Editor {

    namespace {

        const std::filesystem::path kAudioEditorJsonPath = "Assets/Json/AudioEditor.json";

        /// @brief 音量を0から指定上限までの範囲に制限
        /// @param volume 丸める音量
        /// @param maxVolume 許容する音量の上限
        /// @return 丸めた音量
        float ClampVolume(float volume, float maxVolume) {
            return std::clamp(volume, 0.0f, maxVolume);
        }

        /// @brief Json値から音量を読み込み
        /// @param value 読み込むJson値
        /// @param fallback 読み込めない場合の値
        /// @param maxVolume 許容する音量の上限
        /// @return 読み込んだ音量
        float ReadVolumeValue(const nlohmann::json& value, float fallback, float maxVolume = 1.0f) {
            if (!value.is_number()) {
                return fallback;
            }

            return ClampVolume(value.get<float>(), maxVolume);
        }

        /// @brief Jsonオブジェクトから音量を読み込み
        /// @param json 読み込むJsonオブジェクト
        /// @param key 読み込むキー
        /// @param fallback 読み込めない場合の値
        /// @param maxVolume 許容する音量の上限
        /// @return 読み込んだ音量
        float ReadVolumeMember(const nlohmann::json& json, const char* key, float fallback, float maxVolume = 1.0f) {
            if (!json.is_object()) {
                return fallback;
            }

            // 音量項目のない旧Jsonでは現在の設定値を維持
            const auto it = json.find(key);
            if (it == json.end()) {
                return fallback;
            }

            return ReadVolumeValue(*it, fallback, maxVolume);
        }

        /// @brief Jsonから音程設定を読み込み
        /// @param value 個別音声の設定Json
        /// @return 元の音程からの半音数、旧形式または不正値の場合は0
        float ReadPitchSemitones(const nlohmann::json& value) {
            if (!value.is_object()) {
                return 0.0f;
            }
            const auto it = value.find("pitchSemitones");
            if (it == value.end() || !it->is_number()) {
                return 0.0f;
            }

            // 旧Jsonの音程は0へ復元し、極端な数値はFloat変換前に許容範囲へ制限
            const double semitones = it->get<double>();
            if (!std::isfinite(semitones)) {
                return 0.0f;
            }
            return static_cast<float>(std::clamp(semitones,
                static_cast<double>(AudioManager::kPitchSemitoneRange.min),
                static_cast<double>(AudioManager::kPitchSemitoneRange.max)));
        }

        /// @brief Jsonオブジェクトから同時再生数上限を読み込み
        /// @param json 読み込むJsonオブジェクト
        /// @param key 読み込む音声種別のキー
        /// @param fallback 読み込めない場合の値
        /// @return 0以上に制限した同時再生数上限
        int ReadInstanceLimitMember(const nlohmann::json& json, const char* key, int fallback) {
            if (!json.is_object()) {
                return fallback;
            }

            const auto it = json.find(key);
            if (it == json.end() || !it->is_number_integer()) {
                return fallback;
            }

            // 符号なしの巨大値でも整数変換があふれないよう変換前に許容範囲へ制限
            return static_cast<int>(std::clamp(it->get<double>(), 0.0,
                static_cast<double>((std::numeric_limits<int>::max)())));
        }

        /// @brief AudioEditorの音量と音程および同時再生数上限をJsonへ変換
        /// @return 音声設定Json
        nlohmann::json SerializeAudioEditorSettings() {
            AudioManager& audioManager = AudioManager::GetInstance();

            nlohmann::json root;
            root["masterVolume"] = audioManager.GetMasterVolume();
            root["categoryVolumes"] = {
                {"SE", audioManager.GetSEVolume()},
                {"BGM", audioManager.GetBGMVolume()},
                {"Voice", audioManager.GetVoiceVolume()}
            };

            // 起動時と履歴復元時の再生条件を一致させるため音声種別ごとの上限を保存
            root["maxInstancesPerSound"] = {
                {"SE", audioManager.GetMaxInstancesPerSound(AudioType::SE)},
                {"BGM", audioManager.GetMaxInstancesPerSound(AudioType::BGM)},
                {"Voice", audioManager.GetMaxInstancesPerSound(AudioType::Voice)}
            };

            nlohmann::json individualVolumes = nlohmann::json::object();
            std::vector<std::string> keys = audioManager.GetAllKeys();
            std::sort(keys.begin(), keys.end());
            for (const std::string& key : keys) {
                individualVolumes[key] = {
                    {"type", AudioTypeToString(audioManager.GetAudioType(key))},
                    {"volume", audioManager.GetVolume(key)},
                    {"pitchSemitones", audioManager.GetPitch(key)}
                };
            }
            root["individualVolumes"] = individualVolumes;

            return root;
        }

        /// @brief Jsonからカテゴリ音量を読み込み
        /// @param categoryVolumes カテゴリ音量Json
        void ApplyCategoryVolumes(const nlohmann::json& categoryVolumes) {
            if (!categoryVolumes.is_object()) {
                return;
            }

            AudioManager& audioManager = AudioManager::GetInstance();
            audioManager.SetSEVolume(ReadVolumeMember(categoryVolumes, "SE", audioManager.GetSEVolume()));
            audioManager.SetBGMVolume(ReadVolumeMember(categoryVolumes, "BGM", audioManager.GetBGMVolume()));
            audioManager.SetVoiceVolume(ReadVolumeMember(categoryVolumes, "Voice", audioManager.GetVoiceVolume()));
        }

        /// @brief Jsonから音声種別ごとの同時再生数上限を読み込み
        /// @param limits 音声種別ごとの同時再生数上限Json
        void ApplyInstanceLimits(const nlohmann::json& limits) {
            AudioManager& audioManager = AudioManager::GetInstance();

            // 上限設定を持たない旧Jsonでは現在の設定値を維持
            constexpr AudioType types[] = { AudioType::SE, AudioType::BGM, AudioType::Voice };
            for (AudioType type : types) {
                const std::string key = AudioTypeToString(type);
                audioManager.SetMaxInstancesPerSound(type,
                    ReadInstanceLimitMember(limits, key.c_str(), audioManager.GetMaxInstancesPerSound(type)));
            }
        }

        /// @brief Jsonから個別音量と音程を読み込み
        /// @param individualVolumes 個別音声設定Json
        void ApplyIndividualSettings(const nlohmann::json& individualVolumes) {
            if (!individualVolumes.is_object()) {
                return;
            }

            AudioManager& audioManager = AudioManager::GetInstance();
            for (const auto& [key, value] : individualVolumes.items()) {
                if (!audioManager.IsLoaded(key)) {
                    Logger::Output("AudioEditor : Json内の未ロード音声をスキップしました - キー: " + key, Logger::Level::Warning);
                    continue;
                }

                if (value.is_object()) {
                    audioManager.SetVolume(key, ReadVolumeMember(value, "volume", audioManager.GetVolume(key),
                        AudioManager::kMaxIndividualVolume));
                } else if (value.is_number()) {
                    audioManager.SetVolume(key, ReadVolumeValue(value, audioManager.GetVolume(key),
                        AudioManager::kMaxIndividualVolume));
                }

                // 音量だけを持つ旧形式にも対応し、保存前の音程が読込後へ残る状態を防止
                audioManager.SetPitch(key, ReadPitchSemitones(value));
            }
        }

        /// @brief AudioEditorの音量と音程および同時再生数上限をJsonへ保存
        /// @return 保存に成功した場合はtrue
        bool SaveAudioEditorSettingsJson() {
            const bool succeeded = Json::JsonFile::Save(kAudioEditorJsonPath, SerializeAudioEditorSettings(), 4, true);
            Logger::Output(
                [&]() {
                    if (succeeded) {
                        return ("AudioEditor : 音声設定Jsonを保存しました");
                    } else {
                        return ("AudioEditor : 音声設定Jsonの保存に失敗しました");
                    }
                }(),
                [&]() {
                    if (succeeded) {
                        return (Logger::Level::Assets);
                    } else {
                        return (Logger::Level::Error);
                    }
                }()
            );
            return succeeded;
        }

        /// @brief AudioEditorの音量と音程および同時再生数上限をJsonから読み込み
        /// @return 読み込みに成功した場合はtrue
        bool LoadAudioEditorSettingsJson() {
            nlohmann::json root;
            if (!Json::JsonFile::Load(kAudioEditorJsonPath, root)) {
                Logger::Output("AudioEditor : 音声設定Jsonの読み込みに失敗しました", Logger::Level::Error);
                return false;
            }

            if (!root.is_object()) {
                Logger::Output("AudioEditor : 音声設定Jsonの形式が不正です", Logger::Level::Error);
                return false;
            }

            AudioManager& audioManager = AudioManager::GetInstance();
            audioManager.SetMasterVolume(ReadVolumeMember(root, "masterVolume", audioManager.GetMasterVolume()));
            ApplyCategoryVolumes(root.value("categoryVolumes", nlohmann::json::object()));
            ApplyInstanceLimits(root.value("maxInstancesPerSound", nlohmann::json::object()));
            ApplyIndividualSettings(root.value("individualVolumes", nlohmann::json::object()));

            Logger::Output("AudioEditor : 音声設定Jsonを読み込みました", Logger::Level::Assets);
            return true;
        }

    }

    std::string CaptureAudioEditorState() {
        return SerializeAudioEditorSettings().dump();
    }

    bool RestoreAudioEditorState(const std::string& snapshot) {
        const nlohmann::json root = nlohmann::json::parse(snapshot, nullptr, false);
        if (root.is_discarded() || !root.is_object()) {
            return false;
        }

        AudioManager& audioManager = AudioManager::GetInstance();
        audioManager.SetMasterVolume(ReadVolumeMember(root, "masterVolume", audioManager.GetMasterVolume()));
        ApplyCategoryVolumes(root.value("categoryVolumes", nlohmann::json::object()));
        ApplyInstanceLimits(root.value("maxInstancesPerSound", nlohmann::json::object()));
        ApplyIndividualSettings(root.value("individualVolumes", nlohmann::json::object()));
        return true;
    }

    bool SaveAudioEditorJson() {
        return SaveAudioEditorSettingsJson();
    }

    bool LoadAudioEditorJson() {
        return LoadAudioEditorSettingsJson();
    }

} // namespace MadoEngine::Editor
