#include "Chest.h"

Chest::~Chest() {
	MyCollider::RemoveCollider(colliderName_);
	MyModel::Destroy(modelName_);
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

	model_ = MyModel::Create(modelName_, "Chest", SceneType::Test);

	if (model_) {
		model_->SetPosition(transform_.translate);
		model_->SetRotation(transform_.rotate);
		model_->SetScale({ 0.75f,0.75f ,0.75f });
		model_->SetRenderLayer(MadoEngine::Render::RenderLayer::MapEventObject);
		model_->SetTexture("box");
		model_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}
}

void Chest::Update(float deltaTime) {
	(void)deltaTime;

	std::get<AABB>(colliderShape_).center = transform_.translate;
	MyDebugLine::AddShape(std::get<AABB>(colliderShape_), { 0.0f, 0.0f, 0.0f, 1.0f });
}

bool Chest::Interact(Player& player) {
	(void)player;

	return true;
}
