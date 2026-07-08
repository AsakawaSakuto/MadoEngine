#pragma once
#include "UtilityHeaders.h"
#include "RenderHeaders.h"
#include "MathHeaders.h"

class IGameObject {
public:
	virtual ~IGameObject() = default;
	
	virtual void Update(float deltaTime) = 0;

protected:

	Model* model_ = nullptr;
	Transform3D transform_;
	ColliderShape colliderShape_;
};