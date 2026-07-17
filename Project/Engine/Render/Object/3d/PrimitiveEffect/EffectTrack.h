#pragma once
#include "Utility/Easing/Easing.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace MadoEngine::Effect {

	template<class T>
	struct EffectKeyframe {
		float time = 0.0f;
		T value{};
		EaseType easing = EaseType::Linear;
	};

	/// @brief エフェクト用の型付きキーフレームトラック
	/// @tparam T 補間する値の型
	template<class T>
	class EffectTrack {
	public:
		/// @brief 既定値でトラックを構築する
		EffectTrack() = default;

		/// @brief 指定した既定値でトラックを構築する
		/// @param defaultValue キーフレーム未設定時の値
		explicit EffectTrack(const T& defaultValue)
			: defaultValue_(defaultValue) {
		}

		/// @brief 指定時刻の値を評価する
		/// @param time 評価するエフェクト内時刻
		/// @return 補間された値
		T Evaluate(float time) const {
			if (keyframes_.empty()) {
				return defaultValue_;
			}

			if (time <= keyframes_.front().time) {
				return keyframes_.front().value;
			}
			if (time >= keyframes_.back().time) {
				return keyframes_.back().value;
			}

			const auto right = std::upper_bound(
				keyframes_.begin(),
				keyframes_.end(),
				time,
				[](float value, const EffectKeyframe<T>& keyframe) {
					return value < keyframe.time;
				}
			);
			const auto left = std::prev(right);
			const float segmentDuration = right->time - left->time;
			if (segmentDuration <= 0.0f) {
				return right->value;
			}

			const float normalizedTime = (time - left->time) / segmentDuration;
			return Easing::Lerp(left->value, right->value, normalizedTime, left->easing);
		}

		/// @brief キーフレーム未設定時の値を設定する
		/// @param value 設定する値
		void SetDefaultValue(const T& value) {
			defaultValue_ = value;
		}

		/// @brief キーフレーム未設定時の値を取得する
		/// @return 現在の既定値
		const T& GetDefaultValue() const {
			return defaultValue_;
		}

		/// @brief キーフレームを設定して時刻順に整列する
		/// @param keyframes 設定するキーフレーム
		void SetKeyframes(std::vector<EffectKeyframe<T>> keyframes) {
			for (EffectKeyframe<T>& keyframe : keyframes) {
				if (!std::isfinite(keyframe.time) || keyframe.time < 0.0f) {
					keyframe.time = 0.0f;
				}
			}

			std::stable_sort(
				keyframes.begin(),
				keyframes.end(),
				[](const EffectKeyframe<T>& lhs, const EffectKeyframe<T>& rhs) {
					return lhs.time < rhs.time;
				}
			);

			std::vector<EffectKeyframe<T>> normalized;
			normalized.reserve(keyframes.size());
			for (const EffectKeyframe<T>& keyframe : keyframes) {
				if (!normalized.empty() && normalized.back().time == keyframe.time) {
					normalized.back() = keyframe;
					continue;
				}
				normalized.push_back(keyframe);
			}
			keyframes_ = std::move(normalized);
		}

		/// @brief キーフレームを取得する
		/// @return 時刻順に並んだキーフレーム
		const std::vector<EffectKeyframe<T>>& GetKeyframes() const {
			return keyframes_;
		}

		/// @brief キーフレームが存在するか確認する
		/// @return 1つ以上存在する場合はtrue
		bool HasKeyframes() const {
			return !keyframes_.empty();
		}

	private:
		T defaultValue_{};
		std::vector<EffectKeyframe<T>> keyframes_;
	};

} // namespace MadoEngine::Effect
