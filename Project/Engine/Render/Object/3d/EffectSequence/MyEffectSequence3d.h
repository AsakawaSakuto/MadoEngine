#pragma once
#include "EffectSequenceSystem.h"

namespace MadoEngine::EffectSequence {

	/// @brief Effect Sequence HandleをRAIIで管理する薄いWrapper
	class MyEffectSequence3d final {
	public:
		/// @brief 空のWrapperを構築する
		MyEffectSequence3d() = default;

		/// @brief 所有SequenceをImmediate停止して破棄する
		~MyEffectSequence3d() {
			Stop(EffectSequenceStopMode::Immediate);
		}

		/// @brief Copy構築を禁止する
		MyEffectSequence3d(const MyEffectSequence3d&) = delete;

		/// @brief Copy代入を禁止する
		/// @return 自身
		MyEffectSequence3d& operator=(const MyEffectSequence3d&) = delete;

		/// @brief WrapperをMove構築する
		/// @param other Move元Wrapper
		MyEffectSequence3d(MyEffectSequence3d&& other) noexcept
			: handle_(other.handle_) {
			other.handle_ = {};
		}

		/// @brief WrapperへMove代入する
		/// @param other Move元Wrapper
		/// @return 自身
		MyEffectSequence3d& operator=(MyEffectSequence3d&& other) noexcept {
			if (this != &other) {
				Stop(EffectSequenceStopMode::Immediate);
				handle_ = other.handle_;
				other.handle_ = {};
			}
			return *this;
		}

		/// @brief Effect Sequenceを再生してHandleを所有する
		/// @param assetName 再生するAsset名
		/// @param desc 再生設定
		/// @return 再生に成功した場合はtrue
		bool Play(const std::string& assetName, const EffectSequencePlayDesc& desc = {}) {
			Stop(EffectSequenceStopMode::Immediate);
			handle_ = EffectSequenceSystem::GetInstance().Play(assetName, desc);
			return EffectSequenceSystem::GetInstance().IsAlive(handle_);
		}

		/// @brief 所有Sequenceを停止する
		/// @param mode 停止方式
		void Stop(EffectSequenceStopMode mode = EffectSequenceStopMode::Finish) {
			if (EffectSequenceSystem::GetInstance().IsAlive(handle_)) {
				EffectSequenceSystem::GetInstance().Stop(handle_, mode);
			}
			if (mode == EffectSequenceStopMode::Immediate) {
				handle_ = {};
			}
		}

		/// @brief 所有Sequenceを一時停止する
		/// @return 一時停止できた場合はtrue
		bool Pause() {
			return EffectSequenceSystem::GetInstance().Pause(handle_);
		}

		/// @brief 所有Sequenceを再開する
		/// @return 再開できた場合はtrue
		bool Resume() {
			return EffectSequenceSystem::GetInstance().Resume(handle_);
		}

		/// @brief Sequence Root Transformを更新する
		/// @param transform 新しいRoot Transform
		/// @return 更新できた場合はtrue
		bool SetTransform(const Transform3D& transform) {
			return EffectSequenceSystem::GetInstance().SetTransform(handle_, transform);
		}

		/// @brief Sequence再生速度を更新する
		/// @param playbackSpeed 再生速度
		/// @return 更新できた場合はtrue
		bool SetPlaybackSpeed(float playbackSpeed) {
			return EffectSequenceSystem::GetInstance().SetPlaybackSpeed(handle_, playbackSpeed);
		}

		/// @brief 所有Sequenceが再生中か確認する
		/// @return 再生中の場合はtrue
		bool IsAlive() const {
			return EffectSequenceSystem::GetInstance().IsAlive(handle_);
		}

		/// @brief 所有Sequenceが一時停止中か確認する
		/// @return 一時停止中の場合はtrue
		bool IsPaused() const {
			return EffectSequenceSystem::GetInstance().IsPaused(handle_);
		}

		/// @brief 所有Handleを取得する
		/// @return 所有Handle
		EffectSequenceHandle GetHandle() const {
			return handle_;
		}

	private:
		EffectSequenceHandle handle_;
	};

} // namespace MadoEngine::EffectSequence
