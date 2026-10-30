#pragma once
#include "PlayLogData.h"
#include <limits>

namespace System {

	/// @brief 最終スコアの計算と表示に共通する内訳
	struct PlayScoreBreakdown {
		double explorationPoints = 0.0;
		double survivalMultiplier = 1.0;
		std::uint64_t finalScore = 0;
	};

	/// @brief プレイ記録に基づくスコア内訳の計算
	class PlayScoreCalculator final {
	public:
		/// @brief 撃破数と探索数と生存時間からスコア内訳を計算
		/// @param data スコア計算の対象となるプレイ記録
		/// @return 探索ポイントと生存時間倍率と整数範囲内の最終スコア
		static PlayScoreBreakdown Calculate(const PlayLogData& data) {
			PlayScoreBreakdown breakdown;

			// 報酬種別や支払いの有無によらず、探索成功回数を共通の配点で評価
			const double jarCount = static_cast<double>(data.expJarCount) + static_cast<double>(data.moneyJarCount);
			const double chestCount = static_cast<double>(data.paidChestCount) + static_cast<double>(data.freeChestCount);
			const double jarPoints = jarCount * kScorePerJar;
			const double chestPoints = chestCount * kScorePerChest;
			breakdown.explorationPoints = jarPoints + chestPoints;
			breakdown.survivalMultiplier = 1.0 + data.elapsedSeconds / kScoreMultiplierDoubleSeconds;

			// 従来の演算順序を維持し、整数境界付近での丸め誤差によるスコア変化を防止
			const double baseScore = static_cast<double>(data.totalCombat.killCount) * kScorePerKill +
				jarPoints + chestPoints;
			const double survivalBonus = baseScore * data.elapsedSeconds / kScoreMultiplierDoubleSeconds;
			const double totalScore = baseScore + survivalBonus;

			// 整数型の範囲外への変換を避け、保存可能なスコアの上限へ制限
			const auto maxScore = (std::numeric_limits<std::uint64_t>::max)();
			if (totalScore >= static_cast<double>(maxScore)) {
				breakdown.finalScore = maxScore;
			} else {
				breakdown.finalScore = static_cast<std::uint64_t>(totalScore);
			}
			return breakdown;
		}

	private:
		static constexpr double kScorePerKill = 1.0;
		static constexpr double kScorePerJar = 5.0;
		static constexpr double kScorePerChest = 50.0;
		static constexpr double kScoreMultiplierDoubleSeconds = 300.0;
	};

}
