#include "RankingPresentation.h"
#include <algorithm>
#include <cmath>

namespace {
	constexpr double kTimeEpsilon = 1.0e-9;
}

namespace UI::Result {

	void RankingPresentation::Start(std::size_t entryCount) {
		entryCount_ = entryCount;
		elapsedSeconds_ = 0.0;
		state_ = State::Playing;

		// 表示対象がない場合は後続の決定操作を妨げない完了状態へ移行
		if (entryCount_ == 0) {
			Complete();
		}
	}

	void RankingPresentation::Update(double deltaTime) {

		// 待機中と完了後は進めず、不正な時間による移動順序の逆行を防止
		if (state_ != State::Playing || !std::isfinite(deltaTime) || deltaTime <= 0.0) {
			return;
		}
		const double totalDuration = static_cast<double>(entryCount_) * kSlideDurationSeconds;
		elapsedSeconds_ = (std::min)(elapsedSeconds_ + deltaTime, totalDuration);
		if (elapsedSeconds_ + kTimeEpsilon >= totalDuration) {
			Complete();
		}
	}

	void RankingPresentation::Complete() {
		elapsedSeconds_ = static_cast<double>(entryCount_) * kSlideDurationSeconds;
		state_ = State::Finished;
	}

	bool RankingPresentation::HasEntryStarted(std::size_t rankIndex) const {
		return HasStarted() && rankIndex < entryCount_ &&
			elapsedSeconds_ + kTimeEpsilon >= static_cast<double>(rankIndex) * kSlideDurationSeconds;
	}

	float RankingPresentation::GetSlideProgress(std::size_t rankIndex) const {

		// 開始前と範囲外の順位は下端外へ保持し、到着後は最終位置へ固定
		if (!HasEntryStarted(rankIndex)) {
			return 0.0f;
		}
		const double slideElapsed = elapsedSeconds_ - static_cast<double>(rankIndex) * kSlideDurationSeconds;
		if (slideElapsed + kTimeEpsilon >= kSlideDurationSeconds) {
			return 1.0f;
		}
		return static_cast<float>(std::clamp(slideElapsed / kSlideDurationSeconds, 0.0, 1.0));
	}

}
