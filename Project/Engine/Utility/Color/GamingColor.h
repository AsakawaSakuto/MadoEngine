#pragma once
#include "Math/Vector4.h"

/// @brief 時間経過に応じて循環するゲーミングカラーを生成するクラス
class GamingColor {
public:
    /// @brief 経過時間を進めて現在のゲーミングカラーを取得する
    /// @param deltaTime 前回の更新からの経過時間（秒）
    /// @param changeTime RGB成分の1つが0から255相当まで変化する時間（秒）
    /// @return 0.0fから1.0fで正規化されたRGBAカラー
    Vector4 Update(float deltaTime, float changeTime);

    /// @brief 色の循環を赤色の開始位置へ戻す
    void Reset();

private:
    /// @brief 循環内の位置からRGBAカラーを計算する
    /// @param phasePosition 0.0fから6.0f未満の循環位置
    /// @return 0.0fから1.0fで正規化されたRGBAカラー
    Vector4 CalculateColor(float phasePosition) const;

    double elapsedTime_ = 0.0;
};