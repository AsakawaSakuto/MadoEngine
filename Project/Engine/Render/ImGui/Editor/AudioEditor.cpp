#include "AudioEditor.h"

#ifdef USE_IMGUI

#include "Audio/AudioManager.h"
#include "ImGuiHeaders.h"
#include "Utility/Settings/AudioSettings.h"
#include "EditorToolbar.h"
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

        /// @brief 音源一覧のファイル情報
        struct AudioFileItem {
            std::string key;
            std::string fileName;
            std::string filePath;
        };

        /// @brief 音源一覧のフォルダー階層
        struct AudioDirectory {
            std::map<std::string, AudioDirectory> directories;
            std::map<std::string, AudioFileItem> files;
        };

        /// @brief 読み込み済みの音源パスから種別ごとのフォルダー階層を構築
        /// @param type 表示対象の音声種別
        /// @return フォルダー名と音源名の昇順に整列した階層
        AudioDirectory CreateAudioDirectory(AudioType type) {
            AudioDirectory root;
            const AudioManager& audioManager = AudioManager::GetInstance();
            const std::filesystem::path currentDirectory = std::filesystem::current_path();
            const std::filesystem::path audioDirectory = (currentDirectory / "Assets/Audio").lexically_normal();
            for (const std::string& key : audioManager.GetAllKeys()) {
                if (audioManager.GetAudioType(key) != type) {
                    continue;
                }

                // 読み込み済みのパスだけで階層を構築し、毎Frameのディスク走査を回避
                const std::filesystem::path filePath = audioManager.GetFilePath(key);
                std::filesystem::path relativePath =
                    (currentDirectory / filePath).lexically_normal().lexically_relative(audioDirectory);
                AudioDirectory* directory = &root;
                if (filePath.empty() || relativePath.empty() || *relativePath.begin() == "..") {

                    // 基準Directory外から手動登録した音源も操作できるよう専用の項目へ集約
                    directory = &root.directories["その他（Assets/Audio外）"];
                    relativePath = filePath.filename();
                } else {

                    // タブ名と重複する先頭の種別Directoryだけを表示階層から省略
                    auto component = relativePath.begin();
                    if (AssetKey::Equals(component->string(), AudioTypeToString(type))) {
                        std::filesystem::path categoryRelativePath;
                        for (++component; component != relativePath.end(); ++component) {
                            categoryRelativePath /= *component;
                        }
                        relativePath = std::move(categoryRelativePath);
                    }
                }

                // mapの整列順により、各階層でフォルダーを先頭、その後に音源を名前順で配置
                for (const std::filesystem::path& component : relativePath.parent_path()) {
                    directory = &directory->directories[component.string()];
                }
                std::string fileName = relativePath.filename().string();
                if (fileName.empty()) {
                    fileName = key;
                }
                directory->files.emplace(key, AudioFileItem{ key, fileName, filePath.generic_string() });
            }
            return root;
        }

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

        /// @brief 階層内の音源名と個別設定および再生操作を一行で描画
        /// @param item 描画対象の音源情報
        /// @param loopStates 音源キーごとのループ再生設定
        void DrawAudioFileRow(const AudioFileItem& item, std::unordered_map<std::string, bool>& loopStates) {
            AudioManager& audioManager = AudioManager::GetInstance();
            ImGui::PushID(item.key.c_str());
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            // Leafの余白をフォルダーと共通化し、Name列だけを階層に応じて字下げ
            ImGui::TreeNodeEx("##AudioFile", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding, "%s", item.fileName.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s\nキー: %s", item.filePath.c_str(), item.key.c_str());
            }

            ImGui::TableNextColumn();
            float volume = audioManager.GetVolume(item.key);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::DragFloat("##Volume", &volume, 0.01f, 0.0f, AudioManager::kMaxIndividualVolume,
                "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
                audioManager.SetVolume(item.key, volume);
            }

            // 解析済みの元音源と音量倍率だけを参照し、毎FrameのPCM再解析を回避
            const AudioLoudnessMetrics sourceMetrics = audioManager.GetLoudnessMetrics(item.key);
            const AudioLoudnessMetrics adjustedMetrics = AudioLoudnessAnalyzer::EstimateAfterGain(sourceMetrics, volume);
            ImGui::TableNextColumn();
            DrawLoudnessCell(adjustedMetrics, false);
            ImGui::TableNextColumn();
            DrawLoudnessCell(adjustedMetrics, true);

            // 半音数を保存対象へ直接反映して再生中と次回再生の設定を一致
            ImGui::TableNextColumn();
            float semitones = audioManager.GetPitch(item.key);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::DragFloat("##Pitch", &semitones, 0.1f,
                AudioManager::kPitchSemitoneRange.min, AudioManager::kPitchSemitoneRange.max,
                "%+.1f", ImGuiSliderFlags_AlwaysClamp)) {
                audioManager.SetPitch(item.key, semitones);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("再生速度を維持して音程を変更\n0: 元の音程 / +12: 1オクターブ上 / -12: 1オクターブ下\nCtrl+クリックで数値入力");
            }

            // 音源キーを使用して階層の開閉後も同じループ設定で再生
            ImGui::TableNextColumn();
            ImGui::Checkbox("##Loop", &loopStates[item.key]);
            ImGui::TableNextColumn();
            if (ImGui::Button("Play", ImVec2(60, 0))) {
                audioManager.Play(item.key, loopStates[item.key]);
            }
            ImGui::PopID();
        }

        /// @brief フォルダーの開閉状態に応じて音源一覧を再帰的に描画
        /// @param directory 描画対象のフォルダー階層
        /// @param loopStates 音源キーごとのループ再生設定
        void DrawAudioDirectory(const AudioDirectory& directory, std::unordered_map<std::string, bool>& loopStates) {

            // フォルダー行にも通常の操作部品と同じ上下余白を確保し、開閉操作のクリック領域を拡張
            for (const auto& [name, child] : directory.directories) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID(name.c_str());
                if (ImGui::TreeNodeEx("##AudioDirectory", ImGuiTreeNodeFlags_DefaultOpen |
                    ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_LabelSpanAllColumns |
                    ImGuiTreeNodeFlags_FramePadding, "%s", name.c_str())) {
                    DrawAudioDirectory(child, loopStates);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            for (const auto& [key, item] : directory.files) {
                DrawAudioFileRow(item, loopStates);
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
                    ImGui::PushID(i);
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
                        ImGuiTableFlags_ScrollX | ImGuiTableFlags_Resizable)) {
                        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 240.0f);
                        ImGui::TableSetupColumn("Volume", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                        ImGui::TableSetupColumn("LUFS", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                        ImGui::TableSetupColumn("Peak (dBFS)", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                        ImGui::TableSetupColumn("Pitch (半音)", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                        ImGui::TableSetupColumn("Loop", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                        ImGui::TableHeadersRow();

                        const AudioDirectory directory = CreateAudioDirectory(tabTypes[i]);
                        DrawAudioDirectory(directory, loopStates);
                        ImGui::EndTable();
                    }
                    ImGui::TextDisabled("個別Volume反映後の推定値 / *: 400ms未満の参考LUFS");
                    ImGui::PopID();
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
            SaveAudioEditorJson();
        }
        ImGui::SameLine();
        if (ImGui::Button("読込")) {
            EditorToolbar::GetInstance().SuppressCurrentDocumentHistory();
            LoadAudioEditorJson();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 種別Tabとは別に再生中Instanceだけを集約して停止操作を提供
        ImGui::Text("再生中");
        ImGui::Spacing();

        bool anyPlaying = false;
        auto allKeys = AudioManager::GetInstance().GetAllKeys();

        if (ImGui::BeginTable("PlayingTable", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH)) {
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

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
