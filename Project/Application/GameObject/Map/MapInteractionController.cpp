#include "MapInteractionController.h"
#include "Audio/MyAudio.h"
#include "GameObject/Player/Player.h"
#include <algorithm>

namespace {
constexpr const char* kInteractionIconSpriteName = "MapInteractionIcon";
constexpr float kInteractionIconPixelSize = 48.0f;
constexpr float kInteractionIconSpacing = 8.0f;
const Vector4 kInteractionTextDefaultColor = { 1.0f, 1.0f, 1.0f, 1.0f };
const Vector4 kInteractionTextUnavailableColor = { 1.0f, 0.0f, 0.0f, 1.0f };
}

void MapInteractionController::Initialize() {
	eventActionEnabled_.fill(true);
}

void MapInteractionController::Reset() {

	// MapがObjectを破棄する前に強調表示と借用参照を解除
	if (currentHitEventObject_) {
		currentHitEventObject_->SetHighlighted(false);
	}
	currentHitEventObject_ = nullptr;
	pendingEventRequests_.clear();
	pendingInteractionEvents_.clear();

	// 再生成後もEditorで管理される案内Resourceを再取得して初期表示を同期
	interactionMarkerModel_ = MyModel::Find("Interact_x");
	interactionText_ = MyText::Find("InteractText");
	interactionIcon_ = MySprite::Find(kInteractionIconSpriteName);
	if (!interactionIcon_.IsValid()) {
		interactionIcon_ = MySprite::Create(
			kInteractionIconSpriteName,
			"Money",
			SceneType::Game,
			MadoEngine::EditorManagementMode::RuntimeOnly,
			MadoEngine::Render::RenderLayer::Default);
	}
	interactionMarkerScaleTimer_.Reset();
	if (Model* interactionMarkerModel = MyModel::TryGet(interactionMarkerModel_)) {
		interactionMarkerModel->SetVisible(false);
	}
	if (MadoEngine::Text* interactionText = MyText::TryGet(interactionText_)) {
		interactionText->SetVisible(false);
	}
	if (Sprite* interactionIcon = MySprite::TryGet(interactionIcon_)) {
		interactionIcon->SetAnchorPoint({ 0.5f, 0.5f });
		interactionIcon->SetVisible(false);
	}
}

void MapInteractionController::SetInteractionTextVisible(bool isVisible) {

	// 表示要求の変更を既存Textへ即時反映し、非表示時は付随アイコンも解除
	isInteractionTextVisible_ = isVisible;
	if (MadoEngine::Text* interactionText = MyText::TryGet(interactionText_)) {
		interactionText->SetVisible(isInteractionTextVisible_ && currentHitEventObject_ != nullptr);
	}
	if (!isInteractionTextVisible_) {
		if (Sprite* interactionIcon = MySprite::TryGet(interactionIcon_)) {
			interactionIcon->SetVisible(false);
		}
	}
}

void MapInteractionController::SetEventActionEnabled(MapEventAction action, bool isEnabled) {
	const std::size_t actionIndex = static_cast<std::size_t>(action);
	if (actionIndex < eventActionEnabled_.size()) {
		eventActionEnabled_[actionIndex] = isEnabled;
	}
}

std::vector<MapEventRequest> MapInteractionController::ConsumeEventRequests() {
	std::vector<MapEventRequest> requests;

	// 同じ相互作用を複数Frameで処理しないよう未処理要求の所有権を呼び出し側へ移動
	requests.swap(pendingEventRequests_);
	return requests;
}

std::vector<MapInteractionEvent> MapInteractionController::ConsumeInteractionEvents() {
	std::vector<MapInteractionEvent> events;

	// Objectの破棄後も型付きの成功結果をSceneへ一度だけ配送
	events.swap(pendingInteractionEvents_);
	return events;
}

void MapInteractionController::Update(Player::Base& player, float deltaTime, std::vector<std::unique_ptr<MapEventObjectBase>>& eventObjects) {
	MapEventObjectBase* hitObject = nullptr;

	// 複数接触時も一つだけを操作対象として選択
	for (std::unique_ptr<MapEventObjectBase>& object : eventObjects) {
		object->Update(0.0f);

		if (!hitObject && object->IsHitPlayer()) {
			hitObject = object.get();
		}
	}

	if (currentHitEventObject_ != hitObject) {
		interactionMarkerScaleTimer_.Reset();

		// 接触対象が変化したフレームだけOutline表示を切り替え
		if (currentHitEventObject_) {
			currentHitEventObject_->SetHighlighted(false);
		}

		if (hitObject) {
			hitObject->SetHighlighted(true);
		}

		currentHitEventObject_ = hitObject;
	}

	HandleEventObjectInteraction(player, eventObjects);
	UpdateInteractionMarker(deltaTime);
	UpdateInteractionText(player);
}

void MapInteractionController::HandleEventObjectInteraction(Player::Base& player, std::vector<std::unique_ptr<MapEventObjectBase>>& eventObjects) {
	if (!currentHitEventObject_ || !MyInput::Trigger("Interact")) {
		return;
	}

	MapEventObjectBase* interactedObject = currentHitEventObject_;
	const MapEventRequest request = interactedObject->GetInteractionRequest();
	if (!IsEventActionEnabled(request.action)) {
		return;
	}

	// 外部Systemが要求を受け付けられる状態を確認してから支払いとObject削除を確定
	if (!interactedObject->Interact(player)) {
		return;
	}
	pendingInteractionEvents_.push_back(interactedObject->GetInteractionEvent());

	// 相互作用が成立した場合だけEventObject名と同じSEを一度再生
	const std::string eventObjectName(interactedObject->GetEventObjectName());
	if (MyAudio::IsLoaded(eventObjectName)) {
		MyAudio::Play(eventObjectName);
	}

	if (request.action != MapEventAction::None) {
		pendingEventRequests_.push_back(request);
	}

	auto it = std::find_if(eventObjects.begin(), eventObjects.end(), [interactedObject](const std::unique_ptr<MapEventObjectBase>& object) {
		return object.get() == interactedObject;
	});

	if (it != eventObjects.end()) {
		(*it)->SetHighlighted(false);
		eventObjects.erase(it);
	}

	currentHitEventObject_ = nullptr;
}

void MapInteractionController::UpdateInteractionMarker(float deltaTime) {
	Model* interactionMarkerModel = MyModel::TryGet(interactionMarkerModel_);
	if (!interactionMarkerModel) {
		return;
	}

	const bool hasInteractionTarget = currentHitEventObject_ != nullptr;
	interactionMarkerModel->SetVisible(hasInteractionTarget);
	if (!hasInteractionTarget) {
		interactionMarkerScaleTimer_.Reset();
		interactionMarkerModel->SetScale(interactionMarkerStartScale_);
		return;
	}

	// 接触対象のCollider上端へ案内Modelの位置を追従
	interactionMarkerModel->SetPosition(currentHitEventObject_->GetPosition());

	// 接触開始から一周期を繰り返すGameTimerで拡縮の位相を管理
	if (!interactionMarkerScaleTimer_.IsActive()) {
		interactionMarkerScaleTimer_.Start(0.5f, true);
	}
	interactionMarkerScaleTimer_.Update(deltaTime);
	interactionMarkerModel->SetScale(Easing::Lerp(interactionMarkerStartScale_, interactionMarkerEndScale_, interactionMarkerScaleTimer_.GetProgress(), EaseType::EaseOutQuint));
}

void MapInteractionController::UpdateInteractionText(const Player::Base& player) {
	MadoEngine::Text* interactionText = MyText::TryGet(interactionText_);
	Sprite* interactionIcon = MySprite::TryGet(interactionIcon_);
	if (!interactionText) {
		if (interactionIcon) {
			interactionIcon->SetVisible(false);
		}
		return;
	}

	const bool shouldShowInteractionText =
		isInteractionTextVisible_ && currentHitEventObject_ != nullptr;
	interactionText->SetVisible(shouldShowInteractionText);
	if (!shouldShowInteractionText) {
		if (interactionIcon) {
			interactionIcon->SetVisible(false);
		}
		return;
	}

	// 相互作用できない状態を赤色で通知し、対象変更時は通常色へ復帰
	const MapEventRequest request = currentHitEventObject_->GetInteractionRequest();
	const bool canInteract = currentHitEventObject_->CanInteract(player) &&
		IsEventActionEnabled(request.action);
	interactionText->SetColor([&]() {
		if (canInteract) {
			return (kInteractionTextDefaultColor);
		} else {
			return (kInteractionTextUnavailableColor);
		}
	}());

	// 文言の決定を各MapEventObjectへ委譲してObject固有の操作内容を表示
	interactionText->SetText(std::string(currentHitEventObject_->GetInteractionText()));

	if (!interactionIcon) {
		return;
	}

	const std::string_view iconTextureName =
		currentHitEventObject_->GetInteractionIconTextureName();
	if (iconTextureName.empty()) {
		interactionIcon->SetVisible(false);
		return;
	}

	// 操作案内Textの左端を基準に配置し、費用の桁数が変化しても並びを維持
	const std::string iconTextureNameString(iconTextureName);
	if (interactionIcon->GetTextureName() != iconTextureNameString &&
		!interactionIcon->SetTexture(iconTextureNameString)) {
		interactionIcon->SetVisible(false);
		return;
	}

	const Vector2 iconTextureSize = interactionIcon->GetTextureSize();
	const float iconTextureMaxSize = std::max(iconTextureSize.x, iconTextureSize.y);
	if (iconTextureMaxSize <= 0.0f) {
		interactionIcon->SetVisible(false);
		return;
	}

	const Vector2 textTextureSize = interactionText->GetTextureSize();
	const Vector2 textScale = interactionText->GetScale();
	const Vector2 textAnchorPoint = interactionText->GetAnchorPoint();
	const Vector2 textPosition = interactionText->GetPosition();
	const Vector2 textDrawSize = {
		textTextureSize.x * textScale.x,
		textTextureSize.y * textScale.y,
	};
	const float textLeft = textPosition.x - textDrawSize.x * textAnchorPoint.x;
	const float textCenterY =
		textPosition.y + textDrawSize.y * (0.5f - textAnchorPoint.y);
	const float iconScale = kInteractionIconPixelSize / iconTextureMaxSize;
	const float iconWidth = iconTextureSize.x * iconScale;
	interactionIcon->SetPosition({
		textLeft - kInteractionIconSpacing - iconWidth * 0.5f,
		textCenterY,
	});
	interactionIcon->SetScale({ iconScale, iconScale });
	interactionIcon->SetVisible(true);
}

bool MapInteractionController::IsEventActionEnabled(MapEventAction action) const {
	const std::size_t actionIndex = static_cast<std::size_t>(action);
	return actionIndex < eventActionEnabled_.size() && eventActionEnabled_[actionIndex];
}
