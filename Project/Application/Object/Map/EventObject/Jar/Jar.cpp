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

/// @brief Jarのインスタンス描画バッチ名を取得します。
/// @param size Jarのサイズです。
/// @param isOutline アウトライン表示用の場合はtrueです。
/// @return インスタンス描画バッチ名です。
const char* GetJarBatchName(JarSize size, bool isOutline) {
	if (size == JarSize::Big) {
		return isOutline ? "Jar.Big.Outline" : "Jar.Big.Normal";
	}

	return isOutline ? "Jar.Small.Outline" : "Jar.Small.Normal";
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
	HideInstancedDraw();
	if (model_) {
		MyModel::Destroy(modelName_);
	}
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

	Vector4 color = { 0.0f, 0.0f, 1.0f, 1.0f };
	if (type_ == JarType::Money) {
		color = { 1.0f, 1.0f, 0.0f, 1.0f };
	}

	InstancedModel* normalBatch = MyInstancedModel::GetOrCreate(
		GetJarBatchName(size_, false),
		GetJarModelAssetName(size_),
		SceneType::Test,
		MadoEngine::Render::RenderLayer::MapEventObject);
	InstancedModel* outlineBatch = MyInstancedModel::GetOrCreate(
		GetJarBatchName(size_, true),
		GetJarModelAssetName(size_),
		SceneType::Test,
		MadoEngine::Render::RenderLayer::MapEventObjectOutline);

	if (normalBatch && outlineBatch) {
		normalBatch->SetTexture("white16x16");
		outlineBatch->SetTexture("white16x16");

		InstancedModel::InstanceDesc normalInstance;
		normalInstance.transform = transform_;
		normalInstance.color = color;
		normalInstance.isVisible = true;

		InstancedModel::InstanceDesc outlineInstance = normalInstance;
		outlineInstance.isVisible = false;

		uint32_t normalHandle = normalBatch->AddInstance(normalInstance);
		uint32_t outlineHandle = outlineBatch->AddInstance(outlineInstance);
		SetInstancedDraw(normalBatch, normalHandle, outlineBatch, outlineHandle);
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
