#include "GuizmoEditor.h"
#include "./History/EditorHistory.h"
#include "./History/ModelTransformCommand.h"
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

#endif // USE_IMGUI

} // namespace MadoEngine::Editor
