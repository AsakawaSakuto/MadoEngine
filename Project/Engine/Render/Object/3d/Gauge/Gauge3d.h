#pragma once
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Render/Object/ObjectHandle.h"
#include "Render/Object/RenderLayer.h"
#include ".SceneManager/SceneType.h"
#include <string>

class Camera;
class Model;

/// @brief 3Dゲージの伸長方向
enum class Gauge3dDirection {
	Right,
	Left,
	Up,
	Down,
};

/// @brief 2枚のビルボードModelで構成する3D空間用ゲージ
class Gauge3d final {
public:
	explicit Gauge3d(std::string objectName = "Gauge3d");
	~Gauge3d();

	Gauge3d(const Gauge3d&) = delete;
	Gauge3d& operator=(const Gauge3d&) = delete;
	Gauge3d(Gauge3d&&) = delete;
	Gauge3d& operator=(Gauge3d&&) = delete;

	/// @brief ゲージ用Modelを生成
	/// @param gaugeName ゲージ名
	/// @param sceneType 描画対象Scene
	/// @param renderLayer 描画Layer
	/// @return 生成に成功した場合はtrue
	bool Initialize(
		const std::string& gaugeName,
		SceneType sceneType = SceneType::None,
		MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::World
	);

	/// @brief ゲージ用Modelを遅延破棄
	void Finalize();

	/// @brief カメラ基準の位置とサイズをModelへ反映
	/// @param camera 描画に使用するCamera
	void Update(const Camera& camera);

	/// @brief 値を設定してカメラ基準の位置とサイズをModelへ反映
	/// @param camera 描画に使用するCamera
	/// @param currentValue 現在値
	/// @param maxValue 最大値
	void Update(const Camera& camera, float currentValue, float maxValue);

	/// @brief ImGui編集UIを表示
	/// @param name ImGuiウィンドウ名
	void DrawImGui(const char* name);

	/// @brief 設定をJsonへ保存
	/// @return 保存に成功した場合はtrue
	bool SaveToJson() const;

	/// @brief 設定をJsonから読み込み
	/// @return 読み込みに成功した場合はtrue
	bool LoadFromJson();

	/// @brief ゲージ中心のワールド座標を設定
	/// @param position ゲージ全体の中心座標
	void SetPosition(const Vector3& position);

	/// @brief ゲージ中心のワールド座標を取得
	/// @return ゲージ全体の中心座標
	const Vector3& GetPosition() const { return position_; }

	/// @brief 基準座標へ加算する移動量を設定
	/// @param translateOffset ワールド空間の移動量
	void SetTranslateOffset(const Vector3& translateOffset);

	/// @brief 基準座標へ加算する移動量を取得
	/// @return ワールド空間の移動量
	const Vector3& GetTranslateOffset() const { return translateOffset_; }

	/// @brief ゲージのワールド空間サイズを設定
	/// @param size 幅と高さ
	void SetSize(const Vector2& size);

	/// @brief ゲージのワールド空間サイズを取得
	/// @return 幅と高さ
	const Vector2& GetSize() const { return size_; }

	/// @brief 背景色を設定
	/// @param color RGBA色
	void SetBackgroundColor(const Vector4& color);

	/// @brief ゲージ色を設定
	/// @param color RGBA色
	void SetGaugeColor(const Vector4& color);

	/// @brief 伸長方向を設定
	/// @param direction 伸長方向
	void SetDirection(Gauge3dDirection direction);

	/// @brief 背景の表示状態を設定
	/// @param enabled 背景を表示する場合はtrue
	void SetDrawBackground(bool enabled);

	/// @brief ゲージ全体の表示状態を設定
	/// @param visible 表示する場合はtrue
	void SetVisible(bool visible);

	/// @brief 現在値を設定
	/// @param value 現在値
	void SetCurrentValue(float value);

	/// @brief 最大値を設定
	/// @param value 最大値
	void SetMaxValue(float value);

	/// @brief 現在値と最大値をまとめて設定
	/// @param currentValue 現在値
	/// @param maxValue 最大値
	void SetValue(float currentValue, float maxValue);

	/// @brief 前景を背景よりカメラ側へ寄せる距離を設定
	/// @param offset 0以上の深度オフセット
	void SetDepthOffset(float offset);

	/// @brief 描画対象Sceneを設定
	/// @param sceneType 描画対象Scene
	void SetSceneType(SceneType sceneType);

	/// @brief 描画Layerを設定
	/// @param layer 描画Layer
	void SetRenderLayer(MadoEngine::Render::RenderLayer layer);

	/// @brief 現在値を取得
	/// @return 現在値
	float GetCurrentValue() const { return currentValue_; }

	/// @brief 最大値を取得
	/// @return 最大値
	float GetMaxValue() const { return maxValue_; }

	/// @brief 現在値の割合を取得
	/// @return 0.0fから1.0fまでの割合
	float GetRatio() const;

	/// @brief 伸長方向を取得
	/// @return 伸長方向
	Gauge3dDirection GetDirection() const { return direction_; }

	/// @brief 背景を表示するか取得
	/// @return 背景を表示する場合はtrue
	bool IsDrawBackground() const { return drawBackground_; }

	/// @brief ゲージ全体を表示するか取得
	/// @return 表示する場合はtrue
	bool IsVisible() const { return isVisible_; }

	/// @brief 初期化済みか確認
	/// @return 初期化済みの場合はtrue
	bool IsInitialized() const { return isInitialized_; }

private:
	/// @brief 現在値と最大値を有効範囲へ補正
	void ClampValue();

	/// @brief Model共通の描画設定を適用
	/// @param model 設定対象Model
	/// @return 設定に成功した場合はtrue
	bool ApplyCommonModelSettings(Model& model);

	/// @brief 背景と前景へ色を反映
	void ApplyColors();

	/// @brief 背景と前景へ表示状態を反映
	void ApplyVisibility();

	/// @brief 背景と前景へSceneとLayerを反映
	void ApplyRenderSettings();

	/// @brief Json保存先パスを取得
	/// @return Json保存先パス
	std::string GetJsonFilePath() const;

	std::string objectName_;
	MadoEngine::ModelHandle backgroundModel_{};
	MadoEngine::ModelHandle gaugeModel_{};

	Vector3 position_ = { 0.0f, 0.0f, 0.0f };
	Vector3 translateOffset_ = { 0.0f, 0.0f, 0.0f };
	Vector2 size_ = { 2.0f, 0.2f };
	float currentValue_ = 100.0f;
	float maxValue_ = 100.0f;
	float depthOffset_ = 0.001f;

	Vector4 backgroundColor_ = { 0.2f, 0.2f, 0.2f, 1.0f };
	Vector4 gaugeColor_ = { 0.0f, 1.0f, 0.0f, 1.0f };

	Gauge3dDirection direction_ = Gauge3dDirection::Right;
	SceneType sceneType_ = SceneType::None;
	MadoEngine::Render::RenderLayer renderLayer_ = MadoEngine::Render::RenderLayer::World;
	bool drawBackground_ = true;
	bool isVisible_ = true;
	bool isInitialized_ = false;
	bool isTransformReady_ = false;
};
