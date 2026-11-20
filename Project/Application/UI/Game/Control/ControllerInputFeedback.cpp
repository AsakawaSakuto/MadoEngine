#include "ControllerInputFeedback.h"
#include "Input/MyInput.h"
#include "Render/Object/2d/Sprite/SpriteManager.h"
#include "Render/Object/2d/UIGroup/UIGroupManager.h"

namespace {
	struct ButtonBinding {
		const char* name;
		int button;
	};

	constexpr std::array<ButtonBinding, 6> kButtonBindings = {{
		{ "Pad_A", GAMEPAD_A },
		{ "Pad_B", GAMEPAD_B },
		{ "Pad_X", GAMEPAD_X },
		{ "Pad_Y", GAMEPAD_Y },
		{ "Pad_R", GAMEPAD_R },
		{ "Pad_L", GAMEPAD_L },
	}};
	constexpr std::array<const char*, 3> kDirectionalSpriteNames = { "Pad_StickL", "Pad_StickR", "Pad_Cross" };
	constexpr Vector3 kPressedColor{ 0.25f, 0.25f, 0.25f };
	constexpr float kDirectionalMoveDistance = 6.0f;
}

namespace UI::Game {

	void ControllerInputFeedback::Update() {
		auto& groups = MadoEngine::UIGroupManager::GetInstance();
		const auto controller = groups.Find("Controller", SceneType::Game);
		const MadoEngine::UIGroup* group = groups.TryGet(controller);

		// 非表示や再読込で表示先を失った場合は演出を残さず復元
		if (!group || !group->IsVisibleInHierarchy()) {
			Reset();
			return;
		}
		std::array<MadoEngine::SpriteHandle, 6> buttonHandles{};
		std::array<MadoEngine::SpriteHandle, 3> directionalHandles{};
		for (const auto& element : groups.GetElements(controller)) {
			if (!element.resolved || element.type != MadoEngine::UIGroupElementType::Sprite) {
				continue;
			}
			for (std::size_t i = 0; i < kButtonBindings.size(); ++i) {
				if (element.name == kButtonBindings[i].name) {
					buttonHandles[i] = element.sprite;
				}
			}
			for (std::size_t i = 0; i < kDirectionalSpriteNames.size(); ++i) {
				if (element.name == kDirectionalSpriteNames[i]) {
					directionalHandles[i] = element.sprite;
				}
			}
		}

		// 操作割り当てから独立した実ボタンの押下を表示し、RGBだけを変更してAlphaを維持
		const auto* gamePad = MyInput::GetGamePad();
		auto& sprites = MadoEngine::SpriteManager::GetInstance();
		for (std::size_t i = 0; i < buttons_.size(); ++i) {
			ButtonState& state = buttons_[i];
			if (state.sprite != buttonHandles[i]) {
				RestoreButton(state);
			}
			Sprite* sprite = sprites.TryGet(buttonHandles[i]);
			if (!sprite) {
				continue;
			}
			if (gamePad && gamePad->IsPress(kButtonBindings[i].button)) {
				if (!state.sprite.IsValid()) {
					const Vector4& color = sprite->GetColor();
					state.sprite = buttonHandles[i];
					state.baseColor = { color.x, color.y, color.z };
				}
				sprite->SetColor({ kPressedColor.x, kPressedColor.y, kPressedColor.z, sprite->GetColor().w });
			} else {
				RestoreButton(state);
			}
		}

		// 入力側のDeadzoneを利用し、切断時は中立入力としてスティックと十字キーを原点へ復帰
		std::array<Vector2, 3> directionalInputs{};
		if (gamePad) {
			directionalInputs[0] = gamePad->GetLeftStick();
			directionalInputs[1] = gamePad->GetRightStick();

			// 十字キーをスティックと同じ座標系へ変換し、反対方向の同時押下を相殺
			if (gamePad->IsPress(GAMEPAD_RIGHT)) {
				directionalInputs[2].x += 1.0f;
			}
			if (gamePad->IsPress(GAMEPAD_LEFT)) {
				directionalInputs[2].x -= 1.0f;
			}
			if (gamePad->IsPress(GAMEPAD_UP)) {
				directionalInputs[2].y += 1.0f;
			}
			if (gamePad->IsPress(GAMEPAD_DOWN)) {
				directionalInputs[2].y -= 1.0f;
			}
		}
		for (std::size_t i = 0; i < directionalSprites_.size(); ++i) {
			if (directionalSprites_[i] != directionalHandles[i]) {
				if (Sprite* previous = sprites.TryGet(directionalSprites_[i])) {
					previous->SetPosition(previous->GetAuthoringState().transform.translate);
				}
				directionalSprites_[i] = directionalHandles[i];
			}
			if (Sprite* sprite = sprites.TryGet(directionalSprites_[i])) {

				// 斜め方向の移動量を制限し、上向き正の入力を下向き正のUI座標へ変換
				Vector2 input = directionalInputs[i];
				if (input.LengthSq() > 1.0f) {
					input = input.Normalized();
				}
				const Vector2 offset{ input.x * kDirectionalMoveDistance, -input.y * kDirectionalMoveDistance };
				sprite->SetPosition(sprite->GetAuthoringState().transform.translate + offset);
			}
		}
	}

	void ControllerInputFeedback::Reset() {

		// Scene終了やHUD非表示の際に、Editor配置の見た目へ復元
		for (ButtonState& state : buttons_) {
			RestoreButton(state);
		}
		auto& sprites = MadoEngine::SpriteManager::GetInstance();
		for (const auto handle : directionalSprites_) {
			if (Sprite* sprite = sprites.TryGet(handle)) {
				sprite->SetPosition(sprite->GetAuthoringState().transform.translate);
			}
		}
		directionalSprites_ = {};
	}

	void ControllerInputFeedback::RestoreButton(ButtonState& state) {
		if (Sprite* sprite = MadoEngine::SpriteManager::GetInstance().TryGet(state.sprite)) {
			sprite->SetColor({ state.baseColor.x, state.baseColor.y, state.baseColor.z, sprite->GetColor().w });
		}
		state = {};
	}
}
