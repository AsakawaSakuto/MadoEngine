#include "PlayerDamageView.h"
#include ".SceneManager/SceneType.h"
#include "Render/Object/2d/Text/MyText.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <string>

namespace {
	constexpr const char* kTextObjectName = "PlayerDamageText";
	constexpr float kIntegerEpsilon = 0.001f;

	/// @brief 被ダメージ量をマイナス記号付き表示文字列へ変換
	/// @param damage 表示する被ダメージ量
	/// @return 整数または被ダメージ量に応じた小数付き表示文字列
	std::string FormatDamageAmount(float damage) {
		const float roundedDamage = std::round(damage);

		// HP計算の丸め誤差を整数表示へ吸収し、小数の被ダメージ量は小数表示へ維持
		if (roundedDamage > 0.0f && std::abs(damage - roundedDamage) <= kIntegerEpsilon) {
			return std::format("-{:.0f}", roundedDamage);
		}

		// 小さな有効Damageが表示上の0へ丸められないよう有効桁数で表示
		if (damage < 0.1f) {
			return std::format("-{:.1g}", damage);
		}
		return std::format("-{:.1f}", damage);
	}
}

namespace UI::Game {

	void PlayerDamageView::Initialize() {
		Finalize();
		isVisible_ = true;
		if (MadoEngine::Text* text = ResolveText()) {
			text->SetRuntimeVisible(false);
		}
	}

	void PlayerDamageView::Spawn(float damage) {
		if (!std::isfinite(damage) || damage <= 0.0f) {
			return;
		}
		MadoEngine::Text* text = ResolveText();
		if (!text) {
			return;
		}

		// 非表示中のEditor配置変更だけを基準へ取り込み、連続被弾で移動量や拡大率の累積を防止
		if (!isActive_) {
			basePosition_ = text->GetPosition();
			baseScale_ = text->GetScale();
		}
		elapsedTime_ = 0.0f;
		isActive_ = true;
		text->SetText(FormatDamageAmount(damage));
		text->SetPosition(basePosition_);
		text->SetScale(baseScale_ * (1.0f + initialScaleAddition_));
		text->SetRuntimeVisible(isVisible_);
	}

	void PlayerDamageView::Update(float deltaTime) {
		MadoEngine::Text* text = ResolveText();
		if (!text) {
			return;
		}

		// 保存済みのVisibleと装飾を維持したまま通常時のサンプル文字だけを非表示化
		if (!isActive_) {
			text->SetRuntimeVisible(false);
			return;
		}
		if (std::isfinite(deltaTime) && deltaTime > 0.0f) {
			elapsedTime_ += deltaTime;
		}
		if (elapsedTime_ >= displayLifeTime_ + shrinkDuration_) {
			Stop(*text);
			return;
		}

		// Enemyダメージ表示と同じ初期拡大の収束、上昇、最後の縮小を基準配置へ適用
		const float displayProgress = std::clamp(elapsedTime_ / displayLifeTime_, 0.0f, 1.0f);
		const float displayScale = 1.0f + initialScaleAddition_ * (1.0f - displayProgress);
		const float shrinkProgress = std::clamp(
			(elapsedTime_ - displayLifeTime_) / shrinkDuration_, 0.0f, 1.0f);
		const float scale = displayScale * (1.0f - shrinkProgress);
		text->SetPosition(basePosition_ - Vector2{ 0.0f, riseDistance_ * displayProgress });
		text->SetScale(baseScale_ * scale);
		text->SetRuntimeVisible(isVisible_);
	}

	void PlayerDamageView::SetVisible(bool isVisible) {
		isVisible_ = isVisible;
		if (MadoEngine::Text* text = ResolveText()) {
			text->SetRuntimeVisible(isVisible_ && isActive_);
		}
	}

	void PlayerDamageView::Finalize() {
		if (MadoEngine::Text* text = MyText::TryGet(text_)) {

			// Editor所有のTextを破棄せず、演出中に変更した配置だけを復元
			if (isActive_) {
				Stop(*text);
			}
			text->SetRuntimeVisible(false);
		}
		text_ = {};
		elapsedTime_ = 0.0f;
		isActive_ = false;
	}

	MadoEngine::Text* PlayerDamageView::ResolveText() {
		const MadoEngine::TextHandle handle = MyText::Find(kTextObjectName);
		MadoEngine::Text* text = MyText::TryGet(handle);
		if (!text || text->GetSceneType() != SceneType::Game) {
			Finalize();
			return nullptr;
		}

		// Json再読込で世代が変わった場合は前の演出を中断し、新しいEditor配置を基準に再開
		if (text_ != handle) {
			Finalize();
			text_ = handle;
			basePosition_ = text->GetPosition();
			baseScale_ = text->GetScale();
		}
		return text;
	}

	void PlayerDamageView::Stop(MadoEngine::Text& text) {
		text.SetPosition(basePosition_);
		text.SetScale(baseScale_);
		text.SetRuntimeVisible(false);
		elapsedTime_ = 0.0f;
		isActive_ = false;
	}

}
