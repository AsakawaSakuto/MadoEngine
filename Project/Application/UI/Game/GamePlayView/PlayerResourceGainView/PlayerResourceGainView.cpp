#include "PlayerResourceGainView.h"
#include ".SceneManager/SceneType.h"
#include "Math/Vector4.h"
#include "Render/Object/2d/Text/MyText.h"
#ifdef USE_IMGUI
#include "imguiHeaders.h"
#endif
#include <algorithm>
#include <cmath>
#include <format>
#include <string>

namespace {
	constexpr std::array<float, 5> kHorizontalOffsetRates = {
		0.5f,
		0.0f,
		1.0f,
		0.25f,
		0.75f,
	};
	constexpr float kIntegerEpsilon = 0.001f;
	constexpr const char* kTextObjectNamePrefix = "PlayerResourceGainText_";

	/// @brief 獲得量をプラス記号付き表示文字列へ変換
	/// @param amount 表示する獲得量
	/// @return 整数または小数第一位までの表示文字列
	std::string FormatGainAmount(float amount) {
		const float roundedAmount = std::round(amount);
		if (std::abs(amount - roundedAmount) <= kIntegerEpsilon) {
			return std::format("+{:.0f}", roundedAmount);
		}

		return std::format("+{:.1f}", amount);
	}
}

namespace UI::Game {

	void PlayerResourceGainView::Initialize() {
		Finalize();

		// 取得が集中するFrameでも生成破棄を発生させない固定Text Poolを事前構築
		for (std::size_t index = 0; index < slots_.size(); ++index) {
			GainTextSlot& slot = slots_[index];
			slot.text = MyText::Create(
				kTextObjectNamePrefix + std::to_string(index),
				"+0",
				SceneType::Game,
				MadoEngine::EditorManagementMode::RuntimeOnly,
				MadoEngine::Render::RenderLayer::UI);
			MadoEngine::Text* text = MyText::TryGet(slot.text);
			if (!text) {
				continue;
			}

			text->SetFontFamily("Segoe UI");
			text->SetFontSize(fontSize_);
			text->SetAnchorPoint({ 0.5f, 0.5f });
			text->SetWordWrap(false);
			text->SetVisible(false);
		}

		nextSlotIndex_ = 0;
		spawnSequence_ = 0;
		isVisible_ = true;
	}

	void PlayerResourceGainView::Spawn(Player::ResourceGainType type, float amount) {
		if (!std::isfinite(amount) || amount <= 0.0f) {
			return;
		}

		// 短時間に連続取得した同種リソースを一つの数値へ集約
		if (GainTextSlot* mergeTarget = FindMergeTarget(type)) {
			MadoEngine::Text* text = MyText::TryGet(mergeTarget->text);
			if (text) {
				mergeTarget->amount += amount;
				mergeTarget->elapsedTime = 0.0f;
				text->SetText(FormatGainAmount(mergeTarget->amount));
				text->SetScale({
					1.0f + initialScaleAddition_,
					1.0f + initialScaleAddition_,
				});
				text->SetColor(GetTextColor(type));
				text->SetVisible(isVisible_);
				return;
			}
		}

		GainTextSlot* slot = AcquireSlot();
		if (!slot) {
			return;
		}
		MadoEngine::Text* text = MyText::TryGet(slot->text);
		if (!text) {
			return;
		}

		// 合算時間を越えた同種通知が重ならないよう左右位置を循環分散
		const std::size_t offsetIndex =
			static_cast<std::size_t>(spawnSequence_ % kHorizontalOffsetRates.size());
		slot->type = type;
		slot->amount = amount;
		slot->horizontalOffset = std::lerp(
			horizontalOffsetMin_,
			horizontalOffsetMax_,
			kHorizontalOffsetRates[offsetIndex]);
		slot->elapsedTime = 0.0f;
		slot->isActive = true;
		++spawnSequence_;

		text->SetText(FormatGainAmount(amount));
		text->SetPosition(GetBasePosition(type) + Vector2{ slot->horizontalOffset, 0.0f });
		text->SetScale({
			1.0f + initialScaleAddition_,
			1.0f + initialScaleAddition_,
		});
		text->SetColor(GetTextColor(type));
		text->SetVisible(isVisible_);
	}

	void PlayerResourceGainView::Update(float deltaTime) {
		if (!isVisible_) {
			return;
		}

		// Activeな固定Slotだけを更新して通知演出中の動的確保を回避
		for (GainTextSlot& slot : slots_) {
			MadoEngine::Text* text = MyText::TryGet(slot.text);
			if (!slot.isActive || !text) {
				continue;
			}

			if (deltaTime > 0.0f) {
				slot.elapsedTime += deltaTime;
			}

			if (slot.elapsedTime >= displayLifeTime_) {
				slot.isActive = false;
				text->SetVisible(false);
				continue;
			}

			const float progress = std::clamp(
				slot.elapsedTime / displayLifeTime_,
				0.0f,
				1.0f);
			const float fadeProgress = std::clamp(
				(progress - fadeStartProgress_) / (1.0f - fadeStartProgress_),
				0.0f,
				1.0f);
			const float scale = 1.0f +
				initialScaleAddition_ * (1.0f - progress);
			const Vector4 baseColor = GetTextColor(slot.type);
			Vector2 position = GetBasePosition(slot.type);
			position.x += slot.horizontalOffset;
			position.y -= riseDistance_ * progress;

			text->SetPosition(position);
			text->SetScale({ scale, scale });
			text->SetColor({
				baseColor.x,
				baseColor.y,
				baseColor.z,
				baseColor.w * (1.0f - fadeProgress),
			});
			text->SetVisible(true);
		}
	}

	void PlayerResourceGainView::SetVisible(bool isVisible) {
		if (isVisible_ == isVisible) {
			return;
		}

		isVisible_ = isVisible;
		if (isVisible_) {
			return;
		}

		// Pauseや強化選択中は寿命を保持したまま描画だけを抑制
		for (GainTextSlot& slot : slots_) {
			if (MadoEngine::Text* text = MyText::TryGet(slot.text)) {
				text->SetVisible(false);
			}
		}
	}

	void PlayerResourceGainView::Finalize() {
		for (GainTextSlot& slot : slots_) {
			if (slot.text.IsValid()) {
				MyText::Destroy(slot.text);
			}
			slot = {};
		}

		nextSlotIndex_ = 0;
		spawnSequence_ = 0;
	}

	void PlayerResourceGainView::DrawImGui() {
#ifdef USE_IMGUI
		ImGui::Begin("Player Resource Gain View");

		ImGui::SeparatorText("アニメーション");
		ImGui::DragFloat("表示時間", &displayLifeTime_, 0.01f, 0.05f, 5.0f, "%.2f 秒");
		ImGui::DragFloat("合算受付時間", &mergeDuration_, 0.01f, 0.0f, 1.0f, "%.2f 秒");
		ImGui::DragFloat("フェード開始位置", &fadeStartProgress_, 0.01f, 0.0f, 0.99f, "%.2f");
		ImGui::DragFloat("初期スケール加算", &initialScaleAddition_, 0.01f, 0.0f, 3.0f, "%.2f");
		ImGui::DragFloat("上昇距離", &riseDistance_, 1.0f, -500.0f, 500.0f, "%.0f px");
		ImGui::DragFloatRange2(
			"左右オフセット範囲",
			&horizontalOffsetMin_,
			&horizontalOffsetMax_,
			1.0f,
			-500.0f,
			500.0f,
			"最小: %.0f px",
			"最大: %.0f px");
		const bool fontSizeChanged =
			ImGui::DragFloat("フォントサイズ", &fontSize_, 1.0f, 1.0f, 200.0f, "%.0f px");

		ImGui::SeparatorText("表示位置");
		ImGui::DragFloat2("HP", &healthBasePosition_.x, 1.0f, 0.0f, 0.0f, "%.0f px");
		ImGui::DragFloat2("Exp", &expBasePosition_.x, 1.0f, 0.0f, 0.0f, "%.0f px");
		ImGui::DragFloat2("Money", &moneyBasePosition_.x, 1.0f, 0.0f, 0.0f, "%.0f px");

		ImGui::SeparatorText("文字色");
		ImGui::ColorEdit4("HP##ResourceGainColor", &healthTextColor_.x);
		ImGui::ColorEdit4("Exp##ResourceGainColor", &expTextColor_.x);
		ImGui::ColorEdit4("Money##ResourceGainColor", &moneyTextColor_.x);

		ImGui::SeparatorText("プレビュー");
		if (ImGui::Button("HP +10")) {
			Spawn(Player::ResourceGainType::Health, 10.0f);
		}
		ImGui::SameLine();
		if (ImGui::Button("Exp +10")) {
			Spawn(Player::ResourceGainType::Exp, 10.0f);
		}
		ImGui::SameLine();
		if (ImGui::Button("Money +10")) {
			Spawn(Player::ResourceGainType::Money, 10.0f);
		}

		// 手入力を含む調整値を補間と除算が安全な範囲へ制限
		displayLifeTime_ = std::clamp(displayLifeTime_, 0.05f, 5.0f);
		mergeDuration_ = std::clamp(mergeDuration_, 0.0f, displayLifeTime_);
		fadeStartProgress_ = std::clamp(fadeStartProgress_, 0.0f, 0.99f);
		initialScaleAddition_ = std::clamp(initialScaleAddition_, 0.0f, 3.0f);
		riseDistance_ = std::clamp(riseDistance_, -500.0f, 500.0f);
		horizontalOffsetMin_ = std::clamp(horizontalOffsetMin_, -500.0f, 500.0f);
		horizontalOffsetMax_ = std::clamp(horizontalOffsetMax_, -500.0f, 500.0f);
		if (horizontalOffsetMin_ > horizontalOffsetMax_) {
			std::swap(horizontalOffsetMin_, horizontalOffsetMax_);
		}
		fontSize_ = std::clamp(fontSize_, 1.0f, 200.0f);
		if (fontSizeChanged) {
			ApplyFontSize();
		}

		ImGui::End();
#endif
	}

	const Vector2& PlayerResourceGainView::GetBasePosition(
		Player::ResourceGainType type) const {
		switch (type) {
		case Player::ResourceGainType::Health:
			return healthBasePosition_;
		case Player::ResourceGainType::Exp:
			return expBasePosition_;
		case Player::ResourceGainType::Money:
			return moneyBasePosition_;
		}

		return healthBasePosition_;
	}

	const Vector4& PlayerResourceGainView::GetTextColor(
		Player::ResourceGainType type) const {
		switch (type) {
		case Player::ResourceGainType::Health:
			return healthTextColor_;
		case Player::ResourceGainType::Exp:
			return expTextColor_;
		case Player::ResourceGainType::Money:
			return moneyTextColor_;
		}

		return healthTextColor_;
	}

	void PlayerResourceGainView::ApplyFontSize() {
		for (GainTextSlot& slot : slots_) {
			if (MadoEngine::Text* text = MyText::TryGet(slot.text)) {
				text->SetFontSize(fontSize_);
			}
		}
	}

	PlayerResourceGainView::GainTextSlot* PlayerResourceGainView::FindMergeTarget(
		Player::ResourceGainType type) {
		GainTextSlot* mergeTarget = nullptr;

		// 合算受付中の同種Slotから最も新しい通知を選択
		for (GainTextSlot& slot : slots_) {
			if (!slot.isActive || slot.type != type ||
				slot.elapsedTime > mergeDuration_ || !MyText::TryGet(slot.text)) {
				continue;
			}

			if (!mergeTarget || slot.elapsedTime < mergeTarget->elapsedTime) {
				mergeTarget = &slot;
			}
		}

		return mergeTarget;
	}

	PlayerResourceGainView::GainTextSlot* PlayerResourceGainView::AcquireSlot() {

		// 次回位置から未使用Slotを循環探索して表示順の偏りを防止
		for (std::size_t offset = 0; offset < slots_.size(); ++offset) {
			const std::size_t index = (nextSlotIndex_ + offset) % slots_.size();
			GainTextSlot& slot = slots_[index];
			if (!MyText::TryGet(slot.text) || slot.isActive) {
				continue;
			}

			nextSlotIndex_ = (index + 1) % slots_.size();
			return &slot;
		}

		// 全Slot使用中は古い表示から循環上書きしてPool容量を維持
		for (std::size_t offset = 0; offset < slots_.size(); ++offset) {
			const std::size_t index = (nextSlotIndex_ + offset) % slots_.size();
			GainTextSlot& slot = slots_[index];
			if (!MyText::TryGet(slot.text)) {
				continue;
			}

			nextSlotIndex_ = (index + 1) % slots_.size();
			return &slot;
		}

		return nullptr;
	}

} // namespace UI::Game
