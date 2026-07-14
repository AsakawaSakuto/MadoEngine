#include "GamingColor.h"

#include <cmath>

Vector4 GamingColor::Update(float deltaTime, float changeTime) {
    if (!std::isfinite(changeTime) || changeTime <= 0.0f) {
        return CalculateColor(0.0f);
    }

    if (std::isfinite(deltaTime) && deltaTime > 0.0f) {
        elapsedTime_ += static_cast<double>(deltaTime);
    }

    const double cycleDuration = static_cast<double>(changeTime) * 6.0;
    elapsedTime_ = std::fmod(elapsedTime_, cycleDuration);

    const float phasePosition = static_cast<float>(elapsedTime_ / static_cast<double>(changeTime));
    return CalculateColor(phasePosition);
}

void GamingColor::Reset() {
    elapsedTime_ = 0.0;
}

Vector4 GamingColor::CalculateColor(float phasePosition) const {
    const int phase = static_cast<int>(phasePosition);
    const float progress = phasePosition - static_cast<float>(phase);

    switch (phase) {
    case 0:
        return { 1.0f, progress, 0.0f, 1.0f };
    case 1:
        return { 1.0f - progress, 1.0f, 0.0f, 1.0f };
    case 2:
        return { 0.0f, 1.0f, progress, 1.0f };
    case 3:
        return { 0.0f, 1.0f - progress, 1.0f, 1.0f };
    case 4:
        return { progress, 0.0f, 1.0f, 1.0f };
    default:
        return { 1.0f, 0.0f, 1.0f - progress, 1.0f };
    }
}
