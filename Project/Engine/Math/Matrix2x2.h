#pragma once

/// <summary>
/// 2x2配列
/// </summary>
struct Matrix2x2 {
    float m[2][2];

    // 加算
    Matrix2x2 operator+(const Matrix2x2& rhs) const {
        Matrix2x2 result{};
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                result.m[y][x] = m[y][x] + rhs.m[y][x];
            }
        }
        return result;
    }

    // 減算
    Matrix2x2 operator-(const Matrix2x2& rhs) const {
        Matrix2x2 result{};
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                result.m[y][x] = m[y][x] - rhs.m[y][x];
            }
        }
        return result;
    }

    // 行列積
    Matrix2x2 operator*(const Matrix2x2& rhs) const {
        Matrix2x2 result{};
        for (int row = 0; row < 2; ++row) {
            for (int col = 0; col < 2; ++col) {
                result.m[row][col] = 0.0f;
                for (int k = 0; k < 2; ++k) {
                    result.m[row][col] += m[row][k] * rhs.m[k][col];
                }
            }
        }
        return result;
    }

    // 複合代入演算子
    Matrix2x2& operator+=(const Matrix2x2& rhs) {
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                m[y][x] += rhs.m[y][x];
            }
        }
        return *this;
    }

    Matrix2x2& operator-=(const Matrix2x2& rhs) {
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                m[y][x] -= rhs.m[y][x];
            }
        }
        return *this;
    }

    Matrix2x2& operator*=(const Matrix2x2& rhs) {
        *this = *this * rhs;
        return *this;
    }

    // スカラー乗算
    Matrix2x2 operator*(float scalar) const {
        Matrix2x2 result{};
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                result.m[y][x] = m[y][x] * scalar;
            }
        }
        return result;
    }

    // スカラー除算
    Matrix2x2 operator/(float scalar) const {
        Matrix2x2 result{};
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                result.m[y][x] = m[y][x] / scalar;
            }
        }
        return result;
    }

    Matrix2x2& operator*=(float scalar) {
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                m[y][x] *= scalar;
            }
        }
        return *this;
    }

    Matrix2x2& operator/=(float scalar) {
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                m[y][x] /= scalar;
            }
        }
        return *this;
    }

    // 比較演算子
    bool operator==(const Matrix2x2& rhs) const {
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                if (m[y][x] != rhs.m[y][x]) {
                    return false;
                }
            }
        }
        return true;
    }

    bool operator!=(const Matrix2x2& rhs) const {
        return !(*this == rhs);
    }
};