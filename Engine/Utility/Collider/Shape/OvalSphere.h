#pragma once
#include "../../../Math/Vector3.h"
#include "../../../Math/Matrix4x4.h"
#include "../../../Math/Function/MatrixFunction.h"

// 楕円球体
struct OvalSphere {
	Vector3 center = { 0.0f,0.0f,0.0f }; // 中心点
	Vector3 radius = { 1.0f,1.0f,1.0f }; // 半径
	Vector3 rotate = { 0.0f,0.0f,0.0f }; // 回転角
	Vector3 orientation[3];              // 座標軸 正規化 直交必須

	/// <summary>
	/// 回転角から座標軸を更新
	/// </summary>
	void UpdateOrientation() {
		Matrix4x4 rotateMatrix = Matrix::MakeRotateX(rotate.x) * Matrix::MakeRotateY(rotate.y) * Matrix::MakeRotateZ(rotate.z);

		// 回転行列から軸を抽出
		orientation[0].x = rotateMatrix.m[0][0];
		orientation[0].y = rotateMatrix.m[0][1];
		orientation[0].z = rotateMatrix.m[0][2];

		orientation[1].x = rotateMatrix.m[1][0];
		orientation[1].y = rotateMatrix.m[1][1];
		orientation[1].z = rotateMatrix.m[1][2];

		orientation[2].x = rotateMatrix.m[2][0];
		orientation[2].y = rotateMatrix.m[2][1];
		orientation[2].z = rotateMatrix.m[2][2];
	}
};