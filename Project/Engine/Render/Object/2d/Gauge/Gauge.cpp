#include "Gauge.h"
#include "Render/Object/2d/UIGroup/UIGroup.h"
#include "Render/Object/2d/Sprite/Sprite.h"
#include "Render/Object/2d/Sprite/SpriteManager.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Json/Core/JsonSerializer.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace {

	uint64_t nextGaugeSpriteId = 0;

	/// @brief GaugeDirectionをJson保存用の文字列へ変換
	/// @param direction 変換する方向
	/// @return 保存用文字列
	const char* GaugeDirectionToString(GaugeDirection direction) {

		// 保存名を列挙値の順序から独立させ、既存設定との互換性を維持
		switch (direction) {
		case GaugeDirection::Left: return "Left";
		case GaugeDirection::Up: return "Up";
		case GaugeDirection::Down: return "Down";
		default: return "Right";
		}
	}

	/// @brief ゲージ設定の数値と識別子を検証
	/// @param settings 検証する設定
	/// @return 全項目が有効な場合はtrue
	bool IsValidSettings(const Gauge2d::Settings& settings) {
		const auto finiteColor = [](const Vector4& color) {
			return std::isfinite(color.x) && std::isfinite(color.y) &&
				std::isfinite(color.z) && std::isfinite(color.w);
		};

		// 非有限値と不正な列挙値を適用前に拒否し、描画行列への伝播を防止
		return std::isfinite(settings.size.x) && std::isfinite(settings.size.y) &&
			settings.size.x >= 0.0f && settings.size.y >= 0.0f &&
			finiteColor(settings.backgroundColor) && finiteColor(settings.gaugeColor) &&
			!settings.backgroundTexture.empty() && !settings.gaugeTexture.empty() &&
			settings.direction >= GaugeDirection::Right && settings.direction <= GaugeDirection::Down &&
			MadoEngine::Render::IsValidRenderLayer(settings.renderLayer);
	}

} // namespace

Gauge2d::Gauge2d(std::string objectName)
	: objectName_(std::move(objectName)), ownedGroup_(std::make_unique<MadoEngine::UIGroup>()), group_(ownedGroup_.get()) {
}

Gauge2d::Gauge2d(MadoEngine::UIGroup& group, std::string objectName)
	: objectName_(std::move(objectName)), group_(&group) {
}

Gauge2d::~Gauge2d() {
	Finalize();
}

void Gauge2d::Initialize(const std::string& gaugeName, SceneType sceneType, MadoEngine::Render::RenderLayer renderLayer) {
	Finalize();
	if (!gaugeName.empty()) {
		objectName_ = gaugeName;
	}
	sceneType_ = sceneType;
	settings_.renderLayer = renderLayer;

	// 単独利用時だけ旧保存先を参照し、Manager管理の設定はUIGroupのJsonへ集約
	if (ownedGroup_) {
		LoadFromJson();
	}
	InitializeSprites();
}

bool Gauge2d::InitializeSprites() {
	auto& manager = MadoEngine::SpriteManager::GetInstance();
	const std::string prefix = objectName_ + "_" + std::to_string(++nextGaugeSpriteId);
	backgroundSprite_ = manager.Create(prefix + "_Background", settings_.backgroundTexture, sceneType_);
	gaugeSprite_ = manager.Create(prefix + "_Fill", settings_.gaugeTexture, sceneType_);
	Sprite* background = manager.TryGet(backgroundSprite_);
	Sprite* fill = manager.TryGet(gaugeSprite_);

	// 部分生成の失敗でも残存Spriteを描画させず、GPU安全時点でまとめて回収
	if (!background || !fill || !background->SetParent(*group_) || !fill->SetParent(*group_)) {
		Finalize();
		Logger::Output("2Dゲージ用Spriteの生成に失敗しました: " + objectName_, Logger::Level::Error);
		return false;
	}
	background->SetFitToScreen(false);
	fill->SetFitToScreen(false);
	isInitialized_ = true;
	ApplyRenderSettings();
	Update();
	return true;
}

void Gauge2d::Finalize() {
	auto& manager = MadoEngine::SpriteManager::GetInstance();

	// 描画直後のEditor操作でも参照中のGPUリソースを即時解放しないための遅延破棄
	for (auto handle : { backgroundSprite_, gaugeSprite_ }) {
		if (Sprite* sprite = manager.TryGet(handle)) {
			sprite->SetRuntimeVisible(false);
			sprite->ClearParent();
			manager.RequestDestroy(handle);
		}
	}
	backgroundSprite_ = {};
	gaugeSprite_ = {};
	isInitialized_ = false;
}

void Gauge2d::Update() {

	// 未初期化およびシーン終了後の参照を除外し、編集用設定を維持したまま割合だけを反映
	if (!isInitialized_) {
		return;
	}
	ClampValue();
	ApplyBackgroundSprite();
	ApplyGaugeSprite();
}

void Gauge2d::Update(float currentValue, float maxValue) {
	currentValue_ = currentValue;
	maxValue_ = maxValue;
	Update();
}

void Gauge2d::Update(int currentValue, int maxValue) {
	Update(static_cast<float>(currentValue), static_cast<float>(maxValue));
}

bool Gauge2d::SetSettings(const Settings& settings) {
	if (!IsValidSettings(settings)) {
		return false;
	}
	auto& textures = MadoEngine::TextureManager::GetInstance();

	// テクスチャの両方を検証してから変更し、片側だけの更新を防止
	if (isInitialized_ &&
		(textures.GetTextureIndex(settings.backgroundTexture) == UINT32_MAX ||
		 textures.GetTextureIndex(settings.gaugeTexture) == UINT32_MAX)) {
		return false;
	}
	settings_ = settings;
	if (auto* background = MadoEngine::SpriteManager::GetInstance().TryGet(backgroundSprite_)) {
		background->SetTexture(settings_.backgroundTexture);
	}
	if (auto* fill = MadoEngine::SpriteManager::GetInstance().TryGet(gaugeSprite_)) {
		fill->SetTexture(settings_.gaugeTexture);
	}
	ApplyRenderSettings();
	Update();
	return true;
}

nlohmann::json Gauge2d::ToJson() const {
	return {
		{ "size", MadoEngine::Json::JsonSerializer::ToJson(settings_.size) },
		{ "backgroundColor", MadoEngine::Json::JsonSerializer::ToJson(settings_.backgroundColor) },
		{ "gaugeColor", MadoEngine::Json::JsonSerializer::ToJson(settings_.gaugeColor) },
		{ "backgroundTexture", settings_.backgroundTexture }, { "gaugeTexture", settings_.gaugeTexture },
		{ "direction", GaugeDirectionToString(settings_.direction) },
		{ "drawBackground", settings_.drawBackground },
		{ "renderLayer", MadoEngine::Render::RenderLayerToString(settings_.renderLayer) },
	};
}

bool Gauge2d::DecodeSettings(const nlohmann::json& json, Settings& settings) {
	try {
		Settings parsed;
		const auto& size = json.at("size");
		const auto& background = json.at("backgroundColor");
		const auto& fill = json.at("gaugeColor");

		// 配列長と型を厳密に検証し、不正な設定による部分復元を防止
		if (!size.is_array() || size.size() != 2 || !background.is_array() || background.size() != 4 ||
			!fill.is_array() || fill.size() != 4) {
			return false;
		}
		parsed.size = { size.at(0).get<float>(), size.at(1).get<float>() };
		parsed.backgroundColor = { background.at(0).get<float>(), background.at(1).get<float>(),
			background.at(2).get<float>(), background.at(3).get<float>() };
		parsed.gaugeColor = { fill.at(0).get<float>(), fill.at(1).get<float>(),
			fill.at(2).get<float>(), fill.at(3).get<float>() };
		parsed.backgroundTexture = json.value("backgroundTexture", std::string("white2x2"));
		parsed.gaugeTexture = json.value("gaugeTexture", std::string("white2x2"));
		parsed.drawBackground = json.at("drawBackground").get<bool>();
		const auto direction = json.at("direction").get<std::string>();
		if (direction == "Right") {
			parsed.direction = GaugeDirection::Right;
		} else if (direction == "Left") {
			parsed.direction = GaugeDirection::Left;
		} else if (direction == "Up") {
			parsed.direction = GaugeDirection::Up;
		} else if (direction == "Down") {
			parsed.direction = GaugeDirection::Down;
		} else {
			return false;
		}
		const auto layer = json.value("renderLayer", std::string("UI"));
		parsed.renderLayer = MadoEngine::Render::RenderLayerFromString(layer);
		if (MadoEngine::Render::RenderLayerToString(parsed.renderLayer) != layer || !IsValidSettings(parsed)) {
			return false;
		}
		settings = std::move(parsed);
		return true;
	} catch (const nlohmann::json::exception&) {
		return false;
	}
}

void Gauge2d::DrawImGui(const char* name) {
#ifdef USE_IMGUI
	if (!name) {
		return;
	}
	ImGui::Begin(name);
	ImGui::Text("Gauge: %.1f / %.1f (%.1f%%)", currentValue_, maxValue_, GetRatio() * 100.0f);
	Vector2 position = GetPosition();
	if (ImGui::DragFloat2("位置", &position.x, 1.0f)) {
		SetPosition(position);
	}
	Settings settings = settings_;
	ImGui::DragFloat2("満タン時のサイズ（px）", &settings.size.x, 1.0f, 0.0f, 4096.0f);
	int direction = static_cast<int>(settings.direction);
	if (ImGui::Combo("伸びる方向", &direction, "Right\0Left\0Up\0Down\0")) {
		settings.direction = static_cast<GaugeDirection>(direction);
	}
	ImGui::Checkbox("背景を表示", &settings.drawBackground);
	ImGui::ColorEdit4("背景色", &settings.backgroundColor.x);
	ImGui::ColorEdit4("ゲージ色", &settings.gaugeColor.x);

	// 現在値を満タン時の編集設定と分離し、ゲーム中の割合を保存対象から除外
	SetSettings(settings);
	if (ImGui::Button("保存")) {
		SaveToJson();
	}
	ImGui::End();
#else
	(void)name;
#endif
}

bool Gauge2d::SaveToJson() const {
	if (objectName_.empty() || !ownedGroup_) {
		return false;
	}

	// Manager管理のゲージはUIGroupへ保存し、単独利用時だけ従来の保存先を維持
	nlohmann::json json = ToJson();
	json["version"] = 2;
	json["name"] = objectName_;
	json["position"] = MadoEngine::Json::JsonSerializer::ToJson(GetPosition());
	json["visible"] = IsVisible();
	return MadoEngine::Json::JsonFile::Save(GetJsonFilePath(), json, 4, true);
}

bool Gauge2d::LoadFromJson() {
	nlohmann::json json;
	if (objectName_.empty() || !ownedGroup_ || !MadoEngine::Json::JsonFile::Exists(GetJsonFilePath()) ||
		!MadoEngine::Json::JsonFile::Load(GetJsonFilePath(), json)) {
		return false;
	}
	try {
		Settings settings;
		if (!DecodeSettings(json, settings)) {
			return false;
		}

		// 旧形式のサイズはwhite2x2の倍率であるため、実際のピクセル寸法へ変換
		if (json.value("version", 1) < 2) {
			settings.size = settings.size * 2.0f;
			if (!json.contains("renderLayer")) {
				settings.renderLayer = settings_.renderLayer;
			}
		}
		const Vector2 position = MadoEngine::Json::JsonSerializer::ToVector2(
			json.value("position", nlohmann::json::array()), GetPosition());
		const bool visible = json.value("visible", IsVisible());
		if (!std::isfinite(position.x) || !std::isfinite(position.y) || !SetSettings(settings)) {
			return false;
		}
		SetPosition(position);
		SetVisible(visible);
		return true;
	} catch (const nlohmann::json::exception&) {
		return false;
	}
}

void Gauge2d::SetPosition(const Vector2& position) {
	group_->SetPosition(position);
}

const Vector2& Gauge2d::GetPosition() const {
	return group_->GetPosition();
}

void Gauge2d::SetSize(const Vector2& size) {
	Settings settings = settings_;
	settings.size = { (std::max)(0.0f, size.x), (std::max)(0.0f, size.y) };
	SetSettings(settings);
}

void Gauge2d::SetBackgroundColor(const Vector4& color) {
	Settings settings = settings_;
	settings.backgroundColor = color;
	SetSettings(settings);
}

void Gauge2d::SetGaugeColor(const Vector4& color) {
	Settings settings = settings_;
	settings.gaugeColor = color;
	SetSettings(settings);
}

void Gauge2d::SetDirection(GaugeDirection direction) {
	Settings settings = settings_;
	settings.direction = direction;
	SetSettings(settings);
}

void Gauge2d::SetDrawBackground(bool enabled) {
	settings_.drawBackground = enabled;
	Update();
}

void Gauge2d::SetVisible(bool visible) {
	group_->SetRuntimeVisible(visible);
}

bool Gauge2d::IsVisible() const {
	return group_->IsRuntimeVisible();
}

void Gauge2d::SetCurrentValue(float value) {
	currentValue_ = value;
	Update();
}

void Gauge2d::SetMaxValue(float value) {
	maxValue_ = value;
	Update();
}

void Gauge2d::SetSceneType(SceneType sceneType) {
	sceneType_ = sceneType;
	ApplyRenderSettings();
}

void Gauge2d::SetRenderLayer(MadoEngine::Render::RenderLayer layer) {
	Settings settings = settings_;
	settings.renderLayer = layer;
	SetSettings(settings);
}

float Gauge2d::GetRatio() const {
	if (maxValue_ <= 0.0f) {
		return 0.0f;
	}
	return currentValue_ / maxValue_;
}

void Gauge2d::ClampValue() {

	// 非有限値やゼロ以下の最大値でも描画計算を有限範囲へ固定
	if (!std::isfinite(maxValue_)) {
		maxValue_ = 0.0f;
	}
	if (!std::isfinite(currentValue_)) {
		currentValue_ = 0.0f;
	}
	maxValue_ = (std::max)(0.0f, maxValue_);
	currentValue_ = std::clamp(currentValue_, 0.0f, maxValue_);
}

void Gauge2d::ApplyBackgroundSprite() {
	auto* background = MadoEngine::SpriteManager::GetInstance().TryGet(backgroundSprite_);
	if (!background) {
		return;
	}

	// テクスチャ寸法を相殺し、画像の解像度を変更しても指定ピクセル寸法を維持
	const Vector2 textureSize = background->GetTextureSize();
	background->SetPosition({ 0.0f, 0.0f });
	background->SetScale({ settings_.size.x / (std::max)(1.0f, textureSize.x),
		settings_.size.y / (std::max)(1.0f, textureSize.y) });
	background->SetColor(settings_.backgroundColor);
	background->SetVisible(settings_.drawBackground);
}

void Gauge2d::ApplyGaugeSprite() {
	auto* fill = MadoEngine::SpriteManager::GetInstance().TryGet(gaugeSprite_);
	if (!fill) {
		return;
	}
	const float ratio = std::clamp(GetRatio(), 0.0f, 1.0f);
	Vector2 position{};
	Vector2 size = settings_.size;

	// 親座標上で固定端を決めてから割合を適用し、回転や拡縮後も伸縮方向を維持
	switch (settings_.direction) {
	case GaugeDirection::Right:
		size.x *= ratio;
		break;
	case GaugeDirection::Left:
		size.x *= ratio;
		position.x = settings_.size.x - size.x;
		break;
	case GaugeDirection::Down:
		size.y *= ratio;
		break;
	case GaugeDirection::Up:
		size.y *= ratio;
		position.y = settings_.size.y - size.y;
		break;
	}
	const Vector2 textureSize = fill->GetTextureSize();
	fill->SetPosition(position);
	fill->SetScale({ size.x / (std::max)(1.0f, textureSize.x), size.y / (std::max)(1.0f, textureSize.y) });
	fill->SetColor(settings_.gaugeColor);
	fill->SetRuntimeVisible(ratio > 0.0f);
}

void Gauge2d::ApplyRenderSettings() {

	// 描画対象とレイヤーは既存のSprite描画経路へ委譲
	for (auto handle : { backgroundSprite_, gaugeSprite_ }) {
		if (auto* sprite = MadoEngine::SpriteManager::GetInstance().TryGet(handle)) {
			sprite->SetSceneType(sceneType_);
			sprite->SetRenderLayer(settings_.renderLayer);
		}
	}
}

std::string Gauge2d::GetJsonFilePath() const {
	return "Assets/Json/Gauge/" + objectName_ + ".json";
}
