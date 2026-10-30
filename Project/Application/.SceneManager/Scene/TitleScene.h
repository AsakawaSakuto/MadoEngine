#pragma once
#include ".SceneManager/CommonData.h"
#include ".SceneManager/IScene.h"
#include "GameObject/Player/Player.h"
#include "UI/Title/WeaponSelectIcon/WeaponSelectIcon.h"
#include <array>
#include <cstddef>
#include <memory>
#include <optional>

/// @brief タイトルシーン
/// @details 2行6列のアイコンから初期武器を選択してゲームシーンへ遷移
class TitleScene : public IScene {
public:
	/// @brief コンストラクタ
	/// @param commonData Sceneをまたいで保持するApplication共通データ
	explicit TitleScene(CommonData& commonData);

	/// @brief デストラクタ
	~TitleScene() override;

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
	/// @brief 2行6列の武器アイコンと操作案内の初期化
	void InitializeWeaponSelection();

	/// @brief 行と列を維持した方向入力による武器選択の更新
	void UpdateWeaponSelection();

	/// @brief 中立復帰で再入力可能なLスティックの方向入力を取得
	/// @return 強い方の軸の傾き、入力なしの場合はゼロベクトル
	Vector2 GetWeaponSelectionStickInput();

	/// @brief 選択位置に対応するアイコン強調と武器説明の反映
	void RefreshWeaponSelection();

	CommonData& commonData_;
	std::optional<std::size_t> selectedSeedIndex_;

	static constexpr std::size_t kWeaponColumnCount = 6;
	static constexpr std::size_t kWeaponRowCount = 2;
	static_assert(Projectile::kPlayableWeaponTypes.size() == kWeaponColumnCount * kWeaponRowCount);
	std::array<UI::Title::WeaponSelectIcon, Projectile::kPlayableWeaponTypes.size()> weaponIcons_{};
	std::size_t selectedWeaponIndex_ = 0;
	bool isWeaponSelectionStickReady_ = false;
	MadoEngine::SpriteHandle weaponSelectionBackground_{};
	MadoEngine::TextHandle weaponSelectionHeading_{};
	MadoEngine::TextHandle selectedWeaponName_{};
	MadoEngine::TextHandle selectedWeaponDescription_{};
	MadoEngine::TextHandle weaponSelectionGuide_{};

	CameraHandle debugCameraHandle_{};
	CameraHandle tpsCameraHandle_{};

	std::unique_ptr<Player::Base> player_;
	ColliderShape groundCollider_;
	Vector3 groundPosition_{};
};
