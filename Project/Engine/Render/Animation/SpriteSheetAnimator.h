#pragma once
#include "Math/Transform.h"
#include <cstdint>

namespace MadoEngine {

	/// @brief 左上から行方向へ並ぶスプライトシートの再生設定
	struct SpriteSheetAnimationSettings {
		std::uint32_t columns = 1;
		std::uint32_t rows = 1;
		float frameDuration = 0.1f;
		bool loop = true;
	};

	/// @brief 描画オブジェクトから独立したシートの再生状態とUV範囲の管理
	class SpriteSheetAnimator final {
	public:
		/// @brief 再生設定を検証して先頭コマで停止
		/// @param settings 横と縦のコマ数、表示間隔、ループ設定
		/// @return 設定が有効な場合はtrue
		bool SetAnimation(const SpriteSheetAnimationSettings& settings);

		/// @brief 再生設定を解除して通常のUV範囲へ復元
		void Clear();

		/// @brief 設定済みのシートを再生または再開
		/// @param restart 先頭コマから再生し直す場合はtrue
		/// @return 再生を開始できた場合はtrue
		bool Play(bool restart = true);

		/// @brief 現在のコマと再生時刻を保持して一時停止
		void Pause();

		/// @brief 再生設定を保持したまま先頭コマへ戻して停止
		void Stop();

		/// @brief 経過時間から現在コマと再生終了状態を更新
		/// @param deltaTime 前フレームからの経過時間（秒）
		/// @return 表示コマが変化した場合はtrue
		bool Update(float deltaTime);

		/// @brief 現在コマのUV変換を取得
		/// @return 現在コマの拡縮と移動、未設定の場合は単位変換
		Transform2D GetUVTransform() const;

		/// @brief 現在の再生設定を取得
		/// @return 現在の再生設定
		const SpriteSheetAnimationSettings& GetSettings() const { return settings_; }

		/// @brief 現在のコマ番号を取得
		/// @return 0から始まる現在のコマ番号
		std::uint32_t GetCurrentFrame() const { return currentFrame_; }

		/// @brief 有効な再生設定の有無を取得
		/// @return 再生設定を保持している場合はtrue
		bool IsConfigured() const { return isConfigured_; }

		/// @brief 再生中か判定
		/// @return 再生中の場合はtrue
		bool IsPlaying() const { return state_ == State::Playing; }

		/// @brief 一時停止中か判定
		/// @return 一時停止中の場合はtrue
		bool IsPaused() const { return state_ == State::Paused; }

		/// @brief 非ループ再生が終了したか判定
		/// @return 最終コマの表示時間まで完了した場合はtrue
		bool IsFinished() const { return state_ == State::Finished; }

	private:
		enum class State {
			Stopped,
			Playing,
			Paused,
			Finished,
		};

		SpriteSheetAnimationSettings settings_{};
		double elapsedTime_ = 0.0;
		std::uint32_t frameCount_ = 1;
		std::uint32_t currentFrame_ = 0;
		State state_ = State::Stopped;
		bool isConfigured_ = false;
	};

}
