#include "InputControlFeedback.h"
#include "Input/MyInput.h"
#include "Render/Object/2d/Sprite/MySprite.h"
#include "Render/Object/2d/Text/MyText.h"
#include "Utility/Easing/Easing.h"
#include <algorithm>

namespace {
	constexpr float kControlPressedScale = 0.8f;
	constexpr float kControlReleaseDuration = 0.15f;
}

namespace UI::Game {

	void InputControlFeedback::Update(float deltaTime, bool isVisible, const InputControlFeedbackDefinition& definition) {
		InputControlFeedbackTargets targets;

		// 従来の名前指定を解決済み参照へ変換し、Group内の要素とも同じ押下演出を共有
		if (isVisible) {
			for (std::size_t index = 0; index < targets.sprites.size(); ++index) {
				targets.sprites[index] = MySprite::Find(definition.spriteNames[index]);
			}
			targets.text = MyText::Find(definition.textName);
		}
		Update(deltaTime, isVisible, definition.actionName, targets);
	}

	void InputControlFeedback::Update(float deltaTime, bool isVisible, const char* actionName, const InputControlFeedbackTargets& targets) {
		if (!isVisible) {
			Reset();
			return;
		}

		// 押下は即時に反映し、離した後の補間で短い入力でも反応を視認可能な時間を確保
		const float releaseProgress = (std::max)(deltaTime, 0.0f) / kControlReleaseDuration;
		if (MyInput::Press(actionName)) {
			pressAmount_ = 1.0f;
		} else {
			pressAmount_ = (std::max)(0.0f, pressAmount_ - releaseProgress);
		}
		const float pressedScale = Easing::Lerp(1.0f, kControlPressedScale, pressAmount_, EaseType::EaseOutQuad);

		// Group所属時は保存用Scaleを基準にし、Editorの寸法変更と縦横比を維持
		for (std::size_t index = 0; index < sprites_.size(); ++index) {
			const MadoEngine::SpriteHandle handle = targets.sprites[index];
			Sprite* sprite = MySprite::TryGet(handle);
			if (!sprite || sprite->GetSceneType() != SceneType::Game) {
				continue;
			}
			if (sprites_[index] != handle) {
				sprites_[index] = handle;
				spriteBaseScales_[index] = sprite->GetScale();
			}
			if (sprite->HasAuthoringState()) {
				spriteBaseScales_[index] = sprite->GetAuthoringState().transform.scale;
			}
			sprite->SetScale(spriteBaseScales_[index] * pressedScale);
		}

		// Textも同じ倍率で拡縮し、文字画像の再生成を避けながら背景の押下演出へ同期
		const MadoEngine::TextHandle textHandle = targets.text;
		MadoEngine::Text* text = MyText::TryGet(textHandle);
		if (text && text->GetSceneType() == SceneType::Game) {
			if (text_ != textHandle) {
				text_ = textHandle;
				textBaseScale_ = text->GetScale();
			}
			if (text->HasAuthoringState()) {
				textBaseScale_ = text->GetAuthoringState().transform.scale;
			}
			text->SetScale(textBaseScale_ * pressedScale);
		}
	}

	void InputControlFeedback::Reset() {

		// 非表示時に変更したScaleを戻し、再表示で縮小率が基準値へ累積する状態を防止
		for (std::size_t index = 0; index < sprites_.size(); ++index) {
			Sprite* sprite = MySprite::TryGet(sprites_[index]);
			if (sprite && sprite->GetSceneType() == SceneType::Game) {
				if (sprite->HasAuthoringState()) {
					spriteBaseScales_[index] = sprite->GetAuthoringState().transform.scale;
				}
				sprite->SetScale(spriteBaseScales_[index]);
			}
		}
		MadoEngine::Text* text = MyText::TryGet(text_);
		if (text && text->GetSceneType() == SceneType::Game) {
			if (text->HasAuthoringState()) {
				textBaseScale_ = text->GetAuthoringState().transform.scale;
			}
			text->SetScale(textBaseScale_);
		}
		*this = {};
	}
}
