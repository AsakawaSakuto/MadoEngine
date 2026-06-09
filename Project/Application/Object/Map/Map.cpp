#include "Map.h"

#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI

namespace {

std::string GetMapBlockName(int x, int z) {
	return "MapBlock_x:" + std::to_string(x) + "_z:" + std::to_string(z);
}

std::string GetMapSlopeName(int x, int z) {
	return "MapSlope_x:" + std::to_string(x) + "_z:" + std::to_string(z);
}

std::string GetMapModelName(int x, int z, int mapWidth) {
	return "mapBlockModel" + std::to_string(z * mapWidth + x);
}

std::string GetMapSlopeModelName(int x, int z, int mapWidth) {
	return "mapSlopeModel" + std::to_string(z * mapWidth + x);
}

/// @brief Slopeの低い側がMap外周の壁側を向いているか判定する
/// @param x Map上のX座標
/// @param z Map上のZ座標
/// @param mapWidth Mapの横幅
/// @param mapHeight Mapの奥行き
/// @param direction Slopeの上り方向
/// @return Slopeの低い側がMap外周の壁側を向いていればtrue
bool IsSlopeMinFacingMapWall(int x, int z, int mapWidth, int mapHeight, SlopeDirection direction) {
	switch (direction) {
	case SlopeDirection::PulsX:
		return x == 0;
	case SlopeDirection::MinusX:
		return x == mapWidth - 1;
	case SlopeDirection::PulsZ:
		return z == 0;
	case SlopeDirection::MinusZ:
		return z == mapHeight - 1;
	}

	return false;
}

}

void Map::Initialize() {

	ClampHeightSettings();

	mapPositionY_.assign(mapHeight_, std::vector<uint32_t>(mapWidth_, 0));
	mapBlockType_.assign(mapHeight_, std::vector<MapBlockType>(mapWidth_, MapBlockType::Ground));
	mapTranslate_.assign(mapHeight_, std::vector<Vector3>(mapWidth_));
	mapShape_.assign(mapHeight_, std::vector<Shape>(mapWidth_));
	mapModel_.assign(mapHeight_, std::vector<Model*>(mapWidth_, nullptr));
	mapSlopeModel_.assign(mapHeight_, std::vector<Model*>(mapWidth_, nullptr));

	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			if (x == 0 && z == 0) {
				mapPositionY_[z][x] = MyRand::GetInt(minStartHeight_, std::max(minStartHeight_, maxStartHeight_ / 2));
				continue;
			}

			uint32_t baseHeight = 0;
			if (x > 0 && z > 0) {
				baseHeight = (mapPositionY_[z][x - 1] + mapPositionY_[z - 1][x]) / 2;
			} else if (x > 0) {
				baseHeight = mapPositionY_[z][x - 1];
			} else {
				baseHeight = mapPositionY_[z - 1][x];
			}

			int nextHeight = static_cast<int>(baseHeight) + MyRand::GetInt(minRangeHeight_, maxRangeHeight_);
			mapPositionY_[z][x] = static_cast<uint32_t>(std::clamp(nextHeight, static_cast<int>(minHeight_), static_cast<int>(maxHeight_)));
		}
	}

	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			mapTranslate_[z][x] = Vector3(x * blockSize_.x, 0.0f, z * blockSize_.z);

			uint32_t currentHeight = mapPositionY_[z][x];
			SlopeDirection slopeDirection = SlopeDirection::PulsX;
			bool useSlope = false;

			if (x + 1 < mapWidth_ && mapPositionY_[z][x + 1] == currentHeight + 1 &&
				!IsSlopeMinFacingMapWall(x, z, mapWidth_, mapHeight_, SlopeDirection::PulsX)) {
				slopeDirection = SlopeDirection::PulsX;
				useSlope = true;
			} else if (x > 0 && mapPositionY_[z][x - 1] == currentHeight + 1 &&
				!IsSlopeMinFacingMapWall(x, z, mapWidth_, mapHeight_, SlopeDirection::MinusX)) {
				slopeDirection = SlopeDirection::MinusX;
				useSlope = true;
			} else if (z + 1 < mapHeight_ && mapPositionY_[z + 1][x] == currentHeight + 1 &&
				!IsSlopeMinFacingMapWall(x, z, mapWidth_, mapHeight_, SlopeDirection::PulsZ)) {
				slopeDirection = SlopeDirection::PulsZ;
				useSlope = true;
			} else if (z > 0 && mapPositionY_[z - 1][x] == currentHeight + 1 &&
				!IsSlopeMinFacingMapWall(x, z, mapWidth_, mapHeight_, SlopeDirection::MinusZ)) {
				slopeDirection = SlopeDirection::MinusZ;
				useSlope = true;
			}

			if (useSlope) {
				Slope slopeShape;
				slopeShape.min = Vector3(-blockSize_.x / 2.0f, blockSize_.y * static_cast<float>(currentHeight), -blockSize_.z / 2.0f);
				slopeShape.max = Vector3(blockSize_.x / 2.0f, blockSize_.y * static_cast<float>(currentHeight + 1), blockSize_.z / 2.0f);
				slopeShape.bottomExtendY = slopeShape.min.y;
				slopeShape.direction = slopeDirection;

				mapBlockType_[z][x] = MapBlockType::Slope;
				mapShape_[z][x] = slopeShape;
				MyCollider::RegisterCollider(GetMapSlopeName(x, z), CollisionTag::MapSlope, &mapShape_[z][x], &mapTranslate_[z][x], 1.0f);
			} else {
				AABB blockShape;
				blockShape.min = Vector3(-blockSize_.x / 2.0f, 0.0f, -blockSize_.z / 2.0f);
				blockShape.max = Vector3(blockSize_.x / 2.0f, blockSize_.y * static_cast<float>(currentHeight), blockSize_.z / 2.0f);

				mapBlockType_[z][x] = MapBlockType::Ground;
				mapShape_[z][x] = blockShape;
				MyCollider::RegisterCollider(GetMapBlockName(x, z), CollisionTag::MapBlock, &mapShape_[z][x], &mapTranslate_[z][x], 1.0f);
			}

			mapModel_[z][x] = MyModel::Create(GetMapModelName(x, z, mapWidth_), "block", SceneType::Test);
			mapModel_[z][x]->SetPosition({ mapTranslate_[z][x].x, blockSize_.y * static_cast<float>(currentHeight), mapTranslate_[z][x].z });
			mapModel_[z][x]->SetScale({ blockSize_.x / 2.0f, blockSize_.y / 2.0f * static_cast<float>(currentHeight), blockSize_.z / 2.0f });
			mapModel_[z][x]->SetVisible(isModelDraw_);

			if (useSlope) {
				std::string slopeModelName = "SlopePlusX";
				switch (slopeDirection) {
				case SlopeDirection::PulsX:
					slopeModelName = "SlopeMinusX";
					break;
				case SlopeDirection::MinusX:
					slopeModelName = "SlopePlusX";
					break;
				case SlopeDirection::PulsZ:
					slopeModelName = "SlopeMinusZ";
					break;
				case SlopeDirection::MinusZ:
					slopeModelName = "SlopePlusZ";
					break;
				}

				mapSlopeModel_[z][x] = MyModel::Create(GetMapSlopeModelName(x, z, mapWidth_), slopeModelName, SceneType::Test);
				if (mapSlopeModel_[z][x]) {
					mapSlopeModel_[z][x]->SetPosition({ mapTranslate_[z][x].x, blockSize_.y * static_cast<float>(currentHeight + 1), mapTranslate_[z][x].z });
					mapSlopeModel_[z][x]->SetScale({ blockSize_.x / 2.0f, blockSize_.y / 2.0f, blockSize_.z / 2.0f });
					mapSlopeModel_[z][x]->SetVisible(isModelDraw_);
				}
			}
		}
	}

	Logger::Output("Map : 地形を生成しました", Logger::Level::Application);
}

void Map::RegenerateTerrain() {

	ClearTerrainObjects();
	Initialize();

	Logger::Output("Map : 地形を再生成しました", Logger::Level::Debug);
}

void Map::Finalize() {

	ClearTerrainObjects();

	mapPositionY_.clear();
	mapBlockType_.clear();
	mapTranslate_.clear();
	mapShape_.clear();
	mapModel_.clear();
	mapSlopeModel_.clear();

	Logger::Output("Map : 登録済みColliderとModelを破棄しました", Logger::Level::Application);
}

void Map::ClearTerrainObjects() {

	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			if (z < static_cast<int>(mapBlockType_.size()) && x < static_cast<int>(mapBlockType_[z].size())) {
				if (mapBlockType_[z][x] == MapBlockType::Slope) {
					MyCollider::RemoveCollider(GetMapSlopeName(x, z));
				} else if (mapBlockType_[z][x] == MapBlockType::Ground) {
					MyCollider::RemoveCollider(GetMapBlockName(x, z));
				}
			}

			if (z < static_cast<int>(mapModel_.size()) && x < static_cast<int>(mapModel_[z].size()) && mapModel_[z][x]) {
				MyModel::Destroy(GetMapModelName(x, z, mapWidth_));
				mapModel_[z][x] = nullptr;
			}

			if (z < static_cast<int>(mapSlopeModel_.size()) && x < static_cast<int>(mapSlopeModel_[z].size()) && mapSlopeModel_[z][x]) {
				MyModel::Destroy(GetMapSlopeModelName(x, z, mapWidth_));
				mapSlopeModel_[z][x] = nullptr;
			}
		}
	}
}

void Map::ClampHeightSettings() {

	minHeight_ = std::clamp(minHeight_, 1, 100);
	maxHeight_ = std::clamp(maxHeight_, minHeight_, 100);

	minStartHeight_ = std::clamp(minStartHeight_, 1, 100);
	maxStartHeight_ = std::clamp(maxStartHeight_, minStartHeight_, 100);

	minRangeHeight_ = std::clamp(minRangeHeight_, -10, -1);
	maxRangeHeight_ = std::clamp(maxRangeHeight_, 1, 10);
}

void Map::Update() {

	if (MyInput::GetKeybord()->IsTrigger(DIK_1)) {

		if (isModelDraw_) {
			for (int z = 0; z < mapHeight_; ++z) {
				for (int x = 0; x < mapWidth_; ++x) {
					if (mapModel_[z][x]) {
						mapModel_[z][x]->SetVisible(false);
					}
					if (mapSlopeModel_[z][x]) {
						mapSlopeModel_[z][x]->SetVisible(false);
					}
				}
			}
			isModelDraw_ = false;
		} else {
			for (int z = 0; z < mapHeight_; ++z) {
				for (int x = 0; x < mapWidth_; ++x) {
					if (mapModel_[z][x]) {
						mapModel_[z][x]->SetVisible(true);
					}
					if (mapSlopeModel_[z][x]) {
						mapSlopeModel_[z][x]->SetVisible(true);
					}
				}
			}
			isModelDraw_ = true;
		}

	}

	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			Vector4 lineColor = mapBlockType_[z][x] == MapBlockType::Slope ?
				Vector4(0.0f, 1.0f, 0.35f, 1.0f) :
				Vector4(1.0f, 0.0f, 0.0f, 1.0f);
			MyDebugLine::AddShape(mapShape_[z][x], lineColor);
		}
	}
}

void Map::DrawImGui() {

#ifdef USE_IMGUI

	ImGui::Begin("Map");

	ClampHeightSettings();

	if (ImGui::Button("地形を再生成")) {
		RegenerateTerrain();
	}
	ImGui::Separator();

	ImGui::Text("Block Size");
	ImGui::DragFloat3(".", &blockSize_.x, 0.1f);
	ImGui::Separator();
	
	ImGui::Text("マップの高さ");
	ImGui::DragInt("min", &minHeight_, 1, 1, 100);
	ImGui::DragInt("max", &maxHeight_, 1, 2, 100);
	ImGui::Separator();

	ImGui::Text("最初に生成する地形の高さ");
	ImGui::DragInt("min ", &minStartHeight_, 1, 1, 100);
	ImGui::DragInt("max ", &maxStartHeight_, 1, 2, 100);
	ImGui::Separator();

	ImGui::Text("生成する地形の高さの範囲");
	ImGui::DragInt("min  ", &minRangeHeight_, 1, -10, -1);
	ImGui::DragInt("max  ", &maxRangeHeight_, 1, 1, 10);

	ImGui::End();

#endif // USE_IMGUI

}
