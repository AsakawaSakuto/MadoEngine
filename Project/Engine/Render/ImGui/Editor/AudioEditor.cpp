#include "AudioEditor.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace MadoEngine::Editor {

    namespace {

        const std::filesystem::path kAudioEditorJsonPath = "Assets/Json/AudioEditor.json";

        /// @brief 音量を0.0fから1.0fの範囲に丸める
        /// @param volume 丸める音量
        /// @return 丸めた音量
        float ClampVolume(float volume) {
            return std::clamp(volume, 0.0f, 1.0f);
        }

        /// @brief Json値から音量を読み込む
        /// @param value 読み込むJson値
        /// @param fallback 読み込めない場合の値
        /// @return 読み込んだ音量
        float ReadVolumeValue(const nlohmann::json& value, float fallback) {
            if (!value.is_number()) {
                return fallback;
            }

            return ClampVolume(value.get<float>());
        }

        /// @brief Jsonオブジェクトから音量を読み込む
        /// @param json 読み込むJsonオブジェクト
        /// @param key 読み込むキー
        /// @param fallback 読み込めない場合の値
        /// @return 読み込んだ音量
        float ReadVolumeMember(const nlohmann::json& json, const char* key, float fallback) {
            if (!json.is_object()) {
                return fallback;
            }

            const auto it = json.find(key);
            if (it == json.end()) {
                return fallback;
            }

            return ReadVolumeValue(*it, fallback);
        }

        /// @brief AudioEditorの音量設定をJsonへ変換する
        /// @return 音量設定Json
        nlohmann::json SerializeAudioEditorVolume() {
            AudioManager& audioManager = AudioManager::GetInstance();

            nlohmann::json root;
            root["masterVolume"] = audioManager.GetMasterVolume();
            root["categoryVolumes"] = {
                {"SE", audioManager.GetSEVolume()},
                {"BGM", audioManager.GetBGMVolume()},
                {"Voice", audioManager.GetVoiceVolume()}
            };

            nlohmann::json individualVolumes = nlohmann::json::object();
            std::vector<std::string> keys = audioManager.GetAllKeys();
            std::sort(keys.begin(), keys.end());
            for (const std::string& key : keys) {
                individualVolumes[key] = {
                    {"type", AudioTypeToString(audioManager.GetAudioType(key))},
                    {"volume", audioManager.GetVolume(key)}
                };
            }
            root["individualVolumes"] = individualVolumes;

            return root;
        }

        /// @brief Jsonからカテゴリ音量を読み込む
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

        /// @brief Jsonから個別音量を読み込む
        /// @param individualVolumes 個別音量Json
        void ApplyIndividualVolumes(const nlohmann::json& individualVolumes) {
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
                    audioManager.SetVolume(key, ReadVolumeMember(value, "volume", audioManager.GetVolume(key)));
                } else if (value.is_number()) {
                    audioManager.SetVolume(key, ReadVolumeValue(value, audioManager.GetVolume(key)));
                }
            }
        }

        /// @brief AudioEditorの音量設定をJsonへ保存する
        /// @return 保存に成功した場合はtrue
        bool SaveAudioEditorVolumeJson() {
            const bool succeeded = Json::JsonFile::Save(kAudioEditorJsonPath, SerializeAudioEditorVolume(), 4, true);
            Logger::Output(
                succeeded ? "AudioEditor : 音量設定Jsonを保存しました" : "AudioEditor : 音量設定Jsonの保存に失敗しました",
                succeeded ? Logger::Level::Assets : Logger::Level::Error
            );
            return succeeded;
        }

        /// @brief AudioEditorの音量設定をJsonから読み込む
        /// @return 読み込みに成功した場合はtrue
        bool LoadAudioEditorVolumeJson() {
            nlohmann::json root;
            if (!Json::JsonFile::Load(kAudioEditorJsonPath, root)) {
                Logger::Output("AudioEditor : 音量設定Jsonの読み込みに失敗しました", Logger::Level::Error);
                return false;
            }

            if (!root.is_object()) {
                Logger::Output("AudioEditor : 音量設定Jsonの形式が不正です", Logger::Level::Error);
                return false;
            }

            AudioManager& audioManager = AudioManager::GetInstance();
            audioManager.SetMasterVolume(ReadVolumeMember(root, "masterVolume", audioManager.GetMasterVolume()));
            ApplyCategoryVolumes(root.value("categoryVolumes", nlohmann::json::object()));
            ApplyIndividualVolumes(root.value("individualVolumes", nlohmann::json::object()));

            Logger::Output("AudioEditor : 音量設定Jsonを読み込みました", Logger::Level::Assets);
            return true;
        }

    }

    bool LoadAudioEditorJson() {
        return LoadAudioEditorVolumeJson();
    }

#ifdef USE_IMGUI

    void DrawAudioManagerUI() {
        //auto audioManager = AudioManager::GetInstance();

        if (!ImGui::Begin("Audio Editor")) {
            ImGui::End();
            return;
        }

        // ---------------------------------------------------
        // タブシステム (SE / BGM / Voice / Volume)
        // ---------------------------------------------------
        if (ImGui::BeginTabBar("AudioTypeTabs")) {

            // 1. 音声リストタブ (SE, BGM, Voice)
            const char* tabNames[] = { "SE", "BGM", "Voice" };
            AudioType tabTypes[] = { AudioType::SE, AudioType::BGM, AudioType::Voice };

            for (int i = 0; i < 3; ++i) {
                if (ImGui::BeginTabItem(tabNames[i])) {
                    static std::unordered_map<std::string, bool> loopStates;

                    // 【変更】列数を 3 から 4 に増やしました
                    if (ImGui::BeginTable("AudioTable", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("Volume", ImGuiTableColumnFlags_WidthFixed, 90.0f); // 【追加】個別ボリューム列
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
                                ImGui::PushItemWidth(-1); // スライダーを列の横幅いっぱいに広げる
                                if (ImGui::SliderFloat("##Volume", &vol, 0.0f, 1.0f, "%.2f")) {
                                    AudioManager::GetInstance().SetVolume(key, vol); // 変更されたら即座に反映
                                }
                                ImGui::PopItemWidth();
                                ImGui::PopID();

                                // --- 3列目: Loopチェックボックス ---
                                ImGui::TableNextColumn();
                                ImGui::PushID((key + "_loop").c_str());
                                ImGui::Checkbox("##Loop", &loopStates[key]);
                                ImGui::PopID();

                                // --- 4列目: Playボタン ---
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
                if (ImGui::SliderFloat("Master Volume", &masterVol, 0.0f, 1.0f, "%.2f")) {
                    AudioManager::GetInstance().SetMasterVolume(masterVol);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                float seVol = AudioManager::GetInstance().GetSEVolume();
                if (ImGui::SliderFloat("SE Volume", &seVol, 0.0f, 1.0f, "%.2f")) {
                    AudioManager::GetInstance().SetSEVolume(seVol);
                }

                float bgmVol = AudioManager::GetInstance().GetBGMVolume();
                if (ImGui::SliderFloat("BGM Volume", &bgmVol, 0.0f, 1.0f, "%.2f")) {
                    AudioManager::GetInstance().SetBGMVolume(bgmVol);
                }

                float voiceVol = AudioManager::GetInstance().GetVoiceVolume();
                if (ImGui::SliderFloat("Voice Volume", &voiceVol, 0.0f, 1.0f, "%.2f")) {
                    AudioManager::GetInstance().SetVoiceVolume(voiceVol);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Button("保存")) {
                    SaveAudioEditorVolumeJson();
                }

                ImGui::SameLine();
                if (ImGui::Button("読込")) {
                    LoadAudioEditorVolumeJson();
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ---------------------------------------------------
        // 再生中の音源一覧とStopボタン
        // ---------------------------------------------------
        ImGui::Text("再生中");
        ImGui::Spacing();

        bool anyPlaying = false;
        auto allKeys = AudioManager::GetInstance().GetAllKeys();

        if (ImGui::BeginTable("PlayingTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Playing Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 60.0f);

            for (const auto& key : allKeys) {
                if (AudioManager::GetInstance().IsPlaying(key)) {
                    anyPlaying = true;
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "▶ %s", key.c_str());

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
