#include "Text.h"
#include "Math/Function/MatrixFunction.h"
#include "Utility/Logger/Logger.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include <Windows.h>
#include <algorithm>
#include <cassert>
#include <format>
#include <utility>

namespace MadoEngine {

namespace {

	/// @brief Json配列からVector2を読み込みます。
	/// @param json 読み込むJson。
	/// @param fallback 読み込みに失敗した場合の値。
	/// @return 読み込まれたVector2。
	Vector2 ReadVector2(const nlohmann::json& json, const Vector2& fallback) {
		if (!json.is_array() || json.size() < 2) {
			return fallback;
		}
		return {
			json.at(0).get<float>(),
			json.at(1).get<float>(),
		};
	}

	/// @brief Json配列からVector4を読み込みます。
	/// @param json 読み込むJson。
	/// @param fallback 読み込みに失敗した場合の値。
	/// @return 読み込まれたVector4。
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
	return TextFontFamilyType::Count;
}

Text::Text(std::string objectName) {
	objectName_ = std::move(objectName);
	textureKey_ = "__Text_" + objectName_;
}

void Text::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName) {
	(void)textureName;
	assert(device != nullptr);
	assert(commandList != nullptr);

	device_ = device;
	commandList_ = commandList;

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
	activeVBV_ = &sharedGeo.vbv;
	activeIBV_ = &sharedGeo.ibv;

	InitializeCommonResources();
}

void Text::InitializeCommonResources() {
	materialData_ = CreateMappedBuffer<SpriteMaterial>(device_.Get(), materialResource_);
	materialData_->color = color_;
	materialData_->uvTransformMatrix = Matrix::MakeIdentity();

	transformationData_ = CreateMappedBuffer<SpriteTransformationMatrix>(device_.Get(), transformationResource_);
	transformationData_->WVP = Matrix::MakeIdentity();

	psoDesc_.inputLayout = Render::InputLayoutType::Sprite;
	psoDesc_.vsKey = "Object2d/Sprite/Sprite.VS";
	psoDesc_.psKey = "Object2d/Sprite/Sprite.PS";
	psoDesc_.rootSigKey = "Sprite.RootSig";
	psoDesc_.depthMode = Render::DepthMode::Disable;
	psoDesc_.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc_.blendMode = Render::BlendMode::Normal;

	Logger::Output("[Engine] Textを初期化しました: " + objectName_, Logger::Level::Engine);
}

void Text::Update() {
	RebuildTextureIfNeeded();

	materialData_->color = color_;

	const float sx = size_.x * transform_.scale.x;
	const float sy = size_.y * transform_.scale.y;

	Matrix4x4 scaleMatrix = Matrix::MakeScale({ sx, sy, 1.0f });
	Matrix4x4 anchorMatrix = Matrix::MakeTranslate({ -anchorPoint_.x * sx, -anchorPoint_.y * sy, 0.0f });
	Matrix4x4 rotateMatrix = Matrix::MakeRotateZ(transform_.rotate);
	Matrix4x4 transMatrix = Matrix::MakeTranslate({ transform_.translate.x, transform_.translate.y, 0.0f });
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
	commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Text::ReleaseTexture() {
	if (textureKey_.empty() || textureIndex_ == UINT32_MAX) {
		return;
	}

	TextureManager::GetInstance().DestroyTexture(textureKey_);
	textureIndex_ = UINT32_MAX;
	size_ = {};
	isTextureDirty_ = true;
}

void Text::SetText(const std::string& text) {
	if (text_ == text) {
		return;
	}
	text_ = text;
	MarkDirty();
}

void Text::SetFontFamily(const std::string& fontFamily) {
	if (fontFamily_ == fontFamily) {
		return;
	}
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
	fontSize_ = json.value("fontSize", fontSize_);
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
	renderLayer_ = RenderLayerFromString(json.value("layer", RenderLayerToString(renderLayer_)));
	isVisible_ = json.value("visible", isVisible_);
	MarkDirty();
}

nlohmann::json Text::ToJson() const {
	return {
		{ "type", "Text2D" },
		{ "name", objectName_ },
		{ "text", text_ },
		{ "fontFamily", fontFamily_ },
		{ "fontSize", fontSize_ },
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
		{ "layer", RenderLayerToString(renderLayer_) },
		{ "visible", isVisible_ },
	};
}

void Text::RebuildTextureIfNeeded() {
	if (!isTextureDirty_) {
		return;
	}

	TextTextureDesc desc{};
	desc.text = Utf8ToWide(text_);
	desc.fontFamily = Utf8ToWide(fontFamily_);
	desc.fontSize = fontSize_;
	desc.lineSpacing = lineSpacing_;
	desc.characterSpacing = characterSpacing_;
	desc.areaSize = areaSize_;
	desc.horizontalAlign = horizontalAlign_;
	desc.verticalAlign = verticalAlign_;
	desc.wordWrap = wordWrap_;

	TextTexturePixels pixels{};
	if (!TextTextureGenerator::GetInstance().Generate(desc, pixels)) {
		Logger::Output("[Engine] Textテクスチャの生成に失敗しました: " + objectName_, Logger::Level::Error);
		textureIndex_ = UINT32_MAX;
		return;
	}

	textureIndex_ = TextureManager::GetInstance().RegisterOrUpdateRGBA(
		textureKey_,
		pixels.width,
		pixels.height,
		pixels.pixels.data(),
		static_cast<uint32_t>(pixels.pixels.size()));
	size_ = {
		static_cast<float>(pixels.width),
		static_cast<float>(pixels.height),
	};
	isTextureDirty_ = false;

	Logger::Output(
		"[Engine] Textテクスチャを更新しました: " + objectName_ + " (" +
		std::to_string(pixels.width) + "x" + std::to_string(pixels.height) + ")",
		Logger::Level::Assets);
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

std::string Text::RenderLayerToString(Render::RenderLayer layer) {
	switch (layer) {
	case Render::RenderLayer::World:
		return "World";
	case Render::RenderLayer::MapEventObject:
		return "MapEventObject";
	case Render::RenderLayer::Player:
		return "Player";
	case Render::RenderLayer::Effect:
		return "Effect";
	case Render::RenderLayer::UI:
		return "UI";
	case Render::RenderLayer::Debug:
		return "Debug";
	case Render::RenderLayer::Default:
	default:
		return "Default";
	}
}

Render::RenderLayer Text::RenderLayerFromString(const std::string& value) {
	if (value == "World") { return Render::RenderLayer::World; }
	if (value == "MapEventObject") { return Render::RenderLayer::MapEventObject; }
	if (value == "Player") { return Render::RenderLayer::Player; }
	if (value == "Effect") { return Render::RenderLayer::Effect; }
	if (value == "UI") { return Render::RenderLayer::UI; }
	if (value == "Debug") { return Render::RenderLayer::Debug; }
	return Render::RenderLayer::Default;
}

} // namespace MadoEngine
