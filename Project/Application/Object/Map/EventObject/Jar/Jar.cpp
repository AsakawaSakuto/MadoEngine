#include "Jar.h"
#include "Object/Player/Player.h"

namespace {

/// @brief Jarのサイズに対応するモデル名を取得します。
/// @param size Jarのサイズです。
/// @return 読み込むモデル名です。
const char* GetJarModelAssetName(JarSize size) {
	switch (size) {
	case JarSize::Big:
		return "maxJar";
	case JarSize::Small:
	default:
		return "minJar";
	}
}

/// @brief Jarの取得報酬量を計算します。
/// @param size Jarのサイズです。
/// @return プレイヤーに加算する報酬量です。
int CalculateRewardAmount(JarSize size) {
	return size == JarSize::Big ? 20 : 10;
}

}

Jar::~Jar() {
	MyCollider::RemoveCollider(colliderName_);
	MyModel::Destroy(modelName_);
}

void Jar::Initialize(const InitializeDesc& desc) {
	type_ = desc.type;
	size_ = desc.size;
	modelName_ = desc.modelName;
	SetColliderName(desc.colliderName);
	transform_.translate = desc.position;
	transform_.rotate = desc.rotation;

	AABB aabb{};
	aabb.min = { -1.0f, 0.0f, -1.0f };
	aabb.max = { 1.0f, 1.0f, 1.0f };
	colliderShape_ = aabb;

	MyCollider::RegisterCollider(colliderName_, CollisionTag::MapEventObject, &colliderShape_, &transform_.translate, 0.0f);

	model_ = MyModel::Create(modelName_, GetJarModelAssetName(size_), SceneType::Test);

	if (model_) {
		model_->SetPosition(transform_.translate);
		model_->SetRotation(transform_.rotate);
		model_->SetRenderLayer(MadoEngine::Render::RenderLayer::MapEventObject);
		model_->SetTexture("white16x16");
		if (type_ == JarType::Money) {
			model_->SetColor({ 1.0f,1.0f,0.0f,1.0f });
		} else {
			model_->SetColor({ 0.0f,0.0f,1.0f,1.0f });
		}
	}
}

void Jar::Update(float deltaTime) {
	(void)deltaTime;

	std::get<AABB>(colliderShape_).center = transform_.translate;
	MyDebugLine::AddShape(std::get<AABB>(colliderShape_), { 0.0f, 0.0f, 0.0f, 1.0f });
}

bool Jar::Interact(Player& player) {
	const int rewardAmount = CalculateRewardAmount(size_);
	if (type_ == JarType::Exp) {
		player.AddExp(rewardAmount);
		Logger::Output("Jarを取得しました。経験値を" + std::to_string(rewardAmount) + "加算しました。", Logger::Level::Application);
	} else {
		player.AddMoney(rewardAmount);
		Logger::Output("Jarを取得しました。所持金を" + std::to_string(rewardAmount) + "加算しました。", Logger::Level::Application);
	}

	return true;
}
