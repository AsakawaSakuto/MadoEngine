#include "Jar.h"

void Jar::Initialize() {
	type_ = JarType::Money;
	size_ = JarSize::Small;

	AABB aabb{};
	aabb.min = { -0.5f, 0.0f, -0.5f };
	aabb.max = { 0.5f, 1.0f, 0.5f };
	colliderShape_ = aabb;

	MyCollider::RegisterCollider("JarAABB", CollisionTag::MapEventObject, &colliderShape_, &transform_.translate, 0.0f);

	model_ = MyModel::Create("Jar", "minjar", SceneType::Test);
}

void Jar::Update(float deltaTime) {
	std::get<AABB>(colliderShape_).center = transform_.translate;
	MyDebugLine::AddShape(std::get<AABB>(colliderShape_), { 0.0f,0.0f,0.0f,1.0f });
}