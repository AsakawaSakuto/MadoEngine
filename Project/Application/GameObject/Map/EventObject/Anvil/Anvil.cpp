#include "Anvil.h"

Anvil::~Anvil() {

	// 利用時とMap再生成時にCollider参照と個別Instanceの表示を解除
	if (!colliderName_.empty()) {
		MyCollider::RemoveCollider(colliderName_);
	}
	HideInstancedDraw();
}

void Anvil::Initialize(const InitializeDesc& desc) {
	SetColliderName(desc.colliderName);
	transform_.translate = desc.position;
	transform_.rotate = desc.rotation;
	colliderShape_ = CreatePlacementCollider(desc.position);
	MyCollider::RegisterCollider(colliderName_, CollisionTag::MapEventObject, &colliderShape_, &transform_.translate, 0.0f);

	const MadoEngine::InstancedModelHandle normalBatchHandle = MyInstancedModel::GetOrCreate(
		"Anvil.Normal",
		"Anvil",
		SceneType::Game,
		MadoEngine::Render::RenderLayer::MapEventObject);
	const MadoEngine::InstancedModelHandle outlineBatchHandle = MyInstancedModel::GetOrCreate(
		"Anvil.Outline",
		"Anvil",
		SceneType::Game,
		MadoEngine::Render::RenderLayer::MapEventObjectOutline);

	InstancedModel* normalBatch = MyInstancedModel::TryGet(normalBatchHandle);
	InstancedModel* outlineBatch = MyInstancedModel::TryGet(outlineBatchHandle);
	if (normalBatch && outlineBatch) {

		// Materialの色を維持し、接触時の強調だけを同一姿勢のInstance切り替えで適用
		InstancedModel::InstanceDesc normalInstance;
		normalInstance.transform = transform_;
		normalInstance.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		normalInstance.isVisible = true;
		InstancedModel::InstanceDesc outlineInstance = normalInstance;
		outlineInstance.isVisible = false;
		const uint32_t normalHandle = normalBatch->AddInstance(normalInstance);
		const uint32_t outlineHandle = outlineBatch->AddInstance(outlineInstance);
		SetInstancedDraw(normalBatchHandle, normalHandle, outlineBatchHandle, outlineHandle);
	}
}

AABB Anvil::CreatePlacementCollider(const Vector3& position) {
	AABB collider;
	collider.center = position;

	// 水平回転後の台座と上部のハンマーを含む占有範囲を確保
	collider.min = { -1.5f, 0.0f, -1.5f };
	collider.max = { 1.5f, 4.0f, 1.5f };
	return collider;
}

void Anvil::Update(float deltaTime) {
	(void)deltaTime;
	std::get<AABB>(colliderShape_).center = transform_.translate;
	MyDebugLine::AddShape(std::get<AABB>(colliderShape_), { 0.0f, 0.0f, 0.0f, 1.0f });
}

bool Anvil::Interact(Player::Base& player) {
	(void)player;

	// 強化の進行はSceneへ委譲し、ここでは無料の一回利用だけを確定
	return true;
}

std::string_view Anvil::GetInteractionText() const {
	return "武器を強化する（無料）";
}

MapEventRequest Anvil::GetInteractionRequest() const {
	return { MapEventAction::RequestWeaponUpgrade, GetPosition() };
}
