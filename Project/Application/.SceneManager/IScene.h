#pragma once
#include "MathHeaders.h"
#include "RenderHeaders.h"
#include "UtilityHeaders.h"
#include "ImGuiHeaders.h"
#include "SceneType.h"
#include <string>

class IScene {
public:
	virtual ~IScene() = default;

	virtual void Initialize() = 0;
	virtual SceneType Update(float dt) = 0;
	virtual void Draw() = 0;
	virtual void DrawImGui() = 0;
	virtual void Finalize() = 0;

	/// @brief Map生成Editorを提供しているか確認
	/// @return Map生成Editorを提供している場合はtrue
	virtual bool HasMapGeneratorEditor() const { return false; }

	/// @brief Map生成EditorのImGuiを描画
	virtual void DrawMapGeneratorImGui() {}

	/// @brief Map生成Editor状態を文字列Snapshotへ変換
	/// @return UndoとRedoに使用する文字列Snapshot
	virtual std::string CaptureMapGeneratorEditorState() const { return {}; }

	/// @brief 文字列SnapshotからMap生成Editor状態を復元
	/// @param snapshot 復元する文字列Snapshot
	virtual void RestoreMapGeneratorEditorState(const std::string& snapshot) { (void)snapshot; }

	/// @brief Scene固有のEditor Documentを保存
	/// @return 保存に成功した場合はtrue
	virtual bool SaveEditorDocuments() const { return true; }

	/// @brief Scene固有のEditor Documentを再読込
	/// @return 読み込みに成功した場合はtrue
	virtual bool ReloadEditorDocuments() { return true; }

	/// @brief Frame末尾に保留中のEditor操作を適用
	/// @return Scene状態を変更した場合はtrue
	virtual bool ApplyPendingEditorOperations() { return false; }

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
