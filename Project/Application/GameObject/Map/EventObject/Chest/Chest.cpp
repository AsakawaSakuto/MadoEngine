#include "Chest.h"
#include "GameObject/Player/Player.h"
#include <algorithm>
#include <limits>

Chest::OpenCostState::OpenCostState(int initialCost, int costIncrease)
	: currentCost_(std::max(0, initialCost)),
	costIncrease_(std::max(0, costIncrease)) {
}

void Chest::OpenCostState::IncreaseCost() {

	// 加算による符号反転を防ぎ、到達後は表現可能な最大費用を維持
	const int maxCost = std::numeric_limits<int>::max();
	if (currentCost_ > maxCost - costIncrease_) {
		currentCost_ = maxCost;
		return;
	}

	currentCost_ += costIncrease_;
}

Chest::~Chest() {
	MyCollider::RemoveCollider(colliderName_);
	HideInstancedDraw();
	if (model_.IsValid()) {
		MyModel::RequestDestroy(model_);
	}
}

void Chest::Initialize(const InitializeDesc& desc) {
	type_ = desc.type;
	openCostState_ = desc.openCostState ? desc.openCostState : std::make_shared<OpenCostState>();
	modelName_ = desc.modelName;
	SetColliderName(desc.colliderName);
	transform_.translate = desc.position;
	transform_.rotate = desc.rotation;

	AABB aabb{};
	aabb.min = { -2.0f, 0.0f, -2.0f };
	aabb.max = { 2.0f, 1.5f, 2.0f };
	colliderShape_ = aabb;

	MyCollider::RegisterCollider(colliderName_, CollisionTag::MapEventObject, &colliderShape_, &transform_.translate, 0.0f);

	const MadoEngine::InstancedModelHandle normalBatchHandle = MyInstancedModel::GetOrCreate(
		"Chest.Normal",
		"Chest",
		SceneType::Game,
		MadoEngine::Render::RenderLayer::MapEventObject);
	const MadoEngine::InstancedModelHandle outlineBatchHandle = MyInstancedModel::GetOrCreate(
		"Chest.Outline",
		"Chest",
		SceneType::Game,
		MadoEngine::Render::RenderLayer::MapEventObjectOutline);

	InstancedModel* normalBatch = MyInstancedModel::TryGet(normalBatchHandle);
	InstancedModel* outlineBatch = MyInstancedModel::TryGet(outlineBatchHandle);
	if (normalBatch && outlineBatch) {

		// 強調表示をInstanceの表示切り替えだけで完結させるため同一Transformを二重登録
		normalBatch-> SetTexture("Chest");
		outlineBatch->SetTexture("Chest");

		InstancedModel::InstanceDesc normalInstance;
		normalInstance.transform = transform_;
		normalInstance.transform.scale = { 0.75f, 0.75f, 0.75f };

		// 無料Chestを外見だけで識別できるよう黄色で着色
		normalInstance.color = type_ == ChestType::Free
			? Vector4{ 1.0f, 0.0f, 0.0f, 1.0f }
			: Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
		normalInstance.isVisible = true;

		InstancedModel::InstanceDesc outlineInstance = normalInstance;
		outlineInstance.isVisible = false;

		uint32_t normalHandle = normalBatch->AddInstance(normalInstance);
		uint32_t outlineHandle = outlineBatch->AddInstance(outlineInstance);
		SetInstancedDraw(normalBatchHandle, normalHandle, outlineBatchHandle, outlineHandle);
	}
}

void Chest::Update(float deltaTime) {
	(void)deltaTime;

	std::get<AABB>(colliderShape_).center = transform_.translate;
	MyDebugLine::AddShape(std::get<AABB>(colliderShape_), { 0.0f, 0.0f, 0.0f, 1.0f });
}

bool Chest::Interact(Player::Base& player) {

	// Freeは共有費用の支払いと増加を発生させず、そのまま開封成功
	if (type_ == ChestType::Free) {
		return true;
	}

	// 残高不足時は費用を据え置き、ChestをMap上に維持
	if (!player.TrySpendMoney(openCostState_->GetCurrentCost())) {
		return false;
	}

	// 支払い成立後に次の通常Chestへ適用する費用を更新
	openCostState_->IncreaseCost();
	return true;
}

bool Chest::CanInteract(const Player::Base& player) const {

	// Freeは所持金に関係なく常に開封可能
	if (type_ == ChestType::Free) {
		return true;
	}

	return player.CanAfford(openCostState_->GetCurrentCost());
}

std::string_view Chest::GetInteractionText() const {
	if (type_ == ChestType::Free) {
		return "0G";
	}

	// 共有費用が変化した場合だけ案内文を再構築して毎フレームの文字列確保を抑制
	const int currentOpenCost = openCostState_->GetCurrentCost();
	if (displayedOpenCost_ != currentOpenCost) {
		interactionText_ = std::to_string(currentOpenCost) + "G";
		displayedOpenCost_ = currentOpenCost;
	}

	return interactionText_;
}
