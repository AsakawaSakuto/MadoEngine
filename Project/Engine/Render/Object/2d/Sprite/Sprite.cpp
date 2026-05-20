#include "Sprite.h"
#include "Math/Function/MatrixFunction.h"
#include "Shader/RootSignatureManager.h"
#include "Math/Function/MathFunction.h"

Sprite::Sprite(std::string objectName) {
	objectName_ = objectName;
}

// ----------------------------------------
// 単独使用時: 自前でユニットクワッドVB・IBを生成する
// ----------------------------------------
void Sprite::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName) {

	device_ = device;
	commandList_ = commandList;

	textureIndex_ = MadoEngine::TextureManager::GetInstance()->GetTextureIndex(textureName);
	size_ = MadoEngine::TextureManager::GetInstance()->GetPixelSize(textureName);

	// ユニットクワッド（0〜1）で生成。size・anchorPoint の反映は Update() 側の行列で行う
	SpriteVertexData* vertexData = CreateMappedBuffer<SpriteVertexData>(device_.Get(), vertexResource_, 4);
	vertexData[0].position = { 0.0f, 1.0f, 0.0f, 1.0f }; // 左下
	vertexData[1].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // 左上
	vertexData[2].position = { 1.0f, 1.0f, 0.0f, 1.0f }; // 右下
	vertexData[3].position = { 1.0f, 0.0f, 0.0f, 1.0f }; // 右上
	vertexData[0].texcoord = { 0.0f, 1.0f };
	vertexData[1].texcoord = { 0.0f, 0.0f };
	vertexData[2].texcoord = { 1.0f, 1.0f };
	vertexData[3].texcoord = { 1.0f, 0.0f };

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(SpriteVertexData) * 4;
	vertexBufferView_.StrideInBytes = sizeof(SpriteVertexData);

	uint32_t* indexData = CreateMappedBuffer<uint32_t>(device_.Get(), indexResource_, 6);
	indexData[0] = 0; indexData[1] = 1; indexData[2] = 2;
	indexData[3] = 1; indexData[4] = 3; indexData[5] = 2;

	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	// 自前のVBV・IBVを使用
	activeVBV_ = &vertexBufferView_;
	activeIBV_ = &indexBufferView_;

	InitializeCommonResources(textureName);
}

// ----------------------------------------
// SpriteManager経由: 共有ジオメトリバッファを参照する
// ----------------------------------------
void Sprite::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName, const SpriteSharedGeometry& sharedGeo) {

	device_ = device;
	commandList_ = commandList;

	textureIndex_ = MadoEngine::TextureManager::GetInstance()->GetTextureIndex(textureName);
	size_ = MadoEngine::TextureManager::GetInstance()->GetPixelSize(textureName);

	// 共有ジオメトリのVBV・IBVを参照する（このSpriteはバッファを所有しない）
	activeVBV_ = &sharedGeo.vbv;
	activeIBV_ = &sharedGeo.ibv;

	InitializeCommonResources(textureName);
}

// ----------------------------------------
// マテリアル・変換行列・PSOの共通初期化
// ----------------------------------------
void Sprite::InitializeCommonResources(const std::string& textureName) {

	materialData_ = CreateMappedBuffer<SpriteMaterial>(device_.Get(), materialResource_);
	materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData_->uvTransformMatrix = Matrix::MakeIdentity();

	transformationData_ = CreateMappedBuffer<SpriteTransformationMatrix>(device_.Get(), transformationResource_);
	transformationData_->WVP = Matrix::MakeIdentity();

	psoDesc_.inputLayout = MadoEngine::Render::InputLayoutType::Sprite;
	psoDesc_.vsKey = "Object2d/Sprite/Sprite.VS";
	psoDesc_.psKey = "Object2d/Sprite/Sprite.PS";
	psoDesc_.rootSigKey = "Sprite.RootSig";
	psoDesc_.depthMode = MadoEngine::Render::DepthMode::Disable;
	psoDesc_.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc_.blendMode = MadoEngine::Render::BlendMode::Normal;

	Logger::Output("[Sprite] " + objectName_ + "の初期化が完了しました", Logger::Level::Application);
}

void Sprite::Update() {
	materialData_->color = color_;

	// ユニットクワッド（0〜1）前提のワールド行列
	//
	// 変換順:
	//   1. Scale(size * transform.scale)      ピクセルサイズに拡大
	//   2. Translate(-anchor * size * scale)  アンカー分だけローカル原点をオフセット
	//   3. RotateZ(transform.rotate)          Z軸回転
	//   4. Translate(transform.translate)     ワールド配置

	float sx = size_.x * transform_.scale.x;
	float sy = size_.y * transform_.scale.y;

	Matrix4x4 scaleMatrix = Matrix::MakeScale({ sx, sy, 1.0f });
	Matrix4x4 anchorMatrix = Matrix::MakeTranslate({ -anchorPoint_.x * sx, -anchorPoint_.y * sy, 0.0f });
	Matrix4x4 rotateMatrix = Matrix::MakeRotateZ(transform_.rotate);
	Matrix4x4 transMatrix = Matrix::MakeTranslate({ transform_.translate.x, transform_.translate.y, 0.0f });

	Matrix4x4 worldMatrix = Matrix::Multiply(
		Matrix::Multiply(Matrix::Multiply(scaleMatrix, anchorMatrix), rotateMatrix),
		transMatrix
	);

	// 正射影行列（ピクセル座標 [0,W]×[0,H] → NDC）
	Matrix4x4 orthoMatrix = Matrix::MakeOrthographic(
		0.0f, 0.0f, screenWidth_, screenHeight_, 0.0f, 1.0f
	);

	transformationData_->WVP = Matrix::Multiply(worldMatrix, orthoMatrix);
}

void Sprite::Draw() {
	commandList_->SetGraphicsRootSignature(MadoEngine::RootSignatureManager::GetInstance()->Get(psoDesc_.rootSigKey));
	commandList_->SetPipelineState(psoRegistry_->Get(psoDesc_));
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList_->IASetVertexBuffers(0, 1, activeVBV_);
	commandList_->IASetIndexBuffer(activeIBV_);

	// b0: Material, b1: Transform, t0: Texture
	commandList_->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(1, transformationResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(2, MadoEngine::TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex_));

	commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);
}