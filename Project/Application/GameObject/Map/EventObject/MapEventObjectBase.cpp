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

	if (normalInstancedModel_ && outlineInstancedModel_) {
		normalInstancedModel_->SetInstanceVisible(normalInstanceHandle_, !isHighlighted);
		outlineInstancedModel_->SetInstanceVisible(outlineInstanceHandle_, isHighlighted);
		return;
	}

	if (model_) {
		model_->SetRenderLayer(isHighlighted
			? MadoEngine::Render::RenderLayer::MapEventObjectOutline
			: MadoEngine::Render::RenderLayer::MapEventObject);
	}
}

void MapEventObjectBase::SetColliderName(const std::string& colliderName) {
	colliderName_ = colliderName;
}

void MapEventObjectBase::SetInstancedDraw(
	InstancedModel* normalModel,
	uint32_t normalHandle,
	InstancedModel* outlineModel,
	uint32_t outlineHandle) {
	normalInstancedModel_ = normalModel;
	normalInstanceHandle_ = normalHandle;
	outlineInstancedModel_ = outlineModel;
	outlineInstanceHandle_ = outlineHandle;
}

void MapEventObjectBase::HideInstancedDraw() {
	if (normalInstancedModel_) {
		normalInstancedModel_->SetInstanceVisible(normalInstanceHandle_, false);
	}
	if (outlineInstancedModel_) {
		outlineInstancedModel_->SetInstanceVisible(outlineInstanceHandle_, false);
	}
}
