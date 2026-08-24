#pragma once
#include ".SceneManager/CommonData.h"
#include ".SceneManager/IScene.h"
#include "GameObject/Player/Player.h"
#include <cstddef>
#include <memory>
#include <optional>

/// @brief タイトルシーン
/// @details ゲームのタイトル画面を表示し、スペースキーでゲームシーンに遷移
class Title : public IScene
{
public:
	/// @brief コンストラクタ
	/// @param commonData Sceneをまたいで保持するApplication共通データ
	explicit Title(CommonData& commonData);

	/// @brief デストラクタ
	~Title() override;

	/// @brief 初期化処理
	void Initialize() override;

	/// @brief 更新処理
	/// @param dt デルタタイム
	/// @return 次に遷移するシーンの種類
	SceneType Update(float dt) override;

	/// @brief 終了処理
	void Finalize() override;

	/// @brief 描画処理
	void Draw() override;

	/// @brief ImGui描画処理
	void DrawImGui() override;

	/// @brief シャドウマップ生成時に中心へ置くワールド座標を取得
	/// @return Playerのワールド座標
	Vector3 GetShadowFocusPosition() const override;

	/// @brief シャドウマップ確認用のPlayer描画座標を取得
	/// @param outPosition PlayerのModelワールド座標を受け取る変数
	/// @return Player座標を取得できた場合はtrue
	bool TryGetShadowDebugTargetPosition(Vector3& outPosition) const override;

private:
	CommonData& commonData_;
	std::optional<std::size_t> selectedSeedIndex_;

	MadoEngine::SpriteHandle wallPaperSprite_{};

	CameraHandle debugCameraHandle_{};
	CameraHandle tpsCameraHandle_{};

	std::unique_ptr<Player::Base> player_;
	ColliderShape groundCollider_;
	Vector3 groundPosition_{};
};
