#include "DropObjectGetView.h"
#include ".SceneManager/SceneType.h"
#include "Math/Vector4.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Utility/Easing/Easing.h"
#include <algorithm>
#include <cmath>

namespace {
	constexpr const char* kTextObjectName = "DropObjectGetText";

	/// @brief 特殊Dropの種類を取得時の表示文字列へ変換
	/// @param type 取得したDropObjectの種類
	/// @return 特殊Dropの表示文字列、通常報酬の場合はnullptr
	const char* GetCollectionText(DropObject::Type type) {

		// 経験値と所持金は既存のリソース獲得表示へ委譲し、特殊4種類だけを対象に限定
		switch (type) {
		case DropObject::Type::Heal:
			return "回復";
		case DropObject::Type::AttackUpper:
			return "ダメージ↑";
		case DropObject::Type::SpeedUpper:
			return "加速";
		case DropObject::Type::DefuseUpper:
			return "無敵";
		default:
			return nullptr;
		}
	}

	/// @brief 特殊Dropの種類を取得Textの文字色へ変換
	/// @param type 取得したDropObjectの種類
	/// @return 特殊Dropの文字色、通常報酬の場合は白色
	Vector4 GetCollectionColor(DropObject::Type type) {

		// 指定された8bitのRGBA値をText描画用の0から1の範囲へ正規化
		switch (type) {
		case DropObject::Type::Heal:
			return { 253.0f / 255.0f, 36.0f / 255.0f, 181.0f / 255.0f, 1.0f };
		case DropObject::Type::AttackUpper:
			return { 1.0f, 27.0f / 255.0f, 27.0f / 255.0f, 1.0f };
		case DropObject::Type::SpeedUpper:
			return { 27.0f / 255.0f, 195.0f / 255.0f, 1.0f, 1.0f };
		case DropObject::Type::DefuseUpper:
			return { 27.0f / 255.0f, 1.0f, 68.0f / 255.0f, 1.0f };
		default:
			return { 1.0f, 1.0f, 1.0f, 1.0f };
		}
	}
}

namespace UI::Game {

	void DropObjectGetView::Initialize() {
		Finalize();
		isVisible_ = true;

		// 取得前のサンプル文字を抑制し、EditorのVisible設定を維持
		if (MadoEngine::Text* text = ResolveText()) {
			text->SetRuntimeVisible(false);
		}
	}

	void DropObjectGetView::Spawn(DropObject::Type type) {
		const char* message = GetCollectionText(type);
		if (!message) {
			return;
		}
		MadoEngine::Text* text = ResolveText();
		if (!text) {
			return;
		}

		// 連続取得時も元の配置から再開し、上昇距離や拡大率の累積を防止
		if (!isActive_) {
			basePosition_ = text->GetPosition();
			baseScale_ = text->GetScale();
		}
		elapsedTime_ = 0.0f;
		isActive_ = true;
		text->SetText(message);
		text->SetColor(GetCollectionColor(type));
		text->SetPosition(basePosition_);
		text->SetScale(baseScale_);
		text->SetRuntimeVisible(isVisible_);
	}

	void DropObjectGetView::Update(float deltaTime) {
		MadoEngine::Text* text = ResolveText();
		if (!text) {
			return;
		}

		// 通常時は取得Textだけを隠し、保存済みの表示設定と装飾を維持
		if (!isActive_) {
			text->SetRuntimeVisible(false);
			return;
		}
		if (std::isfinite(deltaTime) && deltaTime > 0.0f) {
			elapsedTime_ += deltaTime;
		}
		const float expandStartTime = riseDuration_ + holdDuration_;
		const float shrinkStartTime = expandStartTime + expandDuration_;
		if (elapsedTime_ >= shrinkStartTime + shrinkDuration_) {
			Stop(*text);
			return;
		}

		// 上昇中だけEaseInQuintで加速し、到達後は拡縮中も終点の座標を維持
		const float riseProgress = std::clamp(elapsedTime_ / riseDuration_, 0.0f, 1.0f);
		float easedRiseProgress = 1.0f;
		if (riseProgress < 1.0f) {
			easedRiseProgress = Easing::Apply(riseProgress, riseEaseType_);
		}
		text->SetPosition(basePosition_ - Vector2{ 0.0f, riseDistance_ * easedRiseProgress });

		// 上昇と停止の間は基準Scaleを維持し、停止後だけ拡大から縮小へ順に遷移
		float scale = 1.0f;
		if (elapsedTime_ >= shrinkStartTime) {
			const float shrinkProgress = std::clamp(
				(elapsedTime_ - shrinkStartTime) / shrinkDuration_, 0.0f, 1.0f);
			scale = expandedScale_ * (1.0f - shrinkProgress);
		} else if (elapsedTime_ >= expandStartTime) {
			const float expandProgress = std::clamp(
				(elapsedTime_ - expandStartTime) / expandDuration_, 0.0f, 1.0f);
			scale = 1.0f + (expandedScale_ - 1.0f) * expandProgress;
		}
		text->SetScale(baseScale_ * scale);
		text->SetRuntimeVisible(isVisible_);
	}

	void DropObjectGetView::SetVisible(bool isVisible) {
		isVisible_ = isVisible;

		// Pauseや報酬選択中も演出状態を保持し、HUDの表示可否だけを同期
		if (MadoEngine::Text* text = ResolveText()) {
			text->SetRuntimeVisible(isVisible_ && isActive_);
		}
	}

	void DropObjectGetView::Finalize() {
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

	MadoEngine::Text* DropObjectGetView::ResolveText() {
		const MadoEngine::TextHandle handle = MyText::Find(kTextObjectName);
		MadoEngine::Text* text = MyText::TryGet(handle);
		if (!text || text->GetSceneType() != SceneType::Game) {
			Finalize();
			return nullptr;
		}

		// Json再読込で世代が変わった場合は旧演出を中断し、新しいEditor配置を基準に設定
		if (text_ != handle) {
			Finalize();
			text_ = handle;
			basePosition_ = text->GetPosition();
			baseScale_ = text->GetScale();
		}
		return text;
	}

	void DropObjectGetView::Stop(MadoEngine::Text& text) {
		text.SetPosition(basePosition_);
		text.SetScale(baseScale_);
		text.SetRuntimeVisible(false);
		elapsedTime_ = 0.0f;
		isActive_ = false;
	}

}
