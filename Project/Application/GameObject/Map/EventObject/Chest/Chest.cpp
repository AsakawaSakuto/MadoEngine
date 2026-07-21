#include "Chest.h"

Chest::~Chest() {
	MyCollider::RemoveCollider(colliderName_);
	HideInstancedDraw();
	if (model_) {
		MyModel::Destroy(modelName_);
	}
}

void Chest::Initialize(const InitializeDesc& desc) {
	type_ = desc.type;
	modelName_ = desc.modelName;
	SetColliderName(desc.colliderName);
	transform_.translate = desc.position;
	transform_.rotate = desc.rotation;

	AABB aabb{};
	aabb.min = { -2.0f, 0.0f, -2.0f };
	aabb.max = { 2.0f, 1.0f, 2.0f };
	colliderShape_ = aabb;

	MyCollider::RegisterCollider(colliderName_, CollisionTag::MapEventObject, &colliderShape_, &transform_.translate, 0.0f);

	InstancedModel* normalBatch = MyInstancedModel::GetOrCreate(
		"Chest.Normal",
		"Chest",
		SceneType::Game,
		MadoEngine::Render::RenderLayer::MapEventObject);
	InstancedModel* outlineBatch = MyInstancedModel::GetOrCreate(
		"Chest.Outline",
		"Chest",
		SceneType::Game,
		MadoEngine::Render::RenderLayer::MapEventObjectOutline);

	if (normalBatch && outlineBatch) {
		normalBatch-> SetTexture("Chest");
		outlineBatch->SetTexture("Chest");

		InstancedModel::InstanceDesc normalInstance;
		normalInstance.transform = transform_;
		normalInstance.transform.scale = { 0.75f, 0.75f, 0.75f };
		normalInstance.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		normalInstance.isVisible = true;

		InstancedModel::InstanceDesc outlineInstance = normalInstance;
		outlineInstance.isVisible = false;

		uint32_t normalHandle = normalBatch->AddInstance(normalInstance);
		uint32_t outlineHandle = outlineBatch->AddInstance(outlineInstance);
		SetInstancedDraw(normalBatch, normalHandle, outlineBatch, outlineHandle);
	}
}

void Chest::Update(float deltaTime) {
	(void)deltaTime;

	std::get<AABB>(colliderShape_).center = transform_.translate;
	MyDebugLine::AddShape(std::get<AABB>(colliderShape_), { 0.0f, 0.0f, 0.0f, 1.0f });
}

bool Chest::Interact(Player::Base& player) {
	(void)player;

	return true;
}
