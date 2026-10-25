#include "SpriteSheetAnimator.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace MadoEngine {

	bool SpriteSheetAnimator::SetAnimation(const SpriteSheetAnimationSettings& settings) {
		const std::uint64_t frameCount = static_cast<std::uint64_t>(settings.columns) * settings.rows;

		// 不正な設定では再生中の状態を変更せず、コマ数の乗算と時間除算の安全性を確保
		if (settings.columns == 0 || settings.rows == 0 ||
			frameCount > (std::numeric_limits<std::uint32_t>::max)() ||
			!std::isfinite(settings.frameDuration) || settings.frameDuration <= 0.0f) {
			return false;
		}
		settings_ = settings;
		frameCount_ = static_cast<std::uint32_t>(frameCount);
		isConfigured_ = true;
		Stop();
		return true;
	}

	void SpriteSheetAnimator::Clear() {

		// 設定解除後はシートの一部分ではなく元のTexture全体を参照
		Stop();
		settings_ = {};
		frameCount_ = 1;
		isConfigured_ = false;
	}

	bool SpriteSheetAnimator::Play(bool restart) {
		if (!isConfigured_) {
			return false;
		}

		// 終了済みの再生は先頭へ戻し、一時停止からの再開だけ途中の時刻を維持
		if (restart || state_ == State::Finished) {
			Stop();
		}
		state_ = State::Playing;
		return true;
	}

	void SpriteSheetAnimator::Pause() {

		// 未再生や終了済みの状態を変えず、再生中だけ再開可能な一時停止へ遷移
		if (state_ == State::Playing) {
			state_ = State::Paused;
		}
	}

	void SpriteSheetAnimator::Stop() {

		// Poolの再利用や再生し直しで同じ初期表示を保証するため先頭コマへ復元
		elapsedTime_ = 0.0;
		currentFrame_ = 0;
		state_ = State::Stopped;
	}

	bool SpriteSheetAnimator::Update(float deltaTime) {

		// 停止中や不正な時間入力では再生時刻を保持し、UV計算への非有限値の伝播を防止
		if (state_ != State::Playing || !std::isfinite(deltaTime) || deltaTime <= 0.0f) {
			return false;
		}
		const std::uint32_t previousFrame = currentFrame_;
		const double duration = static_cast<double>(settings_.frameDuration) * frameCount_;
		elapsedTime_ += static_cast<double>(deltaTime);

		// 超過時間を周回へ引き継ぎ、非ループ再生では最終コマを維持したまま終了
		if (elapsedTime_ >= duration) {
			if (settings_.loop) {
				elapsedTime_ = std::fmod(elapsedTime_, duration);
			} else {
				elapsedTime_ = duration;
				currentFrame_ = frameCount_ - 1;
				state_ = State::Finished;
				return currentFrame_ != previousFrame;
			}
		}

		// 浮動小数点の境界誤差でも行数を超えるUVを作らないよう最終コマまでに制限
		const double frame = std::floor(elapsedTime_ / static_cast<double>(settings_.frameDuration));
		currentFrame_ = static_cast<std::uint32_t>((std::min)(frame, static_cast<double>(frameCount_ - 1)));
		return currentFrame_ != previousFrame;
	}

	Transform2D SpriteSheetAnimator::GetUVTransform() const {
		Transform2D transform{};
		if (!isConfigured_) {
			return transform;
		}

		// 左上から横方向へ進み、行末で次の行の先頭へ移動する並びでUV範囲を算出
		transform.scale = {
			1.0f / static_cast<float>(settings_.columns),
			1.0f / static_cast<float>(settings_.rows),
		};
		transform.translate = {
			static_cast<float>(currentFrame_ % settings_.columns) * transform.scale.x,
			static_cast<float>(currentFrame_ / settings_.columns) * transform.scale.y,
		};
		return transform;
	}

}
