#pragma once

class Object
{
public:
	Object();
	~Object();

	virtual void Initialize();

	virtual void Update(float dt);

	virtual void Draw();

private:

};