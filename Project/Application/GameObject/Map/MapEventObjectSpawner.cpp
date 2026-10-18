#include "MapEventObjectSpawner.h"
#include "EventObject/Anvil/Anvil.h"
#include "EventObject/BossSpawner/BossSpawner.h"
#include "EventObject/Chest/Chest.h"
#include "EventObject/Jar/Jar.h"
#include "EventObject/Karma/Karma.h"
#include "EventObject/Magnet/Magnet.h"
#include "Utility/Collider/CollisionFunction.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace {
constexpr float kRotationEpsilon = 1e-5f;

/// @brief 長さがある場合は正規化し、短すぎる場合は代替ベクトルを返却
/// @param value 正規化するベクトル
/// @param fallback 代替ベクトル
/// @return 正規化済みのベクトル
Vector3 NormalizeOrFallback(const Vector3& value, const Vector3& fallback) {
	const float lengthSq = value.LengthSq();
	if (lengthSq < kRotationEpsilon) {
		return fallback;
	}

	return value * (1.0f / std::sqrt(lengthSq));
}

/// @brief 水平Yawから前方向ベクトルを作成
/// @param yaw 水平Yaw角度
/// @return 水平面上の前方向
Vector3 CreateHorizontalForward(float yaw) {
	return { std::sin(yaw), 0.0f, std::cos(yaw) };
}

/// @brief 水平Yawから右方向ベクトルを作成
/// @param yaw 水平Yaw角度
/// @return 水平面上の右方向
Vector3 CreateHorizontalRight(float yaw) {
	return { std::cos(yaw), 0.0f, -std::sin(yaw) };
}

/// @brief 回転行列の各軸からMakeAffineと同じ順序のEuler角を復元
/// @param right ローカルX軸のワールド方向
/// @param up ローカルY軸のワールド方向
/// @param forward ローカルZ軸のワールド方向
/// @return 復元したEuler角
Vector3 ExtractEulerXYZ(const Vector3& right, const Vector3& up, const Vector3& forward) {
	Vector3 euler = {};
	const float sinY = std::clamp(-right.z, -1.0f, 1.0f);
	euler.y = std::asin(sinY);

	const float cosY = std::cos(euler.y);
	if (std::abs(cosY) > kRotationEpsilon) {
		euler.x = std::atan2(up.z, forward.z);
		euler.z = std::atan2(right.y, right.x);
		return euler;
	}

	// ジンバルロック付近ではZ回転を固定して一意な姿勢へ収束
	euler.x = std::atan2(up.x * sinY, up.y);
	euler.z = 0.0f;
	return euler;
}

/// @brief Slope法線と水平向きに沿ったModel回転を作成
/// @param yaw Modelの水平Yaw角度
/// @param slopeNormal Slope上面の法線
/// @return Slopeに沿ったModel回転
Vector3 CreateSlopeAlignedRotation(float yaw, const Vector3& slopeNormal) {
	const Vector3 up = NormalizeOrFallback(slopeNormal, { 0.0f, 1.0f, 0.0f });
	const Vector3 desiredForward = CreateHorizontalForward(yaw);

	// 水平方向を斜面へ射影してModelの前方向を斜面上に拘束
	Vector3 forward = desiredForward - up * Math::Dot(desiredForward, up);
	if (forward.LengthSq() < kRotationEpsilon) {

		// 前方向と法線が平行に近い場合は右方向から安定した前方向を再構築
		forward = Math::Cross(CreateHorizontalRight(yaw), up);
	}
	forward = NormalizeOrFallback(forward, { 0.0f, 0.0f, 1.0f });

	Vector3 right = NormalizeOrFallback(Math::Cross(up, forward), CreateHorizontalRight(yaw));
	forward = NormalizeOrFallback(Math::Cross(right, up), forward);

	return ExtractEulerXYZ(right, up, forward);
}

/// @brief MapObjectの配置Y座標を計算
/// @param block 配置対象のMapBlock
/// @param blockCenter 配置対象ブロックの中心座標
/// @param blockSize ブロックサイズ
/// @param spawnPosition 配置予定座標
/// @return 配置Y座標
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

/// @brief MapObjectの配置回転を計算
/// @param block 配置対象のMapBlock
/// @param blockSize ブロックサイズ
/// @param yaw Modelの水平Yaw角度
/// @return MapObjectの配置回転
Vector3 CalculateSpawnRotation(const MapBlock& block, const Vector3& blockSize, float yaw) {
	if (block.GetType() != MapBlockType::Slope) {
		return { 0.0f, yaw, 0.0f };
	}

	Slope slope;
	slope.min = Vector3(-blockSize.x / 2.0f, 0.0f, -blockSize.z / 2.0f);
	slope.max = Vector3(blockSize.x / 2.0f, blockSize.y, blockSize.z / 2.0f);
	slope.bottomExtendY = slope.min.y;
	slope.direction = block.GetSlopeDirection();

	return CreateSlopeAlignedRotation(yaw, Collision::Detail::GetSlopeTopNormal(slope));
}
}

MapEventObjectSpawner::MapEventObjectSpawner(const MapGenerationSettings& settings,
	const std::vector<std::vector<MapBlock>>& blocks,
	std::vector<std::unique_ptr<MapEventObjectBase>>& eventObjects)
	: settings_(settings), mapBlocks_(blocks), eventObjects_(eventObjects) {
}

void MapEventObjectSpawner::Generate() {

	// 地形と独立した乱数系列と従来の配置順で同一シードの再現性を維持
	eventObjectRandom_.SetSeed(MyRand::MakeDerivedSeed(settings_.seed, 200));
	GenerateJars();
	GenerateChests();
	GenerateKarmas();
	GenerateBossSpawner();
	GenerateMagnets();
	GenerateAnvils();
}

bool MapEventObjectSpawner::TryCreateSpawnPosition(const AABB& localCollider, Vector3& outPosition, const MapBlock*& outBlock) {
	const int x = eventObjectRandom_.Int(0, settings_.mapWidth - 1);
	const int z = eventObjectRandom_.Int(0, settings_.mapHeight - 1);
	outBlock = &mapBlocks_[z][x];
	if (outBlock->GetType() == MapBlockType::Air) {
		return false;
	}

	// 種類ごとの占有幅を差し引いてBlock内へ配置し、乱数の消費順序も維持
	const float halfSizeX = std::max(std::abs(localCollider.min.x), std::abs(localCollider.max.x));
	const float halfSizeZ = std::max(std::abs(localCollider.min.z), std::abs(localCollider.max.z));
	const float spawnRangeX = std::max(0.0f, settings_.blockSize.x / 2.0f - halfSizeX);
	const float spawnRangeZ = std::max(0.0f, settings_.blockSize.z / 2.0f - halfSizeZ);
	const float offsetX = eventObjectRandom_.Float(-spawnRangeX, spawnRangeX);
	const float offsetZ = eventObjectRandom_.Float(-spawnRangeZ, spawnRangeZ);
	outPosition = {
		static_cast<float>(x) * settings_.blockSize.x + offsetX,
		0.0f,
		static_cast<float>(z) * settings_.blockSize.z + offsetZ
	};

	// 平地と坂で同じ候補選択を使い、接地面の高さだけ地形に合わせて補正
	const Vector3 blockCenter = {
		static_cast<float>(x) * settings_.blockSize.x,
		0.0f,
		static_cast<float>(z) * settings_.blockSize.z
	};
	outPosition.y = CalculateSpawnY(*outBlock, blockCenter, settings_.blockSize, outPosition);
	return true;
}

bool MapEventObjectSpawner::IsEventObjectColliderOverlapping(const AABB& collider) const {

	// 生成前の候補を配置済みの全種類と比較して異種Object間の重複も除外
	for (const std::unique_ptr<MapEventObjectBase>& object : eventObjects_) {
		if (object && object->IsColliderOverlapping(collider)) {
			return true;
		}
	}

	return false;
}

void MapEventObjectSpawner::GenerateJars() {

	const int maxSpawnCount = settings_.jarSpawnCount;
	if (maxSpawnCount <= 0) {
		return;
	}

	eventObjects_.reserve(static_cast<size_t>(maxSpawnCount));
	const AABB jarLocalCollider = Jar::CreatePlacementCollider({});
	std::vector<JarType> jarTypes;
	jarTypes.reserve(static_cast<std::size_t>(maxSpawnCount));
	jarTypes.insert(jarTypes.end(), static_cast<std::size_t>(settings_.moneyJarSpawnCount), JarType::Money);
	jarTypes.insert(jarTypes.end(), static_cast<std::size_t>(settings_.expJarSpawnCount), JarType::Exp);

	// タイプ別の正確な総数を保ったまま配置順だけをシード依存でランダム化
	for (int index = maxSpawnCount - 1; index > 0; --index) {
		const int swapIndex = eventObjectRandom_.Int(0, index);
		std::swap(jarTypes[static_cast<std::size_t>(index)], jarTypes[static_cast<std::size_t>(swapIndex)]);
	}

	int createdCount = 0;
	int retryCount = 0;
	const int maxRetryCount = maxSpawnCount * 20;

	// 配置不能な地形が多い場合でも無限試行にならない回数で打ち切り
	while (createdCount < maxSpawnCount && retryCount < maxRetryCount) {
		++retryCount;

		const MapBlock* spawnBlock = nullptr;
		Vector3 spawnPosition;
		if (!TryCreateSpawnPosition(jarLocalCollider, spawnPosition, spawnBlock)) {
			continue;
		}
		if (IsEventObjectColliderOverlapping(Jar::CreatePlacementCollider(spawnPosition))) {
			continue;
		}

		// 坂では接地面の高さと傾斜へModel姿勢を一致
		Jar::InitializeDesc desc;
		desc.position = spawnPosition;
		desc.rotation = CalculateSpawnRotation(*spawnBlock, settings_.blockSize, 0.0f);
		desc.type = jarTypes[static_cast<std::size_t>(createdCount)];
		if (eventObjectRandom_.Int(0, 1) == 0) {
			desc.size = JarSize::Small;
		} else {
			desc.size = JarSize::Big;
		}
		desc.modelName = "JarModel_" + std::to_string(createdCount);
		desc.colliderName = "JarAABB_" + std::to_string(createdCount);

		std::unique_ptr<Jar> jar = std::make_unique<Jar>();
		jar->Initialize(desc);
		eventObjects_.push_back(std::move(jar));
		++createdCount;
	}

	Logger::Output("Map : Jarを" + std::to_string(createdCount) + "個配置しました", Logger::Level::Application);
}

void MapEventObjectSpawner::GenerateChests() {

	const int maxSpawnCount = settings_.chestSpawnCount;
	if (maxSpawnCount <= 0) {
		return;
	}

	eventObjects_.reserve(eventObjects_.size() + static_cast<size_t>(maxSpawnCount));
	const AABB chestLocalCollider = Chest::CreatePlacementCollider({});
	std::vector<ChestType> chestTypes;
	chestTypes.reserve(static_cast<std::size_t>(maxSpawnCount));
	chestTypes.insert(chestTypes.end(), static_cast<std::size_t>(settings_.normalChestSpawnCount), ChestType::Normal);
	chestTypes.insert(chestTypes.end(), static_cast<std::size_t>(settings_.freeChestSpawnCount), ChestType::Free);

	// タイプ別の正確な総数を保ったまま配置順だけをシード依存でランダム化
	for (int index = maxSpawnCount - 1; index > 0; --index) {
		const int swapIndex = eventObjectRandom_.Int(0, index);
		std::swap(chestTypes[static_cast<std::size_t>(index)], chestTypes[static_cast<std::size_t>(swapIndex)]);
	}

	// すべての通常Chestで同じ費用段階を参照するためMap生成単位の状態を共有
	const std::shared_ptr<Chest::OpenCostState> openCostState = std::make_shared<Chest::OpenCostState>();

	int createdCount = 0;
	int retryCount = 0;
	const int maxRetryCount = maxSpawnCount * 20;

	// 配置不能な地形を考慮しつつ有限回の試行で生成数を確定
	while (createdCount < maxSpawnCount && retryCount < maxRetryCount) {
		++retryCount;

		const MapBlock* spawnBlock = nullptr;
		Vector3 spawnPosition;
		if (!TryCreateSpawnPosition(chestLocalCollider, spawnPosition, spawnBlock)) {
			continue;
		}
		if (IsEventObjectColliderOverlapping(Chest::CreatePlacementCollider(spawnPosition))) {
			continue;
		}

		// 水平向きをランダム化しつつ坂の法線へ姿勢を整合
		Chest::InitializeDesc desc;
		desc.position = spawnPosition;
		const float yaw = eventObjectRandom_.Float(0.0f, std::numbers::pi_v<float> * 2.0f);
		desc.rotation = CalculateSpawnRotation(*spawnBlock, settings_.blockSize, yaw);
		desc.type = chestTypes[static_cast<std::size_t>(createdCount)];
		desc.openCostState = openCostState;
		desc.modelName = "ChestModel_" + std::to_string(createdCount);
		desc.colliderName = "ChestAABB_" + std::to_string(createdCount);

		std::unique_ptr<Chest> chest = std::make_unique<Chest>();
		chest->Initialize(desc);
		eventObjects_.push_back(std::move(chest));
		++createdCount;
	}

	Logger::Output("Map : Chestを" + std::to_string(createdCount) + "個配置しました", Logger::Level::Application);
}

void MapEventObjectSpawner::GenerateKarmas() {

	const int maxSpawnCount = settings_.karmaSpawnCount;
	if (maxSpawnCount <= 0) {
		return;
	}

	eventObjects_.reserve(eventObjects_.size() + static_cast<size_t>(maxSpawnCount));
	const AABB karmaLocalCollider = Karma::CreatePlacementCollider({});

	int createdCount = 0;
	int retryCount = 0;
	const int maxRetryCount = maxSpawnCount * 20;

	// 配置不能な地形が多い場合でも無限試行にならない回数で打ち切り
	while (createdCount < maxSpawnCount && retryCount < maxRetryCount) {
		++retryCount;

		const MapBlock* spawnBlock = nullptr;
		Vector3 spawnPosition;
		if (!TryCreateSpawnPosition(karmaLocalCollider, spawnPosition, spawnBlock)) {
			continue;
		}
		if (IsEventObjectColliderOverlapping(Karma::CreatePlacementCollider(spawnPosition))) {
			continue;
		}

		// 水平向きをランダム化しつつ坂の法線へ姿勢を整合
		Karma::InitializeDesc desc;
		desc.position = spawnPosition;
		const float yaw = eventObjectRandom_.Float(0.0f, std::numbers::pi_v<float> * 2.0f);
		desc.rotation = CalculateSpawnRotation(*spawnBlock, settings_.blockSize, yaw);
		desc.colliderName = "KarmaAABB_" + std::to_string(createdCount);

		std::unique_ptr<Karma> karma = std::make_unique<Karma>();
		karma->Initialize(desc);
		eventObjects_.push_back(std::move(karma));
		++createdCount;
	}

	Logger::Output("Map : Karmaを" + std::to_string(createdCount) + "個配置しました", Logger::Level::Application);
}

void MapEventObjectSpawner::GenerateBossSpawner() {
	if (settings_.bossSpawnerSpawnCount <= 0) {
		return;
	}

	std::vector<Vector3> spawnCandidates;
	spawnCandidates.reserve(static_cast<size_t>(settings_.mapWidth) * static_cast<size_t>(settings_.mapHeight));

	// 坂を除外し、モデルの原点が接地する通常ブロック上面の中心を候補化
	for (int z = 0; z < settings_.mapHeight; ++z) {
		for (int x = 0; x < settings_.mapWidth; ++x) {
			const MapBlock& block = mapBlocks_[z][x];
			if (block.GetType() != MapBlockType::Ground) {
				continue;
			}

			spawnCandidates.push_back({
				static_cast<float>(x) * settings_.blockSize.x,
				settings_.blockSize.y * static_cast<float>(block.GetHeight()),
				static_cast<float>(z) * settings_.blockSize.z
			});
		}
	}

	if (spawnCandidates.empty()) {
		Logger::Output("Map : BossSpawnerを配置できる通常ブロックがありません", Logger::Level::Warning);
		return;
	}

	int createdCount = 0;

	// 各通常ブロックを一度だけ探索し、要求数到達か候補枯渇まで配置
	while (createdCount < settings_.bossSpawnerSpawnCount && !spawnCandidates.empty()) {
		const int spawnIndex = eventObjectRandom_.Int(0, static_cast<int>(spawnCandidates.size()) - 1);
		const Vector3 spawnPosition = spawnCandidates[static_cast<size_t>(spawnIndex)];
		spawnCandidates[static_cast<size_t>(spawnIndex)] = spawnCandidates.back();
		spawnCandidates.pop_back();
		if (IsEventObjectColliderOverlapping(BossSpawner::CreatePlacementCollider(spawnPosition))) {
			continue;
		}

		BossSpawner::InitializeDesc desc;
		desc.position = spawnPosition;
		desc.colliderName = "BossSpawnerAABB_" + std::to_string(createdCount);

		std::unique_ptr<BossSpawner> bossSpawner = std::make_unique<BossSpawner>();
		bossSpawner->Initialize(desc);
		eventObjects_.push_back(std::move(bossSpawner));
		++createdCount;
	}

	Logger::Output("Map : BossSpawnerを" + std::to_string(createdCount) + "個配置しました", Logger::Level::Application);
	if (createdCount < settings_.bossSpawnerSpawnCount) {
		Logger::Output("Map : BossSpawnerの配置可能数が設定数を下回りました", Logger::Level::Warning);
	}
}

void MapEventObjectSpawner::GenerateMagnets() {
	if (settings_.magnetSpawnCount <= 0) {
		return;
	}

	eventObjects_.reserve(eventObjects_.size() + static_cast<std::size_t>(settings_.magnetSpawnCount));
	const AABB localCollider = Magnet::CreatePlacementCollider({});
	int createdCount = 0;
	int retryCount = 0;
	const int maxRetryCount = settings_.magnetSpawnCount * 20;

	// 空き場所が不足するMapでも無限試行にならないよう配置回数を制限
	while (createdCount < settings_.magnetSpawnCount && retryCount < maxRetryCount) {
		++retryCount;
		const MapBlock* spawnBlock = nullptr;
		Vector3 position;
		if (!TryCreateSpawnPosition(localCollider, position, spawnBlock)) {
			continue;
		}
		if (IsEventObjectColliderOverlapping(Magnet::CreatePlacementCollider(position))) {
			continue;
		}

		// 既存イベントとの非重複を確認後にColliderと描画Instanceを登録
		Magnet::InitializeDesc desc;
		desc.position = position;
		const float yaw = eventObjectRandom_.Float(0.0f, std::numbers::pi_v<float> * 2.0f);
		desc.rotation = CalculateSpawnRotation(*spawnBlock, settings_.blockSize, yaw);
		desc.colliderName = "MagnetAABB_" + std::to_string(createdCount);
		auto magnet = std::make_unique<Magnet>();
		magnet->Initialize(desc);
		eventObjects_.push_back(std::move(magnet));
		++createdCount;
	}

	Logger::Output("Map : Magnetを" + std::to_string(createdCount) + "個配置しました", Logger::Level::Application);
}

void MapEventObjectSpawner::GenerateAnvils() {
	if (settings_.anvilSpawnCount <= 0) {
		return;
	}

	eventObjects_.reserve(eventObjects_.size() + static_cast<std::size_t>(settings_.anvilSpawnCount));
	const AABB localCollider = Anvil::CreatePlacementCollider({});
	int createdCount = 0;
	int retryCount = 0;
	const int maxRetryCount = settings_.anvilSpawnCount * 20;

	// 空き場所が不足するMapでも配置処理が終了するよう試行回数を制限
	while (createdCount < settings_.anvilSpawnCount && retryCount < maxRetryCount) {
		++retryCount;
		const MapBlock* spawnBlock = nullptr;
		Vector3 position;
		if (!TryCreateSpawnPosition(localCollider, position, spawnBlock)) {
			continue;
		}
		if (IsEventObjectColliderOverlapping(Anvil::CreatePlacementCollider(position))) {
			continue;
		}

		// 既存イベントとの非重複を確認後にColliderと描画Instanceを登録
		Anvil::InitializeDesc desc;
		desc.position = position;
		const float yaw = eventObjectRandom_.Float(0.0f, std::numbers::pi_v<float> * 2.0f);
		desc.rotation = CalculateSpawnRotation(*spawnBlock, settings_.blockSize, yaw);
		desc.colliderName = "AnvilAABB_" + std::to_string(createdCount);
		auto anvil = std::make_unique<Anvil>();
		anvil->Initialize(desc);
		eventObjects_.push_back(std::move(anvil));
		++createdCount;
	}

	Logger::Output("Map : Anvilを" + std::to_string(createdCount) + "個配置しました", Logger::Level::Application);
	if (createdCount < settings_.anvilSpawnCount) {

		// 重複回避によって設定数を満たせない場合も生成済みの配置を維持
		Logger::Output("Map : Anvilの配置可能数が設定数を下回りました", Logger::Level::Warning);
	}
}
