#pragma once
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"
#include "Utility/Camera/Frustum.h"

/// @brief カメラクラス
/// ビュー行列・プロジェクション行列・ビュープロジェクション行列を管理する
class Camera {
public:
	Camera();
	virtual ~Camera() = default;

	/// @brief 更新処理
	virtual void Update();

	// --- Setter ---

	/// @brief カメラの位置を設定する
	/// @param position ワールド座標
	void SetPosition(const Vector3& position) { position_ = position; }

	/// @brief カメラの回転角を設定する（ラジアン）
	/// @param rotation X,Y,Z軸周りの回転角
	void SetRotation(const Vector3& rotation) { rotation_ = rotation; }

	/// @brief 垂直視野角を設定する
	/// @param fovY ラジアン単位の垂直視野角
	void SetFovY(float fovY) { fovY_ = fovY; }

	/// @brief アスペクト比を設定する
	/// @param aspectRatio 幅/高さ
	void SetAspectRatio(float aspectRatio) { aspectRatio_ = aspectRatio; }

	/// @brief ニアクリップ距離を設定する
	/// @param nearClip ニアクリップ面距離
	void SetNearClip(float nearClip) { nearClip_ = nearClip; }

	/// @brief ファークリップ距離を設定する
	/// @param farClip ファークリップ面距離
	void SetFarClip(float farClip) { farClip_ = farClip; }

	/// @brief カメラパラメータをまとめて設定する
	/// @param position ワールド座標
	float GetFovY() const { return fovY_; }

	/// @brief アスペクト比を取得する
	/// @return 幅/高さ
	float GetAspectRatio() const { return aspectRatio_; }

	/// @brief ニアクリップ距離を取得する
	/// @return ニアクリップ面距離
	float GetNearClip() const { return nearClip_; }

	/// @brief ファークリップ距離を取得する
	/// @return ファークリップ面距離
	float GetFarClip() const { return farClip_; }

	// --- Getter ---

	/// @brief カメラの位置を取得する
	/// @return ワールド座標
	const Vector3& GetPosition() const { return position_; }

	/// @brief カメラの回転角を取得する
	/// @return X,Y,Z軸周りの回転角（ラジアン）
	const Vector3& GetRotation() const { return rotation_; }

	/// @brief ビュー行列を取得する
	/// @return ビュー行列
	const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }

	/// @brief プロジェクション行列を取得する
	/// @return プロジェクション行列
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }

	/// @brief ビュープロジェクション行列を取得する
	/// @return ビュープロジェクション行列
	const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }

	/// @brief フラスタムを取得する
	/// @return 視錐台
	const Frustum& GetFrustum() const { return frustum_; }

protected:
	Vector3 position_ = { 0.0f, 0.0f, -10.0f };
	Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };

	float fovY_ = 0.45f;
	float aspectRatio_ = 16.0f / 9.0f;
	float nearClip_ = 0.1f;
	float farClip_ = 1000.0f;

	Matrix4x4 viewMatrix_;
	Matrix4x4 projectionMatrix_;
	Matrix4x4 viewProjectionMatrix_;

	Frustum frustum_;

	/// @brief フラスタムを更新する
	void UpdateFrustum();
};
