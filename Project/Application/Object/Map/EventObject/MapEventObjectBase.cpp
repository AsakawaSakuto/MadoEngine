#include "MapEventObjectBase.h"
#include "Utility/Collider/MyCollider.h"

bool MapEventObjectBase::IsHitPlayer() const {
	return MyCollider::IsHitWithTag(colliderName_, CollisionTag::PlayerHitBox);
}

void MapEventObjectBase::SetHighlighted(bool isHighlighted) {
	if (isHighlighted_ == isHighlighted || !model_) {
		return;
	}

	isHighlighted_ = isHighlighted;
	model_->SetRenderLayer(isHighlighted
		? MadoEngine::Render::RenderLayer::MapEventObjectOutline
		: MadoEngine::Render::RenderLayer::MapEventObject);
}

void MapEventObjectBase::SetColliderName(const std::string& colliderName) {
	colliderName_ = colliderName;
}
