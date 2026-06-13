#pragma once

class IGameObject
{
public:
	IGameObject();
	virtual ~IGameObject();

	virtual void Initialize();

	virtual void Update(float dt);

private:

};