#pragma once
#include <cstddef>

namespace UI::Result {

	/// @brief ランキング数値の順位順の出現演出の進行管理
	class RankingPresentation final {
	public:
		static constexpr double kSlideDurationSeconds = 0.2;

		/// @brief 1位から順番に数値の移動を開始
		/// @param entryCount 表示する順位の件数
		void Start(std::size_t entryCount);

		/// @brief 順位ごとの移動進行を更新
		/// @param deltaTime 前回更新からの経過秒数
		void Update(double deltaTime);

		/// @brief 全順位を最終位置へ移動した状態へ変更
		void Complete();

		/// @brief ランキング出現演出の開始状態を取得
		/// @return 出現演出を開始済みの場合はtrue
		bool HasStarted() const { return state_ != State::Waiting; }

		/// @brief 指定した順位の移動開始状態を取得
		/// @param rankIndex 0始まりの順位
		/// @return 指定した順位の移動を開始済みの場合はtrue
		bool HasEntryStarted(std::size_t rankIndex) const;

		/// @brief 指定した順位の移動進行率を取得
		/// @param rankIndex 0始まりの順位
		/// @return 移動前は0、完了後は1の進行率
		float GetSlideProgress(std::size_t rankIndex) const;

		/// @brief 全順位の出現完了状態を取得
		/// @return 全順位が最終位置へ到着した場合はtrue
		bool IsFinished() const { return state_ == State::Finished; }

	private:
		enum class State {
			Waiting,
			Playing,
			Finished,
		};

		State state_ = State::Waiting;
		std::size_t entryCount_ = 0;
		double elapsedSeconds_ = 0.0;
	};

}
