#pragma once
#include "BeamEffectSystem3d.h"

namespace MadoEngine::Beam {

	/// @brief ゲーム側からBeam Effectを扱う薄いHandle Wrapper
	class MyBeamEffect3d final {
	public:
		/// @brief Wrapper破棄時に再生中Beamを即時停止
		~MyBeamEffect3d() {
			Stop(BeamStopMode::Immediate);
		}

		/// @brief Beam Effectを再生
		/// @param assetName 再生するAsset名
		/// @param desc 再生設定
		/// @return 再生に成功した場合はtrue
		bool Play(const std::string& assetName, const BeamEffectPlayDesc& desc = {}) {
			Stop(BeamStopMode::Immediate);
			handle_ = BeamEffectSystem3d::GetInstance().Play(assetName, desc);
			return handle_.HasValue();
		}

		/// @brief 再生中Beamを停止
		/// @param mode 停止方式
		void Stop(BeamStopMode mode = BeamStopMode::Finish) {
			if (handle_.HasValue()) {
				BeamEffectSystem3d::GetInstance().Stop(handle_, mode);
				handle_ = {};
			}
		}

		/// @brief 始点と終点を更新
		/// @param startPosition 新しい始点
		/// @param endPosition 新しい終点
		/// @return 更新に成功した場合はtrue
		bool SetEndpoints(const Vector3& startPosition, const Vector3& endPosition) {
			return BeamEffectSystem3d::GetInstance().SetEndpoints(handle_, startPosition, endPosition);
		}

		/// @brief 始点を更新
		/// @param position 新しい始点
		/// @return 更新に成功した場合はtrue
		bool SetStartPosition(const Vector3& position) {
			return BeamEffectSystem3d::GetInstance().SetStartPosition(handle_, position);
		}

		/// @brief 終点を更新
		/// @param position 新しい終点
		/// @return 更新に成功した場合はtrue
		bool SetEndPosition(const Vector3& position) {
			return BeamEffectSystem3d::GetInstance().SetEndPosition(handle_, position);
		}

		/// @brief Beamが再生中か確認
		/// @return 再生中の場合はtrue
		bool IsAlive() const {
			return BeamEffectSystem3d::GetInstance().IsAlive(handle_);
		}

		/// @brief 内部Handleを取得
		/// @return Beam Effect Handle
		BeamEffectHandle GetHandle() const {
			return handle_;
		}

	private:
		BeamEffectHandle handle_;
	};

} // namespace MadoEngine::Beam
