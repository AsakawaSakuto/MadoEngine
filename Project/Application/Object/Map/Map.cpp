#include "Map.h"

void Map::Initialize() {

	// 各ベクターをmapHeight_ x mapWidth_のサイズで初期化
	mapPositionY_.assign(mapHeight_, std::vector<uint32_t>(mapWidth_, 0));
	mapTranslate_.assign(mapHeight_, std::vector<Vector3>(mapWidth_));
	mapShape_.assign(mapHeight_, std::vector<Shape>(mapWidth_));

	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			mapPositionY_[z][x] = MyRand::GetInt(1, 10);
			mapTranslate_[z][x] = Vector3(x * 5.0f, 0.0f, z * 5.0f);

			AABB blockShape;
			blockShape.min = Vector3(-2.5f, 0.0f, -2.5f);
			blockShape.max = Vector3(2.5f, 2.5f * static_cast<float>(mapPositionY_[z][x]), 2.5f);

			mapShape_[z][x] = blockShape;
			MyCollider::RegisterCollider("x:(" + std::to_string(x) + " )z:(" + std::to_string(z) + ")", CollisionTag::MapBlock, &mapShape_[z][x], &mapTranslate_[z][x], 1.0f);
		}
	}
}

void Map::Update() {

	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			MyDebugLine::AddShape(mapShape_[z][x], Vector4(1.0f, 0.0f, 0.0f, 1.0f));
		}
	}
}