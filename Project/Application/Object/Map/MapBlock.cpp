#include "MapBlock.h"

namespace {

/// @brief 坂の向きに対応したモデル名を取得します。
/// @param direction 坂の向きです。
/// @return 坂モデル名です。
std::string GetSlopeModelName(SlopeDirection direction) {
	switch (direction) {
	case SlopeDirection::PulsX:
		return "SlopeMinusX";
	case SlopeDirection::MinusX:
		return "SlopePlusX";
	case SlopeDirection::PulsZ:
		return "SlopeMinusZ";
	case SlopeDirection::MinusZ:
		return "SlopePlusZ";
	}

	return "SlopePlusX";
}

}

void MapBlock::Initialize() {
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

	if (model_) {
		model_->SetVisible(isModelDraw_);
	}

	if (slopeModel_) {
		slopeModel_->SetVisible(isModelDraw_);
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

	model_ = MyModel::Create(CreateGroundModelName(), "block", SceneType::Test);
	if (!model_) {
		return;
	}

	model_->SetRenderLayer(MadoEngine::Render::RenderLayer::Default);
	model_->SetPosition({ transform_.translate.x, blockSize.y * static_cast<float>(height_), transform_.translate.z });
	model_->SetScale({ blockSize.x / 2.0f, blockSize.y / 2.0f * static_cast<float>(height_), blockSize.z / 2.0f });
	model_->SetVisible(isModelDraw_);
	model_->SetTexture("blockTexture");
}

void MapBlock::CreateSlopeModel(const Vector3& blockSize) {

	slopeModel_ = MyModel::Create(CreateSlopeModelName(), GetSlopeModelName(slopeDirection_), SceneType::Test);
	if (!slopeModel_) {
		return;
	}

	slopeModel_->SetRenderLayer(MadoEngine::Render::RenderLayer::Default);
	slopeModel_->SetPosition({ transform_.translate.x, blockSize.y * static_cast<float>(height_ + 1), transform_.translate.z });
	slopeModel_->SetScale({ blockSize.x / 2.0f, blockSize.y / 2.0f, blockSize.z / 2.0f });
	slopeModel_->SetVisible(isModelDraw_);
	slopeModel_->SetTexture("blockTexture");
}

std::string MapBlock::CreateColliderName() const {

	if (type_ == MapBlockType::Slope) {
		return "MapSlope_x:" + std::to_string(x_) + "_z:" + std::to_string(z_);
	}

	return "MapBlock_x:" + std::to_string(x_) + "_z:" + std::to_string(z_);
}

std::string MapBlock::CreateGroundModelName() const {
	return "mapBlockModel" + std::to_string(z_ * mapWidth_ + x_);
}

std::string MapBlock::CreateSlopeModelName() const {
	return "mapSlopeModel" + std::to_string(z_ * mapWidth_ + x_);
}
