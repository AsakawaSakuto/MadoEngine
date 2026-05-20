#include "Sprite.h"
#include "Math/Function/MatrixFunction.h"
#include "Shader/RootSignatureManager.h"
#include "Math/Function/MathFunction.h"

Sprite::Sprite(std::string ObjectName) {
	objectName_ = ObjectName;
}

void Sprite::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName) {

	device_ = device;
	commandList_ = commandList;

	textureIndex_ = MadoEngine::TextureManager::GetInstance()->GetTextureIndex(textureName);
	size_ = MadoEngine::TextureManager::GetInstance()->GetPixelSize(textureName);

	// 2D Sprite専用の頂点データ（Normalなし）
	vertexData_ = CreateMappedBuffer<SpriteVertexData>(device_.Get(), vertexResource_, 4);
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes    = sizeof(SpriteVertexData) * 4;
	vertexBufferView_.StrideInBytes  = sizeof(SpriteVertexData);

	float width = size_.x;
	float height = size_.y;

	float left = 0.0f - anchorPoint_.x * width;
	float right = width - anchorPoint_.x * width;
	float top = 0.0f - anchorPoint_.y * height;
	float bottom = height - anchorPoint_.y * height;

	vertexData_[0].position = { left,  bottom, 0.0f, 1.0f }; // 左下
	vertexData_[1].position = { left,  top,    0.0f, 1.0f }; // 左上
	vertexData_[2].position = { right, bottom, 0.0f, 1.0f }; // 右下
	vertexData_[3].position = { right, top,    0.0f, 1.0f }; // 右上

	vertexData_[0].texcoord = { 0.0f,1.0f };
	vertexData_[1].texcoord = { 0.0f,0.0f };
	vertexData_[2].texcoord = { 1.0f,1.0f };
	vertexData_[3].texcoord = { 1.0f,0.0f };
	
	// -------------------- //

	// IndexResource
	indexData_ = CreateMappedBuffer<uint32_t>(device_.Get(), indexResource_, 6);
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes    = sizeof(uint32_t) * 6;
	indexBufferView_.Format         = DXGI_FORMAT_R32_UINT;
	// 表面（三角形2枚）
	indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2;
	indexData_[3] = 1; indexData_[4] = 3; indexData_[5] = 2;

	// -------------------- //

	// 2D Sprite専用のマテリアルデータ（ライティングなし）
	materialData_ = CreateMappedBuffer<SpriteMaterial>(device_.Get(), materialResource_);
	materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData_->uvTransformMatrix = Matrix::MakeIdentity();

	// 2D Sprite専用の変換行列（WVPのみ）
	transformationData_ = CreateMappedBuffer<SpriteTransformationMatrix>(device_.Get(), transformationResource_);
	transformationData_->WVP = Matrix::MakeIdentity();

	psoDesc_.inputLayout = MadoEngine::Render::InputLayoutType::Sprite;
	psoDesc_.vsKey       = "Object2d/Sprite/Sprite.VS";
	psoDesc_.psKey       = "Object2d/Sprite/Sprite.PS";
	psoDesc_.rootSigKey  = "Sprite.RootSig";
	psoDesc_.depthMode   = MadoEngine::Render::DepthMode::Disable; // 2Dスプライトは深度テスト不要
	psoDesc_.dsvFormat   = DXGI_FORMAT_D32_FLOAT;                  // 深度バッファのフォーマットに合わせる
	psoDesc_.blendMode   = MadoEngine::Render::BlendMode::Normal;

	Logger::Output("[Sprite] " + objectName_ + "の初期化が完了しました", Logger::Level::Application);
}

void Sprite::Update() {
	// カラーをマテリアルに反映
	materialData_->color = color_;

	// ワールド行列（2Dアフィン変換: スケール → Z軸回転 → 平行移動）
	Matrix4x4 worldMatrix = Matrix::MakeAffine(
		{ transform_.scale.x, transform_.scale.y, 1.0f },
		{ 0.0f, 0.0f, transform_.rotate },
		{ transform_.translate.x, transform_.translate.y, 0.0f }
	);

	// 正射影行列（ピクセル座標 [0,W]×[0,H] → NDC）
	Matrix4x4 orthoMatrix = Matrix::MakeOrthographic(
		0.0f, 0.0f, screenWidth_, screenHeight_, 0.0f, 1.0f
	);

	// WVP = World × Projection
	transformationData_->WVP = Matrix::Multiply(worldMatrix, orthoMatrix);
}

void Sprite::Draw() {
	// RootSignatureを設定。PSOに設定しているけど別途設定が必要
	commandList_->SetGraphicsRootSignature(MadoEngine::RootSignatureManager::GetInstance()->Get(psoDesc_.rootSigKey));

	// PSOを設定
	commandList_->SetPipelineState(psoRegistry_->Get(psoDesc_));
	
	// プリミティブトポロジーを設定
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Spriteの描画。変更が必要なものだけ変更する
	commandList_->IASetVertexBuffers(0, 1, &vertexBufferView_);  // VBVを設定
	commandList_->IASetIndexBuffer(&indexBufferView_);

	// 2D Sprite用のシンプルなルートパラメータ設定
	// b0: Material, b1: Transform, t0: Texture
	commandList_->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(1, transformationResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(2, MadoEngine::TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex_));

	// 描画！ (DrawCall/ドローコール)
	commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);
}