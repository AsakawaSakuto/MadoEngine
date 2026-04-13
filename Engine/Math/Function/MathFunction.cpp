#include "MathFunction.h"
#include <cmath>
#include <algorithm>

namespace Math {

    // クロス積
    float Cross(const Vector2& v1, const Vector2& v2) {
        return v1.x * v2.y - v1.y * v2.x;
    }

    // クロス積
    Vector3 Cross(const Vector3& v1, const Vector3& v2) {
        return Vector3(
            v1.y * v2.z - v1.z * v2.y,
            v1.z * v2.x - v1.x * v2.z,
            v1.x * v2.y - v1.y * v2.x
        );
    }

    // 加算
    Vector2 Add(const Vector2& v1, const Vector2& v2) {
        return Vector2{ v1.x + v2.x, v1.y + v2.y };
    }
    Vector3 Add(const Vector3& v1, const Vector3& v2) {
        return Vector3{ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
    }
    Vector4 Add(const Vector4& v1, const Vector4& v2) {
        return Vector4{ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w };
    }

    // 減算
    Vector2 Subtract(const Vector2& v1, const Vector2& v2) {
        return Vector2{ v1.x - v2.x, v1.y - v2.y };
    }
    Vector3 Subtract(const Vector3& v1, const Vector3& v2) {
        return Vector3{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
    }
    Vector4 Subtract(const Vector4& v1, const Vector4& v2) {
        return Vector4{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w };
    }

    // 乗算
    Vector2 Multiply(const Vector2& v1, const Vector2& v2) {
        return Vector2{ v1.x * v2.x, v1.y * v2.y };
    }
    Vector3 Multiply(const Vector3& v1, const Vector3& v2) {
        return Vector3{ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z };
    }
    Vector4 Multiply(const Vector4& v1, const Vector4& v2) {
        return Vector4{ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w };
    }

    // スカラー倍
    Vector2 Multiply(const Vector2& v, float scalar) {
        return Vector2{ scalar * v.x, scalar * v.y };
    }
    Vector3 Multiply(const Vector3& v, float scalar) {
        return Vector3{ scalar * v.x, scalar * v.y, scalar * v.z };
    }
    Vector4 Multiply(const Vector4& v, float scalar) {
        return Vector4{ scalar * v.x, scalar * v.y, scalar * v.z, scalar * v.w };
    }

    // 内積
    float Dot(const Vector2& v1, const Vector2& v2) {
        return v1.x * v2.x + v1.y * v2.y;
    }
    float Dot(const Vector3& v1, const Vector3& v2) {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }
    float Dot(const Vector4& v1, const Vector4& v2) {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
    }

    // 長さ
    float Length(const Vector2& v) {
        return std::sqrt(v.x * v.x + v.y * v.y);
    }
    float Length(const Vector3& v) {
        return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }
    float Length(const Vector4& v) {
        return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    }

    // 正規化
    Vector2 Normalize(const Vector2& v) {
        const float len = Length(v);
        if (len == 0.0f) { return Vector2{ 0.0f, 0.0f }; }
        return Vector2{ v.x / len, v.y / len };
    }
    Vector3 Normalize(const Vector3& v) {
        const float len = Length(v);
        if (len == 0.0f) { return Vector3{ 0.0f, 0.0f, 0.0f }; }
        return Vector3{ v.x / len, v.y / len, v.z / len };
    }
    Vector4 Normalize(const Vector4& v) {
        const float len = Length(v);
        if (len == 0.0f) { return Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }; }
        return Vector4{ v.x / len, v.y / len, v.z / len, v.w / len };
    }

    // 正射影ベクトル
    Vector2 Project(const Vector2& v1, const Vector2& v2) {
        const float v2SqLength = Dot(v2, v2);
        if (v2SqLength == 0.0f) { return Vector2{ 0.0f, 0.0f }; }
        const float dot = Dot(v1, v2);
        return Multiply(v2, dot / v2SqLength);
    }
    Vector3 Project(const Vector3& v1, const Vector3& v2) {
        const float v2SqLength = Dot(v2, v2);
        if (v2SqLength == 0.0f) { return Vector3{ 0.0f, 0.0f, 0.0f }; }
        const float dot = Dot(v1, v2);
        return Multiply(v2, dot / v2SqLength);
    }
    Vector4 Project(const Vector4& v1, const Vector4& v2) {
        const float v2SqLength = Dot(v2, v2);
        if (v2SqLength == 0.0f) { return Vector4{ 0.0f, 0.0f, 0.0f, 0.0f }; }
        const float dot = Dot(v1, v2);
        return Multiply(v2, dot / v2SqLength);
    }

}