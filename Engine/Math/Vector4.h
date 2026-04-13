#pragma once
#include <cstddef>
#include <cmath>

/// <summary>
/// 4次元ベクトル
/// </summary>
struct Vector4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    // --- constructors (コンストラクタ) ---
    constexpr Vector4() = default;
    constexpr Vector4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    // --- index access (インデックスアクセス) ---
    constexpr float& operator[](std::size_t i) noexcept {
        return (i == 0) ? x : (i == 1 ? y : (i == 2 ? z : w));
    }

    constexpr const float& operator[](std::size_t i) const noexcept {
        return (i == 0) ? x : (i == 1 ? y : (i == 2 ? z : w));
    }

    // --- unary (単項演算子) ---
    constexpr Vector4 operator+() const noexcept { return *this; }
    constexpr Vector4 operator-() const noexcept { return { -x, -y, -z, -w }; }

    // --- vector op (ベクトル演算) ---
    constexpr Vector4 operator+(const Vector4& r) const noexcept { return { x + r.x, y + r.y, z + r.z, w + r.w }; }
    constexpr Vector4 operator-(const Vector4& r) const noexcept { return { x - r.x, y - r.y, z - r.z, w - r.w }; }
    constexpr Vector4 operator*(const Vector4& r) const noexcept { return { x * r.x, y * r.y, z * r.z, w * r.w }; }
    constexpr Vector4 operator/(const Vector4& r) const noexcept { return { x / r.x, y / r.y, z / r.z, w / r.w }; }

    constexpr Vector4& operator+=(const Vector4& r) noexcept { x += r.x; y += r.y; z += r.z; w += r.w; return *this; }
    constexpr Vector4& operator-=(const Vector4& r) noexcept { x -= r.x; y -= r.y; z -= r.z; w -= r.w; return *this; }
    constexpr Vector4& operator*=(const Vector4& r) noexcept { x *= r.x; y *= r.y; z *= r.z; w *= r.w; return *this; }
    constexpr Vector4& operator/=(const Vector4& r) noexcept { x /= r.x; y /= r.y; z /= r.z; w /= r.w; return *this; }

    // --- scalar op (スカラー演算) ---
    constexpr Vector4 operator*(float s) const noexcept { return { x * s, y * s, z * s, w * s }; }
    constexpr Vector4 operator/(float s) const noexcept { return { x / s, y / s, z / s, w / s }; }
    constexpr Vector4& operator*=(float s) noexcept { x *= s; y *= s; z *= s; w *= s; return *this; }
    constexpr Vector4& operator/=(float s) noexcept { x /= s; y /= s; z /= s; w /= s; return *this; }

    friend constexpr Vector4 operator*(float s, const Vector4& v) noexcept { return { v.x * s, v.y * s, v.z * s, v.w * s }; }

    // --- compare (比較演算子) ---
    constexpr bool operator==(const Vector4& r) const noexcept { return x == r.x && y == r.y && z == r.z && w == r.w; }

    // --- utilities (ユーティリティ) ---
    /// @brief ベクトルの長さの2乗を返す
    constexpr float LengthSq() const noexcept { return x * x + y * y + z * z + w * w; }
    /// @brief ベクトルの長さを返す
    float Length() const noexcept { return std::sqrt(LengthSq()); }

    /// @brief 長さ1の単位ベクトルを返す
    Vector4 Normalized() const noexcept {
        const float len = Length();
        if (len == 0.0f) { return { 0.0f, 0.0f, 0.0f, 0.0f }; }
        return { x / len, y / len, z / len, w / len };
    }
    /// @brief このベクトルを正規化する (長さ1にする)
    void Normalize() noexcept {
        const float len = Length();
        if (len == 0.0f) { return; }
        x /= len; y /= len; z /= len; w /= len;
    }

    /// @brief 2つのベクトルの内積を返す
    /// @param a ベクトル
    /// @param b ベクトル
    /// @return ベクトルaとbの内積
    static constexpr float Dot(const Vector4& a, const Vector4& b) noexcept {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    /// @brief 3次元成分(x,y,z)での外積を返す (w 成分は 0 に設定される)
    /// @param a ベクトル
    /// @param b ベクトル
    /// @return ベクトルaとbの外積 (w = 0)
    static constexpr Vector4 Cross(const Vector4& a, const Vector4& b) noexcept {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x,
            0.0f
        };
    }
};