#include "CameraManagerEditor.h"

#ifdef USE_IMGUI

#include "ImGuiHeaders.h"
#include "Utility/Camera/DebugCamera.h"
#include "Utility/Camera/TPS_Camera.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <numbers>
#include <string>
#include <typeinfo>
#include <vector>

namespace MadoEngine::Editor {

namespace {

	constexpr float kRadiansToDegrees = 180.0f / std::numbers::pi_v<float>;
	constexpr float kDegreesToRadians = std::numbers::pi_v<float> / 180.0f;
	constexpr float kMinProjectionValue = 0.0001f;

	struct EaseTypeItem {
		EaseType type;
		const char* label;
	};

	struct ShakeTypeItem {
		ShakeType type;
		const char* label;
	};

	struct CameraRemoveRequest {
		bool isRequested = false;
		CameraHandle handle{};
	};

	constexpr std::array<EaseTypeItem, 7> kEaseTypeItems = {
		EaseTypeItem{ EaseType::Linear, "Linear" },
		EaseTypeItem{ EaseType::EaseInCubic, "Ease In Cubic" },
		EaseTypeItem{ EaseType::EaseOutCubic, "Ease Out Cubic" },
		EaseTypeItem{ EaseType::EaseInOutCubic, "Ease In Out Cubic" },
		EaseTypeItem{ EaseType::EaseInOutQuad, "Ease In Out Quad" },
		EaseTypeItem{ EaseType::EaseInOutSine, "Ease In Out Sine" },
		EaseTypeItem{ EaseType::EaseOutBack, "Ease Out Back" },
	};

	constexpr std::array<ShakeTypeItem, 7> kShakeTypeItems = {
		ShakeTypeItem{ ShakeType::X, "X" },
		ShakeTypeItem{ ShakeType::Y, "Y" },
		ShakeTypeItem{ ShakeType::Z, "Z" },
		ShakeTypeItem{ ShakeType::XY, "XY" },
		ShakeTypeItem{ ShakeType::XZ, "XZ" },
		ShakeTypeItem{ ShakeType::YZ, "YZ" },
		ShakeTypeItem{ ShakeType::XYZ, "XYZ" },
	};

	/// @brief 文字列を固定長バッファへコピー
	/// @tparam Size バッファサイズ
	/// @param buffer コピー先バッファ
	/// @param text コピー元文字列
	template<size_t Size>
	void CopyToBuffer(std::array<char, Size>& buffer, const std::string& text) {
		buffer.fill('\0');
		strncpy_s(buffer.data(), buffer.size(), text.c_str(), _TRUNCATE);
	}

	/// @brief Cameraの実行時型に対応する表示名を取得
	/// @param camera 表示対象のCamera
	/// @return Camera型の表示名
	const char* GetCameraTypeLabel(const Camera& camera) {
		if (dynamic_cast<const TPS_Camera*>(&camera)) {
			return "TPS";
		}
		if (dynamic_cast<const DebugCamera*>(&camera)) {
			return "Debug";
		}
		return typeid(camera) == typeid(Camera) ? "Fixed" : "Camera";
	}

	/// @brief Cameraの位置と回転をEditorから直接編集できるか確認
	/// @param camera 確認対象のCamera
	/// @return 固定Cameraの場合はtrue
	bool IsTransformEditable(const Camera& camera) {
		return typeid(camera) == typeid(Camera);
	}

	/// @brief 選択中イージングの表示名を取得
	/// @param easeType 表示対象のイージング
	/// @return イージングの表示名
	const char* GetEaseTypeLabel(EaseType easeType) {
		for (const EaseTypeItem& item : kEaseTypeItems) {
			if (item.type == easeType) {
				return item.label;
			}
		}
		return "Linear";
	}

	/// @brief Camera補間用イージング選択Comboを描画
	/// @param easeType 選択中イージングの入出力先
	/// @return 選択が変更された場合はtrue
	bool DrawEaseTypeCombo(EaseType& easeType) {
		if (!ImGui::BeginCombo("イージング", GetEaseTypeLabel(easeType))) {
			return false;
		}

		bool isChanged = false;
		for (const EaseTypeItem& item : kEaseTypeItems) {
			const bool isSelected = item.type == easeType;
			if (ImGui::Selectable(item.label, isSelected)) {
				easeType = item.type;
				isChanged = true;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
		return isChanged;
	}

	/// @brief Camera Shakeの対象軸を選択するComboを描画
	/// @param shakeType 選択中Shake軸の入出力先
	/// @return 選択が変更された場合はtrue
	bool DrawShakeTypeCombo(ShakeType& shakeType) {
		const auto selectedItem = std::find_if(
			kShakeTypeItems.begin(),
			kShakeTypeItems.end(),
			[shakeType](const ShakeTypeItem& item) { return item.type == shakeType; }
		);
		const char* preview = selectedItem != kShakeTypeItems.end()
			? selectedItem->label
			: "XYZ";
		if (!ImGui::BeginCombo("振動軸", preview)) {
			return false;
		}

		bool isChanged = false;
		for (const ShakeTypeItem& item : kShakeTypeItems) {
			const bool isSelected = item.type == shakeType;
			if (ImGui::Selectable(item.label, isSelected)) {
				shakeType = item.type;
				isChanged = true;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
		return isChanged;
	}

	/// @brief Cameraが保持する移動先Cameraの選択Comboを描画
	/// @param cameraManager Camera一覧を保持するManager
	/// @param ownerHandle 遷移設定を所有するCameraのHandle
	/// @param destinationHandle 選択中の移動先Handle
	/// @return 選択が変更された場合はtrue
	bool DrawTransitionDestinationCombo(
		CameraManager& cameraManager,
		CameraHandle ownerHandle,
		CameraHandle& destinationHandle) {
		const std::string destinationName = cameraManager.GetCameraName(destinationHandle);
		const char* preview = destinationName.empty() ? "移動先を選択" : destinationName.c_str();
		if (!ImGui::BeginCombo("移動先", preview)) {
			return false;
		}

		bool isChanged = false;
		const bool isNoneSelected = !cameraManager.IsValid(destinationHandle);
		if (ImGui::Selectable("未設定", isNoneSelected)) {
			destinationHandle = {};
			isChanged = true;
		}
		if (isNoneSelected) {
			ImGui::SetItemDefaultFocus();
		}

		for (CameraHandle candidateHandle : cameraManager.GetCameraHandles()) {
			if (candidateHandle == ownerHandle) {
				continue;
			}

			const bool isSelected = candidateHandle == destinationHandle;
			const std::string candidateName = cameraManager.GetCameraName(candidateHandle);
			if (ImGui::Selectable(candidateName.c_str(), isSelected)) {
				destinationHandle = candidateHandle;
				isChanged = true;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
		return isChanged;
	}

	/// @brief 追加したCamera名を基準に次の未使用名を生成
	/// @param cameraManager Camera名の使用状況を確認するManager
	/// @param createdName 直前に追加したCamera名
	/// @return 末尾の番号を繰り上げた未使用のCamera名
	std::string MakeNextAvailableCameraName(
		const CameraManager& cameraManager,
		const std::string& createdName) {
		size_t suffixStart = createdName.size();
		while (suffixStart > 0) {
			const char character = createdName[suffixStart - 1];
			if (character < '0' || character > '9') {
				break;
			}
			--suffixStart;
		}

		std::string baseName = createdName.substr(0, suffixStart);
		uint64_t suffix = 1;
		if (suffixStart < createdName.size()) {
			const char* suffixBegin = createdName.data() + suffixStart;
			const char* suffixEnd = createdName.data() + createdName.size();
			const std::from_chars_result result = std::from_chars(suffixBegin, suffixEnd, suffix);
			if (result.ec == std::errc{} && result.ptr == suffixEnd) {
				++suffix;
			} else {
				baseName = createdName;
				suffix = 1;
			}
		}

		for (;;) {
			const std::string candidate = baseName + std::to_string(suffix);
			if (!cameraManager.Find(candidate).IsValid()) {
				return candidate;
			}
			++suffix;
		}
	}

	/// @brief 描画Cameraの共通状態を固定Cameraへ複製
	/// @param source 複製元Camera
	/// @param destination 複製先Camera
	void CopyCameraView(const Camera& source, Camera& destination) {
		destination.SetPosition(source.GetPosition());
		destination.SetRotation(source.GetRotation());
		destination.SetFovY(source.GetFovY());
		destination.SetAspectRatio(source.GetAspectRatio());
		destination.SetNearClip(source.GetNearClip());
		destination.SetFarClip(source.GetFarClip());
		destination.Update(0.0f);
	}

	/// @brief Camera名編集InputTextを描画
	/// @param cameraManager 編集対象のCameraManager
	/// @param handle 編集対象CameraのHandle
	void DrawCameraNameInput(CameraManager& cameraManager, CameraHandle handle) {
		std::array<char, 128> nameBuffer{};
		std::snprintf(
			nameBuffer.data(),
			nameBuffer.size(),
			"%s",
			cameraManager.GetCameraName(handle).c_str()
		);
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::InputText("##CameraName", nameBuffer.data(), nameBuffer.size())) {
			if (nameBuffer[0] != '\0') {
				cameraManager.RenameCamera(handle, nameBuffer.data());
			}
		}
	}

	/// @brief CameraのTransformとProjection編集UIを描画
	/// @param camera 編集対象のCamera
	/// @param transformEditable Transformを直接編集できる場合はtrue
	void DrawCameraProperties(Camera& camera, bool transformEditable) {
		bool isChanged = false;
		Vector3 position = camera.GetPosition();
		Vector3 rotationDegrees = camera.GetRotation() * kRadiansToDegrees;

		if (!transformEditable) {
			ImGui::BeginDisabled();
		}
		if (ImGui::DragFloat3("位置", &position.x, 0.05f)) {
			camera.SetPosition(position);
			isChanged = true;
		}
		if (ImGui::DragFloat3("回転", &rotationDegrees.x, 0.25f)) {
			camera.SetRotation(rotationDegrees * kDegreesToRadians);
			isChanged = true;
		}
		if (!transformEditable) {
			ImGui::EndDisabled();
			ImGui::TextDisabled("位置と回転はCamera固有の追従処理から更新されます");
		}

		ImGui::SeparatorText("Projection");
		float fovDegrees = camera.GetFovY() * kRadiansToDegrees;
		float aspectRatio = camera.GetAspectRatio();
		float nearClip = camera.GetNearClip();
		float farClip = camera.GetFarClip();
		bool isProjectionChanged = false;
		isProjectionChanged |= ImGui::DragFloat(
			"FOV", &fovDegrees, 0.1f, 1.0f, 179.0f, "%.1f deg", ImGuiSliderFlags_AlwaysClamp);
		isProjectionChanged |= ImGui::DragFloat(
			"Aspect Ratio", &aspectRatio, 0.001f, kMinProjectionValue, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		isProjectionChanged |= ImGui::DragFloat(
			"Near Clip", &nearClip, 0.001f, kMinProjectionValue, farClip, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		isProjectionChanged |= ImGui::DragFloat(
			"Far Clip", &farClip, 0.1f, nearClip + kMinProjectionValue, 100000.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
		if (isProjectionChanged) {

			// NearとFarをまとめて確定して一時的な逆転状態を射影行列へ渡さない制約
			nearClip = (std::max)(nearClip, kMinProjectionValue);
			farClip = (std::max)(farClip, nearClip + kMinProjectionValue);
			camera.SetFovY(std::clamp(fovDegrees, 1.0f, 179.0f) * kDegreesToRadians);
			camera.SetAspectRatio((std::max)(aspectRatio, kMinProjectionValue));
			camera.SetNearClip(nearClip);
			camera.SetFarClip(farClip);
			isChanged = true;
		}

		if (isChanged) {

			// Editor操作直後のFrustumと行列を次Frameの切り替え前から最新状態へ同期
			camera.Update(0.0f);
		}
	}

	/// @brief Camera一覧の選択項目を描画
	/// @param cameraManager 表示対象のCameraManager
	/// @param handle 表示対象CameraのHandle
	/// @param selectedHandle 選択中Handleの入出力先
	/// @param removeRequest Editor管理Cameraの削除要求出力先
	void DrawCameraListItem(
		CameraManager& cameraManager,
		CameraHandle handle,
		CameraHandle& selectedHandle,
		CameraRemoveRequest& removeRequest) {
		const Camera* camera = cameraManager.TryGetCamera(handle);
		if (!camera) {
			return;
		}

		ImGui::PushID(static_cast<int>(handle.index));
		ImGui::PushID(static_cast<int>(handle.generation));
		bool isActive = handle == cameraManager.GetActiveCameraHandle();
		const bool isBlendDestination =
			cameraManager.IsBlending() && handle == cameraManager.GetBlendDestinationHandle();
		if (ImGui::Checkbox("##ActiveCamera", &isActive)) {

			// Camera不在を許可せず、未選択CameraへのCheck操作だけを即時切り替えとして受付
			if (isActive) {
				cameraManager.CutTo(handle);
				selectedHandle = handle;
			}
		}
		ImGui::SameLine();

		std::string label;
		if (isBlendDestination) {
			label += "[Blend] ";
		}
		label += "[";
		label += GetCameraTypeLabel(*camera);
		label += "] ";
		label += cameraManager.GetCameraName(handle);

		const bool isEditorManaged = cameraManager.IsEditorManaged(handle);
		float selectableWidth = 0.0f;
		if (isEditorManaged) {
			const float deleteButtonWidth =
				ImGui::CalcTextSize("削除").x + ImGui::GetStyle().FramePadding.x * 2.0f;
			selectableWidth = (std::max)(
				ImGui::GetContentRegionAvail().x - deleteButtonWidth - ImGui::GetStyle().ItemSpacing.x,
				1.0f
			);
		}
		if (ImGui::Selectable(label.c_str(), handle == selectedHandle, 0, ImVec2(selectableWidth, 0.0f))) {
			selectedHandle = handle;
		}
		if (isEditorManaged) {
			ImGui::SameLine();
			if (ImGui::SmallButton("削除")) {
				removeRequest.isRequested = true;
				removeRequest.handle = handle;
			}
		}
		ImGui::PopID();
		ImGui::PopID();
	}

} // namespace

void DrawCameraManagerEditorUI(CameraManager& cameraManager, SceneType currentSceneType) {
	static CameraManager* previousCameraManager = nullptr;
	static SceneType previousSceneType = SceneType::None;
	static CameraHandle selectedHandle{};
	static std::array<char, 128> createName{};
	static bool createFailed = false;
	static int fileOperationResult = 0;

	if (previousCameraManager != &cameraManager || previousSceneType != currentSceneType) {

		// Scene切り替え後に前SceneのHandleと入力状態を持ち越さないEditor Session初期化
		previousCameraManager = &cameraManager;
		previousSceneType = currentSceneType;
		selectedHandle = cameraManager.GetActiveCameraHandle();
		CopyToBuffer(createName, "Camera1");
		createFailed = false;
		fileOperationResult = 0;
	}

	ImGui::SetNextWindowSize(ImVec2(720.0f, 460.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Camera Editor");
	const std::filesystem::path cameraJsonPath =
		CameraManager::CreateDefaultJsonPath(SceneTypeToString(currentSceneType));

	ImGui::SetNextItemWidth(200.0f);
	ImGui::InputText("新規Camera名", createName.data(), createName.size());
	ImGui::SameLine();
	if (ImGui::Button("現在Viewから固定Camera追加")) {
		const std::string requestedName = createName.data();
		const CameraHandle createdHandle = cameraManager.CreateEditorCamera<Camera>(requestedName);
		if (Camera* createdCamera = cameraManager.TryGetCamera(createdHandle)) {

			// 現在の画角を複製して追加直後からCamera Shotとして利用可能な状態を作成
			CopyCameraView(cameraManager.GetRenderCamera(), *createdCamera);
			selectedHandle = createdHandle;
			CopyToBuffer(createName, MakeNextAvailableCameraName(cameraManager, requestedName));
			createFailed = false;
		} else {
			createFailed = true;
		}
	}
	if (createFailed) {
		ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.3f, 1.0f), "空文字または使用済みのCamera名です");
	}

	if (ImGui::Button("保存")) {
		fileOperationResult = cameraManager.SaveToJson(cameraJsonPath) ? 1 : -1;
	}
	ImGui::SameLine();
	if (ImGui::Button("再読込")) {
		fileOperationResult = cameraManager.LoadFromJson(cameraJsonPath) ? 1 : -1;
		if (fileOperationResult > 0) {
			selectedHandle = cameraManager.GetActiveCameraHandle();
		}
	}
	ImGui::SameLine();
	ImGui::TextDisabled("%s", cameraJsonPath.generic_string().c_str());
	if (fileOperationResult > 0) {
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Camera設定の入出力に成功しました");
	} else if (fileOperationResult < 0) {
		ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.3f, 1.0f), "Camera設定の入出力に失敗しました");
	}
	ImGui::TextDisabled("Editor追加Cameraは保存すると次回のScene開始時に自動復元されます");
	ImGui::Separator();

	const std::vector<CameraHandle> cameraHandles = cameraManager.GetCameraHandles();
	if (!cameraManager.IsValid(selectedHandle) && !cameraHandles.empty()) {
		selectedHandle = cameraHandles.front();
	}

	ImGui::BeginChild("CameraList", ImVec2(260.0f, 0.0f), true);
	if (cameraHandles.empty()) {
		ImGui::TextDisabled("登録済みCameraがありません");
	}
	CameraRemoveRequest removeRequest{};
	for (CameraHandle handle : cameraHandles) {
		DrawCameraListItem(cameraManager, handle, selectedHandle, removeRequest);
	}
	ImGui::EndChild();

	if (removeRequest.isRequested) {

		// Sceneコードが参照するRuntime Cameraを保護し、Editor追加Cameraだけを破棄
		const bool wasSelected = removeRequest.handle == selectedHandle;
		if (cameraManager.IsEditorManaged(removeRequest.handle)) {
			cameraManager.DestroyCamera(removeRequest.handle);
		}
		if (wasSelected) {
			selectedHandle = cameraManager.GetActiveCameraHandle();
		}
	}

	ImGui::SameLine();
	ImGui::BeginChild("CameraProperties", ImVec2(0.0f, 0.0f), true);
	Camera* selectedCamera = cameraManager.TryGetCamera(selectedHandle);
	if (!selectedCamera) {
		selectedHandle = {};
		ImGui::TextDisabled("Cameraを選択してください");
		ImGui::EndChild();
		ImGui::End();
		return;
	}

	ImGui::Text("種類: %s", GetCameraTypeLabel(*selectedCamera));
	ImGui::TextUnformatted("名前");
	const bool isEditorManagedCamera = cameraManager.IsEditorManaged(selectedHandle);
	if (!isEditorManagedCamera) {
		ImGui::BeginDisabled();
	}
	DrawCameraNameInput(cameraManager, selectedHandle);
	if (!isEditorManagedCamera) {
		ImGui::EndDisabled();
		ImGui::TextDisabled("Runtime Camera名はJson復元キーとして固定されます");
	}
	ImGui::SeparatorText("遷移設定");
	CameraTransitionSettings transitionSettings =
		cameraManager.GetTransitionSettings(selectedHandle);
	const bool hasDestination =
		cameraManager.IsValid(transitionSettings.destinationHandle);
	bool isTransitionChanged = false;
	if (ImGui::RadioButton(
		"Cut",
		transitionSettings.mode == CameraTransitionMode::Cut)) {
		transitionSettings.mode = CameraTransitionMode::Cut;
		isTransitionChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"Blend",
		transitionSettings.mode == CameraTransitionMode::Blend)) {
		transitionSettings.mode = CameraTransitionMode::Blend;
		isTransitionChanged = true;
	}
	ImGui::SameLine();
	if (!hasDestination) {
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("移動")) {
		cameraManager.ExecuteTransition(selectedHandle);
	}
	if (!hasDestination) {
		ImGui::EndDisabled();
	}
	isTransitionChanged |= DrawTransitionDestinationCombo(
		cameraManager,
		selectedHandle,
		transitionSettings.destinationHandle
	);
	if (transitionSettings.mode == CameraTransitionMode::Blend) {
		isTransitionChanged |= ImGui::DragFloat(
			"補間時間",
			&transitionSettings.blendDuration,
			0.01f,
			0.0f,
			300.0f,
			"%.2f sec",
			ImGuiSliderFlags_AlwaysClamp
		);
		isTransitionChanged |= DrawEaseTypeCombo(transitionSettings.blendEaseType);
	}
	if (isTransitionChanged) {
		cameraManager.SetTransitionSettings(selectedHandle, transitionSettings);
	}

	if (cameraManager.IsBlending()) {
		ImGui::ProgressBar(cameraManager.GetBlendProgress(), ImVec2(-1.0f, 0.0f), "補間中");
	}

	ImGui::SeparatorText("Shake設定");
	CameraShakeSettings shakeSettings = cameraManager.GetShakeSettings(selectedHandle);
	bool isShakeChanged = false;
	isShakeChanged |= ImGui::DragFloat(
		"強さ",
		&shakeSettings.power,
		0.01f,
		0.0f,
		100000.0f,
		"%.2f",
		ImGuiSliderFlags_AlwaysClamp
	);
	isShakeChanged |= ImGui::DragFloat(
		"Shake時間",
		&shakeSettings.duration,
		0.01f,
		0.0f,
		300.0f,
		"%.2f sec",
		ImGuiSliderFlags_AlwaysClamp
	);
	isShakeChanged |= DrawShakeTypeCombo(shakeSettings.type);
	if (isShakeChanged) {
		cameraManager.SetShakeSettings(selectedHandle, shakeSettings);
	}

	const bool canExecuteShake =
		shakeSettings.power > 0.0f && shakeSettings.duration > 0.0f;
	if (!canExecuteShake) {
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("Shake実行")) {
		cameraManager.ExecuteShake(selectedHandle);
	}
	if (!canExecuteShake) {
		ImGui::EndDisabled();
	}

	ImGui::SeparatorText("Camera");
	DrawCameraProperties(*selectedCamera, IsTransformEditable(*selectedCamera));
	ImGui::EndChild();
	ImGui::End();
}

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
