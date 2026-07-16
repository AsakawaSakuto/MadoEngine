#include "LightEditor.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>

namespace MadoEngine::Editor {

#ifdef USE_IMGUI 

    namespace {
        /// @brief 削除ボタンが押されたライト情報
        struct LightRemoveRequest {
            bool isRequested = false;
            LightHandle handle;
        };
        /// @brief ライト種別の表示名を取得する
        /// @param type ライト種別
        /// @return ライト種別の表示名
        const char* GetLightTypeLabel(LightType type) {
            switch (type) {
            case LightType::Directional:
                return "Direction";
            case LightType::Point:
                return "Point";
            case LightType::Spot:
                return "Spot";
            default:
                return "Light";
            }
        }

        /// @brief 重複しないライト名を作成する
        /// @param lightManager 重複確認に使用するLightManager
        /// @param prefix ライト名の接頭辞
        /// @param nextId 次に試すID
        /// @return 重複しないライト名
        /// @brief LightManager Jsonのバックアップパスを作成する。
        /// @return LightManager Jsonのバックアップパス。
        std::filesystem::path CreateLightManagerBackupJsonPath() {
            std::filesystem::path backupPath = LightManager::kDefaultLightJsonPath;
            backupPath += ".bak";
            return backupPath;
        }

        std::string CreateUniqueLightName(LightManager& lightManager, const char* prefix, int& nextId) {
            std::string name;
            do {
                name = std::string(prefix) + std::to_string(nextId);
                ++nextId;
            } while (lightManager.Find(name).IsValid());

            return name;
        }

        /// @brief Editorからライトを追加する
        /// @param type 追加するライト種別
        void AddLightFromEditor(LightType type) {
            LightManager& lightManager = LightManager::GetInstance();
            static int nextDirectionalLightId = 1;
            static int nextPointLightId = 1;
            static int nextSpotLightId = 1;

            if (type == LightType::Directional) {
                DirectionalLight light{};
                light.useLight = 1;
                light.direction = { 0.0f, -1.0f, 0.0f };
                const std::string name = CreateUniqueLightName(lightManager, "Direction Light ", nextDirectionalLightId);
                lightManager.CreateDirectionalLight(
                    name,
                    light,
                    SceneType::None,
                    ToLightLayerMask(LightLayer::World),
                    EditorManagementMode::EditorManaged);
                return;
            }

            if (type == LightType::Point) {
                PointLight light{};
                light.useLight = 1;
                light.position = { 0.0f, 1.0f, 0.0f };
                light.radius = 5.0f;
                const std::string name = CreateUniqueLightName(lightManager, "Point Light ", nextPointLightId);
                lightManager.CreatePointLight(
                    name,
                    light,
                    SceneType::None,
                    ToLightLayerMask(LightLayer::World),
                    EditorManagementMode::EditorManaged);
                return;
            }

            if (type == LightType::Spot) {
                SpotLight light{};
                light.useLight = 1;
                light.position = { 0.0f, 3.0f, 0.0f };
                light.direction = { 0.0f, -1.0f, 0.0f };
                light.distance = 10.0f;
                light.cosAngle = 0.8f;
                light.cosFalloffStart = 0.9f;
                const std::string name = CreateUniqueLightName(lightManager, "Spot Light ", nextSpotLightId);
                lightManager.CreateSpotLight(
                    name,
                    light,
                    SceneType::None,
                    ToLightLayerMask(LightLayer::World),
                    EditorManagementMode::EditorManaged);
            }
        }

        /// @brief SceneTypeの選択表示名を取得する
        /// @param sceneType 表示するSceneType
        /// @return SceneTypeの選択表示名
        std::string GetSceneSelectionLabel(SceneType sceneType) {
            if (sceneType == SceneType::None) {
                return "全シーン";
            }

            return SceneTypeToString(sceneType);
        }

        /// @brief ライトを使用するシーン選択Comboを描画する
        /// @param lightManager 編集対象のLightManager
        /// @param handle 編集対象ライトのハンドル
        void DrawLightSceneSelectionCombo(LightManager& lightManager, LightHandle handle) {
            const SceneType currentSceneType = lightManager.GetSceneType(handle);
            const std::string previewName = GetSceneSelectionLabel(currentSceneType);

            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::BeginCombo("##LightScene", previewName.c_str())) {
                const bool isAllSceneSelected = currentSceneType == SceneType::None;
                if (ImGui::Selectable("全シーン", isAllSceneSelected)) {
                    lightManager.SetSceneType(handle, SceneType::None);
                }

                if (isAllSceneSelected) {
                    ImGui::SetItemDefaultFocus();
                }

                for (uint32_t index = 0; index < kSceneTypeCount; ++index) {
                    const SceneType sceneType = GetSceneTypeByIndex(index);
                    const bool isSelected = currentSceneType == sceneType;
                    const std::string sceneName = SceneTypeToString(sceneType);
                    if (ImGui::Selectable(sceneName.c_str(), isSelected)) {
                        lightManager.SetSceneType(handle, sceneType);
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
        }

        /// @brief ライトレイヤー選択Comboを描画する
        /// @param lightManager 編集対象のLightManager
        /// @param handle 編集対象ライトのハンドル
        void DrawLightLayerSelectionCombo(LightManager& lightManager, LightHandle handle) {
            const LightLayerMask currentLayerMask = lightManager.GetLayerMask(handle);
            const char* previewName = GetLightLayerMaskName(currentLayerMask);

            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::BeginCombo("##LightLayer", previewName)) {
                const LightLayerMask noneLayerMask = ToLightLayerMask(LightLayer::None);
                const bool isNoneSelected = noneLayerMask == currentLayerMask;
                if (ImGui::Selectable(GetLightLayerName(LightLayer::None), isNoneSelected)) {
                    lightManager.SetLayerMask(handle, noneLayerMask);
                }

                if (isNoneSelected) {
                    ImGui::SetItemDefaultFocus();
                }

                for (uint32_t index = 0; index < kLightLayerCount; ++index) {
                    const LightLayer layer = GetLightLayerByIndex(index);
                    const LightLayerMask layerMask = ToLightLayerMask(layer);
                    const bool isSelected = layerMask == currentLayerMask;
                    if (ImGui::Selectable(GetLightLayerName(layer), isSelected)) {
                        lightManager.SetLayerMask(handle, layerMask);
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                const bool isAllSelected = kAllLightLayers == currentLayerMask;
                if (ImGui::Selectable(GetLightLayerName(LightLayer::All), isAllSelected)) {
                    lightManager.SetLayerMask(handle, kAllLightLayers);
                }

                if (isAllSelected) {
                    ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }
        }

        /// @brief ライト名編集InputTextを描画する
        /// @param lightManager 編集対象のLightManager
        /// @param handle 編集対象ライトのハンドル
        void DrawLightNameInput(LightManager& lightManager, LightHandle handle) {
            std::array<char, 128> nameBuffer{};
            std::snprintf(nameBuffer.data(), nameBuffer.size(), "%s", lightManager.GetName(handle).c_str());
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText("##LightName", nameBuffer.data(), nameBuffer.size())) {
                if (nameBuffer[0] != '\0') {
                    lightManager.RenameLight(handle, nameBuffer.data());
                }
            }
        }

        /// @brief ライトパラメータ行のラベル列を開始する
        /// @param label 表示するパラメータ名
        void BeginLightParameterRow(const char* label) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(label);
            ImGui::TableSetColumnIndex(2);
            ImGui::SetNextItemWidth(-1.0f);
        }

        /// @brief 平行光源の値調整UIを描画する
        /// @param lightManager 編集対象のLightManager
        /// @param handle 編集対象ライトのハンドル
        void DrawDirectionalLightParameterRows(LightManager& lightManager, LightHandle handle) {
            const DirectionalLight* sourceLight = lightManager.GetDirectionalLightData(handle);
            if (!sourceLight) {
                return;
            }

            DirectionalLight light = *sourceLight;
            bool isChanged = false;

            BeginLightParameterRow("色");
            isChanged |= ImGui::ColorEdit4("##Color", &light.color.x);

            BeginLightParameterRow("方向");
            isChanged |= ImGui::DragFloat3("##Direction", &light.direction.x, 0.01f, -1.0f, 1.0f);

            BeginLightParameterRow("強度");
            isChanged |= ImGui::DragFloat("##Intensity", &light.intensity, 0.01f, 0.0f, 20.0f);

            BeginLightParameterRow("HalfLambert");
            bool useHalfLambert = light.useHalfLambert != 0;
            if (ImGui::Checkbox("##HalfLambert", &useHalfLambert)) {
                light.useHalfLambert = useHalfLambert ? 1u : 0u;
                isChanged = true;
            }

            if (isChanged) {
                lightManager.SetDirectionalLight(handle, light);
            }
        }

        /// @brief 点光源の値調整UIを描画する
        /// @param lightManager 編集対象のLightManager
        /// @param handle 編集対象ライトのハンドル
        void DrawPointLightParameterRows(LightManager& lightManager, LightHandle handle) {
            const PointLight* sourceLight = lightManager.GetPointLightData(handle);
            if (!sourceLight) {
                return;
            }

            PointLight light = *sourceLight;
            bool isChanged = false;

            BeginLightParameterRow("色");
            isChanged |= ImGui::ColorEdit4("##Color", &light.color.x);

            BeginLightParameterRow("位置");
            isChanged |= ImGui::DragFloat3("##Position", &light.position.x, 0.01f);

            BeginLightParameterRow("強度");
            isChanged |= ImGui::DragFloat("##Intensity", &light.intensity, 0.01f, 0.0f, 100.0f);

            BeginLightParameterRow("半径");
            isChanged |= ImGui::DragFloat("##Radius", &light.radius, 0.01f, 0.0f, 1000.0f);

            BeginLightParameterRow("減衰");
            isChanged |= ImGui::DragFloat("##Decay", &light.decay, 0.01f, 0.0f, 20.0f);

            if (isChanged) {
                lightManager.SetPointLight(handle, light);
            }
        }

        /// @brief スポットライトの値調整UIを描画する
        /// @param lightManager 編集対象のLightManager
        /// @param handle 編集対象ライトのハンドル
        void DrawSpotLightParameterRows(LightManager& lightManager, LightHandle handle) {
            const SpotLight* sourceLight = lightManager.GetSpotLightData(handle);
            if (!sourceLight) {
                return;
            }

            SpotLight light = *sourceLight;
            bool isChanged = false;

            BeginLightParameterRow("色");
            isChanged |= ImGui::ColorEdit4("##Color", &light.color.x);

            BeginLightParameterRow("位置");
            isChanged |= ImGui::DragFloat3("##Position", &light.position.x, 0.01f);

            BeginLightParameterRow("方向");
            isChanged |= ImGui::DragFloat3("##Direction", &light.direction.x, 0.01f, -1.0f, 1.0f);

            BeginLightParameterRow("強度");
            isChanged |= ImGui::DragFloat("##Intensity", &light.intensity, 0.01f, 0.0f, 100.0f);

            BeginLightParameterRow("距離");
            isChanged |= ImGui::DragFloat("##Distance", &light.distance, 0.01f, 0.0f, 1000.0f);

            BeginLightParameterRow("減衰");
            isChanged |= ImGui::DragFloat("##Decay", &light.decay, 0.01f, 0.0f, 20.0f);

            BeginLightParameterRow("角度Cos");
            isChanged |= ImGui::DragFloat("##CosAngle", &light.cosAngle, 0.001f, -1.0f, 1.0f);

            BeginLightParameterRow("Falloff開始Cos");
            isChanged |= ImGui::DragFloat("##CosFalloffStart", &light.cosFalloffStart, 0.001f, -1.0f, 1.0f);

            if (isChanged) {
                lightManager.SetSpotLight(handle, light);
            }
        }

        /// @brief ライト種別に応じた値調整UIを描画する
        /// @param lightManager 編集対象のLightManager
        /// @param handle 編集対象ライトのハンドル
        void DrawLightParameterRows(LightManager& lightManager, LightHandle handle) {
            switch (handle.type) {
            case LightType::Directional:
                DrawDirectionalLightParameterRows(lightManager, handle);
                break;
            case LightType::Point:
                DrawPointLightParameterRows(lightManager, handle);
                break;
            case LightType::Spot:
                DrawSpotLightParameterRows(lightManager, handle);
                break;
            default:
                break;
            }
        }

        /// @brief LightManager Editorの1行を描画する
        /// @param lightManager 編集対象のLightManager
        /// @param handle 編集対象ライトのハンドル
        /// @param removeRequest 削除要求の出力先
        void DrawLightManagerEditorRow(
            LightManager& lightManager,
            LightHandle handle,
            LightRemoveRequest& removeRequest)
        {
            ImGui::PushID(static_cast<int>(handle.type));
            ImGui::PushID(static_cast<int>(handle.index));
            ImGui::PushID(static_cast<int>(handle.generation));

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            bool enabled = lightManager.IsEnabled(handle);
            if (ImGui::Checkbox("##LightEnabled", &enabled)) {
                lightManager.SetEnabled(handle, enabled);
            }

            ImGui::TableNextColumn();
            DrawLightNameInput(lightManager, handle);

            ImGui::TableNextColumn();
            DrawLightSceneSelectionCombo(lightManager, handle);

            ImGui::TableNextColumn();
            DrawLightLayerSelectionCombo(lightManager, handle);

            ImGui::TableNextColumn();
            if (ImGui::Button("削除")) {
                removeRequest.isRequested = true;
                removeRequest.handle = handle;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const std::string treeLabel = std::string(GetLightTypeLabel(handle.type)) + " の値を調整";
            if (ImGui::TreeNodeEx(treeLabel.c_str(), ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_LabelSpanAllColumns)) {
                DrawLightParameterRows(lightManager, handle);
                ImGui::TreePop();
            }

            ImGui::PopID();
            ImGui::PopID();
            ImGui::PopID();
        }

    } // namespace

#endif // USE_IMGUI

    bool LoadLightEditorJson() {
        return LightManager::GetInstance().LoadFromJson();
    }

#ifdef USE_IMGUI

    void DrawLightManagerEditorUI() {
        LightManager& lightManager = LightManager::GetInstance();
        static LightHandle selectedHandle{};

        ImGui::Begin("Light Editor");

        if (ImGui::Button("Direction追加")) {
            AddLightFromEditor(LightType::Directional);
        }
        ImGui::SameLine();
        if (ImGui::Button("Point追加")) {
            AddLightFromEditor(LightType::Point);
        }
        ImGui::SameLine();
        if (ImGui::Button("Spot追加")) {
            AddLightFromEditor(LightType::Spot);
        }
        ImGui::SameLine();
        if (ImGui::Button("保存")) {
            lightManager.SaveToJson();
        }
        ImGui::SameLine();
        if (ImGui::Button("読込")) {
            LoadLightEditorJson();
        }
        ImGui::SameLine();
        if (ImGui::Button("復元")) {
            lightManager.LoadFromJson(CreateLightManagerBackupJsonPath());
        }

        ImGui::Separator();

        LightRemoveRequest removeRequest{};

        ImGui::BeginChild("LightList", ImVec2(220.0f, 0.0f), true);
        auto drawLightListItem = [&](LightHandle handle) {
            ImGui::PushID(static_cast<int>(handle.type));
            ImGui::PushID(static_cast<int>(handle.index));
            ImGui::PushID(static_cast<int>(handle.generation));

            const bool selected =
                selectedHandle.type == handle.type &&
                selectedHandle.index == handle.index &&
                selectedHandle.generation == handle.generation;
            const std::string label = std::string("[") + GetLightTypeLabel(handle.type) + "] " + lightManager.GetName(handle);
            const float deleteButtonWidth = ImGui::CalcTextSize("削除").x + ImGui::GetStyle().FramePadding.x * 2.0f;
            float selectableWidth = ImGui::GetContentRegionAvail().x - deleteButtonWidth - ImGui::GetStyle().ItemSpacing.x;
            if (selectableWidth < 1.0f) {
                selectableWidth = 1.0f;
            }
            if (ImGui::Selectable(label.c_str(), selected, 0, ImVec2(selectableWidth, 0.0f))) {
                selectedHandle = handle;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("削除")) {
                removeRequest.isRequested = true;
                removeRequest.handle = handle;
            }

            ImGui::PopID();
            ImGui::PopID();
            ImGui::PopID();
        };

        const std::vector<LightHandle> directionalHandles = lightManager.GetEditorManagedDirectionalLightHandles();
        for (LightHandle handle : directionalHandles) {
            drawLightListItem(handle);
        }

        const std::vector<LightHandle> pointHandles = lightManager.GetEditorManagedPointLightHandles();
        for (LightHandle handle : pointHandles) {
            drawLightListItem(handle);
        }

        const std::vector<LightHandle> spotHandles = lightManager.GetEditorManagedSpotLightHandles();
        for (LightHandle handle : spotHandles) {
            drawLightListItem(handle);
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("LightProperties", ImVec2(0.0f, 0.0f), true);
        if (lightManager.IsValid(selectedHandle)) {
            ImGui::PushID(static_cast<int>(selectedHandle.type));
            ImGui::PushID(static_cast<int>(selectedHandle.index));
            ImGui::PushID(static_cast<int>(selectedHandle.generation));

            bool enabled = lightManager.IsEnabled(selectedHandle);
            if (ImGui::Checkbox("Enabled", &enabled)) {
                lightManager.SetEnabled(selectedHandle, enabled);
            }

            ImGui::TextUnformatted("Name");
            DrawLightNameInput(lightManager, selectedHandle);

            ImGui::TextUnformatted("Scene");
            DrawLightSceneSelectionCombo(lightManager, selectedHandle);

            ImGui::TextUnformatted("Light Layer");
            DrawLightLayerSelectionCombo(lightManager, selectedHandle);

            constexpr ImGuiTableFlags propertyTableFlags =
                ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_BordersInnerH |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingStretchProp;
            if (ImGui::BeginTable("LightPropertyTable", 3, propertyTableFlags)) {
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 1.0f);
                ImGui::TableSetupColumn("項目", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("値", ImGuiTableColumnFlags_WidthStretch);
                DrawLightParameterRows(lightManager, selectedHandle);
                ImGui::EndTable();
            }

            ImGui::PopID();
            ImGui::PopID();
            ImGui::PopID();
        } else {
            selectedHandle = {};
            ImGui::TextDisabled("Lightを選択してください。");
        }
        ImGui::EndChild();

        if (removeRequest.isRequested) {
            if (selectedHandle.type == removeRequest.handle.type &&
                selectedHandle.index == removeRequest.handle.index &&
                selectedHandle.generation == removeRequest.handle.generation) {
                selectedHandle = {};
            }
            lightManager.Destroy(removeRequest.handle);
        }

        ImGui::End();
    }

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
