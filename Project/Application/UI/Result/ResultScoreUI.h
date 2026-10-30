#pragma once
#include "ScorePresentation.h"
#include "RankingPresentation.h"
#include "Render/Object/2d/Text/MyText.h"
#include "System/PlayLog/PlayLogData.h"
#include "System/ScoreRanking/ScoreRanking.h"
#include <cstddef>
#include <cstdint>
#include <array>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace UI::Result {

	/// @brief スコア内訳と総スコアランキングの表示
	class ScoreUI final {
	public:
		/// @brief 終了時のプレイ記録を使用したスコア画面の初期化
		/// @param result 終了確定済みのプレイ結果
		/// @param rankingScores 総スコアの降順に並んだランキング
		/// @param currentRank 今回ランクインした0始まりの順位、圏外の場合は空
		void Initialize(const System::PlayLogData& result, std::span<const std::uint64_t> rankingScores,
			std::optional<std::size_t> currentRank = std::nullopt);

		/// @brief スコア画面全体の表示状態を設定
		/// @param isVisible 表示する場合はtrue
		void SetVisible(bool isVisible);

		/// @brief 表示中のスコアとランキングの演出を更新
		/// @param deltaTime 前回更新からの経過秒数
		void Update(float deltaTime);

		/// @brief 内訳移動とスコア加算とランキングを最後の表示状態へ変更
		void CompletePresentation();

		/// @brief スコア加算中の演出音を停止
		void StopPresentationAudio();

		/// @brief スコア画面の演出完了状態を取得
		/// @return 内訳移動とスコア加算とランキング出現を完了した場合はtrue
		bool IsPresentationFinished() const { return presentation_.IsFinished() && rankingPresentation_.IsFinished(); }

		/// @brief Scene単位で管理する描画リソースの参照を解放
		void Finalize();

	private:
		/// @brief 演出の進行を数値の位置と可視状態と操作案内へ反映
		void ApplyPresentation();

		/// @brief 最終スコア確定時の拡縮演出を開始
		void StartScorePop();

		/// @brief 最終スコアの拡大と元の大きさへの復帰を更新
		/// @param deltaTime 前回更新からの経過秒数
		void UpdateScorePop(float deltaTime);

		/// @brief 最終スコアの拡縮終了後にランキングの出現を更新
		/// @param deltaTime 前回更新からの経過秒数
		void UpdateRankingPresentation(float deltaTime);

		/// @brief 最終スコアの数字と単位へ共通の中心を基準とする拡縮を適用
		/// @param scale 通常サイズを1とした表示倍率
		void ApplyFinalScoreScale(float scale);

		/// @brief スコア内訳の項目名と数値を同じ行へ配置
		/// @param prefix 描画オブジェクトの識別名の接頭辞
		/// @param label 項目の表示名
		/// @param value 表示する数値文字列
		/// @param rowIndex 表示行の番号
		/// @return 数値表示のTextハンドル
		MadoEngine::TextHandle CreateScoreRow(const std::string& prefix, const std::string& label,
			const std::string& value, std::size_t rowIndex);

		/// @brief スコア画面用の文字列をUI描画レイヤーへ生成
		/// @param name 描画オブジェクトの識別名
		/// @param value 表示する文字列
		/// @param position 表示領域の左上座標
		/// @param areaSize 表示領域の大きさ
		/// @param fontSize 文字の大きさ
		/// @param align 表示領域内の水平方向の配置
		/// @param color 文字のRGBA色
		/// @return 生成したTextハンドル
		MadoEngine::TextHandle CreateText(const std::string& name, const std::string& value,
			const Vector2& position, const Vector2& areaSize, float fontSize,
			MadoEngine::TextHorizontalAlign align = MadoEngine::TextHorizontalAlign::Left,
			const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

		std::vector<MadoEngine::TextHandle> texts_;
		std::array<MadoEngine::TextHandle, ScorePresentation::kSlidingValueCount> movingValueTexts_{};
		std::array<MadoEngine::TextHandle, System::ScoreRanking::kMaxEntryCount> rankingValueTexts_{};
		MadoEngine::TextHandle finalScoreValueText_{};
		MadoEngine::TextHandle finalScoreUnitText_{};
		MadoEngine::TextHandle pageHintText_{};
		ScorePresentation presentation_;
		RankingPresentation rankingPresentation_;
		std::uint64_t finalScore_ = 0;
		bool isVisible_ = false;
		bool isDrumRollPlaying_ = false;
		float scorePopElapsed_ = 0.0f;
		bool isScorePopPlaying_ = false;
	};

}
