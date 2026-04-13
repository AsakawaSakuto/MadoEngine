#pragma once

/// <summary>
/// イージングタイプ
/// </summary>
enum class EaseType {

    // 等速

    Linear,

    // Quadratic

    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseOutInQuad,

    // Cubic

    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseOutInCubic,

    // Quartic

    EaseInQuart,
    EaseOutQuart,
    EaseInOutQuart,
    EaseOutInQuart,

    // Quintic

    EaseInQuint,
    EaseOutQuint,
    EaseInOutQuint,
    EaseOutInQuint,

    // Sine

    EaseInSine,
    EaseOutSine,
    EaseInOutSine,
    EaseOutInSine,

    // Exponential

    EaseInExpo,
    EaseOutExpo,
    EaseInOutExpo,
    EaseOutInExpo,

    // Circular

    EaseInCirc,
    EaseOutCirc,
    EaseInOutCirc,
    EaseOutInCirc,

    // Back

    EaseInBack,
    EaseOutBack,
    EaseInOutBack,
    EaseOutInBack,

    // Elastic

    EaseInElastic,
    EaseOutElastic,
    EaseInOutElastic,
    EaseOutInElastic,

    // Bounce

    EaseInBounce,
    EaseOutBounce,
    EaseInOutBounce,
    EaseOutInBounce
};