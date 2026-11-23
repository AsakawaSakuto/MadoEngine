#include "MapDecorationObjectSpawner.h"
#include "DecorationObject/Rock/Rock.h"
#include "DecorationObject/Tree/Tree.h"
#include "Utility/Collider/CollisionFunction.h"
#include "Utility/Logger/Logger.h"
#include "Utility/Random.h"
#include <numbers>
#include <string>
#include <utility>

namespace {
/// @brief 装飾種別ごとの生成関数と描画Batchと配置条件の対応
struct DecorationSpawnType {
	const char* label;
	int MapGenerationSettings::* spawnCount;
	MapDecorationObjectSettings MapGenerationSettings::* settings;
	std::unique_ptr<MapDecorationObjectBase> (*create)(Random& random);
	void (*destroyBatches)();
};

/// @brief 指定した派生クラスの装飾オブジェクトを生成
/// @tparam DecorationObject 生成する装飾オブジェクトの派生クラス
/// @param random 装飾全体で共有する乱数系列
/// @return 基底クラスの所有権として扱う生成済みオブジェクト
template <typename DecorationObject>
std::unique_ptr<MapDecorationObjectBase> CreateDecorationObject(Random& random) {
	(void)random;
	return std::make_unique<DecorationObject>();
}

/// @brief Model名の末尾番号をランダムに選んで岩の装飾オブジェクトを生成
/// @param random 装飾全体で共有する乱数系列
/// @return 基底クラスの所有権として扱う生成済みの岩
std::unique_ptr<MapDecorationObjectBase> CreateRock(Random& random) {
	return std::make_unique<Rock>(random.Int(0, Rock::kModelCount - 1));
}

constexpr DecorationSpawnType kDecorationTypes[] = {
	{ "木", &MapGenerationSettings::treeSpawnCount, &MapGenerationSettings::treeSettings, &CreateDecorationObject<Tree>, &Tree::DestroyInstancedBatches },
	{ "岩", &MapGenerationSettings::rockSpawnCount, &MapGenerationSettings::rockSettings, &CreateRock, &Rock::DestroyInstancedBatches }
};

/// @brief 装飾の配置地点における地表の高さを計算
/// @param block 配置先のBlock
/// @param blockCenter 配置先のBlock中心座標
/// @param blockSize Blockの共通寸法
/// @param position 配置地点のワールド座標
/// @return 配置地点の地表の高さ
float CalculateGroundY(const MapBlock& block, const Vector3& blockCenter,
	const Vector3& blockSize, const Vector3& position) {
	if (block.GetType() != MapBlockType::Slope) {
		return blockSize.y * static_cast<float>(block.GetHeight());
	}

	// 衝突登録を伴わない地形情報の一時値で既存の坂の高さ計算と一致
	Slope slope;
	slope.center = blockCenter;
	slope.min = { -blockSize.x * 0.5f, blockSize.y * static_cast<float>(block.GetHeight()), -blockSize.z * 0.5f };
	slope.max = { blockSize.x * 0.5f, blockSize.y * static_cast<float>(block.GetHeight() + 1), blockSize.z * 0.5f };
	slope.bottomExtendY = slope.min.y;
	slope.direction = block.GetSlopeDirection();
	return Collision::Detail::GetSlopeSurfaceY(slope, position);
}
}

MapDecorationObjectSpawner::MapDecorationObjectSpawner(const MapGenerationSettings& settings,
	const std::vector<std::vector<MapBlock>>& blocks,
	std::vector<std::unique_ptr<MapDecorationObjectBase>>& decorationObjects)
	: settings_(settings), mapBlocks_(blocks), decorationObjects_(decorationObjects) {
}

void MapDecorationObjectSpawner::Generate(bool isVisible) {
	std::size_t totalSpawnCount = 0;

	// 全種別の必要容量を確保し、木だけが0個の場合も岩の生成を継続
	for (const DecorationSpawnType& type : kDecorationTypes) {
		if (settings_.*type.spawnCount > 0) {
			totalSpawnCount += static_cast<std::size_t>(settings_.*type.spawnCount);
		}
	}
	if (totalSpawnCount == 0) {
		return;
	}

	// 配置可能なBlockだけを収集し、空地形での無限再試行と生成数不足を回避
	std::vector<std::pair<std::size_t, std::size_t>> candidates;
	for (std::size_t z = 0; z < mapBlocks_.size(); ++z) {
		for (std::size_t x = 0; x < mapBlocks_[z].size(); ++x) {
			if (mapBlocks_[z][x].GetType() != MapBlockType::Air) {
				candidates.emplace_back(x, z);
			}
		}
	}
	if (candidates.empty()) {
		return;
	}

	// 装飾全体で一つの乱数系列を共有し、配置順に座標・回転・Model番号を抽選
	Random decorationRandom(MyRand::MakeDerivedSeed(settings_.seed, 400));
	decorationObjects_.reserve(decorationObjects_.size() + totalSpawnCount);
	for (const DecorationSpawnType& type : kDecorationTypes) {
		const int spawnCount = settings_.*type.spawnCount;
		if (spawnCount <= 0) {
			continue;
		}
		int createdCount = 0;
		for (int index = 0; index < spawnCount; ++index) {
			const auto [x, z] = candidates[static_cast<std::size_t>(decorationRandom.Int(0, static_cast<int>(candidates.size()) - 1))];
			const MapBlock& block = mapBlocks_[z][x];
			const Vector3 blockCenter = {
				static_cast<float>(x) * settings_.blockSize.x,
				0.0f,
				static_cast<float>(z) * settings_.blockSize.z
			};

			// 地表の高さへ原点を合わせ、木と岩の上方向を坂でも鉛直に維持
			MapDecorationObjectBase::InitializeDesc desc;
			desc.transform.translate = blockCenter;
			desc.transform.translate.x += decorationRandom.Float(-settings_.blockSize.x * 0.4f, settings_.blockSize.x * 0.4f);
			desc.transform.translate.z += decorationRandom.Float(-settings_.blockSize.z * 0.4f, settings_.blockSize.z * 0.4f);
			desc.transform.translate.y = CalculateGroundY(block, blockCenter, settings_.blockSize, desc.transform.translate);
			desc.transform.rotate.y = decorationRandom.Float(0.0f, std::numbers::pi_v<float> * 2.0f);
			desc.isVisible = isVisible;
			desc.settings = settings_.*type.settings;
			auto object = type.create(decorationRandom);
			if (!object->Initialize(desc)) {

				// 同一Assetの失敗を繰り返さず、後続の装飾種別の生成へ移行
				break;
			}
			decorationObjects_.push_back(std::move(object));
			++createdCount;
		}
		Logger::Output("Map : 装飾の" + std::string(type.label) + "を" + std::to_string(createdCount) + "個配置しました", Logger::Level::Application);
	}
}

void MapDecorationObjectSpawner::DestroyInstancedBatches() {

	// 生成時と同じ種別一覧で岩のBatchも破棄し、再生成によるInstance蓄積を防止
	for (const DecorationSpawnType& type : kDecorationTypes) {
		type.destroyBatches();
	}
}
