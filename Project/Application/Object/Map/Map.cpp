#include "Map.h"

void Map::Initialize() {

	// 各ベクターをmapHeight_ x mapWidth_のサイズで初期化
	mapPositionY_.assign(mapHeight_, std::vector<uint32_t>(mapWidth_, 0));
	mapTranslate_.assign(mapHeight_, std::vector<Vector3>(mapWidth_));
	mapShape_.assign(mapHeight_, std::vector<Shape>(mapWidth_));
	mapModel_.assign(mapHeight_, std::vector<Model*>(mapWidth_, nullptr));

	Vector3 blockSize = { 10.0f, 2.5f, 10.0f };

	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			mapPositionY_[z][x] = MyRand::GetInt(1, 10);
			mapTranslate_[z][x] = Vector3(x * blockSize.x, 0.0f, z * blockSize.z);

			AABB blockShape;
			blockShape.min = Vector3(-blockSize.x / 2.0f, 0.0f, -blockSize.z / 2.0f);
			blockShape.max = Vector3(blockSize.x / 2.0f, blockSize.y * static_cast<float>(mapPositionY_[z][x]), blockSize.z / 2.0f);

			mapShape_[z][x] = blockShape;
			MyCollider::RegisterCollider("x:(" + std::to_string(x) + " )z:(" + std::to_string(z) + ")", CollisionTag::MapBlock, &mapShape_[z][x], &mapTranslate_[z][x], 1.0f);

			mapModel_[z][x] = MyModel::Create("mapBlockModel" + std::to_string(z * mapWidth_ + x), "block", SceneType::Test);
			mapModel_[z][x]->SetPosition({ mapTranslate_[z][x].x ,blockShape.max.y ,mapTranslate_[z][x].z });
			mapModel_[z][x]->SetScale({ blockSize.x / 2.0f, blockSize.y / 2.0f * static_cast<float>(mapPositionY_[z][x]), blockSize.z / 2.0f });
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