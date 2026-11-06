#pragma once
#include "SceneSharedData.h"
#include "SceneBgmController.h"
#include "SceneType.h"
#include "Render/Object/2d/IRenderLayerBatchContext.h"
#include "Render/Object/RenderLayer.h"
#include "Render/Object/3d/Model/ModelManager.h"
#include <functional>
#include <map>
#include <memory>
#include <type_traits>

class IScene;

/// @brief シーンのライフサイクルを管理します
/// @details シーンの登録、更新、描画、および遷移を処理します
class SceneManager
{
public:
	/// @brief シーン生成関数の型
	using CreatorFunc = std::function<std::unique_ptr<IScene>(SceneSharedData&)>;

	/// @brief コンストラクタ
	/// @param parameterStore Sceneより長く保持する設定値管理
	explicit SceneManager(MadoEngine::ParameterStore& parameterStore);

	/// @brief デストラクタ
	~SceneManager();

	/// @brief シーン生成関数を登録
	/// @param type シーンのタイプ
	/// @param creator シーン生成関数
	void RegisterScene(SceneType type, CreatorFunc creator);

	/// @brief 指定したシーン型の生成関数を登録
	/// @tparam TScene 登録するISceneの派生型
	/// @param type 登録先のシーン種別
	template <typename TScene>
	void RegisterScene(SceneType type) {
		static_assert(std::is_base_of_v<IScene, TScene>, "登録するシーンはISceneの派生型である必要があります");
		static_assert(
			std::is_constructible_v<TScene, SceneSharedData&> || std::is_default_constructible_v<TScene>,
			"登録するシーンにはSceneSharedData&または引数なしのコンストラクタが必要です"
		);

		// 登録時は生成関数のみを保持し、初期化済みのシーン共有データをシーン切替時に受け渡し
		RegisterScene(type, [](SceneSharedData& sceneSharedData) -> std::unique_ptr<IScene> {

			// シーン共有データを受け取るコンストラクタを優先し、不要なシーンは引数なしで生成
			if constexpr (std::is_constructible_v<TScene, SceneSharedData&>) {
				return std::make_unique<TScene>(sceneSharedData);
			} else {
				return std::make_unique<TScene>();
			}
		});
	}

	/// @brief SceneManagerを初期化
	/// @param initialScene 初期シーンのタイプ
	void Initialize(SceneType initialScene);

	/// @brief 現在のシーンを更新
	void Update(float dt);

	/// @brief ゲーム時間と独立して2Dの描画データを更新
	void UpdateOverlayRenderData();

	/// @brief 現在のシーンを描画
	void Draw();

	/// @brief 指定した描画レイヤーのみを描画
	/// @param layer 描画対象のレイヤー
	void DrawLayer(MadoEngine::Render::RenderLayer layer);

	/// @brief 指定したレイヤーマスクに含まれる描画対象を描画
	/// @param layerMask 描画対象のレイヤーマスク
	void DrawLayerMask(MadoEngine::Render::RenderLayerMask layerMask);

	/// @brief 指定したレイヤーマスクに含まれるScene段階の描画対象を描画
	/// @param layerMask 描画対象のレイヤーマスク
	void DrawSceneLayerMask(MadoEngine::Render::RenderLayerMask layerMask);

	/// @brief 指定したレイヤーマスクに含まれる透明Modelと透過Effectを描画
	/// @param layerMask 描画対象のレイヤーマスク
	void DrawTransparentLayerMask(MadoEngine::Render::RenderLayerMask layerMask);

	/// @brief 指定したレイヤーマスクに含まれるSpriteとTextを描画
	/// @param layerMask 描画対象のレイヤーマスク
	void DrawOverlayLayerMask(MadoEngine::Render::RenderLayerMask layerMask);

	/// @brief SpriteとTextを元の描画順の連続レイヤーバッチとして描画
	/// @param batchContext バッチ前後の描画処理を受け取るContext
	void DrawOverlayInOrder(MadoEngine::Render::IRenderLayerBatchContext& batchContext);

	/// @brief 現在のシーン固有の描画処理を実行
	void DrawCurrentScene();

	/// @brief 現在のシーンのImGuiを描画
	void DrawImGui();

	/// @brief 現在SceneのEditor Documentを保存
	/// @return 全Documentの保存に成功した場合はtrue
	bool SaveEditorDocuments();

	/// @brief 現在SceneのEditor Documentを再読込
	/// @return 全Documentの再読込に成功した場合はtrue
	bool ReloadEditorDocuments();

	/// @brief 現在のシーン種別を取得
	/// @return 現在のシーン種別
	SceneType GetCurrentSceneType() const { return currentSceneType_; }

	/// @brief シーン遷移Effectの進行度を取得
	/// @return 0.0fから1.0fのEffect進行度
	float GetSceneTransitionEffectProgress() const;

	/// @brief シーン遷移演出中か確認
	/// @return シーン遷移演出中の場合はtrue
	bool IsSceneTransitioning() const;

	/// @brief 現在のシーンカメラを取得
	/// @return 現在のシーンカメラ
	const Camera& GetCurrentCamera() const;

	/// @brief 現在のシーンがシャドウマップ生成で注視したい座標を取得
	/// @return シャドウマップの注視点
	Vector3 GetShadowFocusPosition() const;

	/// @brief 現在のシーンからシャドウマップ確認用の対象座標を取得
	/// @param outPosition 対象のワールド座標を受け取る変数
	/// @return 対象座標を取得できた場合はtrue
	bool TryGetShadowDebugTargetPosition(Vector3& outPosition) const;

	/// @brief 保留中のシーン遷移を適用
	/// @return シーンの切り替えを適用した場合はtrue
	bool ApplyPendingSceneChange();

	/// @brief Frame末尾に現在Sceneの保留中Editor操作を適用
	void ApplyPendingEditorOperations();

	/// @brief Editorの未保存確認で確定したシーン遷移を要求
	/// @param type 遷移先のシーンのタイプ
	void RequestConfirmedEditorSceneChange(SceneType type);

	/// @brief 遷移Effectを伴うScene変更または現在Sceneの再開を要求
	/// @param type 遷移先のScene、現在と同じ場合はSceneの再生成
	void RequestSceneChange(SceneType type);

private:
	/// @brief SceneManagerのデバッグ用ImGuiを描画
	void DrawSceneManagerImGui();

	/// @brief Effect最大到達後のシーン切替をフレーム末尾へ予約
	/// @param type 遷移先のシーンのタイプ
	void QueueSceneChange(SceneType type);

	/// @brief 指定されたシーンに変更
	/// @param type 遷移先のシーンのタイプ
	void ChangeScene(SceneType type);

	SceneSharedData sceneSharedData_; // シーンをまたいで保持する共有データと共通システム
	SceneBgmController sceneBgmController_;     // Scene固有BGMと遷移音量の管理
	std::map<SceneType, CreatorFunc> creators_;  // 登録されたシーン生成関数
	std::unique_ptr<IScene> currentScene_;       // 現在のシーン
	SceneType currentSceneType_;                 // 現在のシーンのタイプ
	SceneType pendingSceneType_;                 // 保留中の遷移先シーン
	bool hasPendingSceneChange_;                 // シーン遷移が保留中かどうか

	MadoEngine::ModelHandle selectedModel_{}; // 選択されたモデル
};
