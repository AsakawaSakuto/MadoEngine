#include "Text.h"
#include "TextManager.h"
#include "TextSdfGenerator.h"
#include "Math/Function/MatrixFunction.h"
#include "Utility/Logger/Logger.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include <Windows.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <format>
#include <utility>

namespace MadoEngine {

namespace {

	/// @brief Text描画方式を保存用文字列へ変換
	/// @param mode 変換する描画方式
	/// @return 保存用の描画方式名
	const char* GetRenderModeName(TextRenderMode mode) {
		if (mode == TextRenderMode::NumericAtlas) {
			return "NumericAtlas";
		}
		return "Rasterized";
	}

	/// @brief Json配列からVector2を読み込み
	/// @param json 読み込むJson
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込まれたVector2
	Vector2 ReadVector2(const nlohmann::json& json, const Vector2& fallback) {
		if (!json.is_array() || json.size() < 2) {
			return fallback;
		}
		return {
			json.at(0).get<float>(),
			json.at(1).get<float>(),
		};
	}

	/// @brief Json配列からVector4を読み込み
	/// @param json 読み込むJson
	/// @param fallback 読み込みに失敗した場合の値
	/// @return 読み込まれたVector4
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

const char* GetTextFontDisplayName(TextFontFamilyType type) {
	for (const TextFontDefinition& definition : kTextFontDefinitions) {
		if (definition.type == type) {
			return definition.displayName;
		}
	}
	return "Unknown";
}

const char* GetTextFontFamilyName(TextFontFamilyType type) {
	for (const TextFontDefinition& definition : kTextFontDefinitions) {
		if (definition.type == type) {
			return definition.familyName;
		}
	}
	return GetTextFontFamilyName(TextFontFamilyType::YuGothicUI);
}

TextFontFamilyType GetTextFontFamilyTypeFromName(const std::string& fontFamily) {
	for (const TextFontDefinition& definition : kTextFontDefinitions) {
		if (fontFamily == definition.familyName) {
			return definition.type;
		}
	}
	return TextFontFamilyType::Invalid;
}

Text::Text(std::string objectName) {
	objectName_ = std::move(objectName);
	textureKey_ = "__Text_" + objectName_;
	textureIndex_ = UINT32_MAX;
}

void Text::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName) {
	(void)textureName;
	assert(device != nullptr);
	assert(commandList != nullptr);

	device_ = device;
	commandList_ = commandList;

	// Managerを介さない単独利用向けにUnit Quad Geometryを所有
	SpriteVertexData* vertexData = CreateMappedBuffer<SpriteVertexData>(device_.Get(), vertexResource_, 4);
	vertexData[0].position = { 0.0f, 1.0f, 0.0f, 1.0f };
	vertexData[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	vertexData[2].position = { 1.0f, 1.0f, 0.0f, 1.0f };
	vertexData[3].position = { 1.0f, 0.0f, 0.0f, 1.0f };
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

	activeVBV_ = &vertexBufferView_;
	activeIBV_ = &indexBufferView_;

	InitializeCommonResources();
}

void Text::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const SpriteSharedGeometry& sharedGeo) {
	assert(device != nullptr);
	assert(commandList != nullptr);

	device_ = device;
	commandList_ = commandList;

	// Manager経由では共有Geometryを参照してTextごとのBuffer重複を回避
	activeVBV_ = &sharedGeo.vbv;
	activeIBV_ = &sharedGeo.ibv;

	InitializeCommonResources();
}

void Text::InitializeCommonResources() {
	materialData_ = CreateMappedBuffer<TextMaterial>(device_.Get(), materialResource_);
	materialData_->color = color_;
	materialData_->uvTransformMatrix = Matrix::MakeIdentity();
	materialData_->outlineColor = outlineColor_;
	materialData_->textureSize = {};
	materialData_->sdfRange = 0.0f;
	materialData_->outlineWidth = outlineWidth_;

	transformationData_ = CreateMappedBuffer<SpriteTransformationMatrix>(device_.Get(), transformationResource_);
	transformationData_->WVP = Matrix::MakeIdentity();

	psoDesc_.inputLayout = Render::InputLayoutType::Sprite;
	psoDesc_.vsKey = "Object2d/Sprite/Sprite.VS";
	psoDesc_.psKey = "Object2d/Text/Text.PS";
	psoDesc_.rootSigKey = "Sprite.RootSig";
	psoDesc_.depthMode = Render::DepthMode::Disable;
	psoDesc_.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc_.blendMode = Render::BlendMode::Normal;
	psoDesc_.rtvFormat = Render::kDisplayRenderTargetFormat;

	Logger::Output("[Engine] Textを初期化しました: " + objectName_, Logger::Level::Engine);
}

void Text::Update() {
	RebuildTextureIfNeeded();

	materialData_->color = color_;
	materialData_->outlineColor = outlineColor_;
	materialData_->outlineWidth = outlineWidth_;
	materialData_->textureSize = texturePixelSize_;
	materialData_->sdfRange = 0.0f;
	if (usesSdf_) {
		materialData_->sdfRange = TextSdfGenerator::kDistanceRange;
	}

	const float sx = size_.x * transform_.scale.x;
	const float sy = size_.y * transform_.scale.y;

	Matrix4x4 scaleMatrix = Matrix::MakeScale({ sx, sy, 1.0f });
	Matrix4x4 anchorMatrix = Matrix::MakeTranslate({ -anchorPoint_.x * sx, -anchorPoint_.y * sy, 0.0f });
	Matrix4x4 rotateMatrix = Matrix::MakeRotateZ(transform_.rotate);
	Matrix4x4 transMatrix = Matrix::MakeTranslate({ transform_.translate.x, transform_.translate.y, 0.0f });

	// Anchor補正をScale後かつRotation前に適用して指定位置を回転中心として維持
	Matrix4x4 worldMatrix = Matrix::Multiply(
		Matrix::Multiply(Matrix::Multiply(scaleMatrix, anchorMatrix), rotateMatrix),
		transMatrix
	);

	Matrix4x4 orthoMatrix = Matrix::MakeOrthographic(0.0f, 0.0f, screenWidth_, screenHeight_, 0.0f, 1.0f);
	transformationData_->WVP = Matrix::Multiply(worldMatrix, orthoMatrix);
}

void Text::Draw() {
	if (textureIndex_ == UINT32_MAX || !activeVBV_ || !activeIBV_) {
		return;
	}

	commandList_->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootConstantBufferView(1, transformationResource_->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance().GetSrvHandleGPU(textureIndex_));
	if (usesAtlas_ || usesSdf_) {

		// 文字列全体を一つの頂点領域から描画し、文字数によるDrawCallの増加を抑制
		if (textVertexCount_ > 0) {
			commandList_->IASetVertexBuffers(0, 1, &textVertexBufferView_);
			commandList_->DrawInstanced(textVertexCount_, 1, 0, 0);
		}
		return;
	}

	// アトラス描画後でも次の通常Textへ共有Quadを復元
	commandList_->IASetVertexBuffers(0, 1, activeVBV_);
	commandList_->IASetIndexBuffer(activeIBV_);
	commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Text::ReleaseTexture() {

	// 共有アトラスの寿命はManagerへ委譲し、Text固有の画像だけを解放
	if (hasOwnedTexture_) {
		TextureManager::GetInstance().DestroyTexture(textureKey_);
	}
	hasOwnedTexture_ = false;
	usesAtlas_ = false;
	usesSdf_ = false;
	texturePixelSize_ = {};
	atlas_ = nullptr;
	textVertexResource_.Reset();
	textVertices_ = nullptr;
	textVertexCount_ = 0;
	textureIndex_ = UINT32_MAX;
	size_ = {};
	isTextureDirty_ = true;
}

bool Text::SetObjectName(const std::string& objectName) {
	if (objectName_ == objectName) {
		return true;
	}

	const std::string newTextureKey = "__Text_" + objectName;
	if (hasOwnedTexture_ &&
		!TextureManager::GetInstance().RenameTexture(textureKey_, newTextureKey)) {
		return false;
	}

	objectName_ = objectName;
	textureKey_ = newTextureKey;
	return true;
}

void Text::SetRenderMode(TextRenderMode mode) {
	if (renderMode_ == mode) {
		return;
	}

	// 方式切り替え後の次回更新で共有画像か個別画像を再選択
	renderMode_ = mode;
	MarkDirty();
}

void Text::SetAtlasRasterSize(float fontSize) {
	if (!std::isfinite(fontSize)) {
		fontSize = 128.0f;
	}
	fontSize = std::clamp(fontSize, 8.0f, 512.0f);
	if (atlasRasterSize_ == fontSize) {
		return;
	}

	// 表示上の拡縮とは独立した画質設定として別の共有アトラスを選択
	atlasRasterSize_ = fontSize;
	MarkDirty();
}

bool Text::PrepareRendering() {
	RebuildTextureIfNeeded();
	return textureIndex_ != UINT32_MAX;
}

void Text::SetSdfEnabled(bool enabled) {
	if (sdfEnabled_ == enabled) {
		return;
	}

	// 表示方式だけを切り替え、縁の色や太さの変更とは生成要求を分離
	sdfEnabled_ = enabled;
	MarkDirty();
}

void Text::SetOutlineWidth(float width) {
	if (!std::isfinite(width)) {
		width = 0.0f;
	}
	outlineWidth_ = std::clamp(width, 0.0f, 32.0f);
}

void Text::SetText(const std::string& text) {
	if (text_ == text) {
		return;
	}
	text_ = text;
	MarkDirty();
}

void Text::SetFontFamily(const std::string& fontFamily) {
	if (fontFamily_ == fontFamily && fontFilePath_.empty()) {
		return;
	}
	fontFamily_ = fontFamily;
	fontFilePath_.clear();
	MarkDirty();
}

void Text::SetFontAsset(const std::string& fontFilePath, const std::string& fontFamily) {
	if (fontFilePath_ == fontFilePath && fontFamily_ == fontFamily) {
		return;
	}
	fontFilePath_ = fontFilePath;
	fontFamily_ = fontFamily;
	MarkDirty();
}

void Text::SetFontSize(float fontSize) {
	fontSize = std::max(1.0f, fontSize);
	if (fontSize_ == fontSize) {
		return;
	}
	fontSize_ = fontSize;
	MarkDirty();
}

void Text::SetLineSpacing(float lineSpacing) {
	lineSpacing = std::max(0.1f, lineSpacing);
	if (lineSpacing_ == lineSpacing) {
		return;
	}
	lineSpacing_ = lineSpacing;
	MarkDirty();
}

void Text::SetCharacterSpacing(float characterSpacing) {
	characterSpacing = std::clamp(characterSpacing, -64.0f, 256.0f);
	if (characterSpacing_ == characterSpacing) {
		return;
	}
	characterSpacing_ = characterSpacing;
	MarkDirty();
}

void Text::SetAreaSize(const Vector2& size) {
	if (areaSize_ == size) {
		return;
	}
	areaSize_ = {
		std::max(0.0f, size.x),
		std::max(0.0f, size.y),
	};
	MarkDirty();
}

void Text::SetAnchorPoint(const Vector2& anchorPoint) {
	const Vector2 clampedAnchorPoint = {
		std::clamp(anchorPoint.x, 0.0f, 1.0f),
		std::clamp(anchorPoint.y, 0.0f, 1.0f),
	};
	if (anchorPoint_ == clampedAnchorPoint) {
		return;
	}
	anchorPoint_ = clampedAnchorPoint;
}

void Text::SetHorizontalAlign(TextHorizontalAlign align) {
	if (horizontalAlign_ == align) {
		return;
	}
	horizontalAlign_ = align;
	MarkDirty();
}

void Text::SetVerticalAlign(TextVerticalAlign align) {
	if (verticalAlign_ == align) {
		return;
	}
	verticalAlign_ = align;
	MarkDirty();
}

void Text::SetWordWrap(bool enabled) {
	if (wordWrap_ == enabled) {
		return;
	}
	wordWrap_ = enabled;
	MarkDirty();
}

void Text::FromJson(const nlohmann::json& json) {
	objectName_ = json.value("name", objectName_);
	textureKey_ = "__Text_" + objectName_;
	text_ = json.value("text", text_);
	fontFamily_ = json.value("fontFamily", fontFamily_);
	fontFilePath_ = json.value("fontFile", std::string{});
	fontSize_ = json.value("fontSize", fontSize_);
	SetRenderMode(TextRenderMode::Rasterized);
	if (json.value("renderMode", std::string{}) == "NumericAtlas") {
		SetRenderMode(TextRenderMode::NumericAtlas);
	}
	SetAtlasRasterSize(json.value("atlasRasterSize", 128.0f));
	SetSdfEnabled(json.value("sdfEnabled", false));
	SetOutlineWidth(json.value("outlineWidth", 0.0f));
	outlineColor_ = ReadVector4(json.value("outlineColor", nlohmann::json::array()), { 0.0f, 0.0f, 0.0f, 1.0f });
	lineSpacing_ = std::max(0.1f, json.value("lineSpacing", lineSpacing_));
	characterSpacing_ = std::clamp(json.value("characterSpacing", characterSpacing_), -64.0f, 256.0f);
	areaSize_ = ReadVector2(json.value("size", nlohmann::json::array()), areaSize_);
	const Vector2 loadedAnchorPoint = ReadVector2(json.value("anchorPoint", nlohmann::json::array()), anchorPoint_);
	anchorPoint_ = {
		std::clamp(loadedAnchorPoint.x, 0.0f, 1.0f),
		std::clamp(loadedAnchorPoint.y, 0.0f, 1.0f),
	};
	transform_.translate = ReadVector2(json.value("position", nlohmann::json::array()), transform_.translate);
	transform_.scale = ReadVector2(json.value("scale", nlohmann::json::array()), transform_.scale);
	transform_.rotate = json.value("rotation", transform_.rotate);
	color_ = ReadVector4(json.value("color", nlohmann::json::array()), color_);
	horizontalAlign_ = HorizontalAlignFromString(json.value("horizontalAlign", HorizontalAlignToString(horizontalAlign_)));
	verticalAlign_ = VerticalAlignFromString(json.value("verticalAlign", VerticalAlignToString(verticalAlign_)));
	wordWrap_ = json.value("wordWrap", wordWrap_);
	targetScreen_ = json.value("screen", targetScreen_);
	sceneType_ = SceneTypeFromString(json.value("scene", SceneTypeToString(sceneType_)));
	renderLayer_ = Render::RenderLayerFromString(json.value("layer", Render::RenderLayerToString(renderLayer_)));
	isVisible_ = json.value("visible", isVisible_);
	MarkDirty();
}

nlohmann::json Text::ToJson() const {
	return {
		{ "type", "Text2D" },
		{ "name", objectName_ },
		{ "text", text_ },
		{ "fontFamily", fontFamily_ },
		{ "fontFile", fontFilePath_ },
		{ "fontSize", fontSize_ },
		{ "renderMode", GetRenderModeName(renderMode_) },
		{ "atlasRasterSize", atlasRasterSize_ },
		{ "sdfEnabled", sdfEnabled_ },
		{ "outlineWidth", outlineWidth_ },
		{ "outlineColor", { outlineColor_.x, outlineColor_.y, outlineColor_.z, outlineColor_.w } },
		{ "lineSpacing", lineSpacing_ },
		{ "characterSpacing", characterSpacing_ },
		{ "color", { color_.x, color_.y, color_.z, color_.w } },
		{ "position", { transform_.translate.x, transform_.translate.y } },
		{ "scale", { transform_.scale.x, transform_.scale.y } },
		{ "rotation", transform_.rotate },
		{ "size", { areaSize_.x, areaSize_.y } },
		{ "anchorPoint", { anchorPoint_.x, anchorPoint_.y } },
		{ "horizontalAlign", HorizontalAlignToString(horizontalAlign_) },
		{ "verticalAlign", VerticalAlignToString(verticalAlign_) },
		{ "wordWrap", wordWrap_ },
		{ "screen", targetScreen_ },
		{ "scene", SceneTypeToString(sceneType_) },
		{ "layer", Render::RenderLayerToString(renderLayer_) },
		{ "visible", isVisible_ },
	};
}

void Text::RebuildTextureIfNeeded() {
	if (!isTextureDirty_) {
		return;
	}

	// 数字更新は共有画像と確保済み頂点で完結し、対象外の文字だけを従来方式へ委譲
	if (renderMode_ == TextRenderMode::NumericAtlas && RebuildAtlasGeometry()) {
		usesAtlas_ = true;
		isTextureDirty_ = false;
		return;
	}
	usesAtlas_ = false;
	usesSdf_ = false;

	TextTextureDesc desc{};
	desc.text = Utf8ToWide(text_);
	desc.fontFamily = Utf8ToWide(fontFamily_);
	desc.fontFilePath = fontFilePath_;
	desc.fontSize = fontSize_;
	desc.lineSpacing = lineSpacing_;
	desc.characterSpacing = characterSpacing_;
	desc.areaSize = areaSize_;
	desc.horizontalAlign = horizontalAlign_;
	desc.verticalAlign = verticalAlign_;
	desc.wordWrap = wordWrap_;

	TextTexturePixels pixels{};
	float rasterScale = 1.0f;
	if (sdfEnabled_ && std::isfinite(fontSize_)) {

		// 小さい文字も輪郭情報を十分に残し、通常文字列の画像化倍率は最大4倍に制限
		rasterScale = std::clamp(128.0f / fontSize_, 1.0f, 4.0f);
	}

	// 文字列やFont設定の変更時だけCPU RasterizeとGPU Texture更新を実行
	if (!TextTextureGenerator::GetInstance().Generate(desc, pixels, rasterScale)) {
		Logger::Output("[Engine] Textテクスチャの生成に失敗しました: " + objectName_, Logger::Level::Error);
		textureIndex_ = UINT32_MAX;
		isTextureDirty_ = false;
		return;
	}
	size_ = {
		std::ceil(static_cast<float>(pixels.width) / rasterScale),
		std::ceil(static_cast<float>(pixels.height) / rasterScale),
	};
	if (sdfEnabled_) {

		// 配置の大きさは余白追加前のまま保持し、縁の領域だけをQuadの外側へ拡張
		TextTexturePixels sdfPixels;
		if (TextSdfGenerator::Generate(pixels, sdfPixels)) {
			pixels = std::move(sdfPixels);
			usesSdf_ = true;
			RebuildSdfQuad({ static_cast<float>(pixels.width), static_cast<float>(pixels.height) }, rasterScale);
		}
	}
	texturePixelSize_ = { static_cast<float>(pixels.width), static_cast<float>(pixels.height) };

	// 同じTexture Keyを更新してSprite側のDescriptor Indexを安定化
	textureIndex_ = TextureManager::GetInstance().RegisterOrUpdateRGBA(
		textureKey_,
		pixels.width,
		pixels.height,
		pixels.pixels.data(),
		static_cast<uint32_t>(pixels.pixels.size()));
	if (textureIndex_ != UINT32_MAX) {
		hasOwnedTexture_ = true;
	}
	isTextureDirty_ = false;
}

bool Text::RebuildAtlasGeometry() {

	// 通常とCriticalの表示サイズが切り替わっても同じ基準サイズの画像を共有
	if (!atlas_ || atlas_->fontFamily != fontFamily_ || atlas_->fontFilePath != fontFilePath_ || atlas_->rasterSize != atlasRasterSize_ || atlas_->sdfEnabled != sdfEnabled_) {
		atlas_ = TextManager::GetInstance().AcquireNumericAtlas(fontFamily_, fontFilePath_, atlasRasterSize_, sdfEnabled_);
	}
	if (!atlas_) {
		return false;
	}
	TextAtlasLayoutDesc desc;
	desc.fontSize = fontSize_;
	desc.lineSpacing = lineSpacing_;
	desc.characterSpacing = characterSpacing_;
	desc.areaSize = areaSize_;
	desc.horizontalAlign = horizontalAlign_;
	desc.verticalAlign = verticalAlign_;
	desc.wordWrap = wordWrap_;
	std::array<SpriteVertexData, TextGlyphAtlas::kMaximumVertices> vertices;
	Vector2 size;
	uint32_t vertexCount = 0;
	if (!atlas_->glyphs.BuildVertices(text_, desc, vertices, vertexCount, size)) {
		return false;
	}
	UpdateTextVertices({ vertices.data(), vertexCount });
	textureIndex_ = atlas_->textureIndex;
	texturePixelSize_ = atlas_->textureSize;
	usesSdf_ = atlas_->sdfEnabled;
	size_ = size;
	return true;
}

void Text::RebuildSdfQuad(const Vector2& textureSize, float rasterScale) {
	const float padding = static_cast<float>(TextSdfGenerator::kPadding) / rasterScale;
	float left = -padding;
	float top = -padding;
	float right = textureSize.x / rasterScale - padding;
	float bottom = textureSize.y / rasterScale - padding;
	Vector2 uvMin = {};
	Vector2 uvMax = { 1.0f, 1.0f };

	// 自動サイズでは縁の余白を許容し、明示サイズでは従来の表示領域によるClipを維持
	if (areaSize_.x > 0.0f) {
		uvMin.x = padding / (right - left);
		uvMax.x = (size_.x + padding) / (right - left);
		left = 0.0f;
		right = size_.x;
	}
	if (areaSize_.y > 0.0f) {
		uvMin.y = padding / (bottom - top);
		uvMax.y = (size_.y + padding) / (bottom - top);
		top = 0.0f;
		bottom = size_.y;
	}
	const float x0 = left / size_.x;
	const float y0 = top / size_.y;
	const float x1 = right / size_.x;
	const float y1 = bottom / size_.y;
	const SpriteVertexData bottomLeft = { { x0, y1, 0.0f, 1.0f }, { uvMin.x, uvMax.y } };
	const SpriteVertexData topLeft = { { x0, y0, 0.0f, 1.0f }, { uvMin.x, uvMin.y } };
	const SpriteVertexData bottomRight = { { x1, y1, 0.0f, 1.0f }, { uvMax.x, uvMax.y } };
	const SpriteVertexData topRight = { { x1, y0, 0.0f, 1.0f }, { uvMax.x, uvMin.y } };
	const std::array<SpriteVertexData, 6> vertices = { bottomLeft, topLeft, bottomRight, topLeft, topRight, bottomRight };
	UpdateTextVertices(vertices);
}

void Text::UpdateTextVertices(std::span<const SpriteVertexData> vertices) {
	if (!textVertexResource_) {

		// 事前準備で最大桁数まで確保し、ダメージ更新時のGPU Buffer再生成を回避
		textVertices_ = CreateMappedBuffer<SpriteVertexData>(device_.Get(), textVertexResource_, TextGlyphAtlas::kMaximumVertices, false);
		textVertexBufferView_.BufferLocation = textVertexResource_->GetGPUVirtualAddress();
		textVertexBufferView_.StrideInBytes = sizeof(SpriteVertexData);
		textVertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(SpriteVertexData) * TextGlyphAtlas::kMaximumVertices);
	}

	// EngineのFrameごとのGPU完了待機後に確保済みUpload領域を更新
	std::copy(vertices.begin(), vertices.end(), textVertices_);
	textVertexCount_ = static_cast<uint32_t>(vertices.size());
}

std::wstring Text::Utf8ToWide(const std::string& text) {
	if (text.empty()) {
		return {};
	}

	const int wideLength = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
	if (wideLength <= 0) {
		return {};
	}

	std::wstring result(static_cast<size_t>(wideLength), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), wideLength);
	return result;
}

std::string Text::HorizontalAlignToString(TextHorizontalAlign align) {
	switch (align) {
	case TextHorizontalAlign::Center:
		return "Center";
	case TextHorizontalAlign::Right:
		return "Right";
	case TextHorizontalAlign::Left:
	default:
		return "Left";
	}
}

TextHorizontalAlign Text::HorizontalAlignFromString(const std::string& value) {
	if (value == "Center") { return TextHorizontalAlign::Center; }
	if (value == "Right") { return TextHorizontalAlign::Right; }
	return TextHorizontalAlign::Left;
}

std::string Text::VerticalAlignToString(TextVerticalAlign align) {
	switch (align) {
	case TextVerticalAlign::Center:
		return "Center";
	case TextVerticalAlign::Bottom:
		return "Bottom";
	case TextVerticalAlign::Top:
	default:
		return "Top";
	}
}

TextVerticalAlign Text::VerticalAlignFromString(const std::string& value) {
	if (value == "Center") { return TextVerticalAlign::Center; }
	if (value == "Bottom") { return TextVerticalAlign::Bottom; }
	return TextVerticalAlign::Top;
}

} // namespace MadoEngine
