#include "Map.h"
#include "GameObject/Map/EventObject/BossSpawner/BossSpawner.h"
#include "GameObject/Map/EventObject/Chest/Chest.h"
#include "GameObject/Map/EventObject/Jar/Jar.h"
#include "GameObject/Map/EventObject/Karma/Karma.h"
#include "GameObject/Player/Player.h"
#include "Utility/Collider/CollisionFunction.h"
#include <algorithm>
#include <cmath>
#include <numbers>

#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI

namespace {
constexpr float kRotationEpsilon = 1e-5f;
const Vector4 kInteractionTextDefaultColor = { 1.0f, 1.0f, 1.0f, 1.0f };
const Vector4 kInteractionTextUnavailableColor = { 1.0f, 0.0f, 0.0f, 1.0f };

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
	MyInstancedModel::Destroy("BossSpawner.Normal");
	MyInstancedModel::Destroy("BossSpawner.Outline");
}

}

/// @brief 指定シードでMapを初期化
/// @param seed Map生成に使用するシード値
void Map::Initialize(uint32_t seed) {

	// 地形とイベント配置を独立した乱数系列に分離して生成条件の変更による相互影響を防止
	terrainRandom_.SetSeed(MyRand::MakeDerivedSeed(seed, 100));
	eventObjectRandom_.SetSeed(MyRand::MakeDerivedSeed(seed, 200));
	ClampHeightSettings();
	eventObjects_.clear();
	pendingEventRequests_.clear();
	DestroyMapInstancedBatches();
	currentHitEventObject_ = nullptr;
	interactionMarkerModel_ = MyModel::Find("Interact_x");
	interactionText_ = MyText::Find("InteractText");
	interactionMarkerScaleTimer_.Reset();
	if (Model* interactionMarkerModel = MyModel::TryGet(interactionMarkerModel_)) {
		interactionMarkerModel->SetVisible(false);
	}
	if (MadoEngine::Text* interactionText = MyText::TryGet(interactionText_)) {
		interactionText->SetVisible(false);
	}

	mapBlocks_.assign(mapHeight_, std::vector<MapBlock>(mapWidth_));

	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			if (x == 0 && z == 0) {
				mapBlocks_[z][x].SetHeight(terrainRandom_.Int(minStartHeight_, std::max(minStartHeight_, maxStartHeight_ / 2)));
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

			int nextHeight = static_cast<int>(baseHeight) + terrainRandom_.Int(minRangeHeight_, maxRangeHeight_);
			mapBlocks_[z][x].SetHeight(static_cast<uint32_t>(std::clamp(nextHeight, minHeight_, maxHeight_)));
		}
	}

	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			uint32_t currentHeight = GetBlockHeight(x, z);
			SlopeDirection slopeDirection = SlopeDirection::PulsX;
			bool useSlope = false;

			// 一段高い隣接ブロックへ接続できる方向だけを坂の候補として選択
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

			// 高さ確定後にColliderと描画インスタンスを一括生成
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
	GenerateKarmas();
	GenerateBossSpawner();
}

Vector3 Map::CreatePlayerSpawnGroundPosition(uint32_t seed) const {
	std::vector<Vector3> spawnCandidates;
	spawnCandidates.reserve(static_cast<size_t>(mapWidth_) * static_cast<size_t>(mapHeight_));

	// Slopeを除外してPlayerが水平に接地できる通常Block上面の中心を候補化
	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			const MapBlock& block = mapBlocks_[z][x];
			if (block.GetType() != MapBlockType::Ground) {
				continue;
			}

			const Vector3 spawnGroundPosition = {
				static_cast<float>(x) * blockSize_.x,
				blockSize_.y * static_cast<float>(block.GetHeight()),
				static_cast<float>(z) * blockSize_.z
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

std::vector<MapEventRequest> Map::ConsumeEventRequests() {
	std::vector<MapEventRequest> requests;

	// 同じ相互作用を複数Frameで処理しないよう未処理要求の所有権を呼び出し側へ移動
	requests.swap(pendingEventRequests_);
	return requests;
}

void Map::Update(Player::Base& player, float deltaTime) {

	if (MyInput::GetKeybord()->IsTrigger(DIK_F1)) {
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

	UpdateEventObjects(player, deltaTime);
}

void Map::DrawImGui() {

#ifdef USE_IMGUI

	ImGui::Begin("Map");

	// Editorからの直接入力を地形生成が扱える範囲へ即時補正
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

bool Map::IsEventObjectColliderOverlapping(const AABB& collider) const {

	// 生成前の候補を配置済みの全種類と比較して異種Object間の重複も除外
	for (const std::unique_ptr<MapEventObjectBase>& object : eventObjects_) {
		if (object && object->IsColliderOverlapping(collider)) {
			return true;
		}
	}

	return false;
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

void Map::GenerateJars() {

	// Jar再生成時は既存イベントとハイライト参照を同時に破棄
	eventObjects_.clear();
	currentHitEventObject_ = nullptr;

	const int maxSpawnCount = jarSpawnCount_;
	if (maxSpawnCount <= 0) {
		return;
	}

	eventObjects_.reserve(static_cast<size_t>(maxSpawnCount));
	const AABB jarLocalCollider = Jar::CreatePlacementCollider({});
	const float jarHalfSizeX = std::max(std::abs(jarLocalCollider.min.x), std::abs(jarLocalCollider.max.x));
	const float jarHalfSizeZ = std::max(std::abs(jarLocalCollider.min.z), std::abs(jarLocalCollider.max.z));

	int createdCount = 0;
	int retryCount = 0;
	const int maxRetryCount = maxSpawnCount * 20;

	// 配置不能な地形が多い場合でも無限試行にならない回数で打ち切り
	while (createdCount < maxSpawnCount && retryCount < maxRetryCount) {
		++retryCount;

		const int x = eventObjectRandom_.Int(0, mapWidth_ - 1);
		const int z = eventObjectRandom_.Int(0, mapHeight_ - 1);

		MapBlock& spawnBlock = mapBlocks_[z][x];
		if (spawnBlock.GetType() == MapBlockType::Air) {
			continue;
		}

		// Jarの占有幅を除いたブロック内から配置座標を選択
		const float spawnRangeX = std::max(0.0f, blockSize_.x / 2.0f - jarHalfSizeX);
		const float spawnRangeZ = std::max(0.0f, blockSize_.z / 2.0f - jarHalfSizeZ);
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
		if (IsEventObjectColliderOverlapping(Jar::CreatePlacementCollider(spawnPosition))) {
			continue;
		}

		// 坂では接地面の高さと傾斜へModel姿勢を一致
		Jar::InitializeDesc desc;
		desc.position = spawnPosition;
		desc.rotation = CalculateSpawnRotation(spawnBlock, blockSize_, 0.0f);
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
	const AABB chestLocalCollider = Chest::CreatePlacementCollider({});
	const float chestHalfSizeX = std::max(std::abs(chestLocalCollider.min.x), std::abs(chestLocalCollider.max.x));
	const float chestHalfSizeZ = std::max(std::abs(chestLocalCollider.min.z), std::abs(chestLocalCollider.max.z));

	// すべての通常Chestで同じ費用段階を参照するためMap生成単位の状態を共有
	const std::shared_ptr<Chest::OpenCostState> openCostState = std::make_shared<Chest::OpenCostState>();

	int createdCount = 0;
	int retryCount = 0;
	const int maxRetryCount = maxSpawnCount * 20;

	// 配置不能な地形を考慮しつつ有限回の試行で生成数を確定
	while (createdCount < maxSpawnCount && retryCount < maxRetryCount) {
		++retryCount;

		const int x = eventObjectRandom_.Int(0, mapWidth_ - 1);
		const int z = eventObjectRandom_.Int(0, mapHeight_ - 1);

		MapBlock& spawnBlock = mapBlocks_[z][x];
		if (spawnBlock.GetType() == MapBlockType::Air) {
			continue;
		}

		const float spawnRangeX = std::max(0.0f, blockSize_.x / 2.0f - chestHalfSizeX);
		const float spawnRangeZ = std::max(0.0f, blockSize_.z / 2.0f - chestHalfSizeZ);
		const float offsetX = eventObjectRandom_.Float(-spawnRangeX, spawnRangeX);
		const float offsetZ = eventObjectRandom_.Float(-spawnRangeZ, spawnRangeZ);

		// Chestの占有幅を除いたブロック内から配置座標を選択
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
		if (IsEventObjectColliderOverlapping(Chest::CreatePlacementCollider(spawnPosition))) {
			continue;
		}

		// 水平向きをランダム化しつつ坂の法線へ姿勢を整合
		Chest::InitializeDesc desc;
		desc.position = spawnPosition;
		const float yaw = eventObjectRandom_.Float(0.0f, std::numbers::pi_v<float> * 2.0f);
		desc.rotation = CalculateSpawnRotation(spawnBlock, blockSize_, yaw);
		desc.type = eventObjectRandom_.Int(0, 1) == 0 ? ChestType::Normal : ChestType::Free;
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

void Map::GenerateKarmas() {

	const int maxSpawnCount = karmaSpawnCount_;
	if (maxSpawnCount <= 0) {
		return;
	}

	eventObjects_.reserve(eventObjects_.size() + static_cast<size_t>(maxSpawnCount));
	const AABB karmaLocalCollider = Karma::CreatePlacementCollider({});
	const float karmaHalfSizeX = std::max(std::abs(karmaLocalCollider.min.x), std::abs(karmaLocalCollider.max.x));
	const float karmaHalfSizeZ = std::max(std::abs(karmaLocalCollider.min.z), std::abs(karmaLocalCollider.max.z));

	int createdCount = 0;
	int retryCount = 0;
	const int maxRetryCount = maxSpawnCount * 20;

	// 配置不能な地形が多い場合でも無限試行にならない回数で打ち切り
	while (createdCount < maxSpawnCount && retryCount < maxRetryCount) {
		++retryCount;

		const int x = eventObjectRandom_.Int(0, mapWidth_ - 1);
		const int z = eventObjectRandom_.Int(0, mapHeight_ - 1);

		MapBlock& spawnBlock = mapBlocks_[z][x];
		if (spawnBlock.GetType() == MapBlockType::Air) {
			continue;
		}

		const float spawnRangeX = std::max(0.0f, blockSize_.x / 2.0f - karmaHalfSizeX);
		const float spawnRangeZ = std::max(0.0f, blockSize_.z / 2.0f - karmaHalfSizeZ);
		const float offsetX = eventObjectRandom_.Float(-spawnRangeX, spawnRangeX);
		const float offsetZ = eventObjectRandom_.Float(-spawnRangeZ, spawnRangeZ);

		// Karmaの占有幅を除いたブロック内から配置座標を選択
		Vector3 spawnPosition = {
			static_cast<float>(x) * blockSize_.x + offsetX,
			0.0f,
			static_cast<float>(z) * blockSize_.z + offsetZ
		};
		const Vector3 blockCenter = {
			static_cast<float>(x) * blockSize_.x,
			0.0f,
			static_cast<float>(z) * blockSize_.z
		};
		spawnPosition.y = CalculateSpawnY(spawnBlock, blockCenter, blockSize_, spawnPosition);
		if (IsEventObjectColliderOverlapping(Karma::CreatePlacementCollider(spawnPosition))) {
			continue;
		}

		// 水平向きをランダム化しつつ坂の法線へ姿勢を整合
		Karma::InitializeDesc desc;
		desc.position = spawnPosition;
		const float yaw = eventObjectRandom_.Float(0.0f, std::numbers::pi_v<float> * 2.0f);
		desc.rotation = CalculateSpawnRotation(spawnBlock, blockSize_, yaw);
		desc.colliderName = "KarmaAABB_" + std::to_string(createdCount);

		std::unique_ptr<Karma> karma = std::make_unique<Karma>();
		karma->Initialize(desc);
		eventObjects_.push_back(std::move(karma));
		++createdCount;
	}

	Logger::Output("Map : Karmaを" + std::to_string(createdCount) + "個配置しました", Logger::Level::Application);
}

void Map::GenerateBossSpawner() {
	std::vector<Vector3> spawnCandidates;
	spawnCandidates.reserve(static_cast<size_t>(mapWidth_) * static_cast<size_t>(mapHeight_));

	// 坂を除外し、モデルの原点が接地する通常ブロック上面の中心を候補化
	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			const MapBlock& block = mapBlocks_[z][x];
			if (block.GetType() != MapBlockType::Ground) {
				continue;
			}

			spawnCandidates.push_back({
				static_cast<float>(x) * blockSize_.x,
				blockSize_.y * static_cast<float>(block.GetHeight()),
				static_cast<float>(z) * blockSize_.z
			});
		}
	}

	if (spawnCandidates.empty()) {
		Logger::Output("Map : BossSpawnerを配置できる通常ブロックがありません", Logger::Level::Warning);
		return;
	}


	// 重複候補を除去しながら乱数選択して全候補を有限回で探索
	while (!spawnCandidates.empty()) {
		const int spawnIndex = eventObjectRandom_.Int(0, static_cast<int>(spawnCandidates.size()) - 1);
		const Vector3 spawnPosition = spawnCandidates[static_cast<size_t>(spawnIndex)];
		if (IsEventObjectColliderOverlapping(BossSpawner::CreatePlacementCollider(spawnPosition))) {
			spawnCandidates[static_cast<size_t>(spawnIndex)] = spawnCandidates.back();
			spawnCandidates.pop_back();
			continue;
		}

		BossSpawner::InitializeDesc desc;
		desc.position = spawnPosition;

		std::unique_ptr<BossSpawner> bossSpawner = std::make_unique<BossSpawner>();
		bossSpawner->Initialize(desc);
		eventObjects_.push_back(std::move(bossSpawner));

		Logger::Output("Map : BossSpawnerを1個配置しました", Logger::Level::Application);
		return;
	}

	Logger::Output("Map : 他のイベントオブジェクトと重ならないBossSpawner配置場所がありません", Logger::Level::Warning);
}

void Map::UpdateEventObjects(Player::Base& player, float deltaTime) {
	MapEventObjectBase* hitObject = nullptr;

	// 複数接触時も一つだけを操作対象として選択
	for (std::unique_ptr<MapEventObjectBase>& object : eventObjects_) {
		object->Update(0.0f);

		if (!hitObject && object->IsHitPlayer()) {
			hitObject = object.get();
		}
	}

	if (currentHitEventObject_ != hitObject) {
		interactionMarkerScaleTimer_.Reset();

		// 接触対象が変化したフレームだけOutline表示を切り替え
		if (currentHitEventObject_) {
			currentHitEventObject_->SetHighlighted(false);
		}

		if (hitObject) {
			hitObject->SetHighlighted(true);
		}

		currentHitEventObject_ = hitObject;
	}

	HandleEventObjectInteraction(player);
	UpdateInteractionMarker(deltaTime);
	UpdateInteractionText(player);
}

void Map::HandleEventObjectInteraction(Player::Base& player) {
	if (!currentHitEventObject_ || !MyInput::Trigger("Interact")) {
		return;
	}

	// 相互作用が成立したObjectの要求を保存してから配置一覧から除去
	MapEventObjectBase* interactedObject = currentHitEventObject_;
	if (!interactedObject->Interact(player)) {
		return;
	}

	const MapEventRequest request = interactedObject->GetInteractionRequest();
	if (request.action != MapEventAction::None) {
		pendingEventRequests_.push_back(request);
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

void Map::UpdateInteractionMarker(float deltaTime) {
	Model* interactionMarkerModel = MyModel::TryGet(interactionMarkerModel_);
	if (!interactionMarkerModel) {
		return;
	}

	const bool hasInteractionTarget = currentHitEventObject_ != nullptr;
	interactionMarkerModel->SetVisible(hasInteractionTarget);
	if (!hasInteractionTarget) {
		interactionMarkerScaleTimer_.Reset();
		interactionMarkerModel->SetScale(interactionMarkerStartScale_);
		return;
	}

	// ModelEditorで設定した見た目を基準に、接触対象の上端と傾きへ追従
	interactionMarkerModel->SetPosition(currentHitEventObject_->GetPosition());
	//interactionMarkerModel->SetRotation(currentHitEventObject_->GetRotation());

	// 接触開始から一周期を繰り返すGameTimerで拡縮の位相を管理
	if (!interactionMarkerScaleTimer_.IsActive()) {
		interactionMarkerScaleTimer_.Start(1.0f, true);
	}
	interactionMarkerScaleTimer_.Update(deltaTime);
	interactionMarkerModel->SetScale(Easing::Lerp(interactionMarkerStartScale_, interactionMarkerEndScale_, interactionMarkerScaleTimer_.GetProgress()));
}

void Map::UpdateInteractionText(const Player::Base& player) {
	MadoEngine::Text* interactionText = MyText::TryGet(interactionText_);
	if (!interactionText) {
		return;
	}

	const bool hasInteractionTarget = currentHitEventObject_ != nullptr;
	interactionText->SetVisible(hasInteractionTarget);
	if (!hasInteractionTarget) {
		return;
	}

	// 相互作用できない状態を赤色で通知し、対象変更時は通常色へ復帰
	interactionText->SetColor(currentHitEventObject_->CanInteract(player)
		? kInteractionTextDefaultColor
		: kInteractionTextUnavailableColor);

	// 文言の決定を各MapEventObjectへ委譲してObject固有の操作内容を表示
	interactionText->SetText(std::string(currentHitEventObject_->GetInteractionText()));
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
