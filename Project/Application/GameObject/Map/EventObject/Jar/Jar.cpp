#include "Jar.h"
#include "GameObject/DropObject/DropObjectManager.h"
#include "GameObject/Player/Player.h"

namespace {

/// @brief Jarのサイズに対応するモデル名を取得
/// @param size Jarのサイズ
/// @param type Jarの種別
/// @return 読み込むモデル名
const char* GetJarModelAssetName(JarSize size, JarType type) {
	switch (size) {
	case JarSize::Big:
		
		switch (type) {
		case JarType::Exp:
			return "maxJarExp";
		case JarType::Money:
		default:
			return "maxJarMoney";
		}

	case JarSize::Small:

		switch (type) {
		case JarType::Exp:
			return "minJarExp";
		case JarType::Money:
		default:
			return "minJarMoney";
		}

	default:
		return "minJar";
	}
}

/// @brief Jarのインスタンス描画バッチ名を取得
/// @param size Jarのサイズ
/// @param type Jarの種別
/// @param isOutline アウトライン表示用の場合はtrue
/// @return インスタンス描画バッチ名
std::string GetJarBatchName(JarSize size, JarType type, bool isOutline) {

	// モデルアセットが異なるJarを共有しないようサイズと種別でBatchを分離
	std::string batchName = "Jar.";
	if (size == JarSize::Big) {
		batchName += "Big.";
	} else {
		batchName += "Small.";
	}

	if (type == JarType::Exp) {
		batchName += "Exp.";
	} else {
		batchName += "Money.";
	}

	if (isOutline) {
		batchName += "Outline";
	} else {
		batchName += "Normal";
	}
	return batchName;
}

/// @brief Jarの取得報酬量を計算
/// @param size Jarのサイズ
/// @return プレイヤーに加算する報酬量
int CalculateRewardAmount(JarSize size) {
	if (size == JarSize::Big) {
		return 20;
	} else {
		return 10;
	}
}

/// @brief Jarの種別に対応するEffect Sequenceアセット名を取得
/// @param type Jarの種別
/// @return 再生するEffect Sequenceアセット名
const char* GetJarEffectSequenceAssetName(JarType type) {
	if (type == JarType::Exp) {
		return "ExpJar";
	} else {
		return "MoneyJar";
	}
}

}

Jar::~Jar() {
	MyCollider::RemoveCollider(colliderName_);
	HideInstancedDraw();
	if (model_.IsValid()) {
		MyModel::RequestDestroy(model_);
	}
}

void Jar::Initialize(const InitializeDesc& desc) {
	type_ = desc.type;
	size_ = desc.size;
	modelName_ = desc.modelName;
	SetColliderName(desc.colliderName);
	transform_.translate = desc.position;
	transform_.rotate = desc.rotation;

	colliderShape_ = CreatePlacementCollider(desc.position);

	MyCollider::RegisterCollider(colliderName_, CollisionTag::MapEventObject, &colliderShape_, &transform_.translate, 0.0f);

	const MadoEngine::InstancedModelHandle normalBatchHandle = MyInstancedModel::GetOrCreate(
		GetJarBatchName(size_, type_, false),
		GetJarModelAssetName(size_, type_),
		SceneType::Game,
		MadoEngine::Render::RenderLayer::MapEventObject);
	const MadoEngine::InstancedModelHandle outlineBatchHandle = MyInstancedModel::GetOrCreate(
		GetJarBatchName(size_, type_, true),
		GetJarModelAssetName(size_, type_),
		SceneType::Game,
		MadoEngine::Render::RenderLayer::MapEventObjectOutline);

	InstancedModel* normalBatch = MyInstancedModel::TryGet(normalBatchHandle);
	InstancedModel* outlineBatch = MyInstancedModel::TryGet(outlineBatchHandle);
	if (normalBatch && outlineBatch) {

		// 強調表示へ切り替えても種別ごとの外観を維持するため両Batchへ同じTextureを適用
		const char* textureName = "JarMoneyModelTexture";
		if (type_ == JarType::Exp) {
			textureName = "JarExpModelTexture";
		}
		normalBatch->SetTexture(textureName);
		outlineBatch->SetTexture(textureName);

		// 強調表示をInstanceの表示切り替えだけで完結させるため同一Transformを二重登録
		InstancedModel::InstanceDesc normalInstance;
		normalInstance.transform = transform_;
		normalInstance.isVisible = true;

		InstancedModel::InstanceDesc outlineInstance = normalInstance;
		outlineInstance.isVisible = false;

		uint32_t normalHandle = normalBatch->AddInstance(normalInstance);
		uint32_t outlineHandle = outlineBatch->AddInstance(outlineInstance);
		SetInstancedDraw(normalBatchHandle, normalHandle, outlineBatchHandle, outlineHandle);
	}
}

AABB Jar::CreatePlacementCollider(const Vector3& position) {
	AABB collider;
	collider.center = position;
	collider.min = { -1.0f, 0.0f, -1.0f };
	collider.max = { 1.0f, 1.0f, 1.0f };
	return collider;
}

void Jar::Update(float deltaTime) {
	(void)deltaTime;

	std::get<AABB>(colliderShape_).center = transform_.translate;
	MyDebugLine::AddShape(std::get<AABB>(colliderShape_), { 0.0f, 0.0f, 0.0f, 1.0f });
}

bool Jar::Interact(Player::Base& player) {

	// Jar種別で付与先を分けつつサイズに応じた共通報酬量を適用
	const int rewardAmount = CalculateRewardAmount(size_);
	if (type_ == JarType::Exp) {
		player.AddExp(rewardAmount);
		Logger::Output("Jarを取得しました。経験値を" + std::to_string(rewardAmount) + "加算しました。",
			Logger::Level::Application);
	} else {
		player.AddMoney(rewardAmount);
		Logger::Output("Jarを取得しました。所持金を" + std::to_string(rewardAmount) + "加算しました。", 
			Logger::Level::Application);
	}

	// 通常報酬の確定後、Jarの種別やサイズにかかわらず破壊位置への特殊Dropを一度抽選
	DropObject::Manager::GetInstance().TrySpawnSpecial(DropObject::SpecialDropSource::Jar, transform_.translate);

	// Jar削除後も破壊位置で演出を完走できるよう独立したSequenceとして再生
	MadoEngine::EffectSequence::EffectSequencePlayDesc effectDesc;
	effectDesc.rootTransform.translate = transform_.translate;
	effectDesc.sceneType = SceneType::Game;
	effectDesc.loopOverride = false;
	MadoEngine::EffectSequence::EffectSequenceSystem::GetInstance().Play(
		GetJarEffectSequenceAssetName(type_), effectDesc);

	return true;
}

std::string_view Jar::GetInteractionText() const {
	if (type_ == JarType::Exp) {
		return "経験値を得る";
	} else {
		return "お金を得る";
	}
}

MapInteractionEvent Jar::GetInteractionEvent() const {

	// 報酬種別を文字列名から推測せず確定したJar種別から通知
	if (type_ == JarType::Exp) {
		return { MapInteractionType::ExpJar, 0 };
	}
	return { MapInteractionType::MoneyJar, 0 };
}
