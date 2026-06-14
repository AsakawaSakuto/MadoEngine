#include "GameTimer.h"
#include <algorithm>
#include <cmath>

GameTimer::GameTimer(float duration, bool loop)
    : duration_((std::max)(0.0f, duration))
    , loop_(loop) {
}

void GameTimer::Update(float deltaTime) {
    if (state_ != State::Running) {
        return;
    }

    loopedThisFrame_ = false;
    finished_ = false;

    currentTime_ += (std::max)(0.0f, deltaTime);

    if (currentTime_ < duration_) {
        return;
    }

    finished_ = true;

    if (loop_ && duration_ > 0.0f) {
        currentTime_ = std::fmod(currentTime_, duration_);
        loopedThisFrame_ = true;
        return;
    }

    currentTime_ = duration_;
    state_ = State::Finished;
}

void GameTimer::Start(float duration, bool loop) {
    duration_ = (std::max)(0.0f, duration);
    loop_ = loop;
    currentTime_ = 0.0f;
    finished_ = false;
    loopedThisFrame_ = false;
    state_ = State::Running;
}

void GameTimer::Stop() {
    state_ = State::Stopped;
    finished_ = false;
    loopedThisFrame_ = false;
}

void GameTimer::Reset() {
    currentTime_ = 0.0f;
    finished_ = false;
    loopedThisFrame_ = false;
    state_ = State::Stopped;
}

void GameTimer::Pause() {
    if (state_ == State::Running) {
        state_ = State::Paused;
    }
}

void GameTimer::Resume() {
    if (state_ == State::Paused && currentTime_ < duration_) {
        state_ = State::Running;
        finished_ = false;
        loopedThisFrame_ = false;
    }
}

bool GameTimer::IsActive() const {
    return state_ == State::Running;
}

bool GameTimer::IsFinished() const {
    return finished_;
}

bool GameTimer::WasLoopedThisFrame() const {
    return loopedThisFrame_;
}

float GameTimer::GetProgress() const {
    if (duration_ <= 0.0f) {
        return 1.0f;
    }

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
    duration_ = (std::max)(0.0f, duration);

    if (currentTime_ < duration_ || state_ != State::Running) {
        return;
    }

    finished_ = true;

    if (loop_ && duration_ > 0.0f) {
        currentTime_ = std::fmod(currentTime_, duration_);
        loopedThisFrame_ = true;
        return;
    }

    currentTime_ = duration_;
    state_ = State::Finished;
}

void GameTimer::SetLoop(bool loop) {
    loop_ = loop;
}
