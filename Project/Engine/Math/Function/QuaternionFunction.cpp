#include "QuaternionFunction.h"

namespace QuaternionFunc {

    // Quaternionの積
    Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs) {
        Quaternion result{};

        result.w = lhs.w * rhs.w
            - lhs.x * rhs.x
            - lhs.y * rhs.y
            - lhs.z * rhs.z;

        result.x = lhs.w * rhs.x
            + lhs.x * rhs.w
            + lhs.y * rhs.z
            - lhs.z * rhs.y;

        result.y = lhs.w * rhs.y
            - lhs.x * rhs.z
            + lhs.y * rhs.w
            + lhs.z * rhs.x;

        result.z = lhs.w * rhs.z
            + lhs.x * rhs.y
            - lhs.y * rhs.x
            + lhs.z * rhs.w;

        return result;
    }

    // 単位Quaternionを返す
    Quaternion IdentityQuaternion() {
        return { 0.0f, 0.0f, 0.0f, 1.0f };
    }

    // 共役Quaternionを返す
    Quaternion Conjugate(const Quaternion& quaternion) {
        return { -quaternion.x, -quaternion.y, -quaternion.z, quaternion.w };
    }

    // Quaternionのnormを返す
    float Norm(const Quaternion& quaternion) {
        return std::sqrt(
            quaternion.x * quaternion.x +
            quaternion.y * quaternion.y +
            quaternion.z * quaternion.z +
            quaternion.w * quaternion.w);
    }

    // 正規化したQuaternionを返す
    Quaternion Normalize(const Quaternion& quaternion) {
        float n = Norm(quaternion);
        if (n == 0.0f) {
            // 0除算を避ける。ここは課題の方針に合わせて変更してOK
            return { 0.0f, 0.0f, 0.0f, 0.0f };
        }
        float invN = 1.0f / n;
        return {
            quaternion.x * invN,
            quaternion.y * invN,
            quaternion.z * invN,
            quaternion.w * invN
        };
    }

    // 逆Quaternionを返す
    Quaternion Inverse(const Quaternion& quaternion) {

        float n2 =
            quaternion.x * quaternion.x +
            quaternion.y * quaternion.y +
            quaternion.z * quaternion.z +
            quaternion.w * quaternion.w;

        if (n2 == 0.0f) {
            // 逆が定義できないのでゼロを返す
            return { 0.0f, 0.0f, 0.0f, 0.0f };
        }

        Quaternion conj = Conjugate(quaternion);
        float invN2 = 1.0f / n2;

        return {
            conj.x * invN2,
            conj.y * invN2,
            conj.z * invN2,
            conj.w * invN2
        };
    }

    // 任意軸回転を表すQuaternionの生成
    Quaternion MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle) {
        Vector3 n = Math::Normalize(axis);

        float half = angle * 0.5f;
        float s = std::sin(half);
        float c = std::cos(half);

        Quaternion q{};
        q.x = n.x * s;
        q.y = n.y * s;
        q.z = n.z * s;
        q.w = c;
        return q;
    }

    // ベクトルをQuaternionで回転させた結果のベクトルを求める
    Vector3 RotateVector(const Vector3& v, const Quaternion& q) {
        Vector3 qv{ q.x, q.y, q.z };

        Vector3 t = Math::Cross(qv, v);
        t.x *= 2.0f * q.w;
        t.y *= 2.0f * q.w;
        t.z *= 2.0f * q.w;

        Vector3 u = Math::Cross(qv, t);
        u = Math::Cross(qv, u);
        u.x *= 2.0f;
        u.y *= 2.0f;
        u.z *= 2.0f;

        Vector3 result{
            v.x + t.x + u.x,
            v.y + t.y + u.y,
            v.z + t.z + u.z
        };
        return result;
    }

    // Quaternionから回転行列を求める
    Matrix4x4 MakeRotateMatrix(const Quaternion& q) {
        Matrix4x4 m{};

        float xx = q.x * q.x;
        float yy = q.y * q.y;
        float zz = q.z * q.z;
        float xy = q.x * q.y;
        float xz = q.x * q.z;
        float yz = q.y * q.z;
        float wx = q.w * q.x;
        float wy = q.w * q.y;
        float wz = q.w * q.z;

        // 行列は row-major, 回転だけを入れる想定
        m.m[0][0] = 1.0f - 2.0f * (yy + zz);
        m.m[0][1] = 2.0f * (xy + wz);
        m.m[0][2] = 2.0f * (xz - wy);
        m.m[0][3] = 0.0f;

        m.m[1][0] = 2.0f * (xy - wz);
        m.m[1][1] = 1.0f - 2.0f * (xx + zz);
        m.m[1][2] = 2.0f * (yz + wx);
        m.m[1][3] = 0.0f;

        m.m[2][0] = 2.0f * (xz + wy);
        m.m[2][1] = 2.0f * (yz - wx);
        m.m[2][2] = 1.0f - 2.0f * (xx + yy);
        m.m[2][3] = 0.0f;

        m.m[3][0] = 0.0f;
        m.m[3][1] = 0.0f;
        m.m[3][2] = 0.0f;
        m.m[3][3] = 1.0f;

        return m;
    }

    // 球面線形補間 Slerp
    Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t) {
        // q0, q1 は単位クォータニオン想定

        // 内積
        float dot =
            q0.x * q1.x +
            q0.y * q1.y +
            q0.z * q1.z +
            q0.w * q1.w;

        // 反対向きの場合は片方を反転させて最短経路にする
        Quaternion q1Adj = q1;
        if (dot < 0.0f) {
            dot = -dot;
            q1Adj.x = -q1Adj.x;
            q1Adj.y = -q1Adj.y;
            q1Adj.z = -q1Adj.z;
            q1Adj.w = -q1Adj.w;
        }

        // ほぼ同じ向きなら Lerp で近似
        const float epsilon = 1e-5f;
        if (1.0f - dot < epsilon) {
            Quaternion r{};
            r.x = q0.x + (q1Adj.x - q0.x) * t;
            r.y = q0.y + (q1Adj.y - q0.y) * t;
            r.z = q0.z + (q1Adj.z - q0.z) * t;
            r.w = q0.w + (q1Adj.w - q0.w) * t;
            return r; // ここでは正規化まではしない
        }

        // 角
        if (dot > 1.0f) dot = 1.0f; // 誤差対策
        float theta = std::acos(dot);
        float sinTheta = std::sin(theta);

        float scale0 = std::sin((1.0f - t) * theta) / sinTheta;
        float scale1 = std::sin(t * theta * 1.0f) / sinTheta;

        Quaternion result{};
        result.x = scale0 * q0.x + scale1 * q1Adj.x;
        result.y = scale0 * q0.y + scale1 * q1Adj.y;
        result.z = scale0 * q0.z + scale1 * q1Adj.z;
        result.w = scale0 * q0.w + scale1 * q1Adj.w;
        return result;
    }
}