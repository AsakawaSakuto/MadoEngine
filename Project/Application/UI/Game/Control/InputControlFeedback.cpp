#include "InputControlFeedback.h"
#include "Input/MyInput.h"
#include "Render/Object/2d/Sprite/SpriteManager.h"
#include "Render/Object/2d/UIGroup/UIGroupManager.h"
#include "Utility/Easing/Easing.h"
#include <algorithm>

namespace {
	constexpr float kControlPressedScale = 0.8f;
	constexpr float kControlReleaseDuration = 0.15f;
}

namespace UI::Game {

	void InputControlFeedback::Update(float deltaTime, const char* actionName, MadoEngine::UIGroupHandle buttonGroup) {
		auto& groups = MadoEngine::UIGroupManager::GetInstance();
		MadoEngine::UIGroup* group = groups.TryGet(buttonGroup);

		// 非表示や再読込による接続先変更時は前のボタンに縮小を残さないよう復元
		if (!group || !group->IsVisibleInHierarchy()) {
			Reset();
			return;
		}
		if (buttonGroup_ != buttonGroup) {
			Reset();
			buttonGroup_ = buttonGroup;
		}

		// 押下は即時に反映し、離した後の補間で短い入力でも反応を視認可能な時間を確保
		const float releaseProgress = (std::max)(deltaTime, 0.0f) / kControlReleaseDuration;
		if (MyInput::Press(actionName)) {
			pressAmount_ = 1.0f;
		} else {
			pressAmount_ = (std::max)(0.0f, pressAmount_ - releaseProgress);
		}
		const float pressedScale = Easing::Lerp(1.0f, kControlPressedScale, pressAmount_, EaseType::EaseOutQuad);
		group->SetScale(group->GetAuthoringState().transform.scale * pressedScale);

		// Spriteの描画順は親から継承されないため、背景と入力アイコンを登録順に前面へ移動
		auto& sprites = MadoEngine::SpriteManager::GetInstance();
		for (const auto& element : groups.GetElements(buttonGroup)) {
			if (element.resolved && element.type == MadoEngine::UIGroupElementType::Sprite) {
				sprites.MoveToFront(element.sprite);
			}
		}
	}

	void InputControlFeedback::Reset() {

		// Editorで編集中のScaleを基準に復元し、再表示時の縮小率の累積を防止
		if (MadoEngine::UIGroup* group = MadoEngine::UIGroupManager::GetInstance().TryGet(buttonGroup_)) {
			group->SetScale(group->GetAuthoringState().transform.scale);
		}
		buttonGroup_ = {};
		pressAmount_ = 0.0f;
	}
}
