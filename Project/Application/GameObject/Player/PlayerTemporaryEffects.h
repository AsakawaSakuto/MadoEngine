#pragma once
#include "Utility/GameTimer/GameTimer.h"
#include <array>
#include <cmath>
#include <cstddef>

namespace Player {

	/// @brief Playerへ一時的に適用する特殊Drop効果の種類
	enum class TemporaryEffectType {
		AttackBoost,
		SpeedBoost,
		Invincibility,
		Count,
	};

	/// @brief 特殊Drop効果の独立した継続時間と倍率を管理するクラス
	class TemporaryEffects {
	public:
		/// @brief 指定効果を開始し、再取得時は残り時間を10秒へ更新
		/// @param type 開始する一時効果の種類
		void Apply(TemporaryEffectType type) {
			const std::size_t index = static_cast<std::size_t>(type);
			if (index >= timers_.size()) {
				return;
			}

			// 同じ効果の再取得では倍率や時間を加算せず、取得時点から継続時間を再計測
			timers_[index].Start(kDuration);
		}

		/// @brief ゲーム進行時間で各効果を更新し、期限切れの効果を終了
		/// @param deltaTime ゲーム進行中の経過時間（秒）
		void Update(float deltaTime) {
			if (!std::isfinite(deltaTime) || deltaTime <= 0.0f) {
				return;
			}

			// 種類ごとにTimerを分離し、別の効果の取得による延長や解除を防止
			for (GameTimer& timer : timers_) {
				timer.Update(deltaTime);
			}
		}

		/// @brief 全効果と継続時間をリセット
		void Clear() {
			for (GameTimer& timer : timers_) {
				timer.Reset();
			}
		}

		/// @brief 指定効果の有効状態を取得
		/// @param type 確認する一時効果の種類
		/// @return 指定効果が継続中の場合はtrue
		bool IsActive(TemporaryEffectType type) const {
			const std::size_t index = static_cast<std::size_t>(type);
			if (index >= timers_.size()) {
				return false;
			}
			return timers_[index].IsActive();
		}

		/// @brief 指定効果の残り時間を取得
		/// @param type 確認する一時効果の種類
		/// @return 効果の残り秒数、無効な種類または終了済みの場合は0
		float GetRemainingTime(TemporaryEffectType type) const {
			if (!IsActive(type)) {
				return 0.0f;
			}
			return timers_[static_cast<std::size_t>(type)].GetRemainingTime();
		}

		/// @brief 指定効果の経過時間を0から1の進行率として取得
		/// @param type 確認する一時効果の種類
		/// @return 効果の進行率、未開始または無効な種類の場合は0、終了済みの場合は1
		float GetProgress(TemporaryEffectType type) const {
			const GameTimer* timer = FindProgressTimer(type);
			if (!timer) {
				return 0.0f;
			}
			return timer->GetProgress();
		}

		/// @brief 指定効果の指定進行率以降の区間進行率を取得
		/// @param type 確認する一時効果の種類
		/// @param startProgress 区間の開始進行率、0.0から1.0へ制限
		/// @return 区間進行率、未開始または無効な種類の場合は0
		float GetProgressAfter(TemporaryEffectType type, float startProgress) const {
			const GameTimer* timer = FindProgressTimer(type);
			if (!timer) {
				return 0.0f;
			}
			return timer->GetProgressAfter(startProgress);
		}

		/// @brief 指定効果の指定進行率までの区間進行率を取得
		/// @param type 確認する一時効果の種類
		/// @param endProgress 区間の終了進行率、0.0から1.0へ制限
		/// @return 区間進行率、未開始または無効な種類の場合は0
		float GetProgressUntil(TemporaryEffectType type, float endProgress) const {
			const GameTimer* timer = FindProgressTimer(type);
			if (!timer) {
				return 0.0f;
			}
			return timer->GetProgressUntil(endProgress);
		}

		/// @brief 一時的な攻撃力倍率を取得
		/// @return AttackBoost継続中の場合は2.0、それ以外の場合は1.0
		float GetAttackMultiplier() const {
			if (IsActive(TemporaryEffectType::AttackBoost)) {
				return kAttackMultiplier;
			}
			return 1.0f;
		}

		/// @brief 一時的な移動速度倍率を取得
		/// @return SpeedBoost継続中の場合は1.5、それ以外の場合は1.0
		float GetMovementSpeedMultiplier() const {
			if (IsActive(TemporaryEffectType::SpeedBoost)) {
				return kMovementSpeedMultiplier;
			}
			return 1.0f;
		}

		/// @brief 被ダメージ無効効果の有効状態を取得
		/// @return Invincibility継続中の場合はtrue
		bool IsInvincible() const { return IsActive(TemporaryEffectType::Invincibility); }

	private:
		/// @brief 指定効果の進行率を参照する開始済みTimerを取得
		/// @param type 確認する一時効果の種類
		/// @return Timerへのconstポインター、未開始または無効な種類の場合はnullptr
		const GameTimer* FindProgressTimer(TemporaryEffectType type) const {
			const std::size_t index = static_cast<std::size_t>(type);
			if (index >= timers_.size()) {
				return nullptr;
			}
			const GameTimer& timer = timers_[index];
			if (!timer.IsActive() && !timer.IsFinished()) {
				return nullptr;
			}
			return &timer;
		}

		static constexpr float kDuration = 10.0f;
		static constexpr float kAttackMultiplier = 2.0f;
		static constexpr float kMovementSpeedMultiplier = 1.5f;
		std::array<GameTimer, static_cast<std::size_t>(TemporaryEffectType::Count)> timers_{};
	};
}
