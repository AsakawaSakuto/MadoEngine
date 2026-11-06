#pragma once
#include "SceneManager/SceneSharedData.h"
#include "SceneManager/IScene.h"
#include "UI/Result/ResultWeaponStatsUI.h"
#include "UI/Result/ResultScoreUI.h"

/// @brief リザルトシーン
/// @details AまたはSpaceでスコア画面の表示と演出完了とTitleへの移行、スコア画面のQまたはGamePad XでGameへ遷移
class ResultScene : public IScene {
public:
	/// @brief コンストラクタ
	/// @param sceneSharedData シーンをまたいで保持する共有データと共通システム
	explicit ResultScene(SceneSharedData& sceneSharedData);

	/// @brief デストラクタ
	~ResultScene() override;

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

private:
	/// @brief 戦績一覧を非表示にしてスコア画面へ切り替え
	void ShowScore();

	SceneSharedData& sceneSharedData_;
	UI::Result::WeaponStatsUI weaponStatsUI_;
	UI::Result::ScoreUI scoreUI_;
	bool isScoreVisible_ = false;
};
