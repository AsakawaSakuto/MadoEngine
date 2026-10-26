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

    // 完了とLoop通知を当Frameだけ有効なEventとして毎更新時に初期化
    loopedThisFrame_ = false;
    finished_ = false;

    currentTime_ += (std::max)(0.0f, deltaTime);

    if (currentTime_ < duration_) {
        return;
    }

    finished_ = true;

    if (loop_ && duration_ > 0.0f) {

        // 大きなdeltaTimeでも超過分を次周期へ繰り越す剰余処理
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

float GameTimer::GetProgressAfter(float startProgress) const {
    if (!std::isfinite(startProgress)) {
        return 0.0f;
    }

    // 開始値1では区間幅がゼロになるため、終了した瞬間だけ完了として処理
    const float start = std::clamp(startProgress, 0.0f, 1.0f);
    const float progress = GetProgress();
    if (start >= 1.0f) {
        if (progress >= 1.0f) {
            return 1.0f;
        }
        return 0.0f;
    }

    // 開始前を0へ制限し、開始以降の残り区間だけを0から1へ正規化
    return std::clamp((progress - start) / (1.0f - start), 0.0f, 1.0f);
}

float GameTimer::GetProgressUntil(float endProgress) const {
    if (!std::isfinite(endProgress)) {
        return 0.0f;
    }

    // 終了値0の空区間は開始時点で完了とし、ゼロ除算を回避
    const float end = std::clamp(endProgress, 0.0f, 1.0f);
    if (end <= 0.0f) {
        return 1.0f;
    }

    // 指定値へ到達後は1を維持し、登場演出の完了状態を後続区間でも保持
    return std::clamp(GetProgress() / end, 0.0f, 1.0f);
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

    // 実行中TimerのDuration短縮で現在時刻が終端を越えた場合は即時確定
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
