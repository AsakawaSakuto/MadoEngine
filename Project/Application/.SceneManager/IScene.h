#pragma once
#include "MathHeaders.h"
#include "RenderHeaders.h"
#include "UtilityHeaders.h"
#include "ImGuiHeaders.h"
#include "SceneType.h"

class IScene {
public:
	virtual ~IScene() = default;

	virtual void Initialize() = 0;
	virtual SceneType Update(float dt) = 0;
	virtual void Draw() = 0;
	virtual void DrawImGui() = 0;
	virtual void Finalize() = 0;

	/// @brief 描画に使用するCameraを取得
	/// @return Sceneの描画Camera
	Camera& GetCamera() { return cameraManager_.GetRenderCamera(); }

	/// @brief 描画に使用する読み取り専用Cameraを取得
	/// @return Sceneの描画Camera
	const Camera& GetCamera() const { return cameraManager_.GetRenderCamera(); }

	/// @brief Scene内Cameraを管理するManagerを取得
	/// @return SceneローカルCameraManager
	CameraManager& GetCameraManager() { return cameraManager_; }

	/// @brief Scene内Cameraを管理する読み取り専用Managerを取得
	/// @return SceneローカルCameraManager
	const CameraManager& GetCameraManager() const { return cameraManager_; }

	/// @brief シャドウマップ生成時に中心へ置くワールド座標を取得
	/// @return シャドウマップの注視点
	virtual Vector3 GetShadowFocusPosition() const { return GetCamera().GetPosition(); }

	/// @brief シャドウマップ確認用の対象座標を取得
	/// @param outPosition 対象のワールド座標を受け取る変数
	/// @return 対象座標を取得できた場合はtrue
	virtual bool TryGetShadowDebugTargetPosition(Vector3& outPosition) const {
		outPosition = {};
		return false;
	}

protected:
	CameraManager cameraManager_;
};
