#include "GameCameraController.h"
#include "Input/MyInput.h"
#include <algorithm>

namespace {
	constexpr float kTpsCameraGamePadZoomSpeed = 15.0f;
	constexpr float kTpsCameraMouseWheelZoomStep = 2.0f;
	constexpr float kTpsCameraMinDistance = 5.0f;
	constexpr float kTpsCameraMaxDistance = 30.0f;
}

namespace System {

	void GameCameraController::Initialize(CameraManager& cameraManager) {
		debugCameraHandle_ = cameraManager.CreateCamera<DebugCamera>("GameDebugCamera");
		tpsCameraHandle_ = cameraManager.CreateCamera<TPS_Camera>("GamePlayerCamera");
		if (DebugCamera* debugCamera = cameraManager.TryGetCamera<DebugCamera>(debugCameraHandle_)) {
			debugCamera->SetPosition({ 0.0f, 10.0f, -20.0f });
		}
		cameraManager.CutTo(tpsCameraHandle_);
		useDebugCamera_ = false;

		if (TPS_Camera* tpsCamera = TryGetPlayerCamera(cameraManager)) {
			tpsCamera->SetDistance(kTpsCameraMinDistance);
		}
	}

	void GameCameraController::Finalize() {
		debugCameraHandle_ = {};
		tpsCameraHandle_ = {};
		useDebugCamera_ = false;
	}

	void GameCameraController::UpdateTracking(CameraManager& cameraManager, const Vector3& playerPosition,
		float gameDeltaTime, bool isInputEnabled) {
		if (TPS_Camera* tpsCamera = TryGetPlayerCamera(cameraManager)) {
			tpsCamera->SetTargetPosition(playerPosition);
			tpsCamera->SetInputEnabled(isInputEnabled);
			if (isInputEnabled) {
				float distanceDelta = 0.0f;

				// 左右トリガーの差分で同時入力を相殺し、時間比例の距離変化へ変換
				if (tpsCamera->GetUseGamePadInput()) {
					MadoEngine::InputDevice::GamePad* gamePad = MyInput::GetGamePad();
					if (gamePad && gamePad->IsConnected()) {
						const float zoomInput = gamePad->GetLeftTrigger() - gamePad->GetRightTrigger();
						distanceDelta += zoomInput * kTpsCameraGamePadZoomSpeed * gameDeltaTime;
					}
				}

				// 前方ホイールで接近、後方ホイールで後退となる距離変化へ変換
				if (MadoEngine::InputDevice::Mouse* mouse = MyInput::GetMouse()) {
					distanceDelta -= mouse->GetWheelDelta() * kTpsCameraMouseWheelZoomStep;
				}
				if (distanceDelta != 0.0f) {
					const float nextDistance = tpsCamera->GetDistance() + distanceDelta;
					tpsCamera->SetDistance(std::clamp(nextDistance, kTpsCameraMinDistance, kTpsCameraMaxDistance));
				}
			}
		}

		// TPS入力を停止中もManager経由の描画Camera同期とデバッグCameraの更新を維持
		cameraManager.Update(gameDeltaTime);
	}

	void GameCameraController::UpdateSwitchInput(CameraManager& cameraManager) {

		// EditorやJson復元後のActive Cameraを基準に次の切り替え先を決定
		const CameraHandle activeCameraHandle = cameraManager.GetActiveCameraHandle();
		if (activeCameraHandle == debugCameraHandle_) {
			useDebugCamera_ = true;
		} else if (activeCameraHandle == tpsCameraHandle_) {
			useDebugCamera_ = false;
		}
		if (MyInput::GetKeyboard()->IsTrigger(DIK_F9)) {
			useDebugCamera_ = !useDebugCamera_;
			if (useDebugCamera_) {
				cameraManager.CutTo(debugCameraHandle_);
			} else {
				cameraManager.CutTo(tpsCameraHandle_);
			}
		}
	}

	TPS_Camera* GameCameraController::TryGetPlayerCamera(CameraManager& cameraManager) const {
		return cameraManager.TryGetCamera<TPS_Camera>(tpsCameraHandle_);
	}

}
