#pragma once
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"
#include "MathFunction.h"

namespace Matrix {

	/// @brief 単位行列を生成する
	/// @return 4x4の単位行列（対角成分が1、それ以外が0）
	Matrix4x4 MakeIdentity();

	/// @brief 行列の加算
	/// @param m1 加算する行列1
	/// @param m2 加算する行列2
	/// @return 各要素を対応する要素同士で加算した行列
	Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);

	/// @brief 行列の減算
	/// @param m1 被減算行列
	/// @param m2 減算する行列
	/// @return 各要素を対応する要素同士で減算した行列（m1 - m2）
	Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);

	/// @brief 行列の積
	/// @param m1 乗算する行列1
	/// @param m2 乗算する行列2
	/// @return 行列の乗算結果（m1 * m2）
	Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

	/// @brief 移動行列を生成する
	/// @param translate X, Y, Z 方向の移動量
	/// @return 移動行列
	Matrix4x4 MakeTranslate(const Vector3& translate);

	/// @brief 拡大縮小行列を生成する
	/// @param scale X, Y, Z 方向の拡大率
	/// @return 拡大縮小行列
	Matrix4x4 MakeScale(const Vector3& scale);

	/// @brief X軸周りの回転行列を生成する
	/// @param rotate 回転角度（ラジアン）
	/// @return X軸周りの回転行列
	Matrix4x4 MakeRotateX(float rotate);

	/// @brief Y軸周りの回転行列を生成する
	/// @param rotate 回転角度（ラジアン）
	/// @return Y軸周りの回転行列
	Matrix4x4 MakeRotateY(float rotate);

	/// @brief Z軸周りの回転行列を生成する
	/// @param rotate 回転角度（ラジアン）
	/// @return Z軸周りの回転行列
	Matrix4x4 MakeRotateZ(float rotate);

	/// @brief X, Y, Z軸周りの回転を合成した回転行列を生成する
	/// @param rotate X, Y, Z軸の回転角度（ラジアン）
	/// @return 合成された回転行列
	Matrix4x4 MakeRotateXYZ(const Vector3& rotate);

	/// @brief アフィン変換行列を生成する
	/// @param scale 拡大縮小率
	/// @param rotate 回転角度（ラジアン）
	/// @param translate 移動量
	/// @return アフィン変換行列（拡大縮小 → 回転 → 移動の順に変換を適用）
	Matrix4x4 MakeAffine(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

	/// @brief 逆行列を計算する
	/// @param m 逆行列を求める行列
	/// @return 逆行列
	Matrix4x4 Inverse(const Matrix4x4& m);

	/// @brief 透視投影行列を生成する
	/// @param fovY 垂直視野角（ラジアン）
	/// @param aspectRatio アスペクト比（幅/高さ）
	/// @param nearClip ニアクリップ面までの距離
	/// @param farClip ファークリップ面までの距離
	/// @return 透視投影行列
	Matrix4x4 MakePerspectiveFov(float fovY, float aspectRatio, float nearClip, float farClip);

	/// @brief 平行投影行列を生成する
	/// @param left 投影領域の左端
	/// @param top 投影領域の上端
	/// @param right 投影領域の右端
	/// @param bottom 投影領域の下端
	/// @param nearClip ニアクリップ面までの距離
	/// @param farClip ファークリップ面までの距離
	/// @return 平行投影行列
	Matrix4x4 MakeOrthographic(float left, float top, float right, float bottom, float nearClip, float farClip);

	/// @brief 転置行列を計算する
	/// @param m 転置する行列
	/// @return 行と列を入れ替えた行列
	Matrix4x4 Transpose(const Matrix4x4& m);

	/// @brief ビューポート変換行列を生成する（スクリーン座標への変換）
	/// @param left ビューポートの左上X座標
	/// @param top ビューポートの左上Y座標
	/// @param width ビューポートの幅
	/// @param height ビューポートの高さ
	/// @param minDepth 最小深度値
	/// @param maxDepth 最大深度値
	/// @return ビューポート変換行列
	Matrix4x4 MakeViewport(float left, float top, float width, float height, float minDepth, float maxDepth);

	/// @brief 任意軸周りの回転行列を生成する
	/// @param axis 回転軸（正規化されたベクトル）
	/// @param angle 回転角度（ラジアン）
	/// @return 任意軸周りの回転行列
	Matrix4x4 MakeRotateAxisAngle(const Vector3& axis, float angle);

	/// @brief from方向からto方向へ向ける回転行列を生成する
	/// @param from 開始方向ベクトル
	/// @param to 目標方向ベクトル
	/// @return 方向変換の回転行列
	Matrix4x4 DirectionToDirection(const Vector3& from, const Vector3& to);

	/// @brief ベクトルを行列で変換する
	/// @param v 変換するベクトル
	/// @param m 変換行列
	/// @return 変換されたベクトル（同次座標として扱い、w成分は1として計算）
	Vector3 Transform(const Vector3& v, const Matrix4x4& m);

}