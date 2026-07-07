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

	Camera GetCamera() const { return sceneCamera_; }

	/// @brief シャドウマップ生成時に中心へ置くワールド座標を取得します。
	/// @return シャドウマップの注視点です。
	virtual Vector3 GetShadowFocusPosition() const { return sceneCamera_.GetPosition(); }

	/// @brief シャドウマップ確認用の対象座標を取得します。
	/// @param outPosition 対象のワールド座標を受け取る変数です。
	/// @return 対象座標を取得できた場合はtrueを返します。
	virtual bool TryGetShadowDebugTargetPosition(Vector3& outPosition) const {
		outPosition = {};
		return false;
	}

	Camera sceneCamera_;
};
