#include "Map.h"
#include "Object/Player/Player.h"
#include "Utility/Collider/CollisionFunction.h"

#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI

namespace {

/// @brief 坂の低い側がマップ外周の壁を向いているか判定します。
/// @param x マップ上のX座標です。
/// @param z マップ上のZ座標です。
/// @param mapWidth マップの横幅です。
/// @param mapHeight マップの奥行きです。
/// @param direction 坂の上り方向です。
/// @return 坂の低い側がマップ外周の壁を向いていればtrueです。
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

/// @brief Jarの配置Y座標を計算します。
/// @param block 配置対象のMapBlockです。
/// @param blockCenter 配置対象ブロックの中心座標です。
/// @param blockSize ブロックサイズです。
/// @param spawnPosition Jarの配置予定座標です。
/// @return Jarの配置Y座標です。
float CalculateJarSpawnY(const MapBlock& block, const Vector3& blockCenter, const Vector3& blockSize, const Vector3& spawnPosition) {
	if (block.GetType() != MapBlockType::Slope) {
		return blockSize.y * static_cast<float>(block.GetHeight());
	}

	Slope slope;
	slope.center = blockCenter;
	slope.min = Vector3(-blockSize.x / 2.0f, blockSize.y * static_cast<float>(block.GetHeight()), -blockSize.z / 2.0f);
	slope.max = Vector3(blockSize.x / 2.0f, blockSize.y * static_cast<float>(block.GetHeight() + 1), blockSize.z / 2.0f);
	slope.bottomExtendY = slope.min.y;
	slope.direction = block.GetSlopeDirection();

	return Collision::Detail::GetSlopeSurfaceY(slope, spawnPosition);
}

/// @brief Jarの配置回転を計算します。
/// @param block 配置対象のMapBlockです。
/// @param blockSize ブロックサイズです。
/// @return Jarの配置回転です。
Vector3 CalculateJarSpawnRotation(const MapBlock& block, const Vector3& blockSize) {
	if (block.GetType() != MapBlockType::Slope) {
		return { 0.0f, 0.0f, 0.0f };
	}

	Vector3 rotation = { 0.0f, 0.0f, 0.0f };
	switch (block.GetSlopeDirection()) {
	case SlopeDirection::PulsX:
		rotation.z = std::atan2(blockSize.y, blockSize.x);
		break;
	case SlopeDirection::MinusX:
		rotation.z = -std::atan2(blockSize.y, blockSize.x);
		break;
	case SlopeDirection::PulsZ:
		rotation.x = -std::atan2(blockSize.y, blockSize.z);
		break;
	case SlopeDirection::MinusZ:
		rotation.x = std::atan2(blockSize.y, blockSize.z);
		break;
	}

	return rotation;
}

}

void Map::Initialize() {

	ClampHeightSettings();
	jars_.clear();

	mapBlocks_.assign(mapHeight_, std::vector<MapBlock>(mapWidth_));

	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			if (x == 0 && z == 0) {
				mapBlocks_[z][x].SetHeight(MyRand::GetInt(minStartHeight_, std::max(minStartHeight_, maxStartHeight_ / 2)));
				continue;
			}

			uint32_t baseHeight = 0;
			if (x > 0 && z > 0) {
				baseHeight = (GetBlockHeight(x - 1, z) + GetBlockHeight(x, z - 1)) / 2;
			} else if (x > 0) {
				baseHeight = GetBlockHeight(x - 1, z);
			} else {
				baseHeight = GetBlockHeight(x, z - 1);
			}

			int nextHeight = static_cast<int>(baseHeight) + MyRand::GetInt(minRangeHeight_, maxRangeHeight_);
			mapBlocks_[z][x].SetHeight(static_cast<uint32_t>(std::clamp(nextHeight, minHeight_, maxHeight_)));
		}
	}

	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			uint32_t currentHeight = GetBlockHeight(x, z);
			SlopeDirection slopeDirection = SlopeDirection::PulsX;
			bool useSlope = false;

			if (x + 1 < mapWidth_ && GetBlockHeight(x + 1, z) == currentHeight + 1 &&
				!IsSlopeMinFacingMapWall(x, z, mapWidth_, mapHeight_, SlopeDirection::PulsX)) {
				slopeDirection = SlopeDirection::PulsX;
				useSlope = true;
			} else if (x > 0 && GetBlockHeight(x - 1, z) == currentHeight + 1 &&
				!IsSlopeMinFacingMapWall(x, z, mapWidth_, mapHeight_, SlopeDirection::MinusX)) {
				slopeDirection = SlopeDirection::MinusX;
				useSlope = true;
			} else if (z + 1 < mapHeight_ && GetBlockHeight(x, z + 1) == currentHeight + 1 &&
				!IsSlopeMinFacingMapWall(x, z, mapWidth_, mapHeight_, SlopeDirection::PulsZ)) {
				slopeDirection = SlopeDirection::PulsZ;
				useSlope = true;
			} else if (z > 0 && GetBlockHeight(x, z - 1) == currentHeight + 1 &&
				!IsSlopeMinFacingMapWall(x, z, mapWidth_, mapHeight_, SlopeDirection::MinusZ)) {
				slopeDirection = SlopeDirection::MinusZ;
				useSlope = true;
			}

			if (useSlope && MyRand::GetFloat(0.0f, 1.0f) >= slopeSpawnRate_) {
				useSlope = false;
			}

			MapBlock::InitializeDesc desc;
			desc.x = x;
			desc.z = z;
			desc.mapWidth = mapWidth_;
			desc.height = currentHeight;
			desc.type = useSlope ? MapBlockType::Slope : MapBlockType::Ground;
			desc.slopeDirection = slopeDirection;
			desc.blockSize = blockSize_;
			desc.isModelDraw = isModelDraw_;

			mapBlocks_[z][x].Initialize(desc);
		}
	}

	Logger::Output("Map : 地形を生成しました", Logger::Level::Application);
	GenerateJars();
}

void Map::Update(Player& player) {

	if (MyInput::GetKeybord()->IsTrigger(DIK_F1)) {
		isModelDraw_ = !isModelDraw_;

		for (std::vector<MapBlock>& row : mapBlocks_) {
			for (MapBlock& block : row) {
				block.SetVisible(isModelDraw_);
			}
		}
	}

	for (std::vector<MapBlock>& row : mapBlocks_) {
		for (MapBlock& block : row) {
			block.Update(0.0f);
			block.DrawDebugLine();
		}
	}

	for (std::unique_ptr<Jar>& jar : jars_) {
		jar->Update(0.0f);
	}

	HandleJarInteraction(player);
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

	ImGui::Text("生成する地形の高さ変化幅");
	ImGui::DragInt("min  ", &minRangeHeight_, 1, -10, -1);
	ImGui::DragInt("max  ", &maxRangeHeight_, 1, 1, 10);
	ImGui::Separator();

	ImGui::Text("Slope出現率");
	ImGui::SliderFloat("出現率", &slopeSpawnRate_, 0.0f, 1.0f);

	ImGui::End();

#endif // USE_IMGUI

}

void Map::RegenerateTerrain() {

	Initialize();

	Logger::Output("Map : 地形を再生成しました", Logger::Level::Debug);
}

void Map::GenerateJars() {

	jars_.clear();

	const int maxSpawnCount = jarSpawnCount_;
	if (maxSpawnCount <= 0) {
		return;
	}

	jars_.reserve(static_cast<size_t>(maxSpawnCount));

	int createdCount = 0;
	int retryCount = 0;
	const int maxRetryCount = maxSpawnCount * 20;

	while (createdCount < maxSpawnCount && retryCount < maxRetryCount) {
		++retryCount;

		const int x = MyRand::GetInt(0, mapWidth_ - 1);
		const int z = MyRand::GetInt(0, mapHeight_ - 1);

		MapBlock& spawnBlock = mapBlocks_[z][x];
		if (spawnBlock.GetType() == MapBlockType::Air) {
			continue;
		}

		const float jarHalfSize = 0.5f;
		const float spawnRangeX = std::max(0.0f, blockSize_.x / 2.0f - jarHalfSize);
		const float spawnRangeZ = std::max(0.0f, blockSize_.z / 2.0f - jarHalfSize);
		const float offsetX = MyRand::GetFloat(-spawnRangeX, spawnRangeX);
		const float offsetZ = MyRand::GetFloat(-spawnRangeZ, spawnRangeZ);

		Vector3 spawnPosition = {
			static_cast<float>(x) * blockSize_.x + offsetX,
			0.0f,
			static_cast<float>(z) * blockSize_.z + offsetZ
		};
		Vector3 blockCenter = {
			static_cast<float>(x) * blockSize_.x,
			0.0f,
			static_cast<float>(z) * blockSize_.z
		};
		spawnPosition.y = CalculateJarSpawnY(spawnBlock, blockCenter, blockSize_, spawnPosition);

		Jar::InitializeDesc desc;
		desc.position = spawnPosition;
		desc.rotation = CalculateJarSpawnRotation(spawnBlock, blockSize_);
		desc.modelName = "JarModel_" + std::to_string(createdCount);
		desc.colliderName = "JarAABB_" + std::to_string(createdCount);

		std::unique_ptr<Jar> jar = std::make_unique<Jar>();
		jar->Initialize(desc);
		jars_.push_back(std::move(jar));
		++createdCount;
	}

	Logger::Output("Map : Jarを" + std::to_string(createdCount) + "個配置しました", Logger::Level::Application);
}

void Map::HandleJarInteraction(Player& player) {
	if (!MyInput::Trigger("Interact")) {
		return;
	}

	for (auto it = jars_.begin(); it != jars_.end(); ++it) {
		if (!(*it)->IsHitPlayer()) {
			continue;
		}

		player.AddMoney(10);
		jars_.erase(it);
		Logger::Output("Jarを回収しました。所持金を10加算しました。", Logger::Level::Application);
		return;
	}
}

void Map::ClampHeightSettings() {

	minHeight_ = std::clamp(minHeight_, 1, 100);
	maxHeight_ = std::clamp(maxHeight_, minHeight_, 100);

	minStartHeight_ = std::clamp(minStartHeight_, 1, 100);
	maxStartHeight_ = std::clamp(maxStartHeight_, minStartHeight_, 100);

	minRangeHeight_ = std::clamp(minRangeHeight_, -10, -1);
	maxRangeHeight_ = std::clamp(maxRangeHeight_, 1, 10);

	slopeSpawnRate_ = std::clamp(slopeSpawnRate_, 0.0f, 1.0f);
}

uint32_t Map::GetBlockHeight(int x, int z) const {
	return mapBlocks_[z][x].GetHeight();
}
