#pragma once
#include "Utility/Camera/Camera.h"

/// @brief デバッグカメラクラス
/// マウス操作によるオービット回転・パン・ズームに対応したカメラ
class DebugCamera : public Camera {
public:
	DebugCamera();
	~DebugCamera() override = default;

	/// @brief 更新処理（マウス入力によるカメラ操作）
	void Update(float deltaTime) override;

	/// @brief 回転感度を設定する
	/// @param sensitivity 回転感度（デフォルト: 0.005f）
	void SetRotateSensitivity(float sensitivity) { rotateSensitivity_ = sensitivity; }

	/// @brief パン感度を設定する
	/// @param sensitivity パン感度（デフォルト: 0.01f）
	void SetPanSensitivity(float sensitivity) { panSensitivity_ = sensitivity; }

	/// @brief ドリー感度を設定する
	/// @param sensitivity ドリー感度（デフォルト: 1.0f）
	void SetDollySensitivity(float sensitivity) { dollySensitivity_ = sensitivity; }

	/// @brief ImGui描画処理
	void DrawImGui();
private:
	// ターゲット（注視点）
	Vector3 target_ = { 0.0f, 0.0f, 0.0f };

	// カメラとターゲット間の距離
	float distance_ = 10.0f;

	// 球面座標上の角度
	float yaw_ = 0.0f;   // Y軸周りの回転
	float pitch_ = 0.3f; // X軸周りの回転

	float rotateSensitivity_ = 0.25f; // 回転感度
	float panSensitivity_ = 0.25f;    // パン感度
	float dollySensitivity_ = 10.0f;     // ドリー感度

	/// @brief 球面座標からカメラ位置・回転を計算してベースクラスへ反映する
	void ApplySphericalCoord();
};
