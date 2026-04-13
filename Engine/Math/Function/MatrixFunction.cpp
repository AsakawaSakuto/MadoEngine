#include "MatrixFunction.h"
#include <algorithm>

constexpr float PI = 3.14159265358979323846f;

namespace Matrix {

	// 単位行列
	Matrix4x4 MakeIdentity() {
		Matrix4x4 result{};

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				if (i == j) {
					result.m[j][i] = 1.0f;
				} else {
					result.m[j][i] = 0.0f;
				}
			}
		}
		return result;
	}

	// 行列の加算
	Matrix4x4 Add(const Matrix4x4& a, const Matrix4x4& b) {
		Matrix4x4 result{};
		for (int y = 0; y < 4; ++y) {
			for (int x = 0; x < 4; ++x) {
				result.m[y][x] = a.m[y][x] + b.m[y][x];
			}
		}
		return result;
	}

	// 行列の減算
	Matrix4x4 Subtract(const Matrix4x4& a, const Matrix4x4& b) {
		Matrix4x4 result{};
		for (int y = 0; y < 4; ++y) {
			for (int x = 0; x < 4; ++x) {
				result.m[y][x] = a.m[y][x] - b.m[y][x];
			}
		}
		return result;
	}

	// 行列の積
	Matrix4x4 Multiply(const Matrix4x4& v1, const Matrix4x4& v2) {
		Matrix4x4 result{};

		for (int x = 0; x < 4; x++) {
			for (int y = 0; y < 4; y++) {
				for (int z = 0; z < 4; z++) {
					result.m[y][x] += v1.m[y][z] * v2.m[z][x];
				}
			}
		}

		return result;
	};

	// 移動行列
	Matrix4x4 MakeTranslate(const  Vector3& translate) {
		Matrix4x4 result{};

		result.m[0][0] = 1.f;
		result.m[0][1] = 0.f;
		result.m[0][2] = 0.f;
		result.m[0][3] = 0.f;

		result.m[1][0] = 0.f;
		result.m[1][1] = 1.f;
		result.m[1][2] = 0.f;
		result.m[1][3] = 0.f;

		result.m[2][0] = 0.f;
		result.m[2][1] = 0.f;
		result.m[2][2] = 1.f;
		result.m[2][3] = 0.f;

		result.m[3][0] = translate.x;
		result.m[3][1] = translate.y;
		result.m[3][2] = translate.z;
		result.m[3][3] = 1.f;

		return result;
	}

	// 拡大縮小行列
	Matrix4x4 MakeScale(const  Vector3& scale) {
		Matrix4x4 result{};

		result.m[0][0] = scale.x;
		result.m[0][1] = 0.f;
		result.m[0][2] = 0.f;
		result.m[0][3] = 0.f;

		result.m[1][0] = 0.f;
		result.m[1][1] = scale.y;
		result.m[1][2] = 0.f;
		result.m[1][3] = 0.f;

		result.m[2][0] = 0.f;
		result.m[2][1] = 0.f;
		result.m[2][2] = scale.z;
		result.m[2][3] = 0.f;

		result.m[3][0] = 0.f;
		result.m[3][1] = 0.f;
		result.m[3][2] = 0.f;
		result.m[3][3] = 1.f;

		return result;
	}

	// 回転行列X
	Matrix4x4 MakeRotateX(float rotate) {
		Matrix4x4 result{};

		result.m[0][0] = 1.f;
		result.m[0][1] = 0.f;
		result.m[0][2] = 0.f;
		result.m[0][3] = 0.f;
		result.m[1][0] = 0.f;
		result.m[1][1] = std::cos(rotate);
		result.m[1][2] = std::sin(rotate);
		result.m[1][3] = 0.f;
		result.m[2][0] = 0.f;
		result.m[2][1] = -std::sin(rotate);
		result.m[2][2] = std::cos(rotate);
		result.m[2][3] = 0.f;
		result.m[3][0] = 0.f;
		result.m[3][1] = 0.f;
		result.m[3][2] = 0.f;
		result.m[3][3] = 1.f;

		return result;
	}
	// 回転行列Y
	Matrix4x4 MakeRotateY(float rotate) {
		Matrix4x4 result{};

		result.m[0][0] = std::cos(rotate);
		result.m[0][1] = 0.f;
		result.m[0][2] = -std::sin(rotate);;
		result.m[0][3] = 0.f;
		result.m[1][0] = 0.f;
		result.m[1][1] = 1.f;
		result.m[1][2] = 0.f;
		result.m[1][3] = 0.f;
		result.m[2][0] = std::sin(rotate);
		result.m[2][1] = 0.f;
		result.m[2][2] = std::cos(rotate);
		result.m[2][3] = 0.f;
		result.m[3][0] = 0.f;
		result.m[3][1] = 0.f;
		result.m[3][2] = 0.f;
		result.m[3][3] = 1.f;

		return result;
	}
	// 回転行列Z
	Matrix4x4 MakeRotateZ(float rotate) {
		Matrix4x4 result{};

		result.m[0][0] = std::cos(rotate);
		result.m[0][1] = std::sin(rotate);
		result.m[0][2] = 0.f;
		result.m[0][3] = 0.f;
		result.m[1][0] = -std::sin(rotate);
		result.m[1][1] = std::cos(rotate);
		result.m[1][2] = 0.f;
		result.m[1][3] = 0.f;
		result.m[2][0] = 0.f;
		result.m[2][1] = 0.f;
		result.m[2][2] = 1.f;
		result.m[2][3] = 0.f;
		result.m[3][0] = 0.f;
		result.m[3][1] = 0.f;
		result.m[3][2] = 0.f;
		result.m[3][3] = 1.f;

		return result;
	}

	// 回転行列XYZ
	Matrix4x4 MakeRotateXYZ(const Vector3& rotate) {
		// 各軸の回転行列を生成
		Matrix4x4 rotX = MakeRotateX(rotate.x);
		Matrix4x4 rotY = MakeRotateY(rotate.y);
		Matrix4x4 rotZ = MakeRotateZ(rotate.z);
		// ローカル空間の回転順 Z → Y → X（右から適用される）
		return Multiply(Multiply(rotZ, rotY), rotX);
	}

	// アフィン変換
	Matrix4x4 MakeAffine(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
		Matrix4x4 result{};

		Matrix4x4 rotateXMatrix = MakeRotateX(rotate.x);
		Matrix4x4 rotateYMatrix = MakeRotateY(rotate.y);
		Matrix4x4 rotateZMatrix = MakeRotateZ(rotate.z);
		Matrix4x4 rotateXYZMatrix = Multiply(rotateXMatrix, Multiply(rotateYMatrix, rotateZMatrix));

		result.m[0][0] = scale.x * rotateXYZMatrix.m[0][0];
		result.m[0][1] = scale.x * rotateXYZMatrix.m[0][1];
		result.m[0][2] = scale.x * rotateXYZMatrix.m[0][2];
		result.m[0][3] = 0.f;
		result.m[1][0] = scale.y * rotateXYZMatrix.m[1][0];
		result.m[1][1] = scale.y * rotateXYZMatrix.m[1][1];
		result.m[1][2] = scale.y * rotateXYZMatrix.m[1][2];
		result.m[1][3] = 0.f;
		result.m[2][0] = scale.z * rotateXYZMatrix.m[2][0];
		result.m[2][1] = scale.z * rotateXYZMatrix.m[2][1];
		result.m[2][2] = scale.z * rotateXYZMatrix.m[2][2];
		result.m[2][3] = 0.f;
		result.m[3][0] = translate.x;
		result.m[3][1] = translate.y;
		result.m[3][2] = translate.z;
		result.m[3][3] = 1.f;

		return result;
	};

	// 逆行列
	Matrix4x4 Inverse(const Matrix4x4& m)
	{
		Matrix4x4 result{};

		float abs;//絶対値はint型にする

		// |A|
		abs = (m.m[0][0] * m.m[1][1] * m.m[2][2] * m.m[3][3]) + (m.m[0][0] * m.m[1][2] * m.m[2][3] * m.m[3][1]) + (m.m[0][0] * m.m[1][3] * m.m[2][1] * m.m[3][2])
			- (m.m[0][0] * m.m[1][3] * m.m[2][2] * m.m[3][1]) - (m.m[0][0] * m.m[1][2] * m.m[2][1] * m.m[3][3]) - (m.m[0][0] * m.m[1][1] * m.m[2][3] * m.m[3][2])
			- (m.m[0][1] * m.m[1][0] * m.m[2][2] * m.m[3][3]) - (m.m[0][2] * m.m[1][0] * m.m[2][3] * m.m[3][1]) - (m.m[0][3] * m.m[1][0] * m.m[2][1] * m.m[3][2])
			+ (m.m[0][3] * m.m[1][0] * m.m[2][2] * m.m[3][1]) + (m.m[0][2] * m.m[1][0] * m.m[2][1] * m.m[3][3]) + (m.m[0][1] * m.m[1][0] * m.m[2][3] * m.m[3][2])
			+ (m.m[0][1] * m.m[1][2] * m.m[2][0] * m.m[3][3]) + (m.m[0][2] * m.m[1][3] * m.m[2][0] * m.m[3][1]) + (m.m[0][3] * m.m[1][1] * m.m[2][0] * m.m[3][2])
			- (m.m[0][3] * m.m[1][2] * m.m[2][0] * m.m[3][1]) - (m.m[0][2] * m.m[1][1] * m.m[2][0] * m.m[3][3]) - (m.m[0][1] * m.m[1][3] * m.m[2][0] * m.m[3][2])
			- (m.m[0][1] * m.m[1][2] * m.m[2][3] * m.m[3][0]) - (m.m[0][2] * m.m[1][3] * m.m[2][1] * m.m[3][0]) - (m.m[0][3] * m.m[1][1] * m.m[2][2] * m.m[3][0])
			+ (m.m[0][3] * m.m[1][2] * m.m[2][1] * m.m[3][0]) + (m.m[0][2] * m.m[1][1] * m.m[2][3] * m.m[3][0]) + (m.m[0][1] * m.m[1][3] * m.m[2][2] * m.m[3][0]
				);

		// 1/A
		result.m[0][0] = 1.0f / abs * (
			(m.m[1][1] * m.m[2][2] * m.m[3][3]) + (m.m[1][2] * m.m[2][3] * m.m[3][1]) + (m.m[1][3] * m.m[2][1] * m.m[3][2])
			- (m.m[1][3] * m.m[2][2] * m.m[3][1]) - (m.m[1][2] * m.m[2][1] * m.m[3][3]) - (m.m[1][1] * m.m[2][3] * m.m[3][2])
			);
		result.m[0][1] = 1.0f / abs * (
			-(m.m[0][1] * m.m[2][2] * m.m[3][3]) - (m.m[0][2] * m.m[2][3] * m.m[3][1]) - (m.m[0][3] * m.m[2][1] * m.m[3][2])
			+ m.m[0][3] * m.m[2][2] * m.m[3][1] + m.m[0][2] * m.m[2][1] * m.m[3][3] + m.m[0][1] * m.m[2][3] * m.m[3][2]
			);
		result.m[0][2] = 1.0f / abs * (
			(m.m[0][1] * m.m[1][2] * m.m[3][3]) + (m.m[0][2] * m.m[1][3] * m.m[3][1]) + (m.m[0][3] * m.m[1][1] * m.m[3][2])
			- (m.m[0][3] * m.m[1][2] * m.m[3][1]) - (m.m[0][2] * m.m[1][1] * m.m[3][3]) - (m.m[0][1] * m.m[1][3] * m.m[3][2])
			);
		result.m[0][3] = 1.0f / abs * (
			-(m.m[0][1] * m.m[1][2] * m.m[2][3]) - (m.m[0][2] * m.m[1][3] * m.m[2][1]) - (m.m[0][3] * m.m[1][1] * m.m[2][2])
			+ (m.m[0][3] * m.m[1][2] * m.m[2][1]) + (m.m[0][2] * m.m[1][1] * m.m[2][3]) + (m.m[0][1] * m.m[1][3] * m.m[2][2])
			);

		result.m[1][0] = 1.0f / abs * (
			-(m.m[1][0] * m.m[2][2] * m.m[3][3]) - (m.m[1][2] * m.m[2][3] * m.m[3][0]) - (m.m[1][3] * m.m[2][0] * m.m[3][2])
			+ (m.m[1][3] * m.m[2][2] * m.m[3][0]) + (m.m[1][2] * m.m[2][0] * m.m[3][3]) + (m.m[1][0] * m.m[2][3] * m.m[3][2])
			);
		result.m[1][1] = 1.0f / abs * (
			(m.m[0][0] * m.m[2][2] * m.m[3][3]) + (m.m[0][2] * m.m[2][3] * m.m[3][0]) + (m.m[0][3] * m.m[2][0] * m.m[3][2])
			- (m.m[0][3] * m.m[2][2] * m.m[3][0]) - (m.m[0][2] * m.m[2][0] * m.m[3][3]) - (m.m[0][0] * m.m[2][3] * m.m[3][2])
			);
		result.m[1][2] = 1.0f / abs * (
			-(m.m[0][0] * m.m[1][2] * m.m[3][3]) - (m.m[0][2] * m.m[1][3] * m.m[3][0]) - (m.m[0][3] * m.m[1][0] * m.m[3][2])
			+ (m.m[0][3] * m.m[1][2] * m.m[3][0]) + (m.m[0][2] * m.m[1][0] * m.m[3][3]) + (m.m[0][0] * m.m[1][3] * m.m[3][2])
			);
		result.m[1][3] = 1.0f / abs * (
			(m.m[0][0] * m.m[1][2] * m.m[2][3]) + (m.m[0][2] * m.m[1][3] * m.m[2][0]) + (m.m[0][3] * m.m[1][0] * m.m[2][2])
			- (m.m[0][3] * m.m[1][2] * m.m[2][0]) - (m.m[0][2] * m.m[1][0] * m.m[2][3]) - (m.m[0][0] * m.m[1][3] * m.m[2][2])
			);

		result.m[2][0] = 1.0f / abs * (
			(m.m[1][0] * m.m[2][1] * m.m[3][3]) + (m.m[1][1] * m.m[2][3] * m.m[3][0]) + (m.m[1][3] * m.m[2][0] * m.m[3][1])
			- (m.m[1][3] * m.m[2][1] * m.m[3][0]) - (m.m[1][1] * m.m[2][0] * m.m[3][3]) - (m.m[1][0] * m.m[2][3] * m.m[3][1])
			);
		result.m[2][1] = 1.0f / abs * (
			-(m.m[0][0] * m.m[2][1] * m.m[3][3]) - (m.m[0][1] * m.m[2][3] * m.m[3][0]) - (m.m[0][3] * m.m[2][0] * m.m[3][1])
			+ (m.m[0][3] * m.m[2][1] * m.m[3][0]) + (m.m[0][1] * m.m[2][0] * m.m[3][3]) + (m.m[0][0] * m.m[2][3] * m.m[3][1])
			);
		result.m[2][2] = 1.0f / abs * (
			(m.m[0][0] * m.m[1][1] * m.m[3][3]) + (m.m[0][1] * m.m[1][3] * m.m[3][0]) + (m.m[0][3] * m.m[1][0] * m.m[3][1])
			- (m.m[0][3] * m.m[1][1] * m.m[3][0]) - (m.m[0][1] * m.m[1][0] * m.m[3][3]) - (m.m[0][0] * m.m[1][3] * m.m[3][1])
			);
		result.m[2][3] = 1.0f / abs * (
			-(m.m[0][0] * m.m[1][1] * m.m[2][3]) - (m.m[0][1] * m.m[1][3] * m.m[2][0]) - (m.m[0][3] * m.m[1][0] * m.m[2][1])
			+ (m.m[0][3] * m.m[1][1] * m.m[2][0]) + (m.m[0][1] * m.m[1][0] * m.m[2][3]) + (m.m[0][0] * m.m[1][3] * m.m[2][1])
			);

		result.m[3][0] = 1.0f / abs * (
			-(m.m[1][0] * m.m[2][1] * m.m[3][2]) - (m.m[1][1] * m.m[2][2] * m.m[3][0]) - (m.m[1][2] * m.m[2][0] * m.m[3][1])
			+ (m.m[1][2] * m.m[2][1] * m.m[3][0]) + (m.m[1][1] * m.m[2][0] * m.m[3][2]) + (m.m[1][0] * m.m[2][2] * m.m[3][1])
			);
		result.m[3][1] = 1.0f / abs * (
			(m.m[0][0] * m.m[2][1] * m.m[3][2]) + (m.m[0][1] * m.m[2][2] * m.m[3][0]) + (m.m[0][2] * m.m[2][0] * m.m[3][1])
			- (m.m[0][2] * m.m[2][1] * m.m[3][0]) - (m.m[0][1] * m.m[2][0] * m.m[3][2]) - (m.m[0][0] * m.m[2][2] * m.m[3][1])
			);
		result.m[3][2] = 1.0f / abs * (
			-(m.m[0][0] * m.m[1][1] * m.m[3][2]) - (m.m[0][1] * m.m[1][2] * m.m[3][0]) - (m.m[0][2] * m.m[1][0] * m.m[3][1])
			+ (m.m[0][2] * m.m[1][1] * m.m[3][0]) + (m.m[0][1] * m.m[1][0] * m.m[3][2]) + (m.m[0][0] * m.m[1][2] * m.m[3][1])
			);
		result.m[3][3] = 1.0f / abs * (
			(m.m[0][0] * m.m[1][1] * m.m[2][2]) + (m.m[0][1] * m.m[1][2] * m.m[2][0]) + (m.m[0][2] * m.m[1][0] * m.m[2][1])
			- (m.m[0][2] * m.m[1][1] * m.m[2][0]) - (m.m[0][1] * m.m[1][0] * m.m[2][2]) - (m.m[0][0] * m.m[1][2] * m.m[2][1])
			);


		return result;
	}

	// 透視投影行列
	Matrix4x4 MakePerspectiveFov(float fovY, float aspectRatio, float nearClip, float farClip)
	{
		Matrix4x4 result{};

		result.m[0][0] = (1.0f / aspectRatio) * 1.0f / std::tan(fovY / 2.0f);
		result.m[1][1] = 1.0f / std::tan(fovY / 2.0f);
		result.m[2][2] = farClip / (farClip - nearClip);
		result.m[2][3] = 1.0f;
		result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);

		return result;
	}

	// 平行投影行列
	Matrix4x4 MakeOrthographic(float left, float top, float right, float bottom, float nearClip, float farClip) {
		Matrix4x4 result{};
		result.m[0][0] = 2.0f / (right - left);
		result.m[1][1] = 2.0f / (top - bottom);
		result.m[2][2] = 1.0f / (farClip - nearClip);
		result.m[3][0] = (left + right) / (left - right);
		result.m[3][1] = (top + bottom) / (bottom - top);
		result.m[3][2] = nearClip / (nearClip - farClip);
		result.m[3][3] = 1.0f;
		return result;
	}

	Matrix4x4 Transpose(const Matrix4x4& m) {
		Matrix4x4 result;

		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				result.m[row][col] = m.m[col][row];
			}
		}

		return result;
	}

	// ビューポート変換行列
	Matrix4x4 MakeViewport(float left, float top, float width, float height, float minDepth, float maxDepth) {

		Matrix4x4 result;

		result.m[0][0] = width / 2;
		result.m[0][1] = 0.0f;
		result.m[0][2] = 0.0f;
		result.m[0][3] = 0.0f;

		result.m[1][0] = 0.0f;
		result.m[1][1] = -height / 2;
		result.m[1][2] = 0.0f;
		result.m[1][3] = 0.0f;

		result.m[2][0] = 0.0f;
		result.m[2][1] = 0.0f;
		result.m[2][2] = maxDepth - minDepth;
		result.m[2][3] = 0.0f;

		result.m[3][0] = left + (width / 2);
		result.m[3][1] = top + (height / 2);
		result.m[3][2] = minDepth;
		result.m[3][3] = 1.0f;

		return result;
	}

	// 任意軸回転行列
	Matrix4x4 MakeRotateAxisAngle(const Vector3& axis, float angle) {
		// 正規化された軸ベクトルを使用
		Vector3 n = Math::Normalize(axis);

		float x = n.x;
		float y = n.y;
		float z = n.z;
		float c = std::cos(angle);
		float s = std::sin(angle);
		float oneMinusC = 1.0f - c;

		Matrix4x4 result{};

		result.m[0][0] = x * x * oneMinusC + c;
		result.m[0][1] = x * y * oneMinusC + z * s;
		result.m[0][2] = x * z * oneMinusC - y * s;
		result.m[0][3] = 0.0f;

		result.m[1][0] = y * x * oneMinusC - z * s;
		result.m[1][1] = y * y * oneMinusC + c;
		result.m[1][2] = y * z * oneMinusC + x * s;
		result.m[1][3] = 0.0f;

		result.m[2][0] = z * x * oneMinusC + y * s;
		result.m[2][1] = z * y * oneMinusC - x * s;
		result.m[2][2] = z * z * oneMinusC + c;
		result.m[2][3] = 0.0f;

		result.m[3][0] = 0.0f;
		result.m[3][1] = 0.0f;
		result.m[3][2] = 0.0f;
		result.m[3][3] = 1.0f;

		return result;
	}

	// from方向 → to方向 に向ける回転行列
	Matrix4x4 DirectionToDirection(const Vector3& from, const Vector3& to) {
		Vector3 f = Math::Normalize(from);
		Vector3 t = Math::Normalize(to);

		float dot = Math::Dot(f, t);
		dot = std::clamp(dot, -1.0f, 1.0f);

		// 同方向
		if (std::abs(dot - 1.0f) < 1e-6f) {
			Matrix4x4 I{};
			I.m[0][0] = I.m[1][1] = I.m[2][2] = I.m[3][3] = 1.0f;
			return I;
		}

		// 真逆方向（180度回転）
		if (std::abs(dot + 1.0f) < 1e-6f) {
			// from と最も垂直な軸を探す
			Vector3 arbitraryAxis;
			if (std::abs(f.x) < std::abs(f.y) && std::abs(f.x) < std::abs(f.z)) {
				arbitraryAxis = { 1, 0, 0 };
			} else if (std::abs(f.y) < std::abs(f.z)) {
				arbitraryAxis = { 0, 1, 0 };
			} else {
				arbitraryAxis = { 0, 0, 1 };
			}

			Vector3 axis = Math::Normalize(Math::Cross(arbitraryAxis, f)); // 安定した直交軸
			return MakeRotateAxisAngle(axis, static_cast<float>(PI));
		}

		// 通常ケース
		Vector3 axis = Math::Cross(t, f); // 左手系
		float angle = std::acos(dot);
		return MakeRotateAxisAngle(axis, angle);
	}

	Vector3 Transform(const Vector3& v, const Matrix4x4& m) {
		Vector3 result;
		result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
		result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
		result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
		float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];

		// w除算で正規化（透視変換対応）
		if (w != 0.0f) {
			result.x /= w;
			result.y /= w;
			result.z /= w;
		}

		return result;
	}
}