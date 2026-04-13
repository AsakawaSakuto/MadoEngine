#pragma once
#include <cstddef>
#include <cmath>

/// <summary>
/// 2次元ベクトル
/// </summary>
struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;

    // --- constructors (コンストラクタ) ---
    constexpr Vector2() = default;
    constexpr Vector2(float x_, float y_) : x(x_), y(y_) {}

    // --- index access (インデックスアクセス) ---
    constexpr float& operator[](std::size_t i) noexcept { return (i == 0) ? x : y; }
    constexpr const float& operator[](std::size_t i) const noexcept { return (i == 0) ? x : y; }

    // --- unary (単項演算子) ---
    constexpr Vector2 operator+() const noexcept { return *this; }
    constexpr Vector2 operator-() const noexcept { return Vector2{ -x, -y }; }

    // --- vector op (ベクトル演算) ---
    constexpr Vector2 operator+(const Vector2& r) const noexcept { return { x + r.x, y + r.y }; }
    constexpr Vector2 operator-(const Vector2& r) const noexcept { return { x - r.x, y - r.y }; }
    constexpr Vector2 operator*(const Vector2& r) const noexcept { return { x * r.x, y * r.y }; }
    constexpr Vector2 operator/(const Vector2& r) const noexcept { return { x / r.x, y / r.y }; }

    constexpr Vector2& operator+=(const Vector2& r) noexcept { x += r.x; y += r.y; return *this; }
    constexpr Vector2& operator-=(const Vector2& r) noexcept { x -= r.x; y -= r.y; return *this; }
    constexpr Vector2& operator*=(const Vector2& r) noexcept { x *= r.x; y *= r.y; return *this; }
    constexpr Vector2& operator/=(const Vector2& r) noexcept { x /= r.x; y /= r.y; return *this; }

    // --- scalar op (スカラー演算) ---
    constexpr Vector2 operator*(float s) const noexcept { return { x * s, y * s }; }
    constexpr Vector2 operator/(float s) const noexcept { return { x / s, y / s }; }
    constexpr Vector2& operator*=(float s) noexcept { x *= s; y *= s; return *this; }
    constexpr Vector2& operator/=(float s) noexcept { x /= s; y /= s; return *this; }

    friend constexpr Vector2 operator*(float s, const Vector2& v) noexcept { return { v.x * s, v.y * s }; }

    // --- compare (比較演算子) ---
    constexpr bool operator==(const Vector2& r) const noexcept { return x == r.x && y == r.y; }

    // --- utilities (ユーティリティ) ---

	/// @brief ベクトルの長さの2乗を返す
    constexpr float LengthSq() const noexcept { return x * x + y * y; }

	/// @brief ベクトルの長さを返す
    float Length() const noexcept { return std::sqrt(LengthSq()); }

	/// @brief 長さ1の単位ベクトルを返す
    Vector2 Normalized() const noexcept {
        const float len = Length();
        if (len == 0.0f) { return { 0.0f, 0.0f }; }
        return { x / len, y / len };
    }
    
	/// @brief ベクトルaとbの内積を返す
	/// @param a ベクトル
	/// @param b ベクトル
	/// @return ベクトルaとbの内積
    constexpr float Dot(const Vector2& a, const Vector2& b) noexcept { return a.x * b.x + a.y * b.y; }

	/// @brief ベクトルaとbの外積を返す
	/// @param a ベクトル
	/// @param b ベクトル
	/// @return ベクトルaとbの外積
    constexpr float Cross(const Vector2& a, const Vector2& b) noexcept { return a.x * b.y - a.y * b.x; }
};
