#pragma once
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Render/Object/RenderLayer.h"
#include "Render/Object/ObjectHandle.h"
#include "SceneManager/SceneType.h"
#include <memory>
#include <string>
#include <nlohmann/json_fwd.hpp>

class Sprite;
namespace MadoEngine {
class UIGroup;
class UIGroupManager;
}

/// @brief ゲージの伸びる方向
enum class GaugeDirection {
	Right,
	Left,
	Up,
	Down,
};

/// @brief UIGroupの配置と二枚のSpriteで構成する2Dゲージ
class Gauge2d {
public:
	/// @brief 実行時の割合と分離したゲージの編集用設定
	struct Settings {
		/// @brief 満タン時のピクセル寸法
		Vector2 size = { 200.0f, 24.0f };
		Vector4 backgroundColor = { 0.2f, 0.2f, 0.2f, 1.0f };
		Vector4 gaugeColor = { 0.0f, 1.0f, 0.0f, 1.0f };
		std::string backgroundTexture = "white2x2";
		std::string gaugeTexture = "white2x2";
		GaugeDirection direction = GaugeDirection::Right;
		bool drawBackground = true;
		MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::UI;
	};

	/// @brief 独立したUIGroupを持つゲージを生成
	/// @param objectName ゲージ名
	explicit Gauge2d(std::string objectName = "Gauge");
	/// @brief ゲージ用Spriteの削除を予約
	~Gauge2d();

	Gauge2d(const Gauge2d&) = delete;
	Gauge2d& operator=(const Gauge2d&) = delete;
	Gauge2d(Gauge2d&&) = delete;
	Gauge2d& operator=(Gauge2d&&) = delete;

	/// @brief ゲージ用Spriteを生成
	/// @param gaugeName ゲージ名
	/// @param sceneType 描画対象Scene
	/// @param renderLayer 描画Layer
	void Initialize(
		const std::string& gaugeName,
		SceneType sceneType = SceneType::None,
		MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::UI
	);

	/// @brief ゲージ用Spriteの削除を予約
	void Finalize();

	/// @brief ゲージ状態を更新
	void Update();

	/// @brief 値を設定してゲージを更新
	/// @param currentValue 現在値
	/// @param maxValue 最大値
	void Update(float currentValue, float maxValue);

	/// @brief 値を設定してゲージを更新
	/// @param currentValue 現在値
	/// @param maxValue 最大値
	void Update(int currentValue, int maxValue);

	/// @brief ImGui編集UIを表示
	/// @param name ImGuiウィンドウ名
	void DrawImGui(const char* name);

	/// @brief 単独利用時の設定をJsonへ保存
	/// @return 保存に成功した場合はtrue
	bool SaveToJson() const;

	/// @brief 単独利用時の設定を旧形式対応でJsonから読み込み
	/// @return 読み込みに成功した場合はtrue
	bool LoadFromJson();

	/// @brief 配置と階層を管理するUIGroupを取得
	/// @return ゲージのUIGroup
	MadoEngine::UIGroup& GetGroup() { return *group_; }
	/// @brief 配置と階層を管理するUIGroupを定数参照として取得
	/// @return ゲージのUIGroup
	const MadoEngine::UIGroup& GetGroup() const { return *group_; }
	/// @brief 満タン時のサイズと見た目の設定を取得
	/// @return 実行時の割合を含まない編集用設定
	const Settings& GetSettings() const { return settings_; }
	/// @brief 検証済みの編集用設定を一括反映
	/// @note Sprite生成前のテクスチャ検証は名前の有無だけを対象
	/// @param settings 反映する編集用設定
	/// @return 数値とテクスチャが有効で反映できた場合はtrue
	bool SetSettings(const Settings& settings);
	/// @brief 実行時の値と配置を除外した設定をJsonへ変換
	/// @return 満タン時の見た目と描画レイヤーのJson
	nlohmann::json ToJson() const;
	/// @brief Jsonのゲージ設定を変更前に検証
	/// @param json 読み込む設定
	/// @param settings 検証済みの設定の出力先
	/// @return 必須項目と値域が有効な場合はtrue
	static bool DecodeSettings(const nlohmann::json& json, Settings& settings);

	/// @brief 親Groupを基準とした左上位置を設定
	/// @param position ピクセル単位のローカル位置
	void SetPosition(const Vector2& position);

	/// @brief 親Groupを基準とした左上位置を取得
	/// @return ピクセル単位のローカル位置
	const Vector2& GetPosition() const;

	/// @brief サイズを設定
	/// @param size ピクセル単位のサイズ
	void SetSize(const Vector2& size);

	/// @brief サイズを取得
	/// @return ピクセル単位のサイズ
	const Vector2& GetSize() const { return settings_.size; }

	/// @brief 背景色を設定
	/// @param color RGBA色
	void SetBackgroundColor(const Vector4& color);

	/// @brief ゲージ色を設定
	/// @param color RGBA色
	void SetGaugeColor(const Vector4& color);

	/// @brief 伸びる方向を設定
	/// @param direction 伸びる方向
	void SetDirection(GaugeDirection direction);

	/// @brief 背景の表示を設定
	/// @param enabled 背景を表示する場合はtrue
	void SetDrawBackground(bool enabled);

	/// @brief ゲージと配下の実行時の表示許可を設定
	/// @param visible 実行時の表示を許可する場合はtrue
	void SetVisible(bool visible);

	/// @brief 現在値を設定
	/// @param value 現在値
	void SetCurrentValue(float value);

	/// @brief 最大値を設定
	/// @param value 最大値
	void SetMaxValue(float value);

	/// @brief Sceneを設定
	/// @param sceneType 描画対象Scene
	void SetSceneType(SceneType sceneType);

	/// @brief Layerを設定
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

	/// @brief 伸びる方向を取得
	/// @return 伸びる方向
	GaugeDirection GetDirection() const { return settings_.direction; }

	/// @brief 背景を表示するか取得
	/// @return 背景を表示する場合はtrue
	bool IsDrawBackground() const { return settings_.drawBackground; }

	/// @brief 実行時の表示許可を取得
	/// @return 実行時の表示を許可する場合はtrue
	bool IsVisible() const;

private:
	friend class MadoEngine::UIGroupManager;

	/// @brief Manager所有のUIGroupへゲージの機能を接続
	/// @param group ゲージより長く生存するUIGroup
	/// @param objectName Sprite生成用の一意な名前
	Gauge2d(MadoEngine::UIGroup& group, std::string objectName);
	/// @brief ローカル座標で描画するSpriteを生成
	/// @return 二枚のSpriteを生成できた場合はtrue
	bool InitializeSprites();
	/// @brief 値を有効範囲に丸め処理
	void ClampValue();

	/// @brief 背景Spriteへ設定を反映
	void ApplyBackgroundSprite();

	/// @brief ゲージSpriteへ設定を反映
	void ApplyGaugeSprite();

	/// @brief SceneとLayerを反映
	void ApplyRenderSettings();

	/// @brief Json保存先パスを取得
	/// @return Json保存先パス
	std::string GetJsonFilePath() const;

	std::string objectName_;
	std::unique_ptr<MadoEngine::UIGroup> ownedGroup_;
	MadoEngine::UIGroup* group_ = nullptr;

	MadoEngine::SpriteHandle backgroundSprite_{};
	MadoEngine::SpriteHandle gaugeSprite_{};

	Settings settings_;
	float currentValue_ = 100.0f;
	float maxValue_ = 100.0f;
	bool isInitialized_ = false;
	SceneType sceneType_ = SceneType::None;
};
