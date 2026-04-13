#pragma once

/// <summary>
/// 4x4配列
/// </summary>
struct Matrix4x4 {
    float m[4][4];

    // 加算
    Matrix4x4 operator+(const Matrix4x4& rhs) const {
        Matrix4x4 result{};
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                result.m[y][x] = m[y][x] + rhs.m[y][x];
            }
        }
        return result;
    }

    // 減算
    Matrix4x4 operator-(const Matrix4x4& rhs) const {
        Matrix4x4 result{};
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                result.m[y][x] = m[y][x] - rhs.m[y][x];
            }
        }
        return result;
    }

    // 行列積
    Matrix4x4 operator*(const Matrix4x4& rhs) const {
        Matrix4x4 result = {};
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                result.m[row][col] = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    result.m[row][col] += m[row][k] * rhs.m[k][col];
                }
            }
        }
        return result;
    }

    // 複合代入演算子
    Matrix4x4& operator+=(const Matrix4x4& rhs) {
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                m[y][x] += rhs.m[y][x];
            }
        }
        return *this;
    }

    Matrix4x4& operator-=(const Matrix4x4& rhs) {
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                m[y][x] -= rhs.m[y][x];
            }
        }
        return *this;
    }

    Matrix4x4& operator*=(const Matrix4x4& rhs) {
        *this = *this * rhs;
        return *this;
    }

    // スカラー乗算
    Matrix4x4 operator*(float scalar) const {
        Matrix4x4 result{};
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                result.m[y][x] = m[y][x] * scalar;
            }
        }
        return result;
    }

    // スカラー除算
    Matrix4x4 operator/(float scalar) const {
        Matrix4x4 result{};
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                result.m[y][x] = m[y][x] / scalar;
            }
        }
        return result;
    }

    Matrix4x4& operator*=(float scalar) {
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                m[y][x] *= scalar;
            }
        }
        return *this;
    }

    Matrix4x4& operator/=(float scalar) {
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                m[y][x] /= scalar;
            }
        }
        return *this;
    }

    // 比較演算子
    bool operator==(const Matrix4x4& rhs) const {
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                if (m[y][x] != rhs.m[y][x]) {
                    return false;
                }
            }
        }
        return true;
    }

    bool operator!=(const Matrix4x4& rhs) const {
        return !(*this == rhs);
    }
};
