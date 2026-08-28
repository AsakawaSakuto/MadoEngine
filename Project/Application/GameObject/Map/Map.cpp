#include "Map.h"
#include "GameObject/Map/EventObject/BossSpawner/BossSpawner.h"
#include "GameObject/Map/EventObject/Chest/Chest.h"
#include "GameObject/Map/EventObject/Jar/Jar.h"
#include "GameObject/Map/EventObject/Karma/Karma.h"
#include "GameObject/Player/Player.h"
#include "Utility/Collider/CollisionFunction.h"
#include "Utility/Json/Core/JsonFile.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <nlohmann/json.hpp>

#ifdef USE_IMGUI
#include "ImGuiHeaders.h"
#endif // USE_IMGUI

namespace {
constexpr float kRotationEpsilon = 1e-5f;
constexpr const char* kMapGeneratorJsonPath = "Assets/Json/MapGenerator.json";
constexpr int kMinimumMapSize = 2;
constexpr int kMaximumMapSize = 128;
constexpr int kMaximumEventObjectCount = 4096;
const Vector4 kInteractionTextDefaultColor = { 1.0f, 1.0f, 1.0f, 1.0f };
const Vector4 kInteractionTextUnavailableColor = { 1.0f, 0.0f, 0.0f, 1.0f };

/// @brief 総数変更後も現在比率を維持して二種類の生成数を再配分
/// @param previousTotal 変更前の総数
/// @param newTotal 変更後の総数
/// @param firstCount 一種類目の生成数
/// @param secondCount 二種類目の生成数
void RebalanceTypeCounts(int previousTotal, int newTotal, int& firstCount, int& secondCount) {
	if (newTotal <= 0) {
		firstCount = 0;
		secondCount = 0;
		return;
	}

	const float firstRatio = previousTotal > 0
		? static_cast<float>(firstCount) / static_cast<float>(previousTotal)
		: 0.5f;
	firstCount = std::clamp(
		static_cast<int>(std::lround(firstRatio * static_cast<float>(newTotal))),
		0,
		newTotal
	);
	secondCount = newTotal - firstCount;
}

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
	editorSettings_ = CreateAppliedSettings();
	editorSettings_.seed = seed;
	LoadEditorSettings(editorSettings_, false);
	editorSettings_.seed = seed;
	ClampGenerationSettings(editorSettings_);
	pendingGenerationSettings_.reset();
	Generate(editorSettings_);
}

void Map::SetInteractionTextVisible(bool isVisible) {
	isInteractionTextVisible_ = isVisible;
	if (MadoEngine::Text* interactionText = MyText::TryGet(interactionText_)) {
		interactionText->SetVisible(isInteractionTextVisible_ && currentHitEventObject_ != nullptr);
	}
}

void Map::Generate(const GenerationSettings& settings) {
	GenerationSettings safeSettings = settings;
	ClampGenerationSettings(safeSettings);
	currentSeed_ = safeSettings.seed;
	mapWidth_ = safeSettings.mapWidth;
	mapHeight_ = safeSettings.mapHeight;
	jarSpawnCount_ = safeSettings.jarSpawnCount;
	moneyJarSpawnCount_ = safeSettings.moneyJarSpawnCount;
	expJarSpawnCount_ = safeSettings.expJarSpawnCount;
	chestSpawnCount_ = safeSettings.chestSpawnCount;
	normalChestSpawnCount_ = safeSettings.normalChestSpawnCount;
	freeChestSpawnCount_ = safeSettings.freeChestSpawnCount;
	karmaSpawnCount_ = safeSettings.karmaSpawnCount;
	bossSpawnerSpawnCount_ = safeSettings.bossSpawnerSpawnCount;
	blockSize_ = safeSettings.blockSize;
	minHeight_ = safeSettings.minHeight;
	maxHeight_ = safeSettings.maxHeight;
	minStartHeight_ = safeSettings.minStartHeight;
	maxStartHeight_ = safeSettings.maxStartHeight;
	minRangeHeight_ = safeSettings.minRangeHeight;
	maxRangeHeight_ = safeSettings.maxRangeHeight;
	slopeSpawnRate_ = safeSettings.slopeSpawnRate;

	// 地形とイベント配置を独立した乱数系列に分離して生成条件の変更による相互影響を防止
	terrainRandom_.SetSeed(MyRand::MakeDerivedSeed(currentSeed_, 100));
	eventObjectRandom_.SetSeed(MyRand::MakeDerivedSeed(currentSeed_, 200));
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


	// 再生成時は登録済みColliderをDestructor経由で解除してから新しい格子を構築
	mapBlocks_.clear();
	mapBlocks_.resize(static_cast<std::size_t>(mapHeight_));
	for (std::vector<MapBlock>& row : mapBlocks_) {
		row.resize(static_cast<std::size_t>(mapWidth_));
	}

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

void Map::DrawImGui(const Player::Base* player) {

#ifdef USE_IMGUI

	ImGui::SetNextWindowSize(ImVec2(520.0f, 720.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Map Generator")) {
		ImGui::TextDisabled("設定ファイル: %s", kMapGeneratorJsonPath);
		if (ImGui::BeginTabBar("MapGeneratorTabs")) {
			if (ImGui::BeginTabItem("MapStatus")) {
				DrawMapStatusEditor();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("EventObjStatus")) {
				DrawEventObjectStatusEditor();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("MapView")) {
				DrawMapViewEditor(player);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		// 非表示タブの設定も含めて相互依存する範囲と内訳を毎Frame補正
		ClampGenerationSettings(editorSettings_);
	}
	ImGui::End();

#endif // USE_IMGUI

}

void Map::DrawMapStatusEditor() {
#ifdef USE_IMGUI
	ImGui::SeparatorText("生成操作");
	ImGui::SetNextItemWidth(180.0f);
	ImGui::InputScalar("シード", ImGuiDataType_U32, &editorSettings_.seed);
	ImGui::SameLine();
	if (ImGui::Button("ランダム")) {
		editorSettings_.seed = MyRand::CreateSeed();
	}

	const bool hasDraftChanges = !AreGenerationSettingsEqual(editorSettings_, CreateAppliedSettings());
	if (ImGui::Button("現在の設定でMapを再生成", ImVec2(-1.0f, 0.0f))) {

		// 描画中のResourceを破棄しないようFrame末尾の適用要求だけを保持
		pendingGenerationSettings_ = editorSettings_;
	}
	if (pendingGenerationSettings_) {
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "再生成を予約済み");
	} else if (hasDraftChanges) {
		ImGui::TextColored(ImVec4(0.25f, 0.75f, 1.0f, 1.0f), "未適用の生成設定あり");
	} else {
		ImGui::TextDisabled("生成済みMapと設定が一致");
	}
	if (ImGui::Button("生成済み設定へ戻す")) {
		editorSettings_ = CreateAppliedSettings();
		pendingGenerationSettings_.reset();
	}

	ImGui::SeparatorText("Mapサイズ");
	ImGui::SetNextItemWidth(180.0f);
	ImGui::DragInt("横幅", &editorSettings_.mapWidth, 1.0f, kMinimumMapSize, kMaximumMapSize, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::SetNextItemWidth(180.0f);
	ImGui::DragInt("奥行き", &editorSettings_.mapHeight, 1.0f, kMinimumMapSize, kMaximumMapSize, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::DragFloat3("Blockサイズ", &editorSettings_.blockSize.x, 0.1f, 0.5f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

	ImGui::SeparatorText("地形の高さ");
	ImGui::DragInt("最小高さ", &editorSettings_.minHeight, 1.0f, 1, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::DragInt("最大高さ", &editorSettings_.maxHeight, 1.0f, 1, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::DragInt("開始高さ Min", &editorSettings_.minStartHeight, 1.0f, 1, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::DragInt("開始高さ Max", &editorSettings_.maxStartHeight, 1.0f, 1, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::DragInt("高さ変化 Min", &editorSettings_.minRangeHeight, 1.0f, -10, 0, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::DragInt("高さ変化 Max", &editorSettings_.maxRangeHeight, 1.0f, 0, 10, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::SliderFloat("Slope出現率", &editorSettings_.slopeSpawnRate, 0.0f, 1.0f, "%.2f");
#endif // USE_IMGUI
}

void Map::DrawEventObjectStatusEditor() {
#ifdef USE_IMGUI
	ImGui::SeparatorText("イベント配置数");
	ImGui::TextUnformatted("Jar");
	ImGui::Indent();
	const int previousJarTotal = editorSettings_.jarSpawnCount;
	if (ImGui::DragInt("総数##Jar", &editorSettings_.jarSpawnCount, 1.0f, 0, kMaximumEventObjectCount, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		RebalanceTypeCounts(
			previousJarTotal,
			editorSettings_.jarSpawnCount,
			editorSettings_.moneyJarSpawnCount,
			editorSettings_.expJarSpawnCount
		);
	}
	if (ImGui::DragInt("Money##Jar", &editorSettings_.moneyJarSpawnCount, 1.0f, 0, editorSettings_.jarSpawnCount, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		editorSettings_.moneyJarSpawnCount = std::clamp(editorSettings_.moneyJarSpawnCount, 0, editorSettings_.jarSpawnCount);
		editorSettings_.expJarSpawnCount = editorSettings_.jarSpawnCount - editorSettings_.moneyJarSpawnCount;
	}
	if (ImGui::DragInt("Exp##Jar", &editorSettings_.expJarSpawnCount, 1.0f, 0, editorSettings_.jarSpawnCount, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		editorSettings_.expJarSpawnCount = std::clamp(editorSettings_.expJarSpawnCount, 0, editorSettings_.jarSpawnCount);
		editorSettings_.moneyJarSpawnCount = editorSettings_.jarSpawnCount - editorSettings_.expJarSpawnCount;
	}
	ImGui::Unindent();

	ImGui::TextUnformatted("Chest");
	ImGui::Indent();
	const int previousChestTotal = editorSettings_.chestSpawnCount;
	if (ImGui::DragInt("総数##Chest", &editorSettings_.chestSpawnCount, 1.0f, 0, kMaximumEventObjectCount, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		RebalanceTypeCounts(
			previousChestTotal,
			editorSettings_.chestSpawnCount,
			editorSettings_.normalChestSpawnCount,
			editorSettings_.freeChestSpawnCount
		);
	}
	if (ImGui::DragInt("Normal##Chest", &editorSettings_.normalChestSpawnCount, 1.0f, 0, editorSettings_.chestSpawnCount, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		editorSettings_.normalChestSpawnCount = std::clamp(editorSettings_.normalChestSpawnCount, 0, editorSettings_.chestSpawnCount);
		editorSettings_.freeChestSpawnCount = editorSettings_.chestSpawnCount - editorSettings_.normalChestSpawnCount;
	}
	if (ImGui::DragInt("Free##Chest", &editorSettings_.freeChestSpawnCount, 1.0f, 0, editorSettings_.chestSpawnCount, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		editorSettings_.freeChestSpawnCount = std::clamp(editorSettings_.freeChestSpawnCount, 0, editorSettings_.chestSpawnCount);
		editorSettings_.normalChestSpawnCount = editorSettings_.chestSpawnCount - editorSettings_.freeChestSpawnCount;
	}
	ImGui::Unindent();

	ImGui::TextUnformatted("Karma");
	ImGui::Indent();
	ImGui::DragInt("総数##Karma", &editorSettings_.karmaSpawnCount, 1.0f, 0, kMaximumEventObjectCount, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::Unindent();

	ImGui::TextUnformatted("BossSpawner");
	ImGui::Indent();
	ImGui::DragInt("総数##BossSpawner", &editorSettings_.bossSpawnerSpawnCount, 1.0f, 0, kMaximumEventObjectCount, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::Unindent();
#endif // USE_IMGUI
}

void Map::DrawMapViewEditor(const Player::Base* player) const {
#ifdef USE_IMGUI
	ImGui::SeparatorText("生成済みMap");
	std::size_t slopeCount = 0;
	for (const std::vector<MapBlock>& row : mapBlocks_) {
		slopeCount += static_cast<std::size_t>(std::count_if(row.begin(), row.end(), [](const MapBlock& block) {
			return block.GetType() == MapBlockType::Slope;
		}));
	}
	ImGui::Text("シード: %u", currentSeed_);
	ImGui::Text("Block: %d x %d = %d", mapWidth_, mapHeight_, mapWidth_ * mapHeight_);
	ImGui::Text("Slope: %zu / Event Object: %zu", slopeCount, eventObjects_.size());
	ImGui::Text("Jar: %d  Money: %d  Exp: %d", jarSpawnCount_, moneyJarSpawnCount_, expJarSpawnCount_);
	ImGui::Text("Chest: %d  Normal: %d  Free: %d", chestSpawnCount_, normalChestSpawnCount_, freeChestSpawnCount_);
	ImGui::Text("Karma: %d  BossSpawner: %d", karmaSpawnCount_, bossSpawnerSpawnCount_);
	if (player) {
		const Vector3 playerPosition = player->GetPosition();
		ImGui::Text("Player: X %.2f  Y %.2f  Z %.2f", playerPosition.x, playerPosition.y, playerPosition.z);
	}
	DrawHeightPreview(player);
#else
	(void)player;
#endif // USE_IMGUI
}

bool Map::ApplyPendingEditorGeneration() {
	if (!pendingGenerationSettings_) {
		return false;
	}

	// Undo復元中も生成要求時点の設定を確実に適用するため値を退避してから予約を解除
	const GenerationSettings settings = *pendingGenerationSettings_;
	pendingGenerationSettings_.reset();
	Generate(settings);
	return true;
}

std::string Map::CaptureEditorState() const {
	nlohmann::json root;
	root["editing"] = GenerationSettingsToJson(editorSettings_);
	root["applied"] = GenerationSettingsToJson(CreateAppliedSettings());
	if (pendingGenerationSettings_) {
		root["pending"] = GenerationSettingsToJson(*pendingGenerationSettings_);
	}
	return root.dump();
}

void Map::RestoreEditorState(const std::string& snapshot) {
	const nlohmann::json root = nlohmann::json::parse(snapshot, nullptr, false);
	if (root.is_discarded() || !root.is_object()) {
		return;
	}

	GenerationSettings editingSettings = editorSettings_;
	const auto editingIt = root.find("editing");
	if (editingIt == root.end() || !GenerationSettingsFromJson(*editingIt, editingSettings)) {
		return;
	}

	GenerationSettings appliedSettings = CreateAppliedSettings();
	const auto appliedIt = root.find("applied");
	if (appliedIt == root.end() || !GenerationSettingsFromJson(*appliedIt, appliedSettings)) {
		return;
	}

	// 編集値と生成済み状態を分離して再生成操作のUndoでも直前の地形を復元
	editorSettings_ = editingSettings;
	const auto pendingIt = root.find("pending");
	if (pendingIt != root.end()) {
		GenerationSettings pendingSettings = appliedSettings;
		if (GenerationSettingsFromJson(*pendingIt, pendingSettings)) {
			pendingGenerationSettings_ = pendingSettings;
		}
	} else if (!AreGenerationSettingsEqual(appliedSettings, CreateAppliedSettings())) {
		pendingGenerationSettings_ = appliedSettings;
	} else {
		pendingGenerationSettings_.reset();
	}
}

bool Map::SaveEditorSettings() const {
	nlohmann::json root;
	root["formatVersion"] = 2;
	root["settings"] = GenerationSettingsToJson(editorSettings_);
	return MadoEngine::Json::JsonFile::Save(kMapGeneratorJsonPath, root, 4, true);
}

bool Map::ReloadEditorSettings() {
	GenerationSettings settings = editorSettings_;
	if (!LoadEditorSettings(settings, true)) {
		return false;
	}

	editorSettings_ = settings;
	pendingGenerationSettings_ = settings;
	return true;
}

MapLimit Map::CreateMapLimit() const {
	MapLimit mapLimit;
	mapLimit.min = { -blockSize_.x * 0.5f, 0.0f, -blockSize_.z * 0.5f };
	mapLimit.max = {
		static_cast<float>(mapWidth_ - 1) * blockSize_.x + blockSize_.x * 0.5f,
		static_cast<float>(maxHeight_ + 2) * blockSize_.y,
		static_cast<float>(mapHeight_ - 1) * blockSize_.z + blockSize_.z * 0.5f
	};
	return mapLimit;
}

Map::GenerationSettings Map::CreateAppliedSettings() const {
	GenerationSettings settings;
	settings.seed = currentSeed_;
	settings.mapWidth = mapWidth_;
	settings.mapHeight = mapHeight_;
	settings.jarSpawnCount = jarSpawnCount_;
	settings.moneyJarSpawnCount = moneyJarSpawnCount_;
	settings.expJarSpawnCount = expJarSpawnCount_;
	settings.chestSpawnCount = chestSpawnCount_;
	settings.normalChestSpawnCount = normalChestSpawnCount_;
	settings.freeChestSpawnCount = freeChestSpawnCount_;
	settings.karmaSpawnCount = karmaSpawnCount_;
	settings.bossSpawnerSpawnCount = bossSpawnerSpawnCount_;
	settings.blockSize = blockSize_;
	settings.minHeight = minHeight_;
	settings.maxHeight = maxHeight_;
	settings.minStartHeight = minStartHeight_;
	settings.maxStartHeight = maxStartHeight_;
	settings.minRangeHeight = minRangeHeight_;
	settings.maxRangeHeight = maxRangeHeight_;
	settings.slopeSpawnRate = slopeSpawnRate_;
	return settings;
}

void Map::ClampGenerationSettings(GenerationSettings& settings) {
	settings.mapWidth = std::clamp(settings.mapWidth, kMinimumMapSize, kMaximumMapSize);
	settings.mapHeight = std::clamp(settings.mapHeight, kMinimumMapSize, kMaximumMapSize);
	settings.jarSpawnCount = std::clamp(settings.jarSpawnCount, 0, kMaximumEventObjectCount);
	settings.moneyJarSpawnCount = std::clamp(settings.moneyJarSpawnCount, 0, settings.jarSpawnCount);
	settings.expJarSpawnCount = settings.jarSpawnCount - settings.moneyJarSpawnCount;
	settings.chestSpawnCount = std::clamp(settings.chestSpawnCount, 0, kMaximumEventObjectCount);
	settings.normalChestSpawnCount = std::clamp(settings.normalChestSpawnCount, 0, settings.chestSpawnCount);
	settings.freeChestSpawnCount = settings.chestSpawnCount - settings.normalChestSpawnCount;
	settings.karmaSpawnCount = std::clamp(settings.karmaSpawnCount, 0, kMaximumEventObjectCount);
	settings.bossSpawnerSpawnCount = std::clamp(settings.bossSpawnerSpawnCount, 0, kMaximumEventObjectCount);
	settings.blockSize.x = std::clamp(std::isfinite(settings.blockSize.x) ? settings.blockSize.x : 15.0f, 0.5f, 100.0f);
	settings.blockSize.y = std::clamp(std::isfinite(settings.blockSize.y) ? settings.blockSize.y : 7.5f, 0.5f, 100.0f);
	settings.blockSize.z = std::clamp(std::isfinite(settings.blockSize.z) ? settings.blockSize.z : 15.0f, 0.5f, 100.0f);
	settings.minHeight = std::clamp(settings.minHeight, 1, 100);
	settings.maxHeight = std::clamp(settings.maxHeight, settings.minHeight, 100);
	settings.minStartHeight = std::clamp(settings.minStartHeight, settings.minHeight, settings.maxHeight);
	settings.maxStartHeight = std::clamp(settings.maxStartHeight, settings.minStartHeight, settings.maxHeight);
	settings.minRangeHeight = std::clamp(settings.minRangeHeight, -10, 0);
	settings.maxRangeHeight = std::clamp(settings.maxRangeHeight, 0, 10);
	settings.slopeSpawnRate = std::clamp(
		std::isfinite(settings.slopeSpawnRate) ? settings.slopeSpawnRate : 1.0f,
		0.0f,
		1.0f
	);
}

bool Map::AreGenerationSettingsEqual(const GenerationSettings& lhs, const GenerationSettings& rhs) {
	return lhs.seed == rhs.seed &&
		lhs.mapWidth == rhs.mapWidth &&
		lhs.mapHeight == rhs.mapHeight &&
		lhs.jarSpawnCount == rhs.jarSpawnCount &&
		lhs.moneyJarSpawnCount == rhs.moneyJarSpawnCount &&
		lhs.expJarSpawnCount == rhs.expJarSpawnCount &&
		lhs.chestSpawnCount == rhs.chestSpawnCount &&
		lhs.normalChestSpawnCount == rhs.normalChestSpawnCount &&
		lhs.freeChestSpawnCount == rhs.freeChestSpawnCount &&
		lhs.karmaSpawnCount == rhs.karmaSpawnCount &&
		lhs.bossSpawnerSpawnCount == rhs.bossSpawnerSpawnCount &&
		lhs.blockSize.x == rhs.blockSize.x &&
		lhs.blockSize.y == rhs.blockSize.y &&
		lhs.blockSize.z == rhs.blockSize.z &&
		lhs.minHeight == rhs.minHeight &&
		lhs.maxHeight == rhs.maxHeight &&
		lhs.minStartHeight == rhs.minStartHeight &&
		lhs.maxStartHeight == rhs.maxStartHeight &&
		lhs.minRangeHeight == rhs.minRangeHeight &&
		lhs.maxRangeHeight == rhs.maxRangeHeight &&
		lhs.slopeSpawnRate == rhs.slopeSpawnRate;
}

nlohmann::json Map::GenerationSettingsToJson(const GenerationSettings& settings) {
	nlohmann::json json;
	json["seed"] = settings.seed;
	json["mapSize"] = { settings.mapWidth, settings.mapHeight };
	json["eventObjectCounts"] = {
		{ "jar", settings.jarSpawnCount },
		{ "chest", settings.chestSpawnCount },
		{ "karma", settings.karmaSpawnCount },
		{ "bossSpawner", settings.bossSpawnerSpawnCount },
		{ "jarTypes", {
			{ "money", settings.moneyJarSpawnCount },
			{ "exp", settings.expJarSpawnCount }
		} },
		{ "chestTypes", {
			{ "normal", settings.normalChestSpawnCount },
			{ "free", settings.freeChestSpawnCount }
		} }
	};
	json["blockSize"] = { settings.blockSize.x, settings.blockSize.y, settings.blockSize.z };
	json["height"] = {
		{ "min", settings.minHeight },
		{ "max", settings.maxHeight },
		{ "startMin", settings.minStartHeight },
		{ "startMax", settings.maxStartHeight },
		{ "rangeMin", settings.minRangeHeight },
		{ "rangeMax", settings.maxRangeHeight }
	};
	json["slopeSpawnRate"] = settings.slopeSpawnRate;
	return json;
}

bool Map::GenerationSettingsFromJson(const nlohmann::json& json, GenerationSettings& outSettings) {
	if (!json.is_object()) {
		return false;
	}

	try {
		GenerationSettings settings = outSettings;
		if (const auto seedIt = json.find("seed"); seedIt != json.end() && seedIt->is_number_unsigned()) {
			settings.seed = seedIt->get<uint32_t>();
		}
		if (const auto sizeIt = json.find("mapSize"); sizeIt != json.end() && sizeIt->is_array() && sizeIt->size() >= 2) {
			settings.mapWidth = (*sizeIt)[0].get<int>();
			settings.mapHeight = (*sizeIt)[1].get<int>();
		}
		if (const auto countIt = json.find("eventObjectCounts"); countIt != json.end() && countIt->is_object()) {
			settings.jarSpawnCount = countIt->value("jar", settings.jarSpawnCount);
			settings.chestSpawnCount = countIt->value("chest", settings.chestSpawnCount);
			settings.karmaSpawnCount = countIt->value("karma", settings.karmaSpawnCount);
			settings.bossSpawnerSpawnCount = countIt->value("bossSpawner", settings.bossSpawnerSpawnCount);

			// 旧Jsonにタイプ別設定がない場合は総数を均等配分して互換性を維持
			if (const auto jarTypesIt = countIt->find("jarTypes"); jarTypesIt != countIt->end() && jarTypesIt->is_object()) {
				settings.moneyJarSpawnCount = jarTypesIt->value("money", settings.moneyJarSpawnCount);
				settings.expJarSpawnCount = jarTypesIt->value("exp", settings.expJarSpawnCount);
			} else {
				settings.moneyJarSpawnCount = (settings.jarSpawnCount + 1) / 2;
				settings.expJarSpawnCount = settings.jarSpawnCount - settings.moneyJarSpawnCount;
			}
			if (const auto chestTypesIt = countIt->find("chestTypes"); chestTypesIt != countIt->end() && chestTypesIt->is_object()) {
				settings.normalChestSpawnCount = chestTypesIt->value("normal", settings.normalChestSpawnCount);
				settings.freeChestSpawnCount = chestTypesIt->value("free", settings.freeChestSpawnCount);
			} else {
				settings.normalChestSpawnCount = (settings.chestSpawnCount + 1) / 2;
				settings.freeChestSpawnCount = settings.chestSpawnCount - settings.normalChestSpawnCount;
			}
		}
		if (const auto blockSizeIt = json.find("blockSize"); blockSizeIt != json.end() && blockSizeIt->is_array() && blockSizeIt->size() >= 3) {
			settings.blockSize = {
				(*blockSizeIt)[0].get<float>(),
				(*blockSizeIt)[1].get<float>(),
				(*blockSizeIt)[2].get<float>()
			};
		}
		if (const auto heightIt = json.find("height"); heightIt != json.end() && heightIt->is_object()) {
			settings.minHeight = heightIt->value("min", settings.minHeight);
			settings.maxHeight = heightIt->value("max", settings.maxHeight);
			settings.minStartHeight = heightIt->value("startMin", settings.minStartHeight);
			settings.maxStartHeight = heightIt->value("startMax", settings.maxStartHeight);
			settings.minRangeHeight = heightIt->value("rangeMin", settings.minRangeHeight);
			settings.maxRangeHeight = heightIt->value("rangeMax", settings.maxRangeHeight);
		}
		settings.slopeSpawnRate = json.value("slopeSpawnRate", settings.slopeSpawnRate);
		ClampGenerationSettings(settings);
		outSettings = settings;
		return true;
	} catch (const nlohmann::json::exception&) {
		return false;
	}
}

bool Map::LoadEditorSettings(GenerationSettings& outSettings, bool useSavedSeed) const {
	if (!MadoEngine::Json::JsonFile::Exists(kMapGeneratorJsonPath)) {
		return false;
	}

	nlohmann::json root;
	if (!MadoEngine::Json::JsonFile::Load(kMapGeneratorJsonPath, root) || !root.is_object()) {
		return false;
	}

	const uint32_t preservedSeed = outSettings.seed;
	const auto settingsIt = root.find("settings");
	if (settingsIt == root.end() || !GenerationSettingsFromJson(*settingsIt, outSettings)) {
		return false;
	}
	if (!useSavedSeed) {
		outSettings.seed = preservedSeed;
	}
	return true;
}

void Map::DrawHeightPreview(const Player::Base* player) const {
#ifdef USE_IMGUI
	if (mapBlocks_.empty() || mapBlocks_.front().empty()) {
		ImGui::TextDisabled("プレビューできるMapがありません");
		return;
	}

	constexpr float cellSize = 14.0f;
	std::vector<bool> bossSpawnerCells(static_cast<std::size_t>(mapWidth_) * static_cast<std::size_t>(mapHeight_), false);
	for (const std::unique_ptr<MapEventObjectBase>& object : eventObjects_) {
		if (!object || dynamic_cast<const BossSpawner*>(object.get()) == nullptr) {
			continue;
		}

		const Vector3 bossSpawnerPosition = object->GetPosition();
		const int bossSpawnerX = std::clamp(static_cast<int>(std::lround(bossSpawnerPosition.x / blockSize_.x)), 0, mapWidth_ - 1);
		const int bossSpawnerZ = std::clamp(static_cast<int>(std::lround(bossSpawnerPosition.z / blockSize_.z)), 0, mapHeight_ - 1);
		const std::size_t cellIndex = static_cast<std::size_t>(bossSpawnerZ) * static_cast<std::size_t>(mapWidth_) + static_cast<std::size_t>(bossSpawnerX);
		bossSpawnerCells[cellIndex] = true;
	}

	ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "■ BossSpawner");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.1f, 0.9f, 1.0f, 1.0f), "● Player");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.15f, 1.0f, 0.4f, 1.0f), "□ Slope");
	const ImVec2 canvasSize(
		static_cast<float>(mapWidth_) * cellSize,
		static_cast<float>(mapHeight_) * cellSize
	);
	ImGui::BeginChild("MapHeightPreview", ImVec2(0.0f, 320.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
	const ImVec2 canvasPosition = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("MapHeightPreviewCanvas", canvasSize);
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float heightRange = static_cast<float>((std::max)(1, maxHeight_ - minHeight_));

	// 高さを寒色から暖色へ変換し、BossSpawner配置Blockだけ赤色で上書き
	for (int z = 0; z < mapHeight_; ++z) {
		for (int x = 0; x < mapWidth_; ++x) {
			const MapBlock& block = mapBlocks_[z][x];
			const std::size_t cellIndex = static_cast<std::size_t>(z) * static_cast<std::size_t>(mapWidth_) + static_cast<std::size_t>(x);
			const float heightRate = std::clamp(
				(static_cast<float>(block.GetHeight()) - static_cast<float>(minHeight_)) / heightRange,
				0.0f,
				1.0f
			);
			ImVec4 color(
				0.12f + heightRate * 0.78f,
				0.28f + (1.0f - std::abs(heightRate - 0.5f) * 2.0f) * 0.42f,
				0.88f - heightRate * 0.68f,
				1.0f
			);
			if (bossSpawnerCells[cellIndex]) {
				color = { 0.95f, 0.08f, 0.08f, 1.0f };
			}
			const ImVec2 cellMin(canvasPosition.x + static_cast<float>(x) * cellSize, canvasPosition.y + static_cast<float>(z) * cellSize);
			const ImVec2 cellMax(cellMin.x + cellSize - 1.0f, cellMin.y + cellSize - 1.0f);
			drawList->AddRectFilled(cellMin, cellMax, ImGui::GetColorU32(color));
			drawList->AddRect(
				cellMin,
				cellMax,
				block.GetType() == MapBlockType::Slope
					? IM_COL32(40, 255, 110, 255)
					: IM_COL32(30, 30, 30, 180)
			);
		}
	}

	if (player) {
		const Vector3 playerPosition = player->GetPosition();
		const float playerGridX = (playerPosition.x + blockSize_.x * 0.5f) / blockSize_.x;
		const float playerGridZ = (playerPosition.z + blockSize_.z * 0.5f) / blockSize_.z;
		if (playerGridX >= 0.0f && playerGridX <= static_cast<float>(mapWidth_) &&
			playerGridZ >= 0.0f && playerGridZ <= static_cast<float>(mapHeight_)) {

			// Block内の移動量も読み取れるようPlayerのワールド座標をセル中心へ丸めず描画座標へ変換
			const ImVec2 playerMarkerPosition(
				canvasPosition.x + playerGridX * cellSize,
				canvasPosition.y + playerGridZ * cellSize
			);
			drawList->AddCircleFilled(playerMarkerPosition, 4.5f, IM_COL32(25, 230, 255, 255));
			drawList->AddCircle(playerMarkerPosition, 5.0f, IM_COL32(255, 255, 255, 255), 0, 1.5f);
		}
	}

	if (ImGui::IsItemHovered()) {
		const ImVec2 mousePosition = ImGui::GetIO().MousePos;
		const int x = std::clamp(static_cast<int>((mousePosition.x - canvasPosition.x) / cellSize), 0, mapWidth_ - 1);
		const int z = std::clamp(static_cast<int>((mousePosition.y - canvasPosition.y) / cellSize), 0, mapHeight_ - 1);
		const MapBlock& block = mapBlocks_[z][x];
		const std::size_t cellIndex = static_cast<std::size_t>(z) * static_cast<std::size_t>(mapWidth_) + static_cast<std::size_t>(x);
		const bool hasBossSpawner = bossSpawnerCells[cellIndex];
		ImGui::SetTooltip(
			"X: %d  Z: %d\n高さ: %u\n種別: %s%s",
			x,
			z,
			block.GetHeight(),
			block.GetType() == MapBlockType::Slope ? "Slope" : "Ground",
			hasBossSpawner ? "\nBossSpawner: あり" : ""
		);
	}
	ImGui::EndChild();
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
	std::vector<JarType> jarTypes;
	jarTypes.reserve(static_cast<std::size_t>(maxSpawnCount));
	jarTypes.insert(jarTypes.end(), static_cast<std::size_t>(moneyJarSpawnCount_), JarType::Money);
	jarTypes.insert(jarTypes.end(), static_cast<std::size_t>(expJarSpawnCount_), JarType::Exp);

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
		desc.type = jarTypes[static_cast<std::size_t>(createdCount)];
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
	std::vector<ChestType> chestTypes;
	chestTypes.reserve(static_cast<std::size_t>(maxSpawnCount));
	chestTypes.insert(chestTypes.end(), static_cast<std::size_t>(normalChestSpawnCount_), ChestType::Normal);
	chestTypes.insert(chestTypes.end(), static_cast<std::size_t>(freeChestSpawnCount_), ChestType::Free);

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
	if (bossSpawnerSpawnCount_ <= 0) {
		return;
	}

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

	int createdCount = 0;

	// 各通常ブロックを一度だけ探索し、要求数到達か候補枯渇まで配置
	while (createdCount < bossSpawnerSpawnCount_ && !spawnCandidates.empty()) {
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
	if (createdCount < bossSpawnerSpawnCount_) {
		Logger::Output("Map : BossSpawnerの配置可能数が設定数を下回りました", Logger::Level::Warning);
	}
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

	const bool shouldShowInteractionText =
		isInteractionTextVisible_ && currentHitEventObject_ != nullptr;
	interactionText->SetVisible(shouldShowInteractionText);
	if (!shouldShowInteractionText) {
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
