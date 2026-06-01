#pragma once
#include <Windows.h>
#include <vector>
#include <memory>
#include <d3d12.h>
#include <wrl/client.h>
#include "CoreHeaders.h"
#include "RenderHeaders.h"
#include "UtilityHeaders.h"
#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#include "Render/ImGui/ImGuiManager.h"
#endif

namespace MadoEngine
{
	/// @brief ゲームエンジンの実行制御を管理するクラス
	class Execution
	{
	public:
		/// @brief 初期化処理
		void Initialize(HINSTANCE hInstance);

		/// @brief 更新処理
		void Update();

		/// @brief 描画前処理
		void PreDraw();

		/// @brief ImGuiレイアウト開始（DockSpace・GameView生成）
		/// @brief シーンの DrawImGui() より前に呼ぶこと
		void BeginImGuiLayout();

		/// @brief 描画後処理（ImGui確定・Present）
		void PostDraw();

		/// @brief 終了処理
		void Finalize();

		/// @brief ゲームループを継続するかどうかを取得
		bool IsRunning();

		float GetDeltaTime() const { return static_cast<float>(deltaTime_->GetDeltaTime()); }
	private:

		//D3DResourceLeakChecker leakChecker;

		std::unique_ptr<MadoEngine::Screen::WindowsAPI> windowsAPI_;
		MadoEngine::Screen::WindowsAPI::WindowDesc winDesc_;

		std::unique_ptr<MadoEngine::DeltaTime> deltaTime_;

		std::unique_ptr<MadoEngine::Core::DxDevice> dxDevice_;

		std::unique_ptr<MadoEngine::Core::CommandManager> commandManager_;

		std::unique_ptr<MadoEngine::Screen::SwapChain> swapChain_;

		std::unique_ptr<MadoEngine::Core::RTVManager> rtvManager_;
		std::unique_ptr<MadoEngine::Core::SRVManager> srvManager_;
		std::unique_ptr<MadoEngine::Core::DSVManager> dsvManager_;

		std::unique_ptr<MadoEngine::Render::PSOFactory> psoFactory_;
		std::unique_ptr<MadoEngine::Render::PSORegistry> psoRegistry_;

		std::unique_ptr<MadoEngine::Core::DepthStencilBuffer> depthStencilBuffer_;

		std::unique_ptr<MadoEngine::Render::ViewportScissor> viewportScissor_; // ビューポート＆シザー矩形

#ifdef USE_IMGUI
		std::unique_ptr<MadoEngine::ImGuiManager> imguiManager_;
		std::unique_ptr<MadoEngine::Render::RenderTexture> offscreenRT_;

        void DrawAudioManagerUI() {
            auto* audioManager = AudioManager::GetInstance();

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

                            auto keys = audioManager->GetAllKeys();
                            for (const auto& key : keys) {
                                if (audioManager->GetAudioType(key) == tabTypes[i]) {
                                    ImGui::TableNextRow();

                                    // --- 1列目: 音源名 ---
                                    ImGui::TableNextColumn();
                                    ImGui::Text("%s", key.c_str());

                                    // --- 2列目: 個別ボリューム (Loopの左側) 【追加】 ---
                                    ImGui::TableNextColumn();
                                    ImGui::PushID((key + "_vol").c_str());

                                    float vol = audioManager->GetVolume(key); // 現在の音量を取得
                                    ImGui::PushItemWidth(-1); // スライダーを列の横幅いっぱいに広げる
                                    if (ImGui::SliderFloat("##Volume", &vol, 0.0f, 1.0f, "%.2f")) {
                                        audioManager->SetVolume(key, vol); // 変更されたら即座に反映
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
                                        audioManager->Play(key, loopStates[key]);
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

                    float masterVol = audioManager->GetMasterVolume();
                    if (ImGui::SliderFloat("Master Volume", &masterVol, 0.0f, 1.0f, "%.2f")) {
                        audioManager->SetMasterVolume(masterVol);
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    float seVol = audioManager->GetSEVolume();
                    if (ImGui::SliderFloat("SE Volume", &seVol, 0.0f, 1.0f, "%.2f")) {
                        audioManager->SetSEVolume(seVol);
                    }

                    float bgmVol = audioManager->GetBGMVolume();
                    if (ImGui::SliderFloat("BGM Volume", &bgmVol, 0.0f, 1.0f, "%.2f")) {
                        audioManager->SetBGMVolume(bgmVol);
                    }

                    float voiceVol = audioManager->GetVoiceVolume();
                    if (ImGui::SliderFloat("Voice Volume", &voiceVol, 0.0f, 1.0f, "%.2f")) {
                        audioManager->SetVoiceVolume(voiceVol);
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
            auto allKeys = audioManager->GetAllKeys();

            if (ImGui::BeginTable("PlayingTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Playing Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 60.0f);

                for (const auto& key : allKeys) {
                    if (audioManager->IsPlaying(key)) {
                        anyPlaying = true;
                        ImGui::TableNextRow();

                        ImGui::TableNextColumn();
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "▶ %s", key.c_str());

                        ImGui::TableNextColumn();
                        ImGui::PushID((key + "_stop").c_str());
                        if (ImGui::Button("Stop", ImVec2(60, 0))) {
                            audioManager->Stop(key);
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
                    audioManager->StopAll();
                }
            }

            ImGui::End();
        }
#endif
	};
}