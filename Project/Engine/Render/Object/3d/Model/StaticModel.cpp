#include "StaticModel.h"

StaticModel::StaticModel(std::string ObjectName) {
	objectName_ = ObjectName;
}

void StaticModel::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
	// モデルの初期化処理をここに実装
}

void StaticModel::Update() {
	// モデルの更新処理をここに実装
}

void StaticModel::Draw() {
	// モデルの描画処理をここに実装
}