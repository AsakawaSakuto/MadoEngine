#pragma once
#include "../../../IGameObject.h"
#include "JarType.h"

class Jar : public IGameObject {
public:
	
	void Initialize() override;

	void Update(float deltaTime) override;

private:

};