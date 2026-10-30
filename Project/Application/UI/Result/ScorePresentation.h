#pragma once
#include <cstddef>
#include <cstdint>

namespace UI::Result {

	/// @brief スコア内訳の順次移動と最終スコアの加算演出の進行管理
	class ScorePresentation final {
	public:
		static constexpr std::size_t kSlidingValueCount = 3;
		static constexpr double kSlideDurationSeconds = 0.75;
		static constexpr double kCountUpDurationSeconds = 2.0;

		/// @brief 内訳の移動と0点からの加算演出を開始
		/// @param finalScore 演出終了時の確定スコア
		void Start(std::uint64_t finalScore);

		/// @brief 経過時間に合わせて移動と加算の段階を更新
		/// @param deltaTime 前回更新からの経過秒数
		void Update(double deltaTime);

		/// @brief 移動と加算の演出を最後の状態へ変更
		void Complete();

		/// @brief 指定した数値の移動開始状態を取得
		/// @param valueIndex 移動する内訳の0始まりの番号
		/// @return 指定した内訳の移動を開始した場合はtrue
		bool HasValueStarted(std::size_t valueIndex) const;

		/// @brief 指定した数値の移動進行率を取得
		/// @param valueIndex 移動する内訳の0始まりの番号
		/// @return 移動前は0、完了後は1の進行率
		float GetSlideProgress(std::size_t valueIndex) const;

		/// @brief 現在の表示用スコアを取得
		/// @return 0点から確定スコアまでの表示用整数値
		std::uint64_t GetDisplayedScore() const { return displayedScore_; }

		/// @brief 最終スコアの加算時間を取得
		/// @return スコアに依存しない2秒の加算時間
		double GetCountUpDuration() const { return kCountUpDurationSeconds; }

		/// @brief 最終スコアの加算開始状態を取得
		/// @return 3つの移動が終了して加算段階を開始した場合はtrue
		bool HasCountUpStarted() const { return state_ == State::CountingScore || state_ == State::Finished; }

		/// @brief 全演出の完了状態を取得
		/// @return 3つの移動と最終スコアの加算を完了した場合はtrue
		bool IsFinished() const { return state_ == State::Finished; }

	private:
		enum class State {
			Ready,
			SlidingValues,
			CountingScore,
			Finished,
		};

		State state_ = State::Ready;
		std::uint64_t finalScore_ = 0;
		std::uint64_t displayedScore_ = 0;
		double elapsedSeconds_ = 0.0;
	};

}
