#pragma once
#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"
#include "Math/Function/MathFunction.h"
#include <vector>
#include <cmath>

namespace QuaternionFunc {

	/// @brief Quaternion同士の積を返す
	/// @param lhs 左辺のQuaternion
	/// @param rhs 右辺のQuaternion
	/// @return 積の結果のQuaternion
	Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs);
	
	/// @brief 単位 Quaternionを返す
	/// @return 単位 Quaternion
	Quaternion IdentityQuaternion();

	/// @brief 共役 Quaternionを返す
	/// @param quaternion 対象のQuaternion
	/// @return 共役 Quaternion
	Quaternion Conjugate(const Quaternion& quaternion);

	/// @brief Quaternionのノルムを返す
	/// @param quaternion 対象のQuaternion
	/// @return ノルム
	float Norm(const Quaternion& quaternion);

	/// @brief 正規化した Quaternionを返す
	/// @param quaternion 対象のQuaternion
	/// @return 正規化された Quaternion
	Quaternion Normalize(const Quaternion& quaternion);

	/// @brief 逆 Quaternionを返す
	/// @param quaternion 対象のQuaternion
	/// @return 逆 Quaternion
	Quaternion Inverse(const Quaternion& quaternion);

	/// @brief 任意軸回転を表すQuaternionの生成
	/// @param axis 回転軸
	/// @param angle 回転角度（ラジアン）
	/// @return 回転を表すQuaternion
	Quaternion MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle);

	/// @brief ベクトルをQuaternionで回転させた結果のベクトルを求める
	/// @param v 回転させるベクトル
	/// @param q 回転を表すQuaternion
	/// @return 回転後のベクトル
	Vector3 RotateVector(const Vector3& v, const Quaternion& q);

	/// @brief Quaternionから回転行列を求める
	/// @param q 対象のQuaternion
	/// @return 回転行列
	Matrix4x4 MakeRotateMatrix(const Quaternion& q);

	/// @brief 球面線形補間 Slerp
	/// @param q0 開始のQuaternion
	/// @param q1 終了のQuaternion
	/// @param t 補間パラメータ（0.0から1.0）
	/// @return 補間結果のQuaternion
	Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t);
}