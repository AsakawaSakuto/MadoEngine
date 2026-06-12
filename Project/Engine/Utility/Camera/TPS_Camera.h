#pragma once
#include "Utility/Camera/Camera.h"
#include <corecrt_math_defines.h>
#include <algorithm>
#include <cmath>

namespace {
	constexpr float kDefaultDeltaTime = 1.0f / 60.0f;
	constexpr float kMinFollowStrength = 0.0f;
	constexpr float kMaxFollowStrength = 1.0f;
	constexpr float kRadiansToDegrees = 180.0f / static_cast<float>(M_PI);
	constexpr float kDegreesToRadians = static_cast<float>(M_PI) / 180.0f;
}

/// @brief TPSカメラクラス
/// ターゲットを球面座標で追従し、肩越しオフセットに対応したサードパーソンカメラ。
class TPS_Camera : public Camera {
public:
	TPS_Camera();
	~TPS_Camera() override = default;

	/// @brief 更新処理（入力・追従・カメラ位置計算）
	/// @param deltaTime フレーム時間（秒）
	void Update(float deltaTime) override;

	// --- ターゲット設定 ---

	/// @brief 追従対象の位置を設定する
	/// @param targetPosition 追従するワールド座標
	void SetTargetPosition(const Vector3& targetPosition) { targetPosition_ = targetPosition; }

	// --- 球面座標パラメータ ---

	/// @brief 旋回角（Yaw）を設定する（ラジアン）
	/// @param yaw Y軸周りの回転角
	void SetYaw(float yaw) { yaw_ = yaw; }

	/// @brief 仰俯角（Pitch）を設定する（ラジアン）
	/// @param pitch X軸周りの回転角
	void SetPitch(float pitch) { pitch_ = pitch; ClampPitch(); }

	/// @brief ターゲットとの距離を設定する
	/// @param distance カメラ-ターゲット間の距離
	void SetDistance(float distance) { distance_ = distance; }

	/// @brief 旋回角（Yaw）を取得する
	/// @return Y軸周りの回転角（ラジアン）
	float GetYaw() const { return yaw_; }

	/// @brief 仰俯角（Pitch）を取得する
	/// @return X軸周りの回転角（ラジアン）
	float GetPitch() const { return pitch_; }

	// --- 入力感度 ---

	/// @brief マウス回転感度を設定する
	/// @param sensitivity マウス回転感度
	void SetMouseSensitivity(float sensitivity) { mouseSensitivity_ = sensitivity; }

	/// @brief ゲームパッド右スティック回転感度を設定する
	/// @param sensitivity スティック回転感度
	void SetGamePadSensitivity(float sensitivity) {	gamePadSensitivity_ = sensitivity; }

	// --- 追従スムージング ---

	/// @brief 追従の補間強度を設定する（0.0=全く動かない / 1.0=即座に追従）
	/// @param strength 補間強度（0.0f〜1.0f）
	void SetFollowStrength(float strength) { followStrength_ = std::clamp(strength, kMinFollowStrength, kMaxFollowStrength); }

	/// @brief 追従の補間強度を取得する
	/// @return 補間強度（0.0f〜1.0f）
	float GetFollowStrength() const { return followStrength_; }

	// --- オフセット ---

	/// @brief 肩越し視点用のローカルオフセットを設定する
	/// @param offset ターゲット座標系でのオフセット（右/上/前方向）
	void SetOffset(const Vector3& offset) { offset_ = offset; }

	/// @brief 現在のオフセットを取得する
	/// @return ローカルオフセット
	const Vector3& GetOffset() const { return offset_; }

	/// @brief Pitch角のクランプ範囲を設定する（ラジアン）
	/// @param minPitch 最小仰俯角（例: -1.2f）
	/// @param maxPitch 最大仰俯角（例: 1.2f）
	void SetPitchLimit(float minPitch, float maxPitch) { minPitch_ = (std::min)(minPitch, maxPitch); maxPitch_ = (std::max)(minPitch, maxPitch); ClampPitch();}

	/// @brief マウス入力の有効/無効を設定する
	/// @param enable trueで有効
	void SetUseMouseInput(bool enable) { useMouseInput_ = enable; }

	/// @brief ゲームパッド入力の有効/無効を設定する
	/// @param enable trueで有効
	void SetUseGamePadInput(bool enable) { useGamePadInput_ = enable; }

	/// @brief マウス入力が有効かどうかを取得する
	/// @return 有効ならtrue
	bool GetUseMouseInput() const { return useMouseInput_; }

	/// @brief ゲームパッド入力が有効かどうかを取得する
	/// @return 有効ならtrue
	bool GetUseGamePadInput() const { return useGamePadInput_; }

	/// @brief ImGui描画処理
	void DrawImGui();

private:
	// ターゲット
	Vector3 targetPosition_;
	
	// 補間後の現在注視点
	Vector3 currentTarget_ = { 0.0f, 0.0f, 0.0f };

	bool useMouseInput_ = false;  // マウス入力を使用するか
	bool useGamePadInput_ = true; // ゲームパッド入力を使用するか

	// 球面座標パラメータ
	float yaw_      = 0.0f;  // Y軸周りの回転（ラジアン）
	float pitch_    = 0.3f;  // X軸周りの回転（ラジアン）
	float distance_ = 20.0f;  // ターゲットからの距離

	// Pitch角クランプ範囲
	float minPitch_ = -0.1f;
	float maxPitch_ = 1.5f;

	// 入力感度
	float mouseSensitivity_   = 0.003f; // マウス感度
	float gamePadSensitivity_ = 2.5f;   // ゲームパッド右スティック感度（rad/sec）

	// 追従スムージング（0.0〜1.0）
	float followStrength_ = 0.5f;

	// 肩越しオフセット（ターゲット座標系のローカルオフセット）
	Vector3 offset_ = { 0.0f, 4.0f, -5.0f };

	/// @brief 入力を処理してYaw/Pitchを更新する
	/// @param deltaTime フレーム時間（秒）
	void HandleInput(float deltaTime);

	/// @brief 追従対象へ現在注視点を補間する
	void UpdateCurrentTarget();

	/// @brief 球面座標からカメラ位置・回転を計算してベースクラスへ反映する
	void ApplySphericalCoord();

	/// @brief オフセット適用後の注視中心を計算する
	/// @return オフセット適用後の注視中心
	Vector3 CalculateViewCenter() const;

	/// @brief 球面座標からカメラ位置を計算する
	/// @param viewCenter 注視中心
	/// @return カメラのワールド座標
	Vector3 CalculateBasePosition(const Vector3& viewCenter) const;

	/// @brief カメラから注視中心へ向かう前方ベクトルを計算する
	/// @return 正規化済み前方ベクトル
	Vector3 CalculateForwardDirection() const;

	/// @brief カメラの右方向ベクトルを計算する
	/// @return 正規化済み右方向ベクトル
	Vector3 CalculateRightDirection() const;

	/// @brief 注視中心へ向く回転を計算する
	/// @param viewCenter 注視中心
	void ApplyLookAtRotation(const Vector3& viewCenter);

	/// @brief Pitch角を設定範囲内へ丸める
	void ClampPitch();
};
