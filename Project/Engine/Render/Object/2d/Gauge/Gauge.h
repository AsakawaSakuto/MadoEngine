#pragma once
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Render/Object/RenderLayer.h"
#include ".SceneManager/SceneType.h"
#include <string>

class Sprite;

/// @brief ゲージの伸びる方向
enum class GaugeDirection {
	Right,
	Left,
	Up,
	Down,
};

/// @brief 二枚のSpriteで構成する2Dゲージ
class Gauge {
public:
	explicit Gauge(std::string objectName = "Gauge");
	~Gauge();

	Gauge(const Gauge&) = delete;
	Gauge& operator=(const Gauge&) = delete;
	Gauge(Gauge&&) = delete;
	Gauge& operator=(Gauge&&) = delete;

	/// @brief ゲージ用Spriteを生成する
	/// @param gaugeName ゲージ名
	/// @param sceneType 描画対象Scene
	/// @param renderLayer 描画Layer
	void Initialize(
		const std::string& gaugeName,
		SceneType sceneType = SceneType::None,
		MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::UI
	);

	/// @brief ゲージ用Spriteを破棄する
	void Finalize();

	/// @brief ゲージ状態を更新する
	void Update();

	/// @brief 値を設定してゲージを更新する
	/// @param currentValue 現在値
	/// @param maxValue 最大値
	void Update(float currentValue, float maxValue);

	/// @brief 値を設定してゲージを更新する
	/// @param currentValue 現在値
	/// @param maxValue 最大値
	void Update(int currentValue, int maxValue);

	/// @brief ImGui編集UIを表示する
	/// @param name ImGuiウィンドウ名
	void DrawImGui(const char* name);

	/// @brief 設定をJsonへ保存する
	/// @return 保存に成功した場合はtrue
	bool SaveToJson() const;

	/// @brief 設定をJsonから読み込む
	/// @return 読み込みに成功した場合はtrue
	bool LoadFromJson();

	/// @brief 左上位置を設定する
	/// @param position ピクセル単位の位置
	void SetPosition(const Vector2& position);

	/// @brief 左上位置を取得する
	/// @return ピクセル単位の位置
	const Vector2& GetPosition() const { return position_; }

	/// @brief サイズを設定する
	/// @param size ピクセル単位のサイズ
	void SetSize(const Vector2& size);

	/// @brief サイズを取得する
	/// @return ピクセル単位のサイズ
	const Vector2& GetSize() const { return size_; }

	/// @brief 背景色を設定する
	/// @param color RGBA色
	void SetBackgroundColor(const Vector4& color);

	/// @brief ゲージ色を設定する
	/// @param color RGBA色
	void SetGaugeColor(const Vector4& color);

	/// @brief 伸びる方向を設定する
	/// @param direction 伸びる方向
	void SetDirection(GaugeDirection direction);

	/// @brief 背景の表示を設定する
	/// @param enabled trueの場合は背景を表示する
	void SetDrawBackground(bool enabled);

	/// @brief 表示状態を設定する
	/// @param visible trueの場合は表示する
	void SetVisible(bool visible);

	/// @brief 現在値を設定する
	/// @param value 現在値
	void SetCurrentValue(float value);

	/// @brief 最大値を設定する
	/// @param value 最大値
	void SetMaxValue(float value);

	/// @brief Sceneを設定する
	/// @param sceneType 描画対象Scene
	void SetSceneType(SceneType sceneType);

	/// @brief Layerを設定する
	/// @param layer 描画Layer
	void SetRenderLayer(MadoEngine::Render::RenderLayer layer);

	/// @brief 現在値を取得する
	/// @return 現在値
	float GetCurrentValue() const { return currentValue_; }

	/// @brief 最大値を取得する
	/// @return 最大値
	float GetMaxValue() const { return maxValue_; }

	/// @brief 現在値の割合を取得する
	/// @return 0.0fから1.0fまでの割合
	float GetRatio() const;

	/// @brief 伸びる方向を取得する
	/// @return 伸びる方向
	GaugeDirection GetDirection() const { return direction_; }

	/// @brief 背景を表示するか取得する
	/// @return 背景を表示する場合はtrue
	bool IsDrawBackground() const { return drawBackground_; }

	/// @brief 表示状態を取得する
	/// @return 表示する場合はtrue
	bool IsVisible() const { return isVisible_; }

private:
	/// @brief 値を有効範囲に丸める
	void ClampValue();

	/// @brief 背景Spriteへ設定を反映する
	void ApplyBackgroundSprite();

	/// @brief ゲージSpriteへ設定を反映する
	void ApplyGaugeSprite();

	/// @brief SceneとLayerを反映する
	void ApplyRenderSettings();

	/// @brief Json保存先パスを取得する
	/// @return Json保存先パス
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