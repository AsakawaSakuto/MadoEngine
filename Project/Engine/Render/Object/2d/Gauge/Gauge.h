#pragma once
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Render/Object/RenderLayer.h"
#include ".SceneManager/SceneType.h"
#include <string>

class Sprite;

/// @brief ゲージの伸びる方向です。
enum class GaugeDirection {
	Right,
	Left,
	Up,
	Down,
};

/// @brief 二枚のSpriteで構成する2Dゲージです。
class Gauge {
public:
	explicit Gauge(std::string objectName = "Gauge");
	~Gauge();

	Gauge(const Gauge&) = delete;
	Gauge& operator=(const Gauge&) = delete;
	Gauge(Gauge&&) = delete;
	Gauge& operator=(Gauge&&) = delete;

	/// @brief ゲージ用Spriteを生成します。
	/// @param gaugeName ゲージ名です。
	/// @param sceneType 描画対象Sceneです。
	/// @param renderLayer 描画Layerです。
	void Initialize(
		const std::string& gaugeName,
		SceneType sceneType = SceneType::None,
		MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::UI
	);

	/// @brief ゲージ用Spriteを破棄します。
	void Finalize();

	/// @brief ゲージ状態を更新します。
	void Update();

	/// @brief 値を設定してゲージを更新します。
	/// @param currentValue 現在値です。
	/// @param maxValue 最大値です。
	void Update(float currentValue, float maxValue);

	/// @brief 値を設定してゲージを更新します。
	/// @param currentValue 現在値です。
	/// @param maxValue 最大値です。
	void Update(int currentValue, int maxValue);

	/// @brief ImGui編集UIを表示します。
	/// @param name ImGuiウィンドウ名です。
	void DrawImGui(const char* name);

	/// @brief 設定をJsonへ保存します。
	/// @return 保存に成功した場合はtrueです。
	bool SaveToJson() const;

	/// @brief 設定をJsonから読み込みます。
	/// @return 読み込みに成功した場合はtrueです。
	bool LoadFromJson();

	/// @brief 左上位置を設定します。
	/// @param position ピクセル単位の位置です。
	void SetPosition(const Vector2& position);

	/// @brief 左上位置を取得します。
	/// @return ピクセル単位の位置です。
	const Vector2& GetPosition() const { return position_; }

	/// @brief サイズを設定します。
	/// @param size ピクセル単位のサイズです。
	void SetSize(const Vector2& size);

	/// @brief サイズを取得します。
	/// @return ピクセル単位のサイズです。
	const Vector2& GetSize() const { return size_; }

	/// @brief 背景色を設定します。
	/// @param color RGBA色です。
	void SetBackgroundColor(const Vector4& color);

	/// @brief ゲージ色を設定します。
	/// @param color RGBA色です。
	void SetGaugeColor(const Vector4& color);

	/// @brief 伸びる方向を設定します。
	/// @param direction 伸びる方向です。
	void SetDirection(GaugeDirection direction);

	/// @brief 背景の表示を設定します。
	/// @param enabled trueの場合は背景を表示します。
	void SetDrawBackground(bool enabled);

	/// @brief 表示状態を設定します。
	/// @param visible trueの場合は表示します。
	void SetVisible(bool visible);

	/// @brief 現在値を設定します。
	/// @param value 現在値です。
	void SetCurrentValue(float value);

	/// @brief 最大値を設定します。
	/// @param value 最大値です。
	void SetMaxValue(float value);

	/// @brief Sceneを設定します。
	/// @param sceneType 描画対象Sceneです。
	void SetSceneType(SceneType sceneType);

	/// @brief Layerを設定します。
	/// @param layer 描画Layerです。
	void SetRenderLayer(MadoEngine::Render::RenderLayer layer);

	/// @brief 現在値を取得します。
	/// @return 現在値です。
	float GetCurrentValue() const { return currentValue_; }

	/// @brief 最大値を取得します。
	/// @return 最大値です。
	float GetMaxValue() const { return maxValue_; }

	/// @brief 現在値の割合を取得します。
	/// @return 0.0fから1.0fまでの割合です。
	float GetRatio() const;

	/// @brief 伸びる方向を取得します。
	/// @return 伸びる方向です。
	GaugeDirection GetDirection() const { return direction_; }

	/// @brief 背景を表示するか取得します。
	/// @return 背景を表示する場合はtrueです。
	bool IsDrawBackground() const { return drawBackground_; }

	/// @brief 表示状態を取得します。
	/// @return 表示する場合はtrueです。
	bool IsVisible() const { return isVisible_; }

private:
	/// @brief 値を有効範囲に丸めます。
	void ClampValue();

	/// @brief 背景Spriteへ設定を反映します。
	void ApplyBackgroundSprite();

	/// @brief ゲージSpriteへ設定を反映します。
	void ApplyGaugeSprite();

	/// @brief SceneとLayerを反映します。
	void ApplyRenderSettings();

	/// @brief Json保存先パスを取得します。
	/// @return Json保存先パスです。
	std::string GetJsonFilePath() const;

	std::string objectName_;
	std::string backgroundSpriteName_;
	std::string gaugeSpriteName_;

	Sprite* backgroundSprite_ = nullptr;
	Sprite* gaugeSprite_ = nullptr;

	Vector2 position_ = { 0.0f, 0.0f };
	Vector2 size_ = { 200.0f, 24.0f };
	float currentValue_ = 100.0f;
	float maxValue_ = 100.0f;

	Vector4 backgroundColor_ = { 0.2f, 0.2f, 0.2f, 1.0f };
	Vector4 gaugeColor_ = { 0.0f, 1.0f, 0.0f, 1.0f };

	bool drawBackground_ = true;
	bool isVisible_ = true;
	bool isInitialized_ = false;
	GaugeDirection direction_ = GaugeDirection::Right;
	SceneType sceneType_ = SceneType::None;
	MadoEngine::Render::RenderLayer renderLayer_ = MadoEngine::Render::RenderLayer::UI;
};