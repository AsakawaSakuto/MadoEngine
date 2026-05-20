#include "Model.h"

Model::Model(std::string ObjectName) {
	objectName_ = ObjectName;
}

void Model::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
	// モデルの初期化処理をここに実装
}

void Model::Update() {
	// モデルの更新処理をここに実装
}

void Model::Draw() {
	// モデルの描画処理をここに実装
}