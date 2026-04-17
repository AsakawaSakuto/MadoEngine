#include "GameTimer.h"
#include <algorithm>
#include <cmath>

GameTimer::GameTimer(float duration, bool loop)
    : duration_(duration), loop_(loop) {
}

void GameTimer::Update(float deltaTime) {
    if (!isActive_) return;

    loopedThisFrame_ = false;
    finished_ = false;

    float scaledDeltaTime = deltaTime;

    currentTime_ += scaledDeltaTime;
    if (currentTime_ >= duration_) {
        finished_ = true;

        if (loop_) {
            currentTime_ = 0.0f;
            loopedThisFrame_ = true;
        } else {
            isActive_ = false;
        }
    }
}


void GameTimer::Start(float duration, bool loop) {
    duration_ = duration;
    loop_ = loop;
    currentTime_ = 0.0f;
    isActive_ = true;
    finished_ = false;
    useFrameMode_ = false;
    loopedThisFrame_ = false;
}

void GameTimer::Stop() {
    isActive_ = false;
}

void GameTimer::Reset() {
    currentTime_ = 0.0f;
    isActive_ = false;
    finished_ = false;
    loopedThisFrame_ = false;
}

void GameTimer::Pause() {
    isActive_ = false;
}

void GameTimer::Resume() {
    if (currentTime_ < duration_) {
        isActive_ = true;
        finished_ = false;
    }
}

bool GameTimer::IsActive() const {
    return isActive_;
}

bool GameTimer::IsFinished() const {
    return finished_;
}

float GameTimer::GetProgress() const {
    if (duration_ <= 0.0f) return 1.0f;

    float progress = currentTime_ / duration_;
    progress = std::clamp(progress, 0.0f, 1.0f);

    return progress;
}

float GameTimer::GetReverseProgress() const {
    return 1.0f - GetProgress();
}

float GameTimer::GetRemainingTime() const {
    return (std::max)(0.0f, duration_ - currentTime_);
}

float GameTimer::GetElapsedTime() const {
    return currentTime_;
}

float GameTimer::GetDuration() const {
    return duration_;
}

void GameTimer::SetDuration(float duration) {
    duration_ = duration;
    if (currentTime_ >= duration_ && isActive_) {
        finished_ = true;
        if (!loop_) {
            isActive_ = false;
        }
    }
}

void GameTimer::SetLoop(bool loop) {
    loop_ = loop;
}