#include "Utility/Camera/Camera.h"
#include "Math/Function/MatrixFunction.h"
#include "Math/Function/MathFunction.h"

Camera::Camera() {
	Update();
}

void Camera::Update() {
	// ビュー行列の生成（アフィン行列の逆行列）
	Matrix4x4 affine = Matrix::MakeAffine({ 1.0f,1.0f,1.0f }, rotation_, position_);
	viewMatrix_ = Matrix::Inverse(affine);

	// プロジェクション行列の生成
	projectionMatrix_ = Matrix::MakePerspectiveFov(fovY_, aspectRatio_, nearClip_, farClip_);

	// ビュープロジェクション行列
	viewProjectionMatrix_ = Matrix::Multiply(viewMatrix_, projectionMatrix_);

	// フラスタム更新
	UpdateFrustum();
}

void Camera::UpdateFrustum() {
	// 6つの平面をビュープロジェクション行列の各列から抽出（行列転置なし、行優先）
	const Matrix4x4& m = viewProjectionMatrix_;

	// 左
	frustum_.planes[0].normal = { m.m[0][3] + m.m[0][0], m.m[1][3] + m.m[1][0], m.m[2][3] + m.m[2][0] };
	frustum_.planes[0].distance = m.m[3][3] + m.m[3][0];
	// 右
	frustum_.planes[1].normal = { m.m[0][3] - m.m[0][0], m.m[1][3] - m.m[1][0], m.m[2][3] - m.m[2][0] };
	frustum_.planes[1].distance = m.m[3][3] - m.m[3][0];
	// 下
	frustum_.planes[2].normal = { m.m[0][3] + m.m[0][1], m.m[1][3] + m.m[1][1], m.m[2][3] + m.m[2][1] };
	frustum_.planes[2].distance = m.m[3][3] + m.m[3][1];
	// 上
	frustum_.planes[3].normal = { m.m[0][3] - m.m[0][1], m.m[1][3] - m.m[1][1], m.m[2][3] - m.m[2][1] };
	frustum_.planes[3].distance = m.m[3][3] - m.m[3][1];
	// ニア
	frustum_.planes[4].normal = { m.m[0][2], m.m[1][2], m.m[2][2] };
	frustum_.planes[4].distance = m.m[3][2];
	// ファー
	frustum_.planes[5].normal = { m.m[0][3] - m.m[0][2], m.m[1][3] - m.m[1][2], m.m[2][3] - m.m[2][2] };
	frustum_.planes[5].distance = m.m[3][3] - m.m[3][2];

	// 法線を正規化
	for (int i = 0; i < 6; ++i) {
		float len = Math::Length(frustum_.planes[i].normal);
		if (len > 0.0f) {
			float inv = 1.0f / len;
			frustum_.planes[i].normal = Math::Multiply(frustum_.planes[i].normal, inv);
			frustum_.planes[i].distance *= inv;
		}
	}
}
