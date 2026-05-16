#include "Sprite.h"
#include "Math/Function/MatrixFunction.h"
#include "Shader/RootSignatureManager.h"
#include "Math/Function/MathFunction.h"

void Sprite::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName) {

	device_ = device;
	commandList_ = commandList;

	textureIndex_ = MadoEngine::TextureManager::GetInstance()->GetTextureIndex(textureName);
	size_ = MadoEngine::TextureManager::GetInstance()->GetPixelSize(textureName);

	// 2D Sprite専用の頂点データ（Normalなし）
	vertexResource_ = CreateBufferResource(device_.Get(), sizeof(SpriteVertexData) * 4);
	// リソースの先頭のアドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズ
	vertexBufferView_.SizeInBytes = sizeof(SpriteVertexData) * 4;
	// 1頂点あたりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(SpriteVertexData);
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

	float width = size_.x;
	float height = size_.y;

	float left   = 0.0f   - anchorPoint_.x * width;
	float right  = width  - anchorPoint_.x * width;
	float top    = 0.0f   - anchorPoint_.y * height;
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
	indexResource_ = CreateBufferResource(device_.Get(), sizeof(uint32_t) * 6);
	// リソースの先頭のアドレスから使う
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズ
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	// format
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
	// 表面（三角形2枚）
	indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2;
	indexData_[3] = 1; indexData_[4] = 3; indexData_[5] = 2;

	// -------------------- //

	// 2D Sprite専用のマテリアルデータ（ライティングなし）
	materialResource_ = CreateBufferResource(device_.Get(), sizeof(SpriteMaterial));
	// 書き込むためのアドレスを取得
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData_->uvTransformMatrix = Matrix::MakeIdentity();

	// 2D Sprite専用の変換行列（WVPのみ）
	transformationResource_ = CreateBufferResource(device_.Get(), sizeof(SpriteTransformationMatrix));
	//
	transformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationData_));
	//
	transformationData_->WVP = Matrix::MakeIdentity();

	// RootSignatureの登録（未登録の場合のみ実行される）
	// b0: SpriteMaterial (VS/PS共通), b1: SpriteTransformationMatrix (VS), t0: Texture (PS), s0: Sampler
	{
		D3D12_DESCRIPTOR_RANGE srvRange{};
		srvRange.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors                    = 1;
		srvRange.BaseShaderRegister                = 0; // t0
		srvRange.RegisterSpace                     = 0;
		srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER rootParams[3]{};
		// b0: SpriteMaterial
		rootParams[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParams[0].Descriptor.ShaderRegister = 0;
		rootParams[0].Descriptor.RegisterSpace  = 0;
		rootParams[0].ShaderVisibility           = D3D12_SHADER_VISIBILITY_ALL;
		// b1: SpriteTransformationMatrix
		rootParams[1].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParams[1].Descriptor.ShaderRegister = 1;
		rootParams[1].Descriptor.RegisterSpace  = 0;
		rootParams[1].ShaderVisibility           = D3D12_SHADER_VISIBILITY_VERTEX;
		// t0: Texture (DescriptorTable)
		rootParams[2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[2].DescriptorTable.pDescriptorRanges   = &srvRange;
		rootParams[2].ShaderVisibility                     = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC staticSampler{};
		staticSampler.Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSampler.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSampler.MipLODBias       = 0.0f;
		staticSampler.MaxAnisotropy    = 0;
		staticSampler.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
		staticSampler.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		staticSampler.MinLOD           = 0.0f;
		staticSampler.MaxLOD           = D3D12_FLOAT32_MAX;
		staticSampler.ShaderRegister   = 0; // s0
		staticSampler.RegisterSpace    = 0;
		staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
		rootSigDesc.NumParameters     = _countof(rootParams);
		rootSigDesc.pParameters       = rootParams;
		rootSigDesc.NumStaticSamplers = 1;
		rootSigDesc.pStaticSamplers   = &staticSampler;
		rootSigDesc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		MadoEngine::RootSignatureManager::GetInstance()->Register("Sprite.RootSig", rootSigDesc);
	}

	psoDesc_.inputLayout = MadoEngine::Render::InputLayoutType::Sprite;
	psoDesc_.vsKey       = "Object2d/Sprite/Sprite.VS";
	psoDesc_.psKey       = "Object2d/Sprite/Sprite.PS";
	psoDesc_.rootSigKey  = "Sprite.RootSig";
	psoDesc_.depthMode   = MadoEngine::Render::DepthMode::Disable; // 2Dスプライトは深度テスト不要
	psoDesc_.dsvFormat   = DXGI_FORMAT_D32_FLOAT;                  // 深度バッファのフォーマットに合わせる
	psoDesc_.blendMode   = MadoEngine::Render::BlendMode::Normal;
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