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
		if (!isVisible) {
			Reset();
			return;
		}

		// 押下は即時に反映し、離した後の補間で短い入力でも反応を視認可能な時間を確保
		const float releaseProgress = (std::max)(deltaTime, 0.0f) / kControlReleaseDuration;
		if (MyInput::Press(definition.actionName)) {
			pressAmount_ = 1.0f;
		} else {
			pressAmount_ = (std::max)(0.0f, pressAmount_ - releaseProgress);
		}
		const float pressedScale = Easing::Lerp(1.0f, kControlPressedScale, pressAmount_, EaseType::EaseOutQuad);

		// Object再生成時だけ基準Scaleを取得し、Editor配置の寸法と各Spriteの縦横比を維持
		for (std::size_t index = 0; index < sprites_.size(); ++index) {
			const MadoEngine::SpriteHandle handle = MySprite::Find(definition.spriteNames[index]);
			Sprite* sprite = MySprite::TryGet(handle);
			if (!sprite || sprite->GetSceneType() != SceneType::Game) {
				continue;
			}
			if (sprites_[index] != handle) {
				sprites_[index] = handle;
				spriteBaseScales_[index] = sprite->GetScale();
			}
			sprite->SetScale(spriteBaseScales_[index] * pressedScale);
		}

		// Textも同じ倍率で拡縮し、文字画像の再生成を避けながら背景の押下演出へ同期
		const MadoEngine::TextHandle textHandle = MyText::Find(definition.textName);
		MadoEngine::Text* text = MyText::TryGet(textHandle);
		if (text && text->GetSceneType() == SceneType::Game) {
			if (text_ != textHandle) {
				text_ = textHandle;
				textBaseScale_ = text->GetScale();
			}
			text->SetScale(textBaseScale_ * pressedScale);
		}
	}

	void InputControlFeedback::Reset() {

		// 非表示時に変更したScaleを戻し、再表示で縮小率が基準値へ累積する状態を防止
		for (std::size_t index = 0; index < sprites_.size(); ++index) {
			Sprite* sprite = MySprite::TryGet(sprites_[index]);
			if (sprite && sprite->GetSceneType() == SceneType::Game) {
				sprite->SetScale(spriteBaseScales_[index]);
			}
		}
		MadoEngine::Text* text = MyText::TryGet(text_);
		if (text && text->GetSceneType() == SceneType::Game) {
			text->SetScale(textBaseScale_);
		}
		*this = {};
	}
}
