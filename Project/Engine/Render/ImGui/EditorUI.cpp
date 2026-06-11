#include "EditorUI.h"

namespace MadoEngine::Editor {

#ifdef USE_IMGUI 

    namespace {

        /// @brief Game Viewの画像表示領域を計算する
        /// @param outImageMin 画像領域の左上座標
        /// @param outImageSize 画像領域のサイズ
        /// @return 有効な画像領域を取得できた場合はtrue
        bool TryGetGameViewImageRect(ImVec2& outImageMin, ImVec2& outImageSize) {
            if (!ImGui::Begin("Game View")) {
                ImGui::End();
                return false;
            }

            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
            ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
            ImVec2 contentSize = {
                contentMax.x - contentMin.x,
                contentMax.y - contentMin.y
            };

            if (contentSize.x <= 0.0f || contentSize.y <= 0.0f) {
                ImGui::End();
                return false;
            }

            constexpr float kAspect = 16.0f / 9.0f;
            if (contentSize.x / contentSize.y > kAspect) {
                outImageSize.y = contentSize.y;
                outImageSize.x = contentSize.y * kAspect;
            } else {
                outImageSize.x = contentSize.x;
                outImageSize.y = contentSize.x / kAspect;
            }

            ImVec2 offset = {
                (contentSize.x - outImageSize.x) * 0.5f,
                (contentSize.y - outImageSize.y) * 0.5f
            };

            outImageMin = {
                windowPos.x + contentMin.x + offset.x,
                windowPos.y + contentMin.y + offset.y
            };

            return true;
        }

        /// @brief ギズモ操作モードの選択ボタンを描画する
        /// @param imageMin Game View画像領域の左上座標
        /// @return 現在選択されているImGuizmo操作
        ImGuizmo::OPERATION DrawGizmoOperationButtons(const ImVec2& imageMin) {
            static ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;

            ImGui::SetCursorScreenPos({ imageMin.x + 8.0f, imageMin.y + 8.0f });
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 0.0f });

            auto drawButton = [](const char* label, ImGuizmo::OPERATION operation) {
                const bool isSelected = currentOperation == operation;
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                }

                if (ImGui::Button(label, { 72.0f, 24.0f })) {
                    currentOperation = operation;
                }

                if (isSelected) {
                    ImGui::PopStyleColor(2);
                }
            };

            drawButton("Translate", ImGuizmo::TRANSLATE);
            ImGui::SameLine();
            drawButton("Rotate", ImGuizmo::ROTATE);
            ImGui::SameLine();
            drawButton("Scale", ImGuizmo::SCALE);

            ImGui::PopStyleVar(2);
            return currentOperation;
        }

        /// @brief ImGuizmoの行列編集結果をTransformへ反映する
        /// @param matrix 分解する行列
        /// @param operation 現在のギズモ操作
        /// @param transform 反映先のTransform
        void ApplyMatrixToTransform(const Matrix4x4& matrix, ImGuizmo::OPERATION operation, Transform3D& transform) {
            if (operation == ImGuizmo::TRANSLATE) {
                transform.translate = {
                    matrix.m[3][0],
                    matrix.m[3][1],
                    matrix.m[3][2]
                };
                return;
            }

            float translate[3]{};
            float rotateDegree[3]{};
            float scale[3]{};

            ImGuizmo::DecomposeMatrixToComponents(
                &matrix.m[0][0],
                translate,
                rotateDegree,
                scale);

            constexpr float kDegreeToRadian = 3.14159265358979323846f / 180.0f;
            transform.translate = { translate[0], translate[1], translate[2] };
            transform.rotate = {
                rotateDegree[0] * kDegreeToRadian,
                rotateDegree[1] * kDegreeToRadian,
                rotateDegree[2] * kDegreeToRadian
            };
            transform.scale = { scale[0], scale[1], scale[2] };
        }

        /// @brief 点が矩形内にあるかを判定する
        /// @param point 判定する点
        /// @param rectMin 矩形の左上座標
        /// @param rectSize 矩形のサイズ
        /// @return 点が矩形内にある場合はtrue
        bool IsPointInRect(const ImVec2& point, const ImVec2& rectMin, const ImVec2& rectSize) {
            return point.x >= rectMin.x &&
                point.x <= rectMin.x + rectSize.x &&
                point.y >= rectMin.y &&
                point.y <= rectMin.y + rectSize.y;
        }

        /// @brief Game View上のマウス座標からワールド空間レイを生成する
        /// @param camera レイ生成に使用するカメラ
        /// @param imageMin Game View画像領域の左上座標
        /// @param imageSize Game View画像領域のサイズ
        /// @param mousePosition マウス座標
        /// @param outRayOrigin レイ始点の出力先
        /// @param outRayDirection レイ方向の出力先
        /// @return レイを生成できた場合はtrue
        bool TryCreateRayFromGameView(
            const Camera& camera,
            const ImVec2& imageMin,
            const ImVec2& imageSize,
            const ImVec2& mousePosition,
            Vector3& outRayOrigin,
            Vector3& outRayDirection) {
            if (imageSize.x <= 0.0f || imageSize.y <= 0.0f || !IsPointInRect(mousePosition, imageMin, imageSize)) {
                return false;
            }

            const float normalizedX = (mousePosition.x - imageMin.x) / imageSize.x;
            const float normalizedY = (mousePosition.y - imageMin.y) / imageSize.y;
            const float ndcX = normalizedX * 2.0f - 1.0f;
            const float ndcY = 1.0f - normalizedY * 2.0f;

            const Matrix4x4 viewProjection = Matrix::Multiply(camera.GetViewMatrix(), camera.GetProjectionMatrix());
            const Matrix4x4 inverseViewProjection = Matrix::Inverse(viewProjection);

            const Vector3 nearPoint = Matrix::Transform({ ndcX, ndcY, 0.0f }, inverseViewProjection);
            const Vector3 farPoint = Matrix::Transform({ ndcX, ndcY, 1.0f }, inverseViewProjection);
            const Vector3 direction = farPoint - nearPoint;
            if (direction.LengthSq() <= 0.000001f) {
                return false;
            }

            outRayOrigin = nearPoint;
            outRayDirection = direction.Normalized();
            return true;
        }

        /// @brief 指定領域にTransformギズモを描画する
        /// @param camera ギズモ表示に使用するカメラ
        /// @param transform 操作対象のTransform
        /// @param imageMin Game View画像領域の左上座標
        /// @param imageSize Game View画像領域のサイズ
        /// @return ギズモ操作でTransformが変更された場合はtrue
        bool DrawTransformGizmoInRect(
            const Camera& camera,
            Transform3D& transform,
            const ImVec2& imageMin,
            const ImVec2& imageSize) {
            Matrix4x4 modelMatrix = Matrix::MakeAffine(transform.scale, transform.rotate, transform.translate);
            ImGuizmo::OPERATION operation = DrawGizmoOperationButtons(imageMin);

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
            ImGuizmo::SetRect(imageMin.x, imageMin.y, imageSize.x, imageSize.y);

            bool isChanged = ImGuizmo::Manipulate(
                &camera.GetViewMatrix().m[0][0],
                &camera.GetProjectionMatrix().m[0][0],
                operation,
                ImGuizmo::WORLD,
                &modelMatrix.m[0][0]);

            if (isChanged) {
                ApplyMatrixToTransform(modelMatrix, operation, transform);
            }

            return isChanged;
        }

    } // namespace

    bool DrawTransformGizmoOnGameView(const Camera& camera, Transform3D& transform) {
        ImVec2 imageMin{};
        ImVec2 imageSize{};
        if (!TryGetGameViewImageRect(imageMin, imageSize)) {
            return false;
        }

        bool isChanged = DrawTransformGizmoInRect(camera, transform, imageMin, imageSize);

        ImGui::End();
        return isChanged;
    }

    bool DrawModelGizmoOnGameView(const Camera& camera, SceneType sceneType, Model*& selectedModel) {
        ImVec2 imageMin{};
        ImVec2 imageSize{};
        if (!TryGetGameViewImageRect(imageMin, imageSize)) {
            return false;
        }

        bool isChanged = false;
        if (selectedModel && !selectedModel->IsVisible()) {
            selectedModel = nullptr;
            isChanged = true;
        }

        if (selectedModel) {
            Transform3D transform = selectedModel->GetTransform();
            if (DrawTransformGizmoInRect(camera, transform, imageMin, imageSize)) {
                selectedModel->SetTransform(transform);
                isChanged = true;
            }
        }

        const ImVec2 mousePosition = ImGui::GetMousePos();
        const bool canSelect =
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            IsPointInRect(mousePosition, imageMin, imageSize) &&
            !ImGui::IsAnyItemHovered() &&
            !ImGuizmo::IsOver() &&
            !ImGuizmo::IsUsing();

        if (canSelect) {
            Vector3 rayOrigin{};
            Vector3 rayDirection{};
            if (TryCreateRayFromGameView(camera, imageMin, imageSize, mousePosition, rayOrigin, rayDirection)) {
                selectedModel = MadoEngine::ModelManager::GetInstance().PickByRay(
                    sceneType,
                    rayOrigin,
                    rayDirection,
                    camera.GetFarClip());
                isChanged = true;
            }
        }

        ImGui::End();
        return isChanged;
    }

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
