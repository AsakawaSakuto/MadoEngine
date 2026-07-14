#include "Sprite.h"
#include "Math/Function/MatrixFunction.h"
#include "Shader/RootSignatureManager.h"
#include "Math/Function/MathFunction.h"
#include <algorithm>
#include <utility>

namespace {

	/// @brief Json配列からVector2を読み込む
	/// @param json 読み込むJson
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込んだVector2
	Vector2 ReadVector2(const nlohmann::json& json, const Vector2& fallback) {
		if (!json.is_array() || json.size() < 2) {
			return fallback;
		}

		return {
			json.at(0).get<float>(),
			json.at(1).get<float>(),
		};
	}

	/// @brief Json配列からVector4を読み込む
	/// @param json 読み込むJson
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込んだVector4
	Vector4 ReadVector4(const nlohmann::json& json, const Vector4& fallback) {
		if (!json.is_array() || json.size() < 4) {
			return fallback;
		}

		return {
			json.at(0).get<float>(),
			json.at(1).get<float>(),
			json.at(2).get<float>(),
			json.at(3).get<float>(),
		};
	}

} // namespace

Sprite::Sprite(std::string objectName) {
	objectName_ = std::move(objectName);
}

// 単独使用時: 自前でユニットクワッドVB・IBを生成する
void Sprite::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName) {

	device_ = device;
	commandList_ = commandList;

	(void)SetTexture(textureName);

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

	InitializeCommonResources();
}

// SpriteManager経由: 共有ジオメトリバッファを参照する
void Sprite::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName, const SpriteSharedGeometry& sharedGeo) {

	device_ = device;
	commandList_ = commandList;

	(void)SetTexture(textureName);

	// 共有ジオメトリのVBV・IBVを参照する（このSpriteはバッファを所有しない）
	activeVBV_ = &sharedGeo.vbv;
	activeIBV_ = &sharedGeo.ibv;

	InitializeCommonResources();
}

// マテリアル・変換行列・PSOの共通初期化
void Sprite::InitializeCommonResources() {

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

	Logger::Output(objectName_ + "の初期化が完了しました", Logger::Level::Application);
}

bool Sprite::SetTexture(const std::string& textureName) {
	MadoEngine::TextureManager& textureManager = MadoEngine::TextureManager::GetInstance();
	const uint32_t textureIndex = textureManager.GetTextureIndex(textureName);
	if (textureIndex == UINT32_MAX) {
		return false;
	}

	textureIndex_ = textureIndex;
	textureName_ = textureName;
	size_ = textureManager.GetPixelSize(textureName);
	return true;
}

void Sprite::SetAnchorPoint(const Vector2& anchorPoint) {
	anchorPoint_ = {
		std::clamp(anchorPoint.x, 0.0f, 1.0f),
		std::clamp(anchorPoint.y, 0.0f, 1.0f),
	};
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

	float sx = isFitToScreen_ ? screenWidth_ : size_.x * transform_.scale.x;
	float sy = isFitToScreen_ ? screenHeight_ : size_.y * transform_.scale.y;

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
	if (textureIndex_ == UINT32_MAX || !activeVBV_ || !activeIBV_) {
		return;
	}

	// RootSignature・PSO・VBV・IBVはSpriteManagerがループ外で1回だけ設定済み

	// b0: Material, b1: Transform, t0: Texture
	commandList_->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(1, transformationResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(2, MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(textureIndex_));

	commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Sprite::FromJson(const nlohmann::json& json) {
	const std::string textureName = json.value("texture", textureName_);
	if (!textureName.empty() && !SetTexture(textureName)) {
		Logger::Output("Spriteのテクスチャ設定を復元できませんでした : " + textureName, Logger::Level::Warning);
	}

	transform_.translate = ReadVector2(json.value("position", nlohmann::json::array()), transform_.translate);
	transform_.scale = ReadVector2(json.value("scale", nlohmann::json::array()), transform_.scale);
	transform_.rotate = json.value("rotation", transform_.rotate);
	color_ = ReadVector4(json.value("color", nlohmann::json::array()), color_);
	SetAnchorPoint(ReadVector2(json.value("anchorPoint", nlohmann::json::array()), anchorPoint_));
	sceneType_ = SceneTypeFromString(json.value("scene", SceneTypeToString(sceneType_)));
	renderLayer_ = MadoEngine::Render::RenderLayerFromString(
		json.value("layer", MadoEngine::Render::RenderLayerToString(renderLayer_)));
	isVisible_ = json.value("visible", isVisible_);
	isFitToScreen_ = json.value("fitToScreen", isFitToScreen_);
}

nlohmann::json Sprite::ToJson() const {
	return {
		{ "type", "Sprite2D" },
		{ "name", objectName_ },
		{ "texture", textureName_ },
		{ "position", { transform_.translate.x, transform_.translate.y } },
		{ "scale", { transform_.scale.x, transform_.scale.y } },
		{ "rotation", transform_.rotate },
		{ "color", { color_.x, color_.y, color_.z, color_.w } },
		{ "anchorPoint", { anchorPoint_.x, anchorPoint_.y } },
		{ "scene", SceneTypeToString(sceneType_) },
		{ "layer", MadoEngine::Render::RenderLayerToString(renderLayer_) },
		{ "visible", isVisible_ },
		{ "fitToScreen", isFitToScreen_ },
	};
}
