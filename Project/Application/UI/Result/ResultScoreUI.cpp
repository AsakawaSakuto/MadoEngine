#include "ResultScoreUI.h"
#include "Audio/MyAudio.h"
#include "System/PlayLog/PlayScoreCalculator.h"
#include "System/ScoreRanking/ScoreRanking.h"
#include "Utility/Easing/Easing.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <format>

namespace {
	constexpr float kFirstRowY = 190.0f;
	constexpr float kRowSpacing = 90.0f;
	constexpr float kRowHeight = 64.0f;
	constexpr float kRankingRowSpacing = 36.0f;
	constexpr float kSlideStartY = 760.0f;
	constexpr float kScorePopDuration = 0.25f;
	constexpr float kScorePopExpandDuration = 0.08f;
	constexpr float kScorePopPeakScale = 1.25f;
	constexpr Vector4 kDefaultTextColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	constexpr Vector4 kCurrentScoreColor = { 1.0f, 1.0f, 0.0f, 1.0f };
	constexpr std::array<Vector4, 3> kMedalColors = {{
		{ 1.0f, 0.78f, 0.18f, 1.0f },
		{ 0.78f, 0.84f, 0.92f, 1.0f },
		{ 0.80f, 0.50f, 0.27f, 1.0f },
	}};
}

namespace UI::Result {

	void ScoreUI::Initialize(const System::PlayLogData& result, std::span<const std::uint64_t> rankingScores,
		std::optional<std::size_t> currentRank) {
		StopPresentationAudio();
		finalScore_ = result.score;
		isVisible_ = false;
		scorePopElapsed_ = 0.0f;
		isScorePopPlaying_ = false;
		rankingPresentation_ = {};
		presentation_.Start(finalScore_);
		const System::PlayScoreBreakdown breakdown = System::PlayScoreCalculator::Calculate(result);

		// 保存用と同じ計算規則で内訳を生成し、最終値には終了時の確定済みスコアを使用
		CreateText("ResultScoreHeading", "スコア", { 220.0f, 120.0f }, { 520.0f, 48.0f }, 32.0f);
		movingValueTexts_[0] = CreateScoreRow("ResultScoreKills", "敵を倒した数", std::format("{}体", result.totalCombat.killCount), 0);
		movingValueTexts_[1] = CreateScoreRow("ResultExplorationPoints", "探索ポイント", std::format("{:.0f} pt", breakdown.explorationPoints), 1);
		movingValueTexts_[2] = CreateScoreRow("ResultSurvivalMultiplier", "生存時間倍率", std::format("×{:.3f}", breakdown.survivalMultiplier), 2);
		finalScoreValueText_ = CreateScoreRow("ResultFinalScore", "最終スコア", "0", 3);

		// 毎フレーム変化する数字だけを共有アトラスで描画し、pt表記の画像再生成を回避
		if (MadoEngine::Text* scoreText = MyText::TryGet(finalScoreValueText_)) {
			scoreText->SetAreaSize({ 220.0f, kRowHeight });
			scoreText->SetRenderMode(MadoEngine::TextRenderMode::NumericAtlas);
		}
		finalScoreUnitText_ = CreateText("ResultFinalScoreUnit", " pt", { 720.0f, kFirstRowY + kRowSpacing * 3.0f },
			{ 40.0f, kRowHeight }, 32.0f);

		// 左側の内訳と干渉しない独立した列へ上位10件を表示し、空き順位は未登録の表記
		CreateText("ResultRankingHeading", "総スコアランキング", { 850.0f, 120.0f }, { 300.0f, 48.0f }, 28.0f);
		for (std::size_t rankIndex = 0; rankIndex < System::ScoreRanking::kMaxEntryCount; ++rankIndex) {
			const std::string prefix = "ResultRanking" + std::to_string(rankIndex + 1);
			const float rowY = kFirstRowY + static_cast<float>(rankIndex) * kRankingRowSpacing;
			std::string scoreText = "---";
			if (rankIndex < rankingScores.size()) {
				scoreText = std::format("{} pt", rankingScores[rankIndex]);
			}

			// 上位3位の順位色と今回のスコアの黄色を分離し、同点の過去記録を強調しない配色
			Vector4 rankColor = kDefaultTextColor;
			if (rankIndex < kMedalColors.size()) {
				rankColor = kMedalColors[rankIndex];
			}
			Vector4 scoreColor = kDefaultTextColor;
			if (currentRank.has_value() && currentRank.value() == rankIndex && rankIndex < rankingScores.size()) {
				scoreColor = kCurrentScoreColor;
			}
			CreateText(prefix + "Rank", std::format("{}位", rankIndex + 1),
				{ 850.0f, rowY }, { 60.0f, 32.0f }, 22.0f, MadoEngine::TextHorizontalAlign::Left, rankColor);
			rankingValueTexts_[rankIndex] = CreateText(prefix + "Score", scoreText, { 865.0f, rowY }, { 235.0f, 32.0f }, 22.0f,
				MadoEngine::TextHorizontalAlign::Right, scoreColor);
		}
		pageHintText_ = CreateText("ResultScorePageHint", "A / Space：演出を完了    X / Q：リトライ",
			{ 220.0f, 620.0f }, { 840.0f, 32.0f }, 20.0f, MadoEngine::TextHorizontalAlign::Center);

		// 最初の決定入力までは戦績一覧だけを表示し、同時描画を防止
		SetVisible(false);
	}

	void ScoreUI::SetVisible(bool isVisible) {
		const bool startsPresentation = isVisible && !isVisible_;
		isVisible_ = isVisible;
		if (!isVisible_) {
			StopPresentationAudio();
		}

		// 内訳とランキングと操作案内を同じページ状態へ同期
		for (MadoEngine::TextHandle handle : texts_) {
			if (MadoEngine::Text* text = MyText::TryGet(handle)) {
				text->SetRuntimeVisible(isVisible);
			}
		}

		// 武器戦績の表示中には進行させず、スコア画面を開いた瞬間から演出を開始
		if (startsPresentation) {
			presentation_.Start(finalScore_);
			scorePopElapsed_ = 0.0f;
			isScorePopPlaying_ = false;
			rankingPresentation_ = {};
			ApplyFinalScoreScale(1.0f);
		}
		if (isVisible_) {
			ApplyPresentation();
		}
	}

	void ScoreUI::Update(float deltaTime) {

		// 非表示中は演出を進めず、スコア確定後も拡縮とランキングの出現を継続
		if (!isVisible_) {
			return;
		}
		UpdateScorePop(deltaTime);
		if (presentation_.IsFinished()) {
			if (!rankingPresentation_.IsFinished()) {
				UpdateRankingPresentation(deltaTime);
				ApplyPresentation();
			}
			return;
		}

		// 更新前後の到達状態を比較し、到着済みの項目で効果音を毎フレーム再生しない制御
		std::array<bool, ScorePresentation::kSlidingValueCount> arrivedBeforeUpdate{};
		for (std::size_t valueIndex = 0; valueIndex < arrivedBeforeUpdate.size(); ++valueIndex) {
			arrivedBeforeUpdate[valueIndex] = presentation_.GetSlideProgress(valueIndex) >= 1.0f;
		}
		const bool hadCountUpStarted = presentation_.HasCountUpStarted();
		presentation_.Update(static_cast<double>(deltaTime));
		for (std::size_t valueIndex = 0; valueIndex < arrivedBeforeUpdate.size(); ++valueIndex) {
			if (!arrivedBeforeUpdate[valueIndex] && presentation_.GetSlideProgress(valueIndex) >= 1.0f) {
				MyAudio::Play("Score");
			}
		}

		// 加算区間だけドラムロールを維持し、完了したフレームで終了音へ切り替え
		if (presentation_.IsFinished()) {
			StopPresentationAudio();
			MyAudio::Play("ResultScore");
			StartScorePop();
		} else if (!hadCountUpStarted && presentation_.HasCountUpStarted()) {
			MyAudio::Play("DrumRoll", true);
			isDrumRollPlaying_ = true;
		}
		ApplyPresentation();
	}

	void ScoreUI::CompletePresentation() {

		// スキップ時はランキングも最終位置へ移動し、確定済みのスコア音を重ねない制御
		if (IsPresentationFinished()) {
			return;
		}
		StopPresentationAudio();
		if (!presentation_.IsFinished()) {
			presentation_.Complete();
			MyAudio::Play("ResultScore");
			StartScorePop();
		}
		rankingPresentation_.Start(rankingValueTexts_.size());
		rankingPresentation_.Complete();
		ApplyPresentation();
	}

	void ScoreUI::StopPresentationAudio() {
		if (!isDrumRollPlaying_) {
			return;
		}
		MyAudio::Stop("DrumRoll");
		isDrumRollPlaying_ = false;
	}

	void ScoreUI::StartScorePop() {
		scorePopElapsed_ = 0.0f;
		isScorePopPlaying_ = true;
		ApplyFinalScoreScale(1.0f);
	}

	void ScoreUI::UpdateScorePop(float deltaTime) {

		// 不正な経過時間で表示倍率が破損しないよう有効な更新だけを進行
		if (!isScorePopPlaying_ || !std::isfinite(deltaTime) || deltaTime <= 0.0f) {
			return;
		}
		scorePopElapsed_ = (std::min)(scorePopElapsed_ + deltaTime, kScorePopDuration);
		if (scorePopElapsed_ >= kScorePopDuration) {

			// 補間誤差で拡大状態が残らないよう、終了時は元の倍率と座標へ固定
			isScorePopPlaying_ = false;
			ApplyFinalScoreScale(1.0f);
			return;
		}

		// 確定直後に素早く拡大し、残り時間で滑らかに元のサイズへ復帰
		float scale = 1.0f;
		if (scorePopElapsed_ < kScorePopExpandDuration) {
			scale = Easing::Lerp(1.0f, kScorePopPeakScale,
				scorePopElapsed_ / kScorePopExpandDuration, EaseType::EaseOutCubic);
		} else {
			scale = Easing::Lerp(kScorePopPeakScale, 1.0f,
				(scorePopElapsed_ - kScorePopExpandDuration) / (kScorePopDuration - kScorePopExpandDuration),
				EaseType::EaseInOutCubic);
		}
		ApplyFinalScoreScale(scale);
	}

	void ScoreUI::ApplyFinalScoreScale(float scale) {
		const float rowY = kFirstRowY + kRowSpacing * 3.0f;
		const Vector2 center = { 630.0f, rowY + kRowHeight * 0.5f };
		const std::array<MadoEngine::TextHandle, 2> handles = { finalScoreValueText_, finalScoreUnitText_ };
		const std::array<Vector2, 2> basePositions = {{ { 500.0f, rowY }, { 720.0f, rowY } }};

		// 数字とptを同じ中心から拡縮し、分割したText間の隙間や高さのずれを防止
		for (std::size_t index = 0; index < handles.size(); ++index) {
			if (MadoEngine::Text* text = MyText::TryGet(handles[index])) {
				text->SetScale({ scale, scale });
				text->SetPosition(center + (basePositions[index] - center) * scale);
			}
		}
	}

	void ScoreUI::UpdateRankingPresentation(float deltaTime) {

		// 確定スコアが通常の大きさへ戻ってから1位の数値の移動を開始
		if (isScorePopPlaying_ || rankingPresentation_.IsFinished()) {
			return;
		}
		if (!rankingPresentation_.HasStarted()) {
			rankingPresentation_.Start(rankingValueTexts_.size());
			return;
		}
		rankingPresentation_.Update(static_cast<double>(deltaTime));
	}

	void ScoreUI::ApplyPresentation() {

		// 3つの数値だけを下端外から現在の配置へ順番に移動し、到着済みの数値は固定
		for (std::size_t valueIndex = 0; valueIndex < movingValueTexts_.size(); ++valueIndex) {
			if (MadoEngine::Text* text = MyText::TryGet(movingValueTexts_[valueIndex])) {
				const Vector2 targetPosition = { 500.0f, kFirstRowY + static_cast<float>(valueIndex) * kRowSpacing };
				text->SetPosition(Easing::Lerp(Vector2{ 500.0f, kSlideStartY }, targetPosition,
					presentation_.GetSlideProgress(valueIndex), EaseType::EaseOutCubic));
				text->SetRuntimeVisible(isVisible_ && presentation_.HasValueStarted(valueIndex));
			}
		}

		// 内訳の通過中には最終値を隠し、3つの移動終了後に数字と単位をまとめて表示
		const bool showsFinalScore = isVisible_ && presentation_.HasCountUpStarted();
		if (MadoEngine::Text* scoreText = MyText::TryGet(finalScoreValueText_)) {
			scoreText->SetRuntimeVisible(showsFinalScore);
			const std::string value = std::format("{}", presentation_.GetDisplayedScore());
			if (scoreText->GetText() != value) {
				scoreText->SetText(value);
			}
		}
		if (MadoEngine::Text* unitText = MyText::TryGet(finalScoreUnitText_)) {
			unitText->SetRuntimeVisible(showsFinalScore);
		}

		// ランキングの数値だけを1位から順番に出現させ、未開始の順位は非表示を維持
		for (std::size_t rankIndex = 0; rankIndex < rankingValueTexts_.size(); ++rankIndex) {
			if (MadoEngine::Text* text = MyText::TryGet(rankingValueTexts_[rankIndex])) {
				const Vector2 targetPosition = { 865.0f, kFirstRowY + static_cast<float>(rankIndex) * kRankingRowSpacing };
				text->SetPosition(Easing::Lerp(Vector2{ 865.0f, kSlideStartY }, targetPosition,
					rankingPresentation_.GetSlideProgress(rankIndex), EaseType::EaseOutCubic));
				text->SetRuntimeVisible(isVisible_ && rankingPresentation_.HasEntryStarted(rankIndex));
			}
		}

		// 演出の完了状態に合わせて決定入力の役割を案内
		if (MadoEngine::Text* hintText = MyText::TryGet(pageHintText_)) {
			std::string hint = "A / Space：演出を完了    X / Q：リトライ";
			if (IsPresentationFinished()) {
				hint = "A / Space：タイトルへ    X / Q：リトライ";
			}
			if (hintText->GetText() != hint) {
				hintText->SetText(hint);
			}
		}
	}

	MadoEngine::TextHandle ScoreUI::CreateScoreRow(const std::string& prefix, const std::string& label,
		const std::string& value, std::size_t rowIndex) {
		const float rowY = kFirstRowY + static_cast<float>(rowIndex) * kRowSpacing;
		CreateText(prefix + "Label", label, { 260.0f, rowY }, { 240.0f, kRowHeight }, 28.0f);
		return CreateText(prefix + "Value", value, { 500.0f, rowY }, { 260.0f, kRowHeight }, 32.0f,
			MadoEngine::TextHorizontalAlign::Right);
	}

	MadoEngine::TextHandle ScoreUI::CreateText(const std::string& name, const std::string& value,
		const Vector2& position, const Vector2& areaSize, float fontSize, MadoEngine::TextHorizontalAlign align,
		const Vector4& color) {
		const MadoEngine::TextHandle handle = MyText::Create(
			name, value, SceneType::Result, MadoEngine::Render::RenderLayer::UI);
		texts_.push_back(handle);
		if (MadoEngine::Text* text = MyText::TryGet(handle)) {
			text->SetColor(color);
			text->SetFontAsset("Assets/Font/YuGothB.ttc", "游ゴシック");
			text->SetFontSize(fontSize);
			text->SetAnchorPoint({ 0.0f, 0.0f });
			text->SetPosition(position);
			text->SetAreaSize(areaSize);
			text->SetHorizontalAlign(align);
			text->SetVerticalAlign(MadoEngine::TextVerticalAlign::Center);
			text->SetWordWrap(false);
		}
		return handle;
	}

	void ScoreUI::Finalize() {
		StopPresentationAudio();

		// 描画リソースの破棄はSceneManagerのScene単位の寿命管理へ委譲
		texts_.clear();
		movingValueTexts_ = {};
		rankingValueTexts_ = {};
		finalScoreValueText_ = {};
		finalScoreUnitText_ = {};
		pageHintText_ = {};
		presentation_ = {};
		rankingPresentation_ = {};
		finalScore_ = 0;
		isVisible_ = false;
		scorePopElapsed_ = 0.0f;
		isScorePopPlaying_ = false;
	}

}
