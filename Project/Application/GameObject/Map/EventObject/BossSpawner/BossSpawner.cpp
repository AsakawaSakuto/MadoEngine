#include "BossSpawner.h"

BossSpawner::~BossSpawner() {
	MyCollider::RemoveCollider(colliderName_);
	HideInstancedDraw();
	if (model_.IsValid()) {
		MyModel::RequestDestroy(model_);
	}
}

void BossSpawner::Initialize(const InitializeDesc& desc) {
	SetColliderName(desc.colliderName);
	transform_.translate = desc.position;

	colliderShape_ = CreatePlacementCollider(desc.position);

	MyCollider::RegisterCollider(colliderName_, CollisionTag::MapEventObject, &colliderShape_, &transform_.translate, 0.0f);

	const MadoEngine::InstancedModelHandle normalBatchHandle = MyInstancedModel::GetOrCreate(
		"BossSpawner.Normal",
		"BossSpawner",
		SceneType::Game,
		MadoEngine::Render::RenderLayer::MapEventObject);
	const MadoEngine::InstancedModelHandle outlineBatchHandle = MyInstancedModel::GetOrCreate(
		"BossSpawner.Outline",
		"BossSpawner",
		SceneType::Game,
		MadoEngine::Render::RenderLayer::MapEventObjectOutline);

	InstancedModel* normalBatch = MyInstancedModel::TryGet(normalBatchHandle);
	InstancedModel* outlineBatch = MyInstancedModel::TryGet(outlineBatchHandle);
	if (normalBatch && outlineBatch) {

		// 強調表示をInstanceの表示切り替えだけで完結させるため同一Transformを二重登録
		normalBatch->SetTexture("white16x16");
		outlineBatch->SetTexture("white16x16");

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

AABB BossSpawner::CreatePlacementCollider(const Vector3& position) {
	AABB collider;
	collider.center = position;
	collider.min = { -1.0f, 0.0f, -1.0f };
	collider.max = { 1.0f, 2.0f, 1.0f };
	return collider;
}

void BossSpawner::Update(float deltaTime) {
	(void)deltaTime;

	std::get<AABB>(colliderShape_).center = transform_.translate;
	MyDebugLine::AddShape(std::get<AABB>(colliderShape_), { 0.0f, 0.0f, 0.0f, 1.0f });
}

bool BossSpawner::Interact(Player::Base& player) {
	(void)player;

	return true;
}

std::string_view BossSpawner::GetInteractionText() const {
	return "ボスを召喚";
}

MapEventRequest BossSpawner::GetInteractionRequest() const {
	return { MapEventAction::SpawnBoss, GetPosition() };
}
