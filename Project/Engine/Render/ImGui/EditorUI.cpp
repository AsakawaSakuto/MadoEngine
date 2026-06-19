#include "EditorUI.h"
#include "EditorHistory.h"
#include "ModelTransformCommand.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>

namespace MadoEngine::Editor {

#ifdef USE_IMGUI 

    namespace {

        constexpr ImVec2 kEditorIconButtonSize = { 28.0f, 28.0f };
        constexpr ImVec2 kEditorIconButtonPadding = { 4.0f, 4.0f };

        /// @brief 固定配列の要素数を取得する
        /// @tparam T 配列要素の型
        /// @tparam N 配列要素数
        /// @param values 要素数を取得する配列
        /// @return 配列要素数
        template<typename T, std::size_t N>
        constexpr std::size_t CountOf(const T(&values)[N]) {
            (void)values;
            return N;
        }

        /// @brief PostEffect定義内のfloatパラメータ情報
        struct PostEffectFloatParameterDefinition {
            const char* key;
            const char* label;
            std::size_t offset;
            float minValue;
            float maxValue;
            float speed;
        };

        /// @brief ImGuiから選択可能なPostEffect定義
        struct PostEffectDefinition {
            const char* displayName;
            const char* shaderKey;
            const float* initialValues;
            std::size_t initialValueCount;
            const PostEffectFloatParameterDefinition* parameters;
            std::size_t parameterCount;
        };

        /// @brief Layer選択用の表示情報
        struct RenderLayerDefinition {
            const char* displayName;
            Render::RenderLayerMask layerMask;
        };

        /// @brief Passの所属配列種別
        enum class LayerEffectPassListType {
            Layer,
            Screen,
        };

        /// @brief 削除ボタンが押されたPass情報
        struct LayerEffectPassRemoveRequest {
            bool isRequested = false;
            LayerEffectPassListType listType = LayerEffectPassListType::Layer;
            std::size_t index = 0;
        };

        constexpr std::size_t kFloatSize = sizeof(float);

        const float kBloomInitialValues[] = {
            0.6f, 0.7f, 4.0f, 0.5f
        };
        const PostEffectFloatParameterDefinition kBloomParameters[] = {
            { "Intensity", "強度", 0 * kFloatSize, 0.0f, 5.0f, 0.01f },
            { "Threshold", "しきい値", 1 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "Radius", "半径", 2 * kFloatSize, 0.0f, 32.0f, 0.1f },
            { "SoftKnee", "ソフトニー", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kVignetteInitialValues[] = {
            0.8f, 0.35f, 2.0f, 0.0f
        };
        const PostEffectFloatParameterDefinition kVignetteParameters[] = {
            { "Intensity", "強度", 0 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "InnerRadius", "内側半径", 1 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "OuterScale", "外側倍率", 2 * kFloatSize, 1.0f, 4.0f, 0.01f },
        };

        const float kPixelArtInitialValues[] = {
            6.0f, 8.0f, 1.15f, 1.0f
        };
        const PostEffectFloatParameterDefinition kPixelArtParameters[] = {
            { "PixelSize", "ピクセルサイズ", 0 * kFloatSize, 1.0f, 64.0f, 1.0f },
            { "ColorSteps", "色階調数", 1 * kFloatSize, 2.0f, 32.0f, 1.0f },
            { "Contrast", "コントラスト", 2 * kFloatSize, 0.0f, 4.0f, 0.01f },
            { "Intensity", "適用率", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const float kOutlineInitialValues[] = {
            1.0f, 0.85f, 0.15f, 1.0f,
            1.0f, 80.0f, 0.005f, 1.0f
        };
        const PostEffectFloatParameterDefinition kOutlineParameters[] = {
            { "ColorR", "色R", 0 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorG", "色G", 1 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorB", "色B", 2 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorA", "色A", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "Thickness", "太さ", 4 * kFloatSize, 0.25f, 12.0f, 0.05f },
            { "DepthSensitivity", "深度感度", 5 * kFloatSize, 1.0f, 300.0f, 1.0f },
            { "EdgeThreshold", "エッジしきい値", 6 * kFloatSize, 0.0001f, 0.1f, 0.0001f },
            { "Intensity", "濃さ", 7 * kFloatSize, 0.0f, 4.0f, 0.01f },
        };

        const float kFogInitialValues[] = {
            0.58f, 0.68f, 0.74f, 1.0f,
            850.0f, 1000.0f, 1.0f, 0.0f,
            0.1f, 1000.0f, 0.0f, 0.0f
        };
        const PostEffectFloatParameterDefinition kFogParameters[] = {
            { "ColorR", "色R", 0 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorG", "色G", 1 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorB", "色B", 2 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "ColorA", "色A", 3 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "StartDistance", "開始距離", 4 * kFloatSize, 0.0f, 5000.0f, 1.0f },
            { "EndDistance", "終了距離", 5 * kFloatSize, 0.0f, 5000.0f, 1.0f },
            { "Density", "濃度", 6 * kFloatSize, 0.0f, 4.0f, 0.01f },
            { "HeightStrength", "高さ強度", 7 * kFloatSize, 0.0f, 4.0f, 0.01f },
            { "NearClip", "NearClip", 8 * kFloatSize, 0.001f, 100.0f, 0.01f },
            { "FarClip", "FarClip", 9 * kFloatSize, 1.0f, 10000.0f, 1.0f },
        };

        const float kDissolveInitialValues[] = {
            0.35f, 0.06f, 1.0f, 2.0f,
            1.0f, 0.45f, 0.05f, 1.0f
        };
        const PostEffectFloatParameterDefinition kDissolveParameters[] = {
            { "Amount", "進行度", 0 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "EdgeWidth", "境界幅", 1 * kFloatSize, 0.001f, 0.5f, 0.001f },
            { "EdgeIntensity", "境界強度", 2 * kFloatSize, 0.0f, 8.0f, 0.01f },
            { "NoiseScale", "ノイズ倍率", 3 * kFloatSize, 0.001f, 32.0f, 0.01f },
            { "EdgeColorR", "境界色R", 4 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "EdgeColorG", "境界色G", 5 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "EdgeColorB", "境界色B", 6 * kFloatSize, 0.0f, 1.0f, 0.01f },
            { "EdgeColorA", "境界色A", 7 * kFloatSize, 0.0f, 1.0f, 0.01f },
        };

        const PostEffectDefinition kPostEffectDefinitions[] = {
            { "CopyImage", "PostEffect/CopyImage.PS", nullptr, 0, nullptr, 0 },
            { "GrayScale", "PostEffect/GrayScale.PS", nullptr, 0, nullptr, 0 },
            { "Sepia", "PostEffect/Sepia.PS", nullptr, 0, nullptr, 0 },
            { "Invert", "PostEffect/Invert.PS", nullptr, 0, nullptr, 0 },
            { "Bloom", "PostEffect/Bloom.PS", kBloomInitialValues, CountOf(kBloomInitialValues), kBloomParameters, CountOf(kBloomParameters) },
            { "Vignette", "PostEffect/Vignette.PS", kVignetteInitialValues, CountOf(kVignetteInitialValues), kVignetteParameters, CountOf(kVignetteParameters) },
            { "PixelArt", "PostEffect/PixelArt.PS", kPixelArtInitialValues, CountOf(kPixelArtInitialValues), kPixelArtParameters, CountOf(kPixelArtParameters) },
            { "Outline", "PostEffect/Outline.PS", kOutlineInitialValues, CountOf(kOutlineInitialValues), kOutlineParameters, CountOf(kOutlineParameters) },
            { "Fog", "PostEffect/Fog.PS", kFogInitialValues, CountOf(kFogInitialValues), kFogParameters, CountOf(kFogParameters) },
            { "Dissolve", "PostEffect/Dissolve.PS", kDissolveInitialValues, CountOf(kDissolveInitialValues), kDissolveParameters, CountOf(kDissolveParameters) },
        };

        const RenderLayerDefinition kRenderLayerDefinitions[] = {
            { "Default", Render::ToRenderLayerMask(Render::RenderLayer::Default) },
            { "World", Render::ToRenderLayerMask(Render::RenderLayer::World) },
            { "MapEventObject", Render::ToRenderLayerMask(Render::RenderLayer::MapEventObject) },
            { "Player", Render::ToRenderLayerMask(Render::RenderLayer::Player) },
            { "Effect", Render::ToRenderLayerMask(Render::RenderLayer::Effect) },
            { "UI", Render::ToRenderLayerMask(Render::RenderLayer::UI) },
            { "Debug", Render::ToRenderLayerMask(Render::RenderLayer::Debug) },
            { "All", Render::kAllRenderLayers },
        };

        /// @brief Editor UIで使用するアイコン情報
        struct EditorIconButtonInfo {
            const char* id;
            const char* textureName;
            const char* fallbackLabel;
            const char* tooltip;
        };

        /// @brief 指定したテクスチャ名のImGui用テクスチャIDを取得する
        /// @param textureName TextureManagerに登録されているテクスチャ名
        /// @return ImGuiへ渡すテクスチャID。取得できない場合は0
        ImTextureID GetEditorIconTextureId(const char* textureName) {
            struct IconTextureCache {
                const char* textureName;
                ImTextureID textureId;
                bool isInitialized;
            };

            static IconTextureCache caches[] = {
                { "Translate", ImTextureID_Invalid, false },
                { "Rotate", ImTextureID_Invalid, false },
                { "Scale", ImTextureID_Invalid, false },
                { "Undo", ImTextureID_Invalid, false },
                { "Redo", ImTextureID_Invalid, false },
            };

            for (IconTextureCache& cache : caches) {
                if (std::strcmp(cache.textureName, textureName) != 0) {
                    continue;
                }

                if (!cache.isInitialized) {
                    const uint32_t textureIndex = TextureManager::GetInstance().GetTextureIndex(textureName);
                    if (textureIndex != UINT32_MAX) {
                        cache.textureId = static_cast<ImTextureID>(TextureManager::GetInstance().GetSrvHandleGPU(textureIndex).ptr);
                    }
                    cache.isInitialized = true;
                }

                return cache.textureId;
            }

            const uint32_t textureIndex = TextureManager::GetInstance().GetTextureIndex(textureName);
            if (textureIndex == UINT32_MAX) {
                return ImTextureID_Invalid;
            }

            return static_cast<ImTextureID>(TextureManager::GetInstance().GetSrvHandleGPU(textureIndex).ptr);
        }

        /// @brief アイコン画像ボタンを描画する
        /// @param info アイコンボタンの表示情報
        /// @param size ボタン内に表示する画像サイズ
        /// @return ボタンが押された場合はtrue
        bool DrawEditorIconButton(const EditorIconButtonInfo& info, const ImVec2& size = kEditorIconButtonSize) {
            const ImTextureID textureId = GetEditorIconTextureId(info.textureName);
            bool isPressed = false;

            if (textureId != ImTextureID_Invalid) {
                isPressed = ImGui::ImageButton(info.id, textureId, size);
            } else {
                isPressed = ImGui::Button(info.fallbackLabel, { 72.0f, 24.0f });
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", info.tooltip);
            }

            return isPressed;
        }

        /// @brief shaderKeyに一致するPostEffect定義のindexを取得する
        /// @param shaderKey 検索するPixelShaderキー
        /// @return 一致した定義index。一致しない場合は0
        int FindPostEffectDefinitionIndex(const std::string& shaderKey) {
            for (int index = 0; index < static_cast<int>(CountOf(kPostEffectDefinitions)); ++index) {
                if (shaderKey == kPostEffectDefinitions[index].shaderKey) {
                    return index;
                }
            }

            return 0;
        }

        /// @brief PostEffect定義をPassへ反映する
        /// @param pass 反映先のPass
        /// @param definition 反映するPostEffect定義
        void ApplyPostEffectDefinition(Render::LayerEffectPass& pass, const PostEffectDefinition& definition) {
            pass.SetEffectShaderKey(definition.shaderKey);
            pass.ClearFloatParameterControls();
            pass.ClearParameterData();

            if (definition.initialValues && definition.initialValueCount > 0) {
                pass.SetParameterData(definition.initialValues, definition.initialValueCount * sizeof(float));

                for (std::size_t i = 0; i < definition.parameterCount; ++i) {
                    const PostEffectFloatParameterDefinition& parameter = definition.parameters[i];
                    pass.AddFloatParameterControl(
                        parameter.key,
                        parameter.label,
                        parameter.offset,
                        parameter.minValue,
                        parameter.maxValue,
                        parameter.speed
                    );
                }
            }
        }

        /// @brief 重複しないPassキーを作成する
        /// @param postEffectManager 重複確認に使用する管理クラス
        /// @param prefix Passキーの接頭辞
        /// @param nextId 次に試すID
        /// @return 重複しないPassキー
        std::string CreateUniquePassKey(
            const Render::PostEffectManager& postEffectManager,
            const char* prefix,
            int& nextId)
        {
            std::string key;
            do {
                key = std::string(prefix) + std::to_string(nextId);
                ++nextId;
            } while (postEffectManager.FindPass(key));

            return key;
        }

        /// @brief Layer Effect Passを追加する
        /// @param postEffectManager 追加先のポストエフェクト管理クラス
        /// @param isScreenPass フルスクリーンPassを追加する場合はtrue
        void AddLayerEffectPassFromEditor(Render::PostEffectManager& postEffectManager, bool isScreenPass) {
            static int nextLayerPassId = 1;
            static int nextScreenPassId = 1;

            const PostEffectDefinition& definition = kPostEffectDefinitions[0];
            Render::LayerEffectPass::Desc desc{};
            if (isScreenPass) {
                desc.key = CreateUniquePassKey(postEffectManager, "ScreenEffectPass_", nextScreenPassId);
                desc.name = "Screen Effect " + std::to_string(nextScreenPassId - 1);
                desc.targetLayerMask = Render::kAllRenderLayers;
            } else {
                desc.key = CreateUniquePassKey(postEffectManager, "LayerEffectPass_", nextLayerPassId);
                desc.name = "Layer Effect " + std::to_string(nextLayerPassId - 1);
                desc.targetLayerMask = Render::ToRenderLayerMask(Render::RenderLayer::Default);
            }
            desc.effectShaderKey = definition.shaderKey;
            desc.enabled = true;
            desc.ignoreDepthForMask = false;

            Render::LayerEffectPass* pass = isScreenPass ?
                postEffectManager.AddScreenPass(desc) :
                postEffectManager.AddLayerPass(desc);
            if (pass) {
                ApplyPostEffectDefinition(*pass, definition);
            }
        }

        /// @brief 対象Layerの選択Comboを描画する
        /// @param pass 編集対象のPass
        void DrawLayerSelectionCombo(Render::LayerEffectPass& pass) {
            const Render::RenderLayerMask currentLayerMask = pass.GetTargetLayerMask();
            const char* previewName = "Custom";
            for (const RenderLayerDefinition& definition : kRenderLayerDefinitions) {
                if (definition.layerMask == currentLayerMask) {
                    previewName = definition.displayName;
                    break;
                }
            }

            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::BeginCombo("##TargetLayer", previewName)) {
                for (const RenderLayerDefinition& definition : kRenderLayerDefinitions) {
                    const bool isSelected = definition.layerMask == currentLayerMask;
                    if (ImGui::Selectable(definition.displayName, isSelected)) {
                        pass.SetTargetLayerMask(definition.layerMask);
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
        }

        /// @brief PostEffect選択Comboを描画する
        /// @param pass 編集対象のPass
        void DrawPostEffectSelectionCombo(Render::LayerEffectPass& pass) {
            const int currentIndex = FindPostEffectDefinitionIndex(pass.GetEffectShaderKey());
            const char* previewName = kPostEffectDefinitions[currentIndex].displayName;

            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::BeginCombo("##PostEffect", previewName)) {
                for (int index = 0; index < static_cast<int>(CountOf(kPostEffectDefinitions)); ++index) {
                    const bool isSelected = index == currentIndex;
                    if (ImGui::Selectable(kPostEffectDefinitions[index].displayName, isSelected)) {
                        ApplyPostEffectDefinition(pass, kPostEffectDefinitions[index]);
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
        }

        /// @brief 選択中PostEffectのパラメータ調整行を描画する
        /// @param pass 編集対象のPass
        void DrawPostEffectParameterRows(Render::LayerEffectPass& pass) {
            const std::vector<Render::LayerEffectPass::FloatParameterControl>& controls = pass.GetFloatParameterControls();
            if (controls.empty()) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(1);
                ImGui::TextDisabled("調整項目はありません");
                return;
            }

            for (const Render::LayerEffectPass::FloatParameterControl& control : controls) {
                float value = 0.0f;
                if (!pass.TryGetFloatParameter(control.offset, value)) {
                    continue;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(control.label.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::SetNextItemWidth(-1.0f);
                const std::string sliderId = "##" + control.key;
                if (ImGui::DragFloat(sliderId.c_str(), &value, control.speed, control.minValue, control.maxValue)) {
                    pass.SetFloatParameter(control.offset, value);
                }
            }
        }

        /// @brief Layer Effect Pass Editorの1行を描画する
        /// @param pass 編集対象のPass
        /// @param listType Passの所属配列種別
        /// @param index Pass配列内のindex
        /// @param removeRequest 削除要求の出力先
        void DrawLayerEffectPassEditorRow(
            Render::LayerEffectPass& pass,
            LayerEffectPassListType listType,
            std::size_t index,
            LayerEffectPassRemoveRequest& removeRequest)
        {
            ImGui::PushID(pass.GetKey().c_str());

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            bool enabled = pass.IsEnabled();
            if (ImGui::Checkbox("##Enabled", &enabled)) {
                pass.SetEnabled(enabled);
            }

            ImGui::TableNextColumn();
            std::array<char, 128> nameBuffer{};
            std::snprintf(nameBuffer.data(), nameBuffer.size(), "%s", pass.GetName().c_str());
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText("##Name", nameBuffer.data(), nameBuffer.size())) {
                if (nameBuffer[0] != '\0') {
                    pass.SetName(nameBuffer.data());
                }
            }

            ImGui::TableNextColumn();
            DrawPostEffectSelectionCombo(pass);

            ImGui::TableNextColumn();
            if (listType == LayerEffectPassListType::Layer) {
                DrawLayerSelectionCombo(pass);
            } else {
                ImGui::TextDisabled("全画面");
            }

            ImGui::TableNextColumn();
            bool ignoreDepth = pass.IsIgnoreDepthForMask();
            if (listType == LayerEffectPassListType::Layer) {
                if (ImGui::Checkbox("##Depth", &ignoreDepth)) {
                    pass.SetIgnoreDepthForMask(ignoreDepth);
                }
            } else {
                ImGui::BeginDisabled();
                ImGui::Checkbox("##Depth", &ignoreDepth);
                ImGui::EndDisabled();
            }

            ImGui::TableNextColumn();
            if (ImGui::Button("削除")) {
                removeRequest.isRequested = true;
                removeRequest.listType = listType;
                removeRequest.index = index;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (ImGui::TreeNodeEx("値を調整", ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_LabelSpanAllColumns)) {
                DrawPostEffectParameterRows(pass);
                ImGui::TreePop();
            }

            ImGui::PopID();
        }

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

        /// @brief 現在のギズモ操作を取得する
        /// @return 現在のギズモ操作
        ImGuizmo::OPERATION& CurrentGizmoOperation() {
            static ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;
            return currentOperation;
        }

        /// @brief ギズモ操作をTranslate、Rotate、Scaleの順に切り替える
        void CycleGizmoOperation() {
            ImGuizmo::OPERATION& currentOperation = CurrentGizmoOperation();
            if (currentOperation == ImGuizmo::TRANSLATE) {
                currentOperation = ImGuizmo::ROTATE;
            } else if (currentOperation == ImGuizmo::ROTATE) {
                currentOperation = ImGuizmo::SCALE;
            } else {
                currentOperation = ImGuizmo::TRANSLATE;
            }
        }

        /// @brief ギズモ操作モードの選択ボタンを描画する
        /// @param imageMin Game View画像領域の左上座標
        /// @return 現在選択されているImGuizmo操作
        ImGuizmo::OPERATION DrawGizmoOperationButtons(const ImVec2& imageMin) {
            ImGuizmo::OPERATION& currentOperation = CurrentGizmoOperation();

            ImGui::SetCursorScreenPos({ imageMin.x + 8.0f, imageMin.y + 8.0f });
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 0.0f });
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, kEditorIconButtonPadding);

            auto drawButton = [&currentOperation](const EditorIconButtonInfo& info, ImGuizmo::OPERATION operation) {
                const bool isSelected = currentOperation == operation;
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                }

                if (DrawEditorIconButton(info)) {
                    currentOperation = operation;
                }

                if (isSelected) {
                    ImGui::PopStyleColor(2);
                }
            };

            drawButton({ "##GizmoTranslate", "Translate", "Translate", "移動" }, ImGuizmo::TRANSLATE);
            ImGui::SameLine();
            drawButton({ "##GizmoRotate", "Rotate", "Rotate", "回転" }, ImGuizmo::ROTATE);
            ImGui::SameLine();
            drawButton({ "##GizmoScale", "Scale", "Scale", "拡縮" }, ImGuizmo::SCALE);

            ImGui::PopStyleVar(3);
            return currentOperation;
        }

        struct ModelGizmoEditState {
            bool wasUsing = false;
            bool isEditing = false;
            Model* target = nullptr;
            Transform3D beforeTransform{};
        };

        /// @brief Modelギズモ操作の開始/終了状態を取得する
        /// @return Modelギズモ操作状態
        ModelGizmoEditState& CurrentModelGizmoEditState() {
            static ModelGizmoEditState state;
            return state;
        }

        /// @brief 2つのfloatがほぼ同じ値か確認する
        /// @param a 比較する値
        /// @param b 比較する値
        /// @return ほぼ同じ場合はtrue
        bool NearlyEqual(float a, float b) {
            return std::fabs(a - b) <= 0.0001f;
        }

        /// @brief 2つのVector3がほぼ同じ値か確認する
        /// @param a 比較する値
        /// @param b 比較する値
        /// @return ほぼ同じ場合はtrue
        bool NearlyEqual(const Vector3& a, const Vector3& b) {
            return NearlyEqual(a.x, b.x) &&
                NearlyEqual(a.y, b.y) &&
                NearlyEqual(a.z, b.z);
        }

        /// @brief 2つのTransformがほぼ同じ値か確認する
        /// @param a 比較するTransform
        /// @param b 比較するTransform
        /// @return ほぼ同じ場合はtrue
        bool NearlyEqual(const Transform3D& a, const Transform3D& b) {
            return NearlyEqual(a.scale, b.scale) &&
                NearlyEqual(a.rotate, b.rotate) &&
                NearlyEqual(a.translate, b.translate);
        }

        /// @brief Undo/Redoボタンを描画する
        /// @param imageMin Game View画像領域の左上座標
        void DrawHistoryButtons(const ImVec2& imageMin) {
            EditorHistory& history = EditorHistory::GetInstance();
            const bool canUndo = history.CanUndo();
            const bool canRedo = history.CanRedo();

            ImGui::SetCursorScreenPos({ imageMin.x + 8.0f, imageMin.y + 48.0f });
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 0.0f });
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, kEditorIconButtonPadding);

            if (!canUndo) {
                ImGui::BeginDisabled();
            }
            if (DrawEditorIconButton({ "##GizmoUndo", "Undo", "Undo", "元に戻す" })) {
                history.Undo();
            }
            if (!canUndo) {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();

            if (!canRedo) {
                ImGui::BeginDisabled();
            }
            if (DrawEditorIconButton({ "##GizmoRedo", "Redo", "Redo", "やり直す" })) {
                history.Redo();
            }
            if (!canRedo) {
                ImGui::EndDisabled();
            }

            ImGui::PopStyleVar(3);
        }

        /// @brief Editor履歴のショートカットを処理する
        void HandleHistoryShortcuts() {
            ImGuiIO& io = ImGui::GetIO();
            if (!io.KeyCtrl || ImGuizmo::IsUsing()) {
                return;
            }

            EditorHistory& history = EditorHistory::GetInstance();
            if (ImGui::IsKeyPressed(ImGuiKey_Z) && io.KeyShift) {
                history.Redo();
                return;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
                history.Redo();
                return;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
                history.Undo();
            }
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
            const bool isSnapEnabled = ImGui::IsKeyDown(ImGuiKey_LeftShift);
            const float snapValue[3] = { 0.1f, 0.1f, 0.1f };
            const float* snap = isSnapEnabled ? snapValue : nullptr;

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
            ImGuizmo::SetRect(imageMin.x, imageMin.y, imageSize.x, imageSize.y);

            bool isChanged = ImGuizmo::Manipulate(
                &camera.GetViewMatrix().m[0][0],
                &camera.GetProjectionMatrix().m[0][0],
                operation,
                ImGuizmo::WORLD,
                &modelMatrix.m[0][0],
                nullptr,
                snap);

            if (isChanged) {
                ApplyMatrixToTransform(modelMatrix, operation, transform);
            }

            return isChanged;
        }

        /// @brief Modelギズモ操作をUndo履歴に記録する
        /// @param selectedModel 現在選択されているModel
        /// @param beforeDrawTransform ギズモ描画前のTransform
        /// @return 履歴が追加された場合はtrue
        bool UpdateModelGizmoHistory(Model* selectedModel, const Transform3D& beforeDrawTransform) {
            ModelGizmoEditState& state = CurrentModelGizmoEditState();
            const bool isUsing = ImGuizmo::IsUsing();
            bool isChanged = false;

            if (!state.wasUsing && isUsing && selectedModel) {
                state.isEditing = true;
                state.target = selectedModel;
                state.beforeTransform = beforeDrawTransform;
            }

            if (state.wasUsing && !isUsing && state.isEditing && state.target) {
                const Transform3D afterTransform = state.target->GetTransform();
                if (!NearlyEqual(state.beforeTransform, afterTransform)) {
                    EditorHistory::GetInstance().Push(std::make_unique<ModelTransformCommand>(
                        state.target,
                        TransformSnapshot{ state.beforeTransform },
                        TransformSnapshot{ afterTransform }));
                    isChanged = true;
                }

                state.isEditing = false;
                state.target = nullptr;
                state.beforeTransform = {};
            }

            if (!selectedModel && !isUsing) {
                state.isEditing = false;
                state.target = nullptr;
                state.beforeTransform = {};
            }

            state.wasUsing = isUsing;
            return isChanged;
        }

    } // namespace

    void DrawLayerEffectPassEditorUI(Render::PostEffectManager& postEffectManager) {
        ImGui::Begin("Layer Effect Pass Editor");

        if (ImGui::Button("追加")) {
            AddLayerEffectPassFromEditor(postEffectManager, false);
        }
        ImGui::SameLine();
        if (ImGui::Button("フルスクリーン追加")) {
            AddLayerEffectPassFromEditor(postEffectManager, true);
        }

        ImGui::Separator();

        LayerEffectPassRemoveRequest removeRequest{};
        constexpr ImGuiTableFlags tableFlags =
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_BordersInnerH |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_SizingStretchProp;

        if (ImGui::BeginTable("LayerEffectPassEditorTable", 6, tableFlags)) {
            ImGui::TableSetupColumn("On", ImGuiTableColumnFlags_WidthFixed, 36.0f);
            ImGui::TableSetupColumn("名前", ImGuiTableColumnFlags_WidthStretch, 1.5f);
            ImGui::TableSetupColumn("ポストエフェクト", ImGuiTableColumnFlags_WidthStretch, 1.2f);
            ImGui::TableSetupColumn("レイヤー", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("Depth", ImGuiTableColumnFlags_WidthFixed, 52.0f);
            ImGui::TableSetupColumn("削除", ImGuiTableColumnFlags_WidthFixed, 56.0f);
            ImGui::TableHeadersRow();

            std::vector<Render::LayerEffectPass>& layerPasses = postEffectManager.GetLayerPasses();
            for (std::size_t i = 0; i < layerPasses.size(); ++i) {
                DrawLayerEffectPassEditorRow(layerPasses[i], LayerEffectPassListType::Layer, i, removeRequest);
            }

            std::vector<Render::LayerEffectPass>& screenPasses = postEffectManager.GetScreenPasses();
            for (std::size_t i = 0; i < screenPasses.size(); ++i) {
                DrawLayerEffectPassEditorRow(screenPasses[i], LayerEffectPassListType::Screen, i, removeRequest);
            }

            ImGui::EndTable();
        }

        if (removeRequest.isRequested) {
            if (removeRequest.listType == LayerEffectPassListType::Layer) {
                postEffectManager.RemoveLayerPass(removeRequest.index);
            } else {
                postEffectManager.RemoveScreenPass(removeRequest.index);
            }
        }

        ImGui::End();
    }

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
        HandleHistoryShortcuts();

        if (selectedModel && !selectedModel->IsVisible()) {
            selectedModel = nullptr;
            isChanged = true;
        }

        const ImVec2 mousePosition = ImGui::GetMousePos();
        const bool isMouseOnGameView = IsPointInRect(mousePosition, imageMin, imageSize);
        if (selectedModel &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
            isMouseOnGameView &&
            !ImGuizmo::IsUsing()) {
            CycleGizmoOperation();
            isChanged = true;
        }

        if (selectedModel) {
            DrawHistoryButtons(imageMin);

            Transform3D transform = selectedModel->GetTransform();
            const Transform3D beforeDrawTransform = transform;
            if (DrawTransformGizmoInRect(camera, transform, imageMin, imageSize)) {
                selectedModel->SetTransform(transform);
                isChanged = true;
            }
            if (UpdateModelGizmoHistory(selectedModel, beforeDrawTransform)) {
                isChanged = true;
            }
        } else {
            Transform3D emptyTransform{};
            UpdateModelGizmoHistory(nullptr, emptyTransform);
        }

        const bool canSelect =
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            isMouseOnGameView &&
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
