#include "MapBlock.h"
#include <numbers>

namespace {

/// @brief 坂の向きに対応するY軸回転角を取得します。
/// @param direction 坂の向きです。
/// @return Y軸回転角（ラジアン）です。
float GetSlopeRotationY(SlopeDirection direction) {
	switch (direction) {
	case SlopeDirection::PulsZ:
		return 0.0f;
	case SlopeDirection::MinusX:
		return std::numbers::pi_v<float> * 1.5f;
	case SlopeDirection::MinusZ:
		return std::numbers::pi_v<float>;
	case SlopeDirection::PulsX:
		return std::numbers::pi_v<float> * 0.5f;
	}

	return 0.0f;
}

}

void MapBlock::Initialize(const InitializeDesc& desc) {

	x_ = desc.x;
	z_ = desc.z;
	mapWidth_ = desc.mapWidth;
	height_ = desc.height;
	type_ = desc.type;
	slopeDirection_ = desc.slopeDirection;
	isModelDraw_ = desc.isModelDraw;
	transform_.translate = Vector3(
		static_cast<float>(x_) * desc.blockSize.x,
		0.0f,
		static_cast<float>(z_) * desc.blockSize.z
	);

	if (type_ == MapBlockType::Slope) {
		CreateSlopeShape(desc.blockSize);
		MyCollider::RegisterCollider(CreateColliderName(), CollisionTag::MapSlope, &colliderShape_, &transform_.translate, 1.0f);
		isColliderRegistered_ = true;
	} else if (type_ == MapBlockType::Ground) {
		CreateGroundShape(desc.blockSize);
		MyCollider::RegisterCollider(CreateColliderName(), CollisionTag::MapBlock, &colliderShape_, &transform_.translate, 1.0f);
		isColliderRegistered_ = true;
	}

	CreateGroundModel(desc.blockSize);
	if (type_ == MapBlockType::Slope) {
		CreateSlopeModel(desc.blockSize);
	}
}

void MapBlock::Update(float deltaTime) {
	(void)deltaTime;
}

void MapBlock::SetVisible(bool isVisible) {

	isModelDraw_ = isVisible;

	if (groundInstancedModel_) {
		groundInstancedModel_->SetInstanceVisible(groundInstanceHandle_, isModelDraw_);
	}

	if (slopeInstancedModel_) {
		slopeInstancedModel_->SetInstanceVisible(slopeInstanceHandle_, isModelDraw_);
	}
}

void MapBlock::DrawDebugLine() const {

	if (type_ == MapBlockType::Air) {
		return;
	}

	Vector4 lineColor = type_ == MapBlockType::Slope ?
		Vector4(0.0f, 1.0f, 0.35f, 1.0f) :
		Vector4(1.0f, 0.0f, 0.0f, 1.0f);

	MyDebugLine::AddShape(colliderShape_, lineColor);
}

void MapBlock::SetHeight(uint32_t height) {
	height_ = height;
}

uint32_t MapBlock::GetHeight() const {
	return height_;
}

MapBlockType MapBlock::GetType() const {
	return type_;
}

SlopeDirection MapBlock::GetSlopeDirection() const {
	return slopeDirection_;
}

void MapBlock::CreateGroundShape(const Vector3& blockSize) {

	AABB blockShape;
	blockShape.min = Vector3(-blockSize.x / 2.0f, 0.0f, -blockSize.z / 2.0f);
	blockShape.max = Vector3(blockSize.x / 2.0f, blockSize.y * static_cast<float>(height_), blockSize.z / 2.0f);

	colliderShape_ = blockShape;
}

void MapBlock::CreateSlopeShape(const Vector3& blockSize) {

	Slope slopeShape;
	slopeShape.min = Vector3(-blockSize.x / 2.0f, blockSize.y * static_cast<float>(height_), -blockSize.z / 2.0f);
	slopeShape.max = Vector3(blockSize.x / 2.0f, blockSize.y * static_cast<float>(height_ + 1), blockSize.z / 2.0f);
	slopeShape.bottomExtendY = slopeShape.min.y;
	slopeShape.direction = slopeDirection_;

	colliderShape_ = slopeShape;
}

void MapBlock::CreateGroundModel(const Vector3& blockSize) {

	groundInstancedModel_ = MyInstancedModel::GetOrCreate(
		CreateGroundModelName(),
		"block",
		SceneType::Game,
		MadoEngine::Render::RenderLayer::Default);
	if (!groundInstancedModel_) {
		return;
	}

	groundInstancedModel_->SetTexture("blockTexture2");
	//groundInstancedModel_->SetReceiveShadow(false);

	InstancedModel::InstanceDesc instanceDesc;
	instanceDesc.transform.translate = { transform_.translate.x, blockSize.y * static_cast<float>(height_), transform_.translate.z };
	instanceDesc.transform.scale = { blockSize.x / 2.0f, blockSize.y / 2.0f * static_cast<float>(height_), blockSize.z / 2.0f };
	instanceDesc.transform.rotate = { 0.0f, 0.0f, 0.0f };
	instanceDesc.isVisible = isModelDraw_;
	groundInstanceHandle_ = groundInstancedModel_->AddInstance(instanceDesc);
}

void MapBlock::CreateSlopeModel(const Vector3& blockSize) {

	slopeInstancedModel_ = MyInstancedModel::GetOrCreate(
		CreateSlopeModelName(),
		"Slope",
		SceneType::Game,
		MadoEngine::Render::RenderLayer::Default);
	if (!slopeInstancedModel_) {
		return;
	}

	slopeInstancedModel_->SetTexture("blockTexture2");

	InstancedModel::InstanceDesc instanceDesc;
	instanceDesc.transform.translate = { transform_.translate.x, blockSize.y * static_cast<float>(height_ + 1), transform_.translate.z };
	instanceDesc.transform.scale = { blockSize.x / 2.0f, blockSize.y / 2.0f, blockSize.z / 2.0f };
	instanceDesc.transform.rotate = { 0.0f, GetSlopeRotationY(slopeDirection_), 0.0f };
	instanceDesc.isVisible = isModelDraw_;
	slopeInstanceHandle_ = slopeInstancedModel_->AddInstance(instanceDesc);
}

std::string MapBlock::CreateColliderName() const {

	if (type_ == MapBlockType::Slope) {
		return "MapSlope_x:" + std::to_string(x_) + "_z:" + std::to_string(z_);
	}

	return "MapBlock_x:" + std::to_string(x_) + "_z:" + std::to_string(z_);
}

std::string MapBlock::CreateGroundModelName() const {
	return "MapBlock.Ground";
}

std::string MapBlock::CreateSlopeModelName() const {
	return "MapBlock.Slope";
}
