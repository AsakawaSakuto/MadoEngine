#include "Gauge3d.h"
#include "Math/Function/MatrixFunction.h"
#include "Render/Object/3d/Model/Model.h"
#include "Render/Object/3d/Model/MyModel.h"
#include "Utility/Camera/Camera.h"
#include "Utility/Json/Core/JsonFile.h"
#include "Utility/Json/Core/JsonSerializer.h"
#include "Utility/Logger/Logger.h"
#include "imguiHeaders.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <utility>

namespace {

	constexpr const char* kGaugeModelAssetName = "Plane";
	constexpr const char* kGaugeTextureName = "white2x2";
	std::atomic_uint64_t gNextGauge3dResourceId = 1;

	/// @brief Gauge3dDirectionをJson保存用文字列へ変換
	/// @param direction 変換する伸長方向
	/// @return Json保存用文字列
	const char* Gauge3dDirectionToString(Gauge3dDirection direction) {
		switch (direction) {
		case Gauge3dDirection::Left:
			return "Left";
		case Gauge3dDirection::Up:
			return "Up";
		case Gauge3dDirection::Down:
			return "Down";
		case Gauge3dDirection::Right:
		default:
			return "Right";
		}
	}

	/// @brief Json保存用文字列からGauge3dDirectionへ変換
	/// @param value 変換する文字列
	/// @return 対応する伸長方向
	Gauge3dDirection Gauge3dDirectionFromString(const std::string& value) {
		if (value == "Left") {
			return Gauge3dDirection::Left;
		}
		if (value == "Up") {
			return Gauge3dDirection::Up;
		}
		if (value == "Down") {
			return Gauge3dDirection::Down;
		}
		return Gauge3dDirection::Right;
	}

	/// @brief Cameraの逆View行列からワールド空間の基底軸を取得
	/// @param inverseView Cameraの逆View行列
	/// @param row 取得する行番号
	/// @return 正規化済み基底軸
	Vector3 GetCameraBasis(const Matrix4x4& inverseView, std::size_t row) {
		return Vector3{
			inverseView.m[row][0],
			inverseView.m[row][1],
			inverseView.m[row][2],
		}.Normalized();
	}

} // namespace

Gauge3d::Gauge3d(std::string objectName)
	: objectName_(std::move(objectName)) {
}

Gauge3d::~Gauge3d() {
	Finalize();
}

bool Gauge3d::Initialize(
	const std::string& gaugeName,
	SceneType sceneType,
	MadoEngine::Render::RenderLayer renderLayer) {
	Finalize();

	objectName_ = gaugeName.empty() ? objectName_ : gaugeName;
	sceneType_ = sceneType;
	renderLayer_ = renderLayer;
	isTransformReady_ = false;

	// Model生成前に保存設定を復元して初回表示へ反映
	LoadFromJson();

	// 遅延破棄待ちの同名Modelと競合しない内部識別名を生成
	const uint64_t resourceId = gNextGauge3dResourceId.fetch_add(1, std::memory_order_relaxed);
	const std::string resourceSuffix = std::to_string(resourceId);
	backgroundModel_ = MyModel::Create(
		objectName_ + "Background3d" + resourceSuffix,
		kGaugeModelAssetName,
		sceneType_,
		renderLayer_
	);
	gaugeModel_ = MyModel::Create(
		objectName_ + "Value3d" + resourceSuffix,
		kGaugeModelAssetName,
		sceneType_,
		renderLayer_
	);

	Model* backgroundModel = MyModel::TryGet(backgroundModel_);
	Model* gaugeModel = MyModel::TryGet(gaugeModel_);
	if (!backgroundModel || !gaugeModel) {
		Logger::Output("3Dゲージ用Modelの生成に失敗しました: " + objectName_, Logger::Level::Error);
		Finalize();
		return false;
	}

	// 2枚のPlaneを同じ非ライティングビルボードとして統一
	if (!ApplyCommonModelSettings(*backgroundModel) || !ApplyCommonModelSettings(*gaugeModel)) {
		Logger::Output("3Dゲージ用Modelの設定に失敗しました: " + objectName_, Logger::Level::Error);
		Finalize();
		return false;
	}

	isInitialized_ = true;
	ClampValue();
	ApplyColors();
	ApplyRenderSettings();
	ApplyVisibility();
	return true;
}

void Gauge3d::Finalize() {

	// Managerの安全な破棄時点まで描画リソースの解放を延期
	if (backgroundModel_.IsValid()) {
		MyModel::RequestDestroy(backgroundModel_);
	}
	if (gaugeModel_.IsValid()) {
		MyModel::RequestDestroy(gaugeModel_);
	}

	backgroundModel_ = {};
	gaugeModel_ = {};
	isInitialized_ = false;
	isTransformReady_ = false;
}

void Gauge3d::Update(const Camera& camera) {
	if (!isInitialized_) {
		return;
	}

	Model* backgroundModel = MyModel::TryGet(backgroundModel_);
	Model* gaugeModel = MyModel::TryGet(gaugeModel_);
	if (!backgroundModel || !gaugeModel) {
		isInitialized_ = false;
		isTransformReady_ = false;
		return;
	}

	ClampValue();
	const float ratio = GetRatio();
	const Matrix4x4 inverseView = Matrix::Inverse(camera.GetViewMatrix());
	const Vector3 cameraRight = GetCameraBasis(inverseView, 0);
	const Vector3 cameraUp = GetCameraBasis(inverseView, 1);
	const Vector3 cameraForward = GetCameraBasis(inverseView, 2);

	const Vector3 backgroundScale = {
		size_.x * 0.5f,
		size_.y * 0.5f,
		1.0f,
	};
	Vector3 gaugeScale = backgroundScale;

	// 所有Objectの基準座標と編集可能な移動量を合成
	const Vector3 displayPosition = position_ + translateOffset_;
	Vector3 gaugePosition = displayPosition;

	// Planeの中心原点を伸長元の端へ合わせるため縮小量の半分だけカメラ基底上で補正
	switch (direction_) {
	case Gauge3dDirection::Right:
		gaugeScale.x *= ratio;
		gaugePosition -= cameraRight * (size_.x * (1.0f - ratio) * 0.5f);
		break;
	case Gauge3dDirection::Left:
		gaugeScale.x *= ratio;
		gaugePosition += cameraRight * (size_.x * (1.0f - ratio) * 0.5f);
		break;
	case Gauge3dDirection::Up:
		gaugeScale.y *= ratio;
		gaugePosition -= cameraUp * (size_.y * (1.0f - ratio) * 0.5f);
		break;
	case Gauge3dDirection::Down:
		gaugeScale.y *= ratio;
		gaugePosition += cameraUp * (size_.y * (1.0f - ratio) * 0.5f);
		break;
	}

	// 描画順に依存せず前景を表示するためCamera側へ僅かに分離
	gaugePosition -= cameraForward * depthOffset_;

	backgroundModel->SetPosition(displayPosition);
	backgroundModel->SetScale(backgroundScale);
	gaugeModel->SetPosition(gaugePosition);
	gaugeModel->SetScale(gaugeScale);

	isTransformReady_ = true;
	ApplyVisibility();
}

void Gauge3d::Update(const Camera& camera, float currentValue, float maxValue) {
	SetValue(currentValue, maxValue);
	Update(camera);
}

void Gauge3d::DrawImGui(const char* name) {
#ifdef USE_IMGUI
	if (!name) {
		return;
	}

	ImGui::Begin(name);
	ImGui::Text("Gauge: %.1f / %.1f (%.1f%%)", currentValue_, maxValue_, GetRatio() * 100.0f);

	bool settingsChanged = false;
	settingsChanged |= ImGui::DragFloat3("Position", &position_.x, 0.05f);
	settingsChanged |= ImGui::DragFloat3("Translate Offset", &translateOffset_.x, 0.05f);
	settingsChanged |= ImGui::DragFloat2("Size", &size_.x, 0.01f, 0.0f, 1000.0f);
	settingsChanged |= ImGui::DragFloat("Current", &currentValue_, 1.0f, 0.0f, (std::max)(0.0f, maxValue_));
	settingsChanged |= ImGui::DragFloat("Max", &maxValue_, 1.0f, 0.0f, 999999.0f);
	settingsChanged |= ImGui::DragFloat("Depth Offset", &depthOffset_, 0.0001f, 0.0f, 1.0f, "%.4f");

	const char* directionNames[] = { "Right", "Left", "Up", "Down" };
	int currentDirection = static_cast<int>(direction_);
	if (ImGui::Combo("Direction", &currentDirection, directionNames, 4)) {
		direction_ = static_cast<Gauge3dDirection>(currentDirection);
		settingsChanged = true;
	}

	settingsChanged |= ImGui::Checkbox("Visible", &isVisible_);
	settingsChanged |= ImGui::Checkbox("Draw Background", &drawBackground_);
	settingsChanged |= ImGui::ColorEdit4("Background Color", &backgroundColor_.x);
	settingsChanged |= ImGui::ColorEdit4("Gauge Color", &gaugeColor_.x);

	if (settingsChanged) {

		// ImGuiの直接編集値を公開Setterと同じ有効範囲へ補正
		size_.x = (std::max)(0.0f, size_.x);
		size_.y = (std::max)(0.0f, size_.y);
		depthOffset_ = (std::max)(0.0f, depthOffset_);
		ClampValue();
		ApplyColors();
		ApplyVisibility();
	}

	if (ImGui::Button("Save")) {
		SaveToJson();
	}
	ImGui::SameLine();
	if (ImGui::Button("Reload")) {
		LoadFromJson();
	}

	ImGui::End();
#else
	(void)name;
#endif
}

bool Gauge3d::SaveToJson() const {
	if (objectName_.empty()) {
		Logger::Output("3Dゲージ名が空のためJson保存をスキップしました", Logger::Level::Warning);
		return false;
	}

	nlohmann::json json;
	json["name"] = objectName_;
	json["position"] = MadoEngine::Json::JsonSerializer::ToJson(position_);
	json["translateOffset"] = MadoEngine::Json::JsonSerializer::ToJson(translateOffset_);
	json["size"] = MadoEngine::Json::JsonSerializer::ToJson(size_);
	json["currentValue"] = currentValue_;
	json["maxValue"] = maxValue_;
	json["depthOffset"] = depthOffset_;
	json["backgroundColor"] = MadoEngine::Json::JsonSerializer::ToJson(backgroundColor_);
	json["gaugeColor"] = MadoEngine::Json::JsonSerializer::ToJson(gaugeColor_);
	json["direction"] = Gauge3dDirectionToString(direction_);
	json["drawBackground"] = drawBackground_;
	json["visible"] = isVisible_;

	return MadoEngine::Json::JsonFile::Save(GetJsonFilePath(), json, 4, true);
}

bool Gauge3d::LoadFromJson() {
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

	// 欠落項目では現在設定を維持して旧形式や部分設定を受け入れ
	position_ = MadoEngine::Json::JsonSerializer::ToVector3(
		json.value("position", nlohmann::json::array()),
		position_
	);
	translateOffset_ = MadoEngine::Json::JsonSerializer::ToVector3(
		json.value("translateOffset", nlohmann::json::array()),
		translateOffset_
	);
	size_ = MadoEngine::Json::JsonSerializer::ToVector2(
		json.value("size", nlohmann::json::array()),
		size_
	);
	size_.x = (std::max)(0.0f, size_.x);
	size_.y = (std::max)(0.0f, size_.y);
	currentValue_ = json.value("currentValue", currentValue_);
	maxValue_ = json.value("maxValue", maxValue_);
	depthOffset_ = (std::max)(0.0f, json.value("depthOffset", depthOffset_));
	backgroundColor_ = MadoEngine::Json::JsonSerializer::ToVector4(
		json.value("backgroundColor", nlohmann::json::array()),
		backgroundColor_
	);
	gaugeColor_ = MadoEngine::Json::JsonSerializer::ToVector4(
		json.value("gaugeColor", nlohmann::json::array()),
		gaugeColor_
	);
	direction_ = Gauge3dDirectionFromString(
		json.value("direction", Gauge3dDirectionToString(direction_))
	);
	drawBackground_ = json.value("drawBackground", drawBackground_);
	isVisible_ = json.value("visible", isVisible_);
	ClampValue();
	ApplyColors();
	ApplyVisibility();

	Logger::Output("3Dゲージ設定をJsonから読み込みました: " + filePath, Logger::Level::Assets);
	return true;
}

void Gauge3d::SetPosition(const Vector3& position) {
	position_ = position;
}

void Gauge3d::SetTranslateOffset(const Vector3& translateOffset) {
	translateOffset_ = translateOffset;
}

void Gauge3d::SetSize(const Vector2& size) {
	size_ = {
		(std::max)(0.0f, size.x),
		(std::max)(0.0f, size.y),
	};
}

void Gauge3d::SetBackgroundColor(const Vector4& color) {
	backgroundColor_ = color;
	ApplyColors();
}

void Gauge3d::SetGaugeColor(const Vector4& color) {
	gaugeColor_ = color;
	ApplyColors();
}

void Gauge3d::SetDirection(Gauge3dDirection direction) {
	direction_ = direction;
}

void Gauge3d::SetDrawBackground(bool enabled) {
	drawBackground_ = enabled;
	ApplyVisibility();
}

void Gauge3d::SetVisible(bool visible) {
	isVisible_ = visible;
	ApplyVisibility();
}

void Gauge3d::SetCurrentValue(float value) {
	currentValue_ = value;
	ClampValue();
	ApplyVisibility();
}

void Gauge3d::SetMaxValue(float value) {
	maxValue_ = value;
	ClampValue();
	ApplyVisibility();
}

void Gauge3d::SetValue(float currentValue, float maxValue) {
	currentValue_ = currentValue;
	maxValue_ = maxValue;
	ClampValue();
	ApplyVisibility();
}

void Gauge3d::SetDepthOffset(float offset) {
	depthOffset_ = (std::max)(0.0f, offset);
}

void Gauge3d::SetSceneType(SceneType sceneType) {
	sceneType_ = sceneType;
	ApplyRenderSettings();
}

void Gauge3d::SetRenderLayer(MadoEngine::Render::RenderLayer layer) {
	renderLayer_ = layer;
	ApplyRenderSettings();
}

float Gauge3d::GetRatio() const {
	if (maxValue_ <= 0.0f) {
		return 0.0f;
	}

	return std::clamp(currentValue_ / maxValue_, 0.0f, 1.0f);
}

void Gauge3d::ClampValue() {
	maxValue_ = (std::max)(0.0f, maxValue_);
	currentValue_ = std::clamp(currentValue_, 0.0f, maxValue_);
}

bool Gauge3d::ApplyCommonModelSettings(Model& model) {
	if (!model.SetTexture(kGaugeTextureName)) {
		return false;
	}

	model.SetUseBillboard(true);
	model.SetCastShadow(false);
	model.SetReceiveShadow(false);
	model.SetLightingEnabled(false);
	return true;
}

void Gauge3d::ApplyColors() {
	if (Model* backgroundModel = MyModel::TryGet(backgroundModel_)) {
		backgroundModel->SetColor(backgroundColor_);
	}
	if (Model* gaugeModel = MyModel::TryGet(gaugeModel_)) {
		gaugeModel->SetColor(gaugeColor_);
	}
}

void Gauge3d::ApplyVisibility() {

	// 初回Camera反映前の重なったPlaneを描画対象外に維持
	if (Model* backgroundModel = MyModel::TryGet(backgroundModel_)) {
		backgroundModel->SetVisible(isInitialized_ && isTransformReady_ && isVisible_ && drawBackground_);
	}
	if (Model* gaugeModel = MyModel::TryGet(gaugeModel_)) {
		gaugeModel->SetVisible(isInitialized_ && isTransformReady_ && isVisible_ && GetRatio() > 0.0f);
	}
}

void Gauge3d::ApplyRenderSettings() {
	if (Model* backgroundModel = MyModel::TryGet(backgroundModel_)) {
		backgroundModel->SetSceneType(sceneType_);
		backgroundModel->SetRenderLayer(renderLayer_);
	}
	if (Model* gaugeModel = MyModel::TryGet(gaugeModel_)) {
		gaugeModel->SetSceneType(sceneType_);
		gaugeModel->SetRenderLayer(renderLayer_);
	}
}

std::string Gauge3d::GetJsonFilePath() const {
	return "Assets/Json/Gauge3d/" + objectName_ + ".json";
}
