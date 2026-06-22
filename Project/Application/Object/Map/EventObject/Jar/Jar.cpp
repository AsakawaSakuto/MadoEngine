#include "Jar.h"
#include "Object/Player/Player.h"

Jar::~Jar() {
	MyCollider::RemoveCollider(colliderName_);
	MyModel::Destroy(modelName_);
}

void Jar::Initialize(const InitializeDesc& desc) {
	type_ = JarType::Money;
	size_ = JarSize::Small;
	modelName_ = desc.modelName;
	SetColliderName(desc.colliderName);
	transform_.translate = desc.position;
	transform_.rotate = desc.rotation;

	AABB aabb{};
	aabb.min = { -0.5f, 0.0f, -0.5f };
	aabb.max = { 0.5f, 1.0f, 0.5f };
	colliderShape_ = aabb;

	MyCollider::RegisterCollider(colliderName_, CollisionTag::MapEventObject, &colliderShape_, &transform_.translate, 0.0f);

	model_ = MyModel::Create(modelName_, "minJar", SceneType::Test);

	if (model_) {
		model_->SetPosition(transform_.translate);
		model_->SetRotation(transform_.rotate);
		model_->SetRenderLayer(MadoEngine::Render::RenderLayer::MapEventObject);
	}
}

void Jar::Update(float deltaTime) {
	(void)deltaTime;

	std::get<AABB>(colliderShape_).center = transform_.translate;
	MyDebugLine::AddShape(std::get<AABB>(colliderShape_), { 0.0f,0.0f,0.0f,1.0f });
}

bool Jar::Interact(Player& player) {
	player.AddMoney(10);
	Logger::Output("Jarを取得しました。所持金を10加算しました。", Logger::Level::Application);
	return true;
}
