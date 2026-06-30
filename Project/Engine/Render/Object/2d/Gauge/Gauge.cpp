#include "Gauge.h"
#include "Render/Object/2d/Sprite/Sprite.h"
#include "Render/Object/2d/Sprite/SpriteManager.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Json/Core/JsonSerializer.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <utility>

namespace {

	/// @brief GaugeDirectionをJson保存用の文字列へ変換する
	/// @param direction 変換する方向
	/// @return 保存用文字列
	const char* GaugeDirectionToString(GaugeDirection direction) {
		switch (direction) {
		case GaugeDirection::Left:
			return "Left";
		case GaugeDirection::Up:
			return "Up";
		case GaugeDirection::Down:
			return "Down";
		case GaugeDirection::Right:
		default:
			return "Right";
		}
	}

	/// @brief 文字列からGaugeDirectionへ変換する
	/// @param value 読み込んだ文字列
	/// @return 対応する方向
	GaugeDirection GaugeDirectionFromString(const std::string& value) {
		if (value == "Left") { return GaugeDirection::Left; }
		if (value == "Up") { return GaugeDirection::Up; }
		if (value == "Down") { return GaugeDirection::Down; }
		return GaugeDirection::Right;
	}

} // namespace

Gauge::Gauge(std::string objectName) {
	objectName_ = std::move(objectName);
}

Gauge::~Gauge() {
	Finalize();
}

void Gauge::Initialize(const std::string& gaugeName, SceneType sceneType, MadoEngine::Render::RenderLayer renderLayer) {
	Finalize();

	objectName_ = gaugeName.empty() ? objectName_ : gaugeName;
	backgroundSpriteName_ = objectName_ + "1";
	gaugeSpriteName_ = objectName_ + "2";
	sceneType_ = sceneType;
	renderLayer_ = renderLayer;

	LoadFromJson();

	backgroundSprite_ = MadoEngine::SpriteManager::GetInstance().Create(backgroundSpriteName_, "white2x2", sceneType_);
	gaugeSprite_ = MadoEngine::SpriteManager::GetInstance().Create(gaugeSpriteName_, "white2x2", sceneType_);

	if (!backgroundSprite_ || !gaugeSprite_) {
		Logger::Output("[Engine] Gauge用Spriteの生成に失敗しました: " + objectName_, Logger::Level::Error);
		isInitialized_ = false;
		return;
	}

	backgroundSprite_->SetFitToScreen(false);
	gaugeSprite_->SetFitToScreen(false);
	ApplyRenderSettings();

	isInitialized_ = true;
	Update();

	Logger::Output("[Engine] Gaugeを初期化しました: " + objectName_, Logger::Level::Engine);
}

void Gauge::Finalize() {
	if (!backgroundSpriteName_.empty()) {
		MadoEngine::SpriteManager::GetInstance().Destroy(backgroundSpriteName_);
	}
	if (!gaugeSpriteName_.empty()) {
		MadoEngine::SpriteManager::GetInstance().Destroy(gaugeSpriteName_);
	}

	backgroundSpriteName_.clear();
	gaugeSpriteName_.clear();
	backgroundSprite_ = nullptr;
	gaugeSprite_ = nullptr;
	isInitialized_ = false;
}

void Gauge::Update() {
	if (!isInitialized_) {
		return;
	}

	ClampValue();
	ApplyBackgroundSprite();
	ApplyGaugeSprite();
}

void Gauge::Update(float currentValue, float maxValue) {
	currentValue_ = currentValue;
	maxValue_ = maxValue;
	Update();
}

void Gauge::Update(int currentValue, int maxValue) {
	Update(static_cast<float>(currentValue), static_cast<float>(maxValue));
}

void Gauge::DrawImGui(const char* name) {
#ifdef USE_IMGUI
	if (!name) {
		return;
	}

	ImGui::Begin(name);

	ImGui::Text("Gauge: %.1f / %.1f (%.1f%%)", currentValue_, maxValue_, GetRatio() * 100.0f);
	ImGui::DragFloat2("Position", &position_.x, 1.0f);
	ImGui::DragFloat2("Size", &size_.x, 1.0f, 0.0f, 4096.0f);
	ImGui::DragFloat("Current", &currentValue_, 1.0f, 0.0f, maxValue_);
	ImGui::DragFloat("Max", &maxValue_, 1.0f, 0.0f, 999999.0f);

	const char* directionNames[] = { "Right", "Left", "Up", "Down" };
	int currentDirection = static_cast<int>(direction_);
	if (ImGui::Combo("Direction", &currentDirection, directionNames, 4)) {
		direction_ = static_cast<GaugeDirection>(currentDirection);
	}

	ImGui::Checkbox("Visible", &isVisible_);
	ImGui::Checkbox("Draw Background", &drawBackground_);
	ImGui::ColorEdit4("Background Color", &backgroundColor_.x);
	ImGui::ColorEdit4("Gauge Color", &gaugeColor_.x);

	if (ImGui::Button("Save")) {
		SaveToJson();
	}

	Update();
	ImGui::End();
#else
	(void)name;
#endif
}

bool Gauge::SaveToJson() const {
	if (objectName_.empty()) {
		Logger::Output("[Engine] Gauge名が空のためJson保存をスキップしました", Logger::Level::Warning);
		return false;
	}

	nlohmann::json json;
	json["name"] = objectName_;
	json["position"] = MadoEngine::Json::JsonSerializer::ToJson(position_);
	json["size"] = MadoEngine::Json::JsonSerializer::ToJson(size_);
	json["currentValue"] = currentValue_;
	json["maxValue"] = maxValue_;
	json["backgroundColor"] = MadoEngine::Json::JsonSerializer::ToJson(backgroundColor_);
	json["gaugeColor"] = MadoEngine::Json::JsonSerializer::ToJson(gaugeColor_);
	json["direction"] = GaugeDirectionToString(direction_);
	json["drawBackground"] = drawBackground_;
	json["visible"] = isVisible_;

	return MadoEngine::Json::JsonFile::Save(GetJsonFilePath(), json, 4, true);
}

bool Gauge::LoadFromJson() {
	if (objectName_.empty()) {
		return false;
	}

	nlohmann::json json;
	const std::string filePath = GetJsonFilePath();
	if (!MadoEngine::Json::JsonFile::Exists(filePath)) {
		return false;
	}
	if (!MadoEngine::Json::JsonFile::Load(filePath, json)) {
		return false;
	}

	position_ = MadoEngine::Json::JsonSerializer::ToVector2(json.value("position", nlohmann::json::array()), position_);
	size_ = MadoEngine::Json::JsonSerializer::ToVector2(json.value("size", nlohmann::json::array()), size_);
	size_.x = std::max(0.0f, size_.x);
	size_.y = std::max(0.0f, size_.y);
	currentValue_ = json.value("currentValue", currentValue_);
	maxValue_ = json.value("maxValue", maxValue_);
	backgroundColor_ = MadoEngine::Json::JsonSerializer::ToVector4(json.value("backgroundColor", nlohmann::json::array()), backgroundColor_);
	gaugeColor_ = MadoEngine::Json::JsonSerializer::ToVector4(json.value("gaugeColor", nlohmann::json::array()), gaugeColor_);
	direction_ = GaugeDirectionFromString(json.value("direction", GaugeDirectionToString(direction_)));
	drawBackground_ = json.value("drawBackground", drawBackground_);
	isVisible_ = json.value("visible", isVisible_);
	ClampValue();

	Logger::Output("[Engine] Gauge設定をJsonから読み込みました: " + filePath, Logger::Level::Assets);
	return true;
}

void Gauge::SetPosition(const Vector2& position) {
	position_ = position;
	Update();
}

void Gauge::SetSize(const Vector2& size) {
	size_ = {
		std::max(0.0f, size.x),
		std::max(0.0f, size.y),
	};
	Update();
}

void Gauge::SetBackgroundColor(const Vector4& color) {
	backgroundColor_ = color;
	Update();
}

void Gauge::SetGaugeColor(const Vector4& color) {
	gaugeColor_ = color;
	Update();
}

void Gauge::SetDirection(GaugeDirection direction) {
	direction_ = direction;
	Update();
}

void Gauge::SetDrawBackground(bool enabled) {
	drawBackground_ = enabled;
	Update();
}

void Gauge::SetVisible(bool visible) {
	isVisible_ = visible;
	Update();
}

void Gauge::SetCurrentValue(float value) {
	currentValue_ = value;
	Update();
}

void Gauge::SetMaxValue(float value) {
	maxValue_ = value;
	Update();
}

void Gauge::SetSceneType(SceneType sceneType) {
	sceneType_ = sceneType;
	ApplyRenderSettings();
}

void Gauge::SetRenderLayer(MadoEngine::Render::RenderLayer layer) {
	renderLayer_ = layer;
	ApplyRenderSettings();
}

float Gauge::GetRatio() const {
	if (maxValue_ <= 0.0f) {
		return 0.0f;
	}
	return currentValue_ / maxValue_;
}

void Gauge::ClampValue() {
	maxValue_ = std::max(0.0f, maxValue_);
	currentValue_ = std::clamp(currentValue_, 0.0f, maxValue_);
}

void Gauge::ApplyBackgroundSprite() {
	if (!backgroundSprite_) {
		return;
	}

	backgroundSprite_->SetPosition(position_);
	backgroundSprite_->SetScale(size_);
	backgroundSprite_->SetColor(backgroundColor_);
	backgroundSprite_->SetVisible(isVisible_ && drawBackground_);
}

void Gauge::ApplyGaugeSprite() {
	if (!gaugeSprite_) {
		return;
	}

	const float ratio = std::clamp(GetRatio(), 0.0f, 1.0f);
	Vector2 gaugePosition = position_;
	Vector2 gaugeSize = size_;

	switch (direction_) {
	case GaugeDirection::Right:
		gaugeSize.x = size_.x * ratio;
		break;
	case GaugeDirection::Left:
		gaugeSize.x = size_.x * ratio;
		gaugePosition.x = position_.x + size_.x - gaugeSize.x;
		break;
	case GaugeDirection::Down:
		gaugeSize.y = size_.y * ratio;
		break;
	case GaugeDirection::Up:
		gaugeSize.y = size_.y * ratio;
		gaugePosition.y = position_.y + size_.y - gaugeSize.y;
		break;
	}

	gaugeSprite_->SetPosition(gaugePosition);
	gaugeSprite_->SetScale(gaugeSize);
	gaugeSprite_->SetColor(gaugeColor_);
	gaugeSprite_->SetVisible(isVisible_ && ratio > 0.0f);
}

void Gauge::ApplyRenderSettings() {
	if (backgroundSprite_) {
		backgroundSprite_->SetSceneType(sceneType_);
		backgroundSprite_->SetRenderLayer(renderLayer_);
	}
	if (gaugeSprite_) {
		gaugeSprite_->SetSceneType(sceneType_);
		gaugeSprite_->SetRenderLayer(renderLayer_);
	}
}

std::string Gauge::GetJsonFilePath() const {
	return "Assets/Json/Gauge/" + objectName_ + ".json";
}