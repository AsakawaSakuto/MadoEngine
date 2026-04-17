#pragma once

#include "../Vector2.h"
#include "../Vector3.h"
#include "../Vector4.h"

namespace Math {

	// クロス積
	// 2Dはスカラー（Z成分）を返す
	float   Cross(const Vector2& v1, const Vector2& v2);
	Vector3 Cross(const Vector3& v1, const Vector3& v2);

	// 加算
	Vector2 Add(const Vector2& v1, const Vector2& v2);
	Vector3 Add(const Vector3& v1, const Vector3& v2);
	Vector4 Add(const Vector4& v1, const Vector4& v2);

	// 減算
	Vector2 Subtract(const Vector2& v1, const Vector2& v2);
	Vector3 Subtract(const Vector3& v1, const Vector3& v2);
	Vector4 Subtract(const Vector4& v1, const Vector4& v2);

	// 乗算（要素ごと）
	Vector2 Multiply(const Vector2& v1, const Vector2& v2);
	Vector3 Multiply(const Vector3& v1, const Vector3& v2);
	Vector4 Multiply(const Vector4& v1, const Vector4& v2);

	// スカラー倍
	Vector2 Multiply(const Vector2& v, float scalar);
	Vector3 Multiply(const Vector3& v, float scalar);
	Vector4 Multiply(const Vector4& v, float scalar);

	// 内積
	float Dot(const Vector2& v1, const Vector2& v2);
	float Dot(const Vector3& v1, const Vector3& v2);
	float Dot(const Vector4& v1, const Vector4& v2);

	// 長さ
	float Length(const Vector2& v);
	float Length(const Vector3& v);
	float Length(const Vector4& v);

	// 正規化
	Vector2 Normalize(const Vector2& v);
	Vector3 Normalize(const Vector3& v);
	Vector4 Normalize(const Vector4& v);

	// 正射影ベクトル
	Vector2 Project(const Vector2& v1, const Vector2& v2);
	Vector3 Project(const Vector3& v1, const Vector3& v2);
	Vector4 Project(const Vector4& v1, const Vector4& v2);

	// 線形補間
	Vector2 Lerp(const Vector2& v1, const Vector2& v2, float t);
	Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t);
	Vector4 Lerp(const Vector4& v1, const Vector4& v2, float t);

	// 球面線形補間
	Vector2 Slerp(const Vector2& v1, const Vector2& v2, float t);
	Vector3 Slerp(const Vector3& v1, const Vector3& v2, float t);
	Vector4 Slerp(const Vector4& v1, const Vector4& v2, float t);

}