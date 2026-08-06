#include "MapEventObjectBase.h"
#include "Utility/Collider/MyCollider.h"

bool MapEventObjectBase::IsHitPlayer() const {
	return MyCollider::IsHitWithTag(colliderName_, CollisionTag::PlayerHitBox);
}

void MapEventObjectBase::SetHighlighted(bool isHighlighted) {
	if (isHighlighted_ == isHighlighted) {
		return;
	}

	isHighlighted_ = isHighlighted;

	InstancedModel* normalModel = MyInstancedModel::TryGet(normalInstancedModel_);
	InstancedModel* outlineModel = MyInstancedModel::TryGet(outlineInstancedModel_);
	if (normalModel && outlineModel) {
		normalModel->SetInstanceVisible(normalInstanceHandle_, !isHighlighted);
		outlineModel->SetInstanceVisible(outlineInstanceHandle_, isHighlighted);
		return;
	}

	if (Model* model = MyModel::TryGet(model_)) {
		model->SetRenderLayer(isHighlighted
			? MadoEngine::Render::RenderLayer::MapEventObjectOutline
			: MadoEngine::Render::RenderLayer::MapEventObject);
	}
}

void MapEventObjectBase::SetColliderName(const std::string& colliderName) {
	colliderName_ = colliderName;
}

void MapEventObjectBase::SetInstancedDraw(
	MadoEngine::InstancedModelHandle normalModel,
	uint32_t normalHandle,
	MadoEngine::InstancedModelHandle outlineModel,
	uint32_t outlineHandle) {
	normalInstancedModel_ = normalModel;
	normalInstanceHandle_ = normalHandle;
	outlineInstancedModel_ = outlineModel;
	outlineInstanceHandle_ = outlineHandle;
}

void MapEventObjectBase::HideInstancedDraw() {
	if (InstancedModel* normalModel = MyInstancedModel::TryGet(normalInstancedModel_)) {
		normalModel->SetInstanceVisible(normalInstanceHandle_, false);
	}
	if (InstancedModel* outlineModel = MyInstancedModel::TryGet(outlineInstancedModel_)) {
		outlineModel->SetInstanceVisible(outlineInstanceHandle_, false);
	}
}
