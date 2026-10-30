#include "AudioEditor.h"
#include "EditorToolbar.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
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
                static_cast<double>(AudioManager::kMinPitchSemitones),
                static_cast<double>(AudioManager::kMaxPitchSemitones)));
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

#ifdef USE_IMGUI

    namespace {

        /// @brief 個別音量反映後のラウドネスまたはピークを表示
        /// @param adjusted 個別音量反映後の推定値
        /// @param isPeak サンプルピークを表示する場合はtrue
        void DrawLoudnessCell(const AudioLoudnessMetrics& adjusted, bool isPeak) {
            if (!adjusted.isValid) {
                ImGui::TextDisabled("--");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("解析可能な音声データなし\nモノラル／ステレオの整数PCMまたはFloat32に対応");
                }
                return;
            }

            double adjustedLevel = adjusted.integratedLufs;
            const char* marker = "";
            if (isPeak) {
                adjustedLevel = adjusted.samplePeakDbfs;
            } else if (adjusted.isShortClip) {
                marker = "*";
            }

            // 個別音量反映後のピークがフルスケールを超えた場合だけ色で識別
            if (isPeak && adjustedLevel > 0.0) {
                ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.2f, 1.0f), "%.1f%s", adjustedLevel, marker);
            } else {
                ImGui::Text("%.1f%s", adjustedLevel, marker);
            }

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("個別Volume適用後の推定値");
                ImGui::TextUnformatted("Master・カテゴリ音量・音程変更・複数音の重なりは未反映");
                if (isPeak) {
                    ImGui::TextUnformatted("Peakは波形サンプルの最大振幅（dBFS）");
                } else {

                    // 参考値と通常のIntegrated LUFSを区別し、非常に小さい信号の推定条件も明示
                    ImGui::TextUnformatted("Integrated LUFS / 調整後はゲート再判定を含まない目安");
                    ImGui::TextUnformatted("-inf: ゲートを通過する信号なし、または無音");
                    if (adjusted.isShortClip) {
                        ImGui::TextUnformatted("*: 400ms未満の音源へ無音を補完した参考LUFS");
                    }
                }
                ImGui::EndTooltip();
            }
        }
    }

    void DrawAudioManagerUI() {

        if (!ImGui::Begin("Audio Editor")) {
            ImGui::End();
            return;
        }

        // 音声種別ごとの一覧と全体音量設定をTab単位で分離
        if (ImGui::BeginTabBar("AudioTypeTabs")) {

            // SE、BGM、Voiceで共通の再生操作UIを同一Loopで構築
            const char* tabNames[] = { "SE", "BGM", "Voice" };
            AudioType tabTypes[] = { AudioType::SE, AudioType::BGM, AudioType::Voice };

            for (int i = 0; i < 3; ++i) {
                if (ImGui::BeginTabItem(tabNames[i])) {
                    static std::unordered_map<std::string, bool> loopStates;

                    // 種別内の全音源へ共通の上限を適用し、再生中の音を維持したまま変更
                    int maxInstances = AudioManager::GetInstance().GetMaxInstancesPerSound(tabTypes[i]);
                    ImGui::SetNextItemWidth(120.0f);
                    if (ImGui::DragInt("同じ音声の同時再生上限", &maxInstances, 1.0f, 0,
                        (std::numeric_limits<int>::max)(), "%d", ImGuiSliderFlags_AlwaysClamp)) {
                        AudioManager::GetInstance().SetMaxInstancesPerSound(tabTypes[i], maxInstances);
                    }
                    ImGui::TextDisabled("上限到達時は追加要求を無視 / 0: 新規再生を停止");
                    ImGui::Separator();

                    // 調整後の音量指標を個別音量の隣へ配置し、狭いWindowでは横スクロールで一覧を維持
                    if (ImGui::BeginTable("AudioTable", 7, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH |
                        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_Resizable)) {
                        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 240.0f);
                        ImGui::TableSetupColumn("Volume", ImGuiTableColumnFlags_WidthFixed, 90.0f); // 【追加】個別ボリューム列
                        ImGui::TableSetupColumn("LUFS", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                        ImGui::TableSetupColumn("Peak (dBFS)", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                        ImGui::TableSetupColumn("Pitch (半音)", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                        ImGui::TableSetupColumn("Loop", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                        ImGui::TableHeadersRow();

                        auto keys = AudioManager::GetInstance().GetAllKeys();
                        for (const auto& key : keys) {
                            if (AudioManager::GetInstance().GetAudioType(key) == tabTypes[i]) {
                                ImGui::TableNextRow();

                                // --- 1列目: 音源名 ---
                                ImGui::TableNextColumn();
                                ImGui::Text("%s", key.c_str());

                                // --- 2列目: 個別ボリューム (Loopの左側) 【追加】 ---
                                ImGui::TableNextColumn();
                                ImGui::PushID((key + "_vol").c_str());

                                float vol = AudioManager::GetInstance().GetVolume(key); // 現在の音量を取得
                                ImGui::PushItemWidth(-1);
                                if (ImGui::DragFloat("##Volume", &vol, 0.01f, 0.0f, AudioManager::kMaxIndividualVolume,
                                    "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
                                    AudioManager::GetInstance().SetVolume(key, vol); // 変更されたら即座に反映
                                }
                                ImGui::PopItemWidth();
                                ImGui::PopID();

                                // 解析済みの元音源と音量倍率だけを参照し、毎フレームのPCM再解析を回避
                                const AudioLoudnessMetrics sourceMetrics = AudioManager::GetInstance().GetLoudnessMetrics(key);
                                const AudioLoudnessMetrics adjustedMetrics = AudioLoudnessAnalyzer::EstimateAfterGain(sourceMetrics, vol);
                                ImGui::TableNextColumn();
                                DrawLoudnessCell(adjustedMetrics, false);
                                ImGui::TableNextColumn();
                                DrawLoudnessCell(adjustedMetrics, true);

                                // 半音数を保存対象へ直接反映して再生中と次回再生の設定を一致
                                ImGui::TableNextColumn();
                                ImGui::PushID((key + "_pitch").c_str());
                                float semitones = AudioManager::GetInstance().GetPitch(key);
                                ImGui::SetNextItemWidth(-1);
                                if (ImGui::DragFloat("##Pitch", &semitones, 0.1f,
                                    AudioManager::kMinPitchSemitones, AudioManager::kMaxPitchSemitones,
                                    "%+.1f", ImGuiSliderFlags_AlwaysClamp)) {
                                    AudioManager::GetInstance().SetPitch(key, semitones);
                                }
                                if (ImGui::IsItemHovered()) {
                                    ImGui::SetTooltip("再生速度を維持して音程を変更\n0: 元の音程 / +12: 1オクターブ上 / -12: 1オクターブ下\nCtrl+クリックで数値入力");
                                }
                                ImGui::PopID();

                                // ループ指定は既存の再生操作へ引き継ぎ
                                ImGui::TableNextColumn();
                                ImGui::PushID((key + "_loop").c_str());
                                ImGui::Checkbox("##Loop", &loopStates[key]);
                                ImGui::PopID();

                                // 現在の音量と音程設定で新しい再生インスタンスを開始
                                ImGui::TableNextColumn();
                                ImGui::PushID((key + "_play").c_str());
                                if (ImGui::Button("Play", ImVec2(60, 0))) {
                                    AudioManager::GetInstance().Play(key, loopStates[key]);
                                }
                                ImGui::PopID();
                            }
                        }
                        ImGui::EndTable();
                    }
                    ImGui::TextDisabled("個別Volume反映後の推定値 / *: 400ms未満の参考LUFS");
                    ImGui::EndTabItem();
                }
            }

            // 2. Volume 調整タブ (前回追加分)
            if (ImGui::BeginTabItem("Volume")) {
                ImGui::Spacing();
                ImGui::Text("全体音量設定");
                ImGui::Separator();
                ImGui::Spacing();

                float masterVol = AudioManager::GetInstance().GetMasterVolume();
                if (ImGui::DragFloat("Master Volume", &masterVol, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
                    AudioManager::GetInstance().SetMasterVolume(masterVol);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                float seVol = AudioManager::GetInstance().GetSEVolume();
                if (ImGui::DragFloat("SE Volume", &seVol, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
                    AudioManager::GetInstance().SetSEVolume(seVol);
                }

                float bgmVol = AudioManager::GetInstance().GetBGMVolume();
                if (ImGui::DragFloat("BGM Volume", &bgmVol, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
                    AudioManager::GetInstance().SetBGMVolume(bgmVol);
                }

                float voiceVol = AudioManager::GetInstance().GetVoiceVolume();
                if (ImGui::DragFloat("Voice Volume", &voiceVol, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
                    AudioManager::GetInstance().SetVoiceVolume(voiceVol);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        // 音声種別のTabからも上限値を保存できる共通操作として配置
        if (ImGui::Button("保存")) {
            SaveAudioEditorSettingsJson();
        }
        ImGui::SameLine();
        if (ImGui::Button("読込")) {
            EditorToolbar::GetInstance().SuppressCurrentDocumentHistory();
            LoadAudioEditorSettingsJson();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 種別Tabとは別に再生中Instanceだけを集約して停止操作を提供
        ImGui::Text("再生中");
        ImGui::Spacing();

        bool anyPlaying = false;
        auto allKeys = AudioManager::GetInstance().GetAllKeys();

        if (ImGui::BeginTable("PlayingTable", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Playing Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("再生数 / 上限", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableHeadersRow();

            for (const auto& key : allKeys) {
                const std::size_t instanceCount = AudioManager::GetInstance().GetPlayingInstanceCount(key);
                if (instanceCount > 0) {
                    anyPlaying = true;
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "▶ %s", key.c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text("%zu / %d", instanceCount, AudioManager::GetInstance().GetMaxInstancesPerSound(
                        AudioManager::GetInstance().GetAudioType(key)));

                    ImGui::TableNextColumn();
                    ImGui::PushID((key + "_stop").c_str());
                    if (ImGui::Button("Stop", ImVec2(60, 0))) {
                        AudioManager::GetInstance().Stop(key);
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }

        if (!anyPlaying) {
            ImGui::TextDisabled("  No audio is currently playing.");
        } else {
            ImGui::Spacing();
            if (ImGui::Button("Stop All", ImVec2(-1, 0))) {
                AudioManager::GetInstance().StopAll();
            }
        }

        ImGui::End();
    }

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
