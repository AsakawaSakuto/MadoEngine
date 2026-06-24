#include "Map.h"
#include "Object/Map/EventObject/Chest/Chest.h"
#include "Object/Map/EventObject/Jar/Jar.h"
#include "Object/Player/Player.h"
#include "Utility/Collider/CollisionFunction.h"
#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI

namespace {

/// @brief 低い側がMap外周の壁を向いている坂か判定します。
/// @param x Map上のX座標です。
/// @param z Map上のZ座標です。
/// @param mapWidth Mapの横幅です。
/// @param mapHeight Mapの奥行きです。
/// @param direction 坂の上り方向です。
/// @return 低い側がMap外周の壁を向いていればtrueを返します。
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
/// @param spawnPosition 配置予定座標です。
/// @return 配置Y座標です。
float CalculateSpawnY(const MapBlock& block, const Vector3& blockCenter, const Vector3& blockSize, const Vector3& spawnPosition) {
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

/// @brief 配置回転を計算します。
/// @param block 配置対象のMapBlockです。
/// @param blockSize ブロックサイズです。
/// @return Jarの配置回転です。
Vector3 CalculateSpawnRotation(const MapBlock& block, const Vector3& blockSize) {
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

/// @brief 指定シードでMapを初期化します。
/// @param seed Map生成に使用するシード値です。
void Map::Initialize(uint32_t seed) {

	seed_ = seed;
	terrainRandom_.SetSeed(MyRand::MakeDerivedSeed(seed_, 100));
	eventObjectRandom_.SetSeed(MyRand::MakeDerivedSeed(seed_, 200));
	ClampHeightSettings();
	eventObjects_.clear();
	currentHitEventObject_ = nullptr;

	mapBlocks_.assign(mapHeight_, std::vector<MapBlock>(mapWidth_));

	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			if (x == 0 && z == 0) {
				mapBlocks_[z][x].SetHeight(terrainRandom_.Int(minStartHeight_, std::max(minStartHeight_, maxStartHeight_ / 2)));
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

			int nextHeight = static_cast<int>(baseHeight) + terrainRandom_.Int(minRangeHeight_, maxRangeHeight_);
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

			if (useSlope && terrainRandom_.Float(0.0f, 1.0f) >= slopeSpawnRate_) {
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
	GenerateChests();
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

	UpdateEventObjects(player);
}

void Map::DrawImGui() {

#ifdef USE_IMGUI

	ImGui::Begin("Map");

	ClampHeightSettings();

	ImGui::Separator();

	ImGui::Text("Block Size");
	ImGui::DragFloat3(".", &blockSize_.x, 0.1f);
	ImGui::Separator();

	ImGui::Text("Mapの高さ");
	ImGui::DragInt("min", &minHeight_, 1, 1, 100);
	ImGui::DragInt("max", &maxHeight_, 1, 2, 100);
	ImGui::Separator();

	ImGui::Text("初期生成する地形の高さ");
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

/// @brief Map生成に使用したシード値を取得します。
/// @return uint32_t Map生成に使用したシード値です。
uint32_t Map::GetSeed() const {
	return seed_;
}

void Map::GenerateJars() {

	eventObjects_.clear();
	currentHitEventObject_ = nullptr;

	const int maxSpawnCount = jarSpawnCount_;
	if (maxSpawnCount <= 0) {
		return;
	}

	eventObjects_.reserve(static_cast<size_t>(maxSpawnCount));

	int createdCount = 0;
	int retryCount = 0;
	const int maxRetryCount = maxSpawnCount * 20;

	while (createdCount < maxSpawnCount && retryCount < maxRetryCount) {
		++retryCount;

		const int x = eventObjectRandom_.Int(0, mapWidth_ - 1);
		const int z = eventObjectRandom_.Int(0, mapHeight_ - 1);

		MapBlock& spawnBlock = mapBlocks_[z][x];
		if (spawnBlock.GetType() == MapBlockType::Air) {
			continue;
		}

		const float jarHalfSize = 0.5f;
		const float spawnRangeX = std::max(0.0f, blockSize_.x / 2.0f - jarHalfSize);
		const float spawnRangeZ = std::max(0.0f, blockSize_.z / 2.0f - jarHalfSize);
		const float offsetX = eventObjectRandom_.Float(-spawnRangeX, spawnRangeX);
		const float offsetZ = eventObjectRandom_.Float(-spawnRangeZ, spawnRangeZ);

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
		spawnPosition.y = CalculateSpawnY(spawnBlock, blockCenter, blockSize_, spawnPosition);

		Jar::InitializeDesc desc;
		desc.position = spawnPosition;
		desc.rotation = CalculateSpawnRotation(spawnBlock, blockSize_);
		desc.type = eventObjectRandom_.Int(0, 1) == 0 ? JarType::Money : JarType::Exp;
		desc.size = eventObjectRandom_.Int(0, 1) == 0 ? JarSize::Small : JarSize::Big;
		desc.modelName = "JarModel_" + std::to_string(createdCount);
		desc.colliderName = "JarAABB_" + std::to_string(createdCount);

		std::unique_ptr<Jar> jar = std::make_unique<Jar>();
		jar->Initialize(desc);
		eventObjects_.push_back(std::move(jar));
		++createdCount;
	}

	Logger::Output("Map : Jarを" + std::to_string(createdCount) + "個配置しました", Logger::Level::Application);
}

void Map::GenerateChests() {

	const int maxSpawnCount = chestSpawnCount_;
	if (maxSpawnCount <= 0) {
		return;
	}

	eventObjects_.reserve(eventObjects_.size() + static_cast<size_t>(maxSpawnCount));

	int createdCount = 0;
	int retryCount = 0;
	const int maxRetryCount = maxSpawnCount * 20;

	while (createdCount < maxSpawnCount && retryCount < maxRetryCount) {
		++retryCount;

		const int x = eventObjectRandom_.Int(0, mapWidth_ - 1);
		const int z = eventObjectRandom_.Int(0, mapHeight_ - 1);

		MapBlock& spawnBlock = mapBlocks_[z][x];
		if (spawnBlock.GetType() == MapBlockType::Air) {
			continue;
		}

		const float chestHalfSize = 0.6f;
		const float spawnRangeX = std::max(0.0f, blockSize_.x / 2.0f - chestHalfSize);
		const float spawnRangeZ = std::max(0.0f, blockSize_.z / 2.0f - chestHalfSize);
		const float offsetX = eventObjectRandom_.Float(-spawnRangeX, spawnRangeX);
		const float offsetZ = eventObjectRandom_.Float(-spawnRangeZ, spawnRangeZ);

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
		spawnPosition.y = CalculateSpawnY(spawnBlock, blockCenter, blockSize_, spawnPosition);

		Chest::InitializeDesc desc;
		desc.position = spawnPosition;
		desc.rotation = CalculateSpawnRotation(spawnBlock, blockSize_);
		desc.rotation.y = eventObjectRandom_.Float(0.0f, 360.0f);
		desc.type = eventObjectRandom_.Int(0, 1) == 0 ? ChestType::Normal : ChestType::Free;
		desc.modelName = "ChestModel_" + std::to_string(createdCount);
		desc.colliderName = "ChestAABB_" + std::to_string(createdCount);

		std::unique_ptr<Chest> chest = std::make_unique<Chest>();
		chest->Initialize(desc);
		eventObjects_.push_back(std::move(chest));
		++createdCount;
	}

	Logger::Output("Map : Chestを" + std::to_string(createdCount) + "個配置しました", Logger::Level::Application);
}

void Map::UpdateEventObjects(Player& player) {
	MapEventObjectBase* hitObject = nullptr;

	for (std::unique_ptr<MapEventObjectBase>& object : eventObjects_) {
		object->Update(0.0f);

		if (!hitObject && object->IsHitPlayer()) {
			hitObject = object.get();
		}
	}

	if (currentHitEventObject_ != hitObject) {
		if (currentHitEventObject_) {
			currentHitEventObject_->SetHighlighted(false);
		}

		if (hitObject) {
			hitObject->SetHighlighted(true);
		}

		currentHitEventObject_ = hitObject;
	}

	HandleEventObjectInteraction(player);
}

void Map::HandleEventObjectInteraction(Player& player) {
	if (!currentHitEventObject_ || !MyInput::Trigger("Interact")) {
		return;
	}

	MapEventObjectBase* interactedObject = currentHitEventObject_;
	if (!interactedObject->Interact(player)) {
		return;
	}

	auto it = std::find_if(eventObjects_.begin(), eventObjects_.end(), [interactedObject](const std::unique_ptr<MapEventObjectBase>& object) {
		return object.get() == interactedObject;
	});

	if (it != eventObjects_.end()) {
		(*it)->SetHighlighted(false);
		eventObjects_.erase(it);
	}

	currentHitEventObject_ = nullptr;
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
