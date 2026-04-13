#pragma once
#include <cstddef>
#include <cmath>

/// <summary>
/// 3次元ベクトル
/// </summary>
struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    // --- constructors (コンストラクタ) ---
    constexpr Vector3() = default;
    constexpr Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    // --- index access (インデックスアクセス) ---
    constexpr float& operator[](std::size_t i) noexcept {
        return (i == 0) ? x : (i == 1 ? y : z);
    }

    constexpr const float& operator[](std::size_t i) const noexcept {
        return (i == 0) ? x : (i == 1 ? y : z);
    }

    // --- unary (単項演算子) ---
    constexpr Vector3 operator+() const noexcept { return *this; }
    constexpr Vector3 operator-() const noexcept { return Vector3{ -x, -y, -z }; }

    // --- vector op (ベクトル演算) ---
    constexpr Vector3 operator+(const Vector3& r) const noexcept { return { x + r.x, y + r.y, z + r.z }; }
    constexpr Vector3 operator-(const Vector3& r) const noexcept { return { x - r.x, y - r.y, z - r.z }; }
    constexpr Vector3 operator*(const Vector3& r) const noexcept { return { x * r.x, y * r.y, z * r.z }; }
    constexpr Vector3 operator/(const Vector3& r) const noexcept { return { x / r.x, y / r.y, z / r.z }; }

    constexpr Vector3& operator+=(const Vector3& r) noexcept { x += r.x; y += r.y; z += r.z; return *this; }
    constexpr Vector3& operator-=(const Vector3& r) noexcept { x -= r.x; y -= r.y; z -= r.z; return *this; }
    constexpr Vector3& operator*=(const Vector3& r) noexcept { x *= r.x; y *= r.y; z *= r.z; return *this; }
    constexpr Vector3& operator/=(const Vector3& r) noexcept { x /= r.x; y /= r.y; z /= r.z; return *this; }

    // --- scalar op (スカラー演算) ---
    constexpr Vector3 operator*(float s) const noexcept { return { x * s, y * s, z * s }; }
    constexpr Vector3 operator/(float s) const noexcept { return { x / s, y / s, z / s }; }
    constexpr Vector3& operator*=(float s) noexcept { x *= s; y *= s; z *= s; return *this; }
    constexpr Vector3& operator/=(float s) noexcept { x /= s; y /= s; z /= s; return *this; }

    friend constexpr Vector3 operator*(float s, const Vector3& v) noexcept { return { v.x * s, v.y * s, v.z * s }; }

    // --- compare (比較演算子) ---
    constexpr bool operator==(const Vector3& r) const noexcept { return x == r.x && y == r.y && z == r.z; }

    // --- utilities (ユーティリティ) ---

    /// @brief ベクトルの長さの2乗を返す
    constexpr float LengthSq() const noexcept { return x * x + y * y + z * z; }

    /// @brief ベクトルの長さを返す
    float Length() const noexcept { return std::sqrt(LengthSq()); }

    /// @brief 長さ1の単位ベクトルを返す
    Vector3 Normalized() const noexcept {
        const float len = Length();
        if (len == 0.0f) { return { 0.0f, 0.0f, 0.0f }; }
        return { x / len, y / len, z / len };
    }
    /// @brief このベクトルを正規化する (長さ1にする)
    void Normalize() noexcept {
        const float len = Length();
        if (len == 0.0f) { return; }
        x /= len; y /= len; z /= len;
    }

    /// @brief 2つのベクトルの内積を返す
    /// @param a ベクトル
    /// @param b ベクトル
    /// @return ベクトルaとbの内積
    static constexpr float Dot(const Vector3& a, const Vector3& b) noexcept {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    /// @brief 2つのベクトルの外積を返す
    /// @param a ベクトル
    /// @param b ベクトル
    /// @return ベクトルaとbの外積
    static constexpr Vector3 Cross(const Vector3& a, const Vector3& b) noexcept {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
};