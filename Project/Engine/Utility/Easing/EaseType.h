#pragma once

/// <summary>
/// イージングタイプ
/// </summary>
enum class EaseType {

    // 等速
    Linear,

    // 2次関数的に加速
    EaseInQuad,

    // 2次関数的に減速
    EaseOutQuad,

    // 2次関数的に加速してから減速
    EaseInOutQuad,

    // 2次関数的に減速してから加速
    EaseOutInQuad,

    // 3次関数的に加速
    EaseInCubic,

    // 3次関数的に減速
    EaseOutCubic,

    // 3次関数的に加速してから減速
    EaseInOutCubic,

    // 3次関数的に減速してから加速
    EaseOutInCubic,

    // 4次関数的に加速
    EaseInQuart,

    // 4次関数的に減速
    EaseOutQuart,

    // 4次関数的に加速してから減速
    EaseInOutQuart,

    // 4次関数的に減速してから加速
    EaseOutInQuart,

    // 5次関数的に加速
    EaseInQuint,

    // 5次関数的に減速
    EaseOutQuint,

    // 5次関数的に加速してから減速
    EaseInOutQuint,

    // 5次関数的に減速してから加速
    EaseOutInQuint,

    // サイン波で滑らかに加速
    EaseInSine,

    // サイン波で滑らかに減速
    EaseOutSine,

    // サイン波で滑らかに加速してから減速
    EaseInOutSine,

    // サイン波で滑らかに減速してから加速
    EaseOutInSine,

    // 指数関数的に加速
    EaseInExpo,

    // 指数関数的に減速
    EaseOutExpo,

    // 指数関数的に加速してから減速
    EaseInOutExpo,

    // 指数関数的に減速してから加速
    EaseOutInExpo,

    // 円運動で加速
    EaseInCirc,

    // 円運動で減速
    EaseOutCirc,
    
    // 円運動で加速してから減速
    EaseInOutCirc,
    
    // 円運動で減速してから加速
    EaseOutInCirc,

    // 少し後ろに引いてから加速
    EaseInBack,
    
    // 行き過ぎてから戻る減速
    EaseOutBack,
    
    // 後ろに引いてから行き過ぎて戻る
    EaseInOutBack,
    
    // 行き過ぎてから後ろに引く
    EaseOutInBack,

    // バネのように振動しながら加速
    EaseInElastic,
    
    // バネのように振動しながら減速
    EaseOutElastic,
    
    // バネのように振動しながら加速してから減速
    EaseInOutElastic,
    
    // バネのように振動しながら減速してから加速
    EaseOutInElastic,

    // バウンドしながら加速
    EaseInBounce,
    
    // バウンドしながら減速
    EaseOutBounce,
    
    // バウンドしながら加速してから減速
    EaseInOutBounce,
    
    // バウンドしながら減速してから加速
    EaseOutInBounce,

    // イージングを適用しない
    None,
};