#include "Map.h"
#include "MapEventObjectSpawner.h"
#include "MapDecorationObjectSpawner.h"
#include "MapTerrainPostProcessor.h"
#include "EventObject/BossSpawner/BossSpawner.h"
#include "EventObject/Chest/Chest.h"
#include "EventObject/Jar/Jar.h"
#include "EventObject/Karma/Karma.h"
#include "EventObject/Magnet/Magnet.h"
#include "EventObject/Anvil/Anvil.h"
#include "GameObject/Player/Player.h"
#include <algorithm>
#include <optional>

namespace {
/// @brief Mapで使用するインスタンス描画バッチを破棄
void DestroyMapInstancedBatches() {
	MyInstancedModel::Destroy("MapBlock.Ground");
	MyInstancedModel::Destroy("MapBlock.Slope");
	MyInstancedModel::Destroy("MapBlock.Ground.Occluded");
	MyInstancedModel::Destroy("MapBlock.Slope.Occluded");
	MyInstancedModel::Destroy("Jar.Small.Money.Normal");
	MyInstancedModel::Destroy("Jar.Small.Money.Outline");
	MyInstancedModel::Destroy("Jar.Small.Exp.Normal");
	MyInstancedModel::Destroy("Jar.Small.Exp.Outline");
	MyInstancedModel::Destroy("Jar.Big.Money.Normal");
	MyInstancedModel::Destroy("Jar.Big.Money.Outline");
	MyInstancedModel::Destroy("Jar.Big.Exp.Normal");
	MyInstancedModel::Destroy("Jar.Big.Exp.Outline");
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

void Map::UpdateInteractionGauge(const Camera& camera) {
	interactionController_.UpdateInteractionGauge(camera);
}

void Map::SetEventActionEnabled(MapEventAction action, bool isEnabled) {
	interactionController_.SetEventActionEnabled(action, isEnabled);
}

#ifdef USE_IMGUI
void Map::DrawImGui(const Player::Base* player) {
	editor_.DrawImGui({ appliedSettings_, mapBlocks_, eventObjects_, player, decorationObjects_.size() });
}
#endif // USE_IMGUI

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
	decorationObjects_.clear();
	MapDecorationObjectSpawner::DestroyInstancedBatches();
	DestroyMapInstancedBatches();
	terrainRandom_.SetSeed(MyRand::MakeDerivedSeed(appliedSettings_.seed, 100));

	// 再生成時は登録済みColliderをDestructor経由で解除してから新しい格子を構築
	mapBlocks_.clear();
	mapBlocks_.resize(static_cast<std::size_t>(appliedSettings_.mapHeight));
	for (std::vector<MapBlock>& row : mapBlocks_) {
		row.resize(static_cast<std::size_t>(appliedSettings_.mapWidth));
	}
	MapTerrainPostProcessor::Grid terrain(static_cast<std::size_t>(appliedSettings_.mapHeight),
		std::vector<MapTerrainPostProcessor::Cell>(static_cast<std::size_t>(appliedSettings_.mapWidth)));

	// 高さ変化の抽選順序を維持するため、平均の端数処理には専用の乱数系列を使用
	Random terrainHeightRoundingRandom(MyRand::MakeDerivedSeed(appliedSettings_.seed, 101));
	for (int z = 0; z < appliedSettings_.mapHeight; ++z) {
		for (int x = 0; x < appliedSettings_.mapWidth; ++x) {
			if (x == 0 && z == 0) {
				terrain[z][x].height = static_cast<uint32_t>(terrainRandom_.Int(appliedSettings_.minStartHeight, std::max(appliedSettings_.minStartHeight, appliedSettings_.maxStartHeight / 2)));
				continue;
			}

			// 生成済みの隣接ブロックを基準にして急激な高低差を抑制
			uint32_t baseHeight = 0;
			if (x > 0 && z > 0) {

				// 平均の端数を等確率で上下へ丸め、生成方向に沿った低地への偏りを防止
				const uint32_t neighborHeightSum = terrain[z][x - 1].height + terrain[z - 1][x].height;
				baseHeight = neighborHeightSum / 2;
				if (neighborHeightSum % 2 != 0) {
					baseHeight += static_cast<uint32_t>(terrainHeightRoundingRandom.Int(0, 1));
				}
			} else if (x > 0) {
				baseHeight = terrain[z][x - 1].height;
			} else {
				baseHeight = terrain[z - 1][x].height;
			}

			int nextHeight = static_cast<int>(baseHeight) + terrainRandom_.Int(appliedSettings_.minRangeHeight, appliedSettings_.maxRangeHeight);
			terrain[z][x].height = static_cast<uint32_t>(std::clamp(nextHeight, appliedSettings_.minHeight, appliedSettings_.maxHeight));
		}
	}

	// リソースの生成前に地形だけを補正し、接地位置やミニマップとの不一致を防止
	MapTerrainPostProcessor postProcessor(terrain);
	if (appliedSettings_.terrainCleanupEnabled) {
		postProcessor.SmoothHeights(appliedSettings_.terrainSmoothingPassCount);
		postProcessor.FinalizeTerrain(terrainRandom_, appliedSettings_.slopeSpawnRate);
	} else {
		postProcessor.GenerateSlopes(terrainRandom_, appliedSettings_.slopeSpawnRate, false);
	}

	for (int z = 0; z < appliedSettings_.mapHeight; ++z) {
		for (int x = 0; x < appliedSettings_.mapWidth; ++x) {
			const MapTerrainPostProcessor::Cell& cell = terrain[z][x];

			// 補正済みの高さと坂の接続をColliderと描画インスタンスへ共通適用
			MapBlock::InitializeDesc desc;
			desc.x = x;
			desc.z = z;
			desc.mapWidth = appliedSettings_.mapWidth;
			desc.height = cell.height;
			desc.type = cell.type;
			desc.slopeDirection = cell.slopeDirection;
			desc.blockSize = appliedSettings_.blockSize;
			desc.isModelDraw = isModelDraw_;

			mapBlocks_[z][x].Initialize(desc);
		}
	}

	Logger::Output("Map : 地形を生成しました", Logger::Level::Application);
	MapEventObjectSpawner spawner(appliedSettings_, mapBlocks_, eventObjects_);
	spawner.Generate();
	MapDecorationObjectSpawner decorationSpawner(appliedSettings_, mapBlocks_, decorationObjects_);
	decorationSpawner.Generate(isModelDraw_);
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
		for (const std::unique_ptr<MapDecorationObjectBase>& object : decorationObjects_) {
			object->SetVisible(isModelDraw_);
		}
	}

	for (std::vector<MapBlock>& row : mapBlocks_) {
		for (MapBlock& block : row) {
			block.Update(0.0f);
			block.DrawDebugLine();
		}
	}

	// 装飾の固定AABBもF2のデバッグ表示へ追加し、Editor調整後の判定範囲を確認
	for (const std::unique_ptr<MapDecorationObjectBase>& object : decorationObjects_) {
		object->DrawDebugLine();
	}

	interactionController_.Update(player, deltaTime, eventObjects_);
}

void Map::UpdateCameraOcclusion(const Vector3& cameraPosition, const Vector3& playerPosition) {
	const Segment sightLine{ cameraPosition, playerPosition - cameraPosition };

	// 最初の遮蔽物より奥も判定し、前方の地形を消した後の再遮蔽を防止
	for (std::vector<MapBlock>& row : mapBlocks_) {
		for (MapBlock& block : row) {
			block.UpdateCameraOcclusion(sightLine);
		}
	}

	// 地形と同じ最終Camera座標で装飾も判定し、PauseやCamera切り替え時の表示を同期
	for (const std::unique_ptr<MapDecorationObjectBase>& object : decorationObjects_) {
		object->UpdateCameraOcclusion(sightLine);
	}
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

MapMiniMapData Map::CreateMiniMapData() const {
	MapMiniMapData data;
	data.limit = CreateMapLimit();
	data.width = static_cast<std::uint32_t>(appliedSettings_.mapWidth);
	data.depth = static_cast<std::uint32_t>(appliedSettings_.mapHeight);
	data.cells.reserve(static_cast<std::size_t>(data.width) * data.depth);
	bool hasTerrainHeight = false;

	// 描画側へMapBlockの所有権や参照を渡さず、生成時だけ必要な値を複製
	for (const std::vector<MapBlock>& row : mapBlocks_) {
		for (const MapBlock& block : row) {
			const MapMiniMapCell cell{ block.GetType(), block.GetSlopeDirection(), block.GetHeight() };
			data.cells.push_back(cell);

			// 空セルを高度範囲から除外し、実際に表示する地形の最低値と最高値を補間の基準に使用
			if (cell.type == MapBlockType::Air) {
				continue;
			}
			if (!hasTerrainHeight) {
				data.minHeight = cell.height;
				data.maxHeight = cell.height;
				hasTerrainHeight = true;
			} else {
				data.minHeight = (std::min)(data.minHeight, cell.height);
				data.maxHeight = (std::max)(data.maxHeight, cell.height);
			}
		}
	}
	CollectMiniMapObjectPositions(data.objectPositions);
	return data;
}

void Map::CollectMiniMapObjectPositions(MapMiniMapObjectPositions& objectPositions) const {
	for (std::vector<Vector3>& positions : objectPositions) {
		positions.clear();
	}

	// 一度の走査で種別ごとに残存Objectだけを収集し、開封済みや使用済みの対象を表示から除外
	for (const std::unique_ptr<MapEventObjectBase>& object : eventObjects_) {
		if (!object) {
			continue;
		}
		MapMiniMapObjectType type = MapMiniMapObjectType::Count;
		if (dynamic_cast<const BossSpawner*>(object.get())) {
			type = MapMiniMapObjectType::BossSpawner;
		} else if (const Chest* chest = dynamic_cast<const Chest*>(object.get())) {
			type = MapMiniMapObjectType::Chest;

			// 無料Chestを専用アイコンで識別できるよう通常Chestと座標一覧を分離
			if (chest->GetType() == ChestType::Free) {
				type = MapMiniMapObjectType::FreeChest;
			}
		} else if (dynamic_cast<const Jar*>(object.get())) {
			type = MapMiniMapObjectType::Jar;
		} else if (dynamic_cast<const Karma*>(object.get())) {
			type = MapMiniMapObjectType::Karma;
		} else if (dynamic_cast<const Magnet*>(object.get())) {
			type = MapMiniMapObjectType::Magnet;
		} else if (dynamic_cast<const Anvil*>(object.get())) {
			type = MapMiniMapObjectType::Anvil;
		} else {
			continue;
		}
		objectPositions[static_cast<std::size_t>(type)].push_back(object->GetPosition());
	}
}

bool Map::IsPlayerSpawnBlocked(const Sphere& collider) const {

	// 種類を問わず、配置済みのEventObjectに重なる位置をPlayerの初期配置候補から除外
	for (const std::unique_ptr<MapEventObjectBase>& object : eventObjects_) {
		if (object && object->IsColliderOverlapping(collider)) {
			return true;
		}
	}

	return false;
}
