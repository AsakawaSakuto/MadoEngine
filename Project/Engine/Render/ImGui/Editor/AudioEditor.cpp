#include "AudioEditor.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>

namespace MadoEngine::Editor {

#ifdef USE_IMGUI 

    void DrawAudioManagerUI() {
        //auto audioManager = AudioManager::GetInstance();

        ImGui::Begin("Audio Manager");

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
                ImGui::Text("Global Volume Settings");
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
        ImGui::Text("Currently Playing");
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
