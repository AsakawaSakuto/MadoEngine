#include "Magnet.h"

Magnet::~Magnet() {

	// 取得時とMap再生成時にCollider参照と個別Instanceの表示を解除
	if (!colliderName_.empty()) {
		MyCollider::RemoveCollider(colliderName_);
	}
	HideInstancedDraw();
}

void Magnet::Initialize(const InitializeDesc& desc) {
	SetColliderName(desc.colliderName);
	transform_.translate = desc.position;
	transform_.rotate = desc.rotation;
	colliderShape_ = CreatePlacementCollider(desc.position);
	MyCollider::RegisterCollider(colliderName_, CollisionTag::MapEventObject, &colliderShape_, &transform_.translate, 0.0f);

	const MadoEngine::InstancedModelHandle normalBatchHandle = MyInstancedModel::GetOrCreate(
		"Magnet.Normal",
		"Magnet",
		SceneType::Game,
		MadoEngine::Render::RenderLayer::MapEventObject);
	const MadoEngine::InstancedModelHandle outlineBatchHandle = MyInstancedModel::GetOrCreate(
		"Magnet.Outline",
		"Magnet",
		SceneType::Game,
		MadoEngine::Render::RenderLayer::MapEventObjectOutline);

	InstancedModel* normalBatch = MyInstancedModel::TryGet(normalBatchHandle);
	InstancedModel* outlineBatch = MyInstancedModel::TryGet(outlineBatchHandle);
	if (normalBatch && outlineBatch) {

		// 接触時の強調を描画Instanceの切り替えだけで適用するため同一姿勢を二重登録
		//normalBatch->SetTexture("white16x16");
		//outlineBatch->SetTexture("white16x16");
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

AABB Magnet::CreatePlacementCollider(const Vector3& position) {
	AABB collider;
	collider.center = position;
	collider.min = { -1.0f, 0.0f, -1.0f };
	collider.max = { 1.0f, 4.0f, 1.0f };
	return collider;
}

void Magnet::Update(float deltaTime) {
	(void)deltaTime;
	std::get<AABB>(colliderShape_).center = transform_.translate;
	MyDebugLine::AddShape(std::get<AABB>(colliderShape_), { 0.0f, 0.0f, 0.0f, 1.0f });
}

bool Magnet::Interact(Player::Base& player) {
	(void)player;

	// DropObjectへの操作はSceneへ委譲し、Mapオブジェクトでは取得だけを確定
	return true;
}

std::string_view Magnet::GetInteractionText() const {
	return "経験値を引き寄せる";
}

MapEventRequest Magnet::GetInteractionRequest() const {
	return { MapEventAction::AttractAllExp, GetPosition() };
}
