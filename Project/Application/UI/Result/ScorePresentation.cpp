#include "ScorePresentation.h"
#include <algorithm>
#include <cmath>

namespace {
	constexpr double kTimeEpsilon = 1.0e-9;
	constexpr double kScoreRoundingEpsilon = 1.0e-6;
}

namespace UI::Result {

	void ScorePresentation::Start(std::uint64_t finalScore) {
		finalScore_ = finalScore;
		displayedScore_ = 0;
		elapsedSeconds_ = 0.0;
		state_ = State::SlidingValues;
	}

	void ScorePresentation::Update(double deltaTime) {

		// 非有限値と逆行時間を除外し、完了後や開始前の表示値を固定
		if (state_ == State::Ready || state_ == State::Finished || !std::isfinite(deltaTime) || deltaTime <= 0.0) {
			return;
		}
		const double slideDuration = kSlideDurationSeconds * static_cast<double>(kSlidingValueCount);
		const double totalDuration = slideDuration + GetCountUpDuration();
		elapsedSeconds_ = (std::min)(elapsedSeconds_ + deltaTime, totalDuration);
		if (elapsedSeconds_ + kTimeEpsilon < slideDuration) {
			state_ = State::SlidingValues;
			return;
		}

		// 0点も2秒の加算区間を経て完了し、大きなデルタ時間は後続の区間へ引き継ぎ
		if (elapsedSeconds_ + kTimeEpsilon >= totalDuration) {
			Complete();
			return;
		}
		state_ = State::CountingScore;
		const double countUpSeconds = (std::max)(0.0, elapsedSeconds_ - slideDuration);
		const double countUpProgress = std::clamp(countUpSeconds / kCountUpDurationSeconds, 0.0, 1.0);
		const double countedScore = std::floor(static_cast<double>(finalScore_) * countUpProgress + kScoreRoundingEpsilon);

		// 64bit上限付近の浮動小数点の丸めを変換前に抑止し、完了値を超えない整数へ制限
		if (countedScore >= static_cast<double>(finalScore_)) {
			displayedScore_ = finalScore_;
		} else {
			displayedScore_ = static_cast<std::uint64_t>(countedScore);
		}
	}

	void ScorePresentation::Complete() {
		elapsedSeconds_ = kSlideDurationSeconds * static_cast<double>(kSlidingValueCount) + GetCountUpDuration();
		displayedScore_ = finalScore_;
		state_ = State::Finished;
	}

	bool ScorePresentation::HasValueStarted(std::size_t valueIndex) const {
		return state_ != State::Ready && valueIndex < kSlidingValueCount &&
			elapsedSeconds_ + kTimeEpsilon >= static_cast<double>(valueIndex) * kSlideDurationSeconds;
	}

	float ScorePresentation::GetSlideProgress(std::size_t valueIndex) const {

		// まだ開始していない内訳と範囲外の番号を移動前の位置へ固定
		if (!HasValueStarted(valueIndex)) {
			return 0.0f;
		}
		const double slideElapsed = elapsedSeconds_ - static_cast<double>(valueIndex) * kSlideDurationSeconds;
		if (slideElapsed + kTimeEpsilon >= kSlideDurationSeconds) {
			return 1.0f;
		}
		return static_cast<float>(std::clamp(slideElapsed / kSlideDurationSeconds, 0.0, 1.0));
	}

}
