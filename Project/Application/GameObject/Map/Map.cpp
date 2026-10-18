#include "Map.h"
#include "MapEventObjectSpawner.h"
#include "GameObject/Player/Player.h"
#include <algorithm>
#include <optional>

namespace {
/// @brief 低い側がMap外周の壁を向いている坂か判定
/// @param x Map上のX座標
/// @param z Map上のZ座標
/// @param mapWidth Mapの横幅
/// @param mapHeight Mapの奥行き
/// @param direction 坂の上り方向
/// @return 低い側がMap外周の壁を向いていればtrue
bool IsSlopeMinFacingMapWall(int x, int z, int mapWidth, int mapHeight, SlopeDirection direction) {

	// 上り方向と反対側の低端が外周へ接する組み合わせを判定
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

/// @brief Mapで使用するインスタンス描画バッチを破棄
void DestroyMapInstancedBatches() {
	MyInstancedModel::Destroy("MapBlock.Ground");
	MyInstancedModel::Destroy("MapBlock.Slope");
	MyInstancedModel::Destroy("Jar.Small.Normal");
	MyInstancedModel::Destroy("Jar.Small.Outline");
	MyInstancedModel::Destroy("Jar.Big.Normal");
	MyInstancedModel::Destroy("Jar.Big.Outline");
	MyInstancedModel::Destroy("Chest.Normal");
	MyInstancedModel::Destroy("Chest.Outline");
	MyInstancedModel::Destroy("Karma.Normal");
	MyInstancedModel::Destroy("Karma.Outline");
	MyInstancedModel::Destroy("Magnet.Normal");
	MyInstancedModel::Destroy("Magnet.Outline");
	MyInstancedModel::Destroy("Anvil.Normal");
	MyInstancedModel::Destroy("Anvil.Outline");
	MyInstancedModel::Destroy("BossSpawner.Normal");
	MyInstancedModel::Destroy("BossSpawner.Outline");
}
}

void Map::Initialize(uint32_t seed) {
	interactionController_.Initialize();
	editor_.Initialize(seed, appliedSettings_);
	Generate(editor_.GetSettings());
}

void Map::SetInteractionTextVisible(bool isVisible) {
	interactionController_.SetInteractionTextVisible(isVisible);
}

void Map::SetEventActionEnabled(MapEventAction action, bool isEnabled) {
	interactionController_.SetEventActionEnabled(action, isEnabled);
}

void Map::DrawImGui(const Player::Base* player) {
	editor_.DrawImGui({ appliedSettings_, mapBlocks_, eventObjects_, player });
}

bool Map::ApplyPendingEditorGeneration() {
	const std::optional<MapGenerationSettings> settings = editor_.ConsumePendingGeneration();
	if (!settings) {
		return false;
	}

	// Editor描画中のResource破棄を避けるため呼び出し側が決めた時点で生成
	Generate(*settings);
	return true;
}

std::string Map::CaptureEditorState() const {
	return editor_.CaptureState(appliedSettings_);
}

void Map::RestoreEditorState(const std::string& snapshot) {
	editor_.RestoreState(snapshot, appliedSettings_);
}

bool Map::SaveEditorSettings() const {
	return editor_.SaveSettings();
}

bool Map::ReloadEditorSettings() {
	return editor_.ReloadSettings();
}

std::vector<MapEventRequest> Map::ConsumeEventRequests() {
	return interactionController_.ConsumeEventRequests();
}

std::vector<MapInteractionEvent> Map::ConsumeInteractionEvents() {
	return interactionController_.ConsumeInteractionEvents();
}

void Map::Generate(const MapGenerationSettings& settings) {
	appliedSettings_ = settings;
	appliedSettings_.ClampForGeneration();

	// 借用中の接触参照を解除してからObjectと共有描画バッチを破棄
	interactionController_.Reset();
	eventObjects_.clear();
	DestroyMapInstancedBatches();
	terrainRandom_.SetSeed(MyRand::MakeDerivedSeed(appliedSettings_.seed, 100));

	// 再生成時は登録済みColliderをDestructor経由で解除してから新しい格子を構築
	mapBlocks_.clear();
	mapBlocks_.resize(static_cast<std::size_t>(appliedSettings_.mapHeight));
	for (std::vector<MapBlock>& row : mapBlocks_) {
		row.resize(static_cast<std::size_t>(appliedSettings_.mapWidth));
	}

	for (int z = 0; z < appliedSettings_.mapHeight; ++z) {
		for (int x = 0; x < appliedSettings_.mapWidth; ++x) {
			if (x == 0 && z == 0) {
				mapBlocks_[z][x].SetHeight(terrainRandom_.Int(appliedSettings_.minStartHeight, std::max(appliedSettings_.minStartHeight, appliedSettings_.maxStartHeight / 2)));
				continue;
			}

			// 生成済みの隣接ブロックを基準にして急激な高低差を抑制
			uint32_t baseHeight = 0;
			if (x > 0 && z > 0) {
				baseHeight = (GetBlockHeight(x - 1, z) + GetBlockHeight(x, z - 1)) / 2;
			} else if (x > 0) {
				baseHeight = GetBlockHeight(x - 1, z);
			} else {
				baseHeight = GetBlockHeight(x, z - 1);
			}

			int nextHeight = static_cast<int>(baseHeight) + terrainRandom_.Int(appliedSettings_.minRangeHeight, appliedSettings_.maxRangeHeight);
			mapBlocks_[z][x].SetHeight(static_cast<uint32_t>(std::clamp(nextHeight, appliedSettings_.minHeight, appliedSettings_.maxHeight)));
		}
	}

	for (int z = 0; z < appliedSettings_.mapHeight; ++z) {
		for (int x = 0; x < appliedSettings_.mapWidth; ++x) {
			uint32_t currentHeight = GetBlockHeight(x, z);
			SlopeDirection slopeDirection = SlopeDirection::PulsX;
			bool useSlope = false;

			// 一段高い隣接ブロックへ接続できる方向だけを坂の候補として選択
			if (x + 1 < appliedSettings_.mapWidth && GetBlockHeight(x + 1, z) == currentHeight + 1 &&
				!IsSlopeMinFacingMapWall(x, z, appliedSettings_.mapWidth, appliedSettings_.mapHeight, SlopeDirection::PulsX)) {
				slopeDirection = SlopeDirection::PulsX;
				useSlope = true;
			} else if (x > 0 && GetBlockHeight(x - 1, z) == currentHeight + 1 &&
				!IsSlopeMinFacingMapWall(x, z, appliedSettings_.mapWidth, appliedSettings_.mapHeight, SlopeDirection::MinusX)) {
				slopeDirection = SlopeDirection::MinusX;
				useSlope = true;
			} else if (z + 1 < appliedSettings_.mapHeight && GetBlockHeight(x, z + 1) == currentHeight + 1 &&
				!IsSlopeMinFacingMapWall(x, z, appliedSettings_.mapWidth, appliedSettings_.mapHeight, SlopeDirection::PulsZ)) {
				slopeDirection = SlopeDirection::PulsZ;
				useSlope = true;
			} else if (z > 0 && GetBlockHeight(x, z - 1) == currentHeight + 1 &&
				!IsSlopeMinFacingMapWall(x, z, appliedSettings_.mapWidth, appliedSettings_.mapHeight, SlopeDirection::MinusZ)) {
				slopeDirection = SlopeDirection::MinusZ;
				useSlope = true;
			}

			if (useSlope && terrainRandom_.Float(0.0f, 1.0f) >= appliedSettings_.slopeSpawnRate) {
				useSlope = false;
			}

			// 高さ確定後にColliderと描画インスタンスを一括生成
			MapBlock::InitializeDesc desc;
			desc.x = x;
			desc.z = z;
			desc.mapWidth = appliedSettings_.mapWidth;
			desc.height = currentHeight;
			if (useSlope) {
				desc.type = MapBlockType::Slope;
			} else {
				desc.type = MapBlockType::Ground;
			}
			desc.slopeDirection = slopeDirection;
			desc.blockSize = appliedSettings_.blockSize;
			desc.isModelDraw = isModelDraw_;

			mapBlocks_[z][x].Initialize(desc);
		}
	}

	Logger::Output("Map : 地形を生成しました", Logger::Level::Application);
	MapEventObjectSpawner spawner(appliedSettings_, mapBlocks_, eventObjects_);
	spawner.Generate();
}

Vector3 Map::CreatePlayerSpawnGroundPosition(uint32_t seed) const {
	std::vector<Vector3> spawnCandidates;
	spawnCandidates.reserve(static_cast<size_t>(appliedSettings_.mapWidth) * static_cast<size_t>(appliedSettings_.mapHeight));

	// Slopeを除外してPlayerが水平に接地できる通常Block上面の中心を候補化
	for (int z = 0; z < appliedSettings_.mapHeight; ++z) {
		for (int x = 0; x < appliedSettings_.mapWidth; ++x) {
			const MapBlock& block = mapBlocks_[z][x];
			if (block.GetType() != MapBlockType::Ground) {
				continue;
			}

			const Vector3 spawnGroundPosition = {
				static_cast<float>(x) * appliedSettings_.blockSize.x,
				appliedSettings_.blockSize.y * static_cast<float>(block.GetHeight()),
				static_cast<float>(z) * appliedSettings_.blockSize.z
			};
			const Sphere playerSpawnCollider = Player::Base::CreateSpawnMovementCollider(spawnGroundPosition);
			if (IsPlayerSpawnBlocked(playerSpawnCollider)) {
				continue;
			}

			spawnCandidates.push_back(spawnGroundPosition);
		}
	}

	if (spawnCandidates.empty()) {
		Logger::Output("Map : Playerを配置できる通常Blockがありません", Logger::Level::Warning);
		return {};
	}

	// 他用途の乱数消費に影響されない専用系列から配置Blockを選択
	Random playerSpawnRandom(MyRand::MakeDerivedSeed(seed, 300));
	const int spawnIndex = playerSpawnRandom.Int(0, static_cast<int>(spawnCandidates.size()) - 1);
	return spawnCandidates[static_cast<size_t>(spawnIndex)];
}

void Map::Update(Player::Base& player, float deltaTime) {

	if (MyInput::GetKeyboard()->IsTrigger(DIK_F1)) {
		isModelDraw_ = !isModelDraw_;

		// 共有描画バッチの各インスタンスへ表示状態を同期
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

	interactionController_.Update(player, deltaTime, eventObjects_);
}

MapLimit Map::CreateMapLimit() const {
	MapLimit mapLimit;
	mapLimit.min = { -appliedSettings_.blockSize.x * 0.5f, 0.0f, -appliedSettings_.blockSize.z * 0.5f };
	mapLimit.max = {
		static_cast<float>(appliedSettings_.mapWidth - 1) * appliedSettings_.blockSize.x + appliedSettings_.blockSize.x * 0.5f,
		static_cast<float>(appliedSettings_.maxHeight + 2) * appliedSettings_.blockSize.y,
		static_cast<float>(appliedSettings_.mapHeight - 1) * appliedSettings_.blockSize.z + appliedSettings_.blockSize.z * 0.5f
	};
	return mapLimit;
}

bool Map::IsPlayerSpawnBlocked(const Sphere& collider) const {

	// Player配置を禁止するObjectだけを対象にして他のイベント配置ルールと分離
	for (const std::unique_ptr<MapEventObjectBase>& object : eventObjects_) {
		if (object && object->ShouldBlockPlayerSpawn() && object->IsColliderOverlapping(collider)) {
			return true;
		}
	}

	return false;
}

uint32_t Map::GetBlockHeight(int x, int z) const {
	return mapBlocks_[z][x].GetHeight();
}
