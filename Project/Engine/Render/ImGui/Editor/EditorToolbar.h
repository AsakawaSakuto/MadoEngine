#pragma once

#ifdef USE_IMGUI

#include <array>
#include <cstddef>
#include <string>

namespace MadoEngine::Editor {

/// @brief Editorから表示を切り替えるウィンドウ種別
enum class EditorWindow {
	GameView,
	FixedGameView,
	EngineInfo,
	SceneManager,
	Camera,
	SceneDebug,
	ModelGizmo,
	PostEffect,
	Audio,
	Light,
	Model,
	Sprite,
	Text,
	Particle,
	Cylinder,
	Ribbon,
	Beam,
	EffectSequence,
	StyleColor,
	Logger,
	Count,
};

/// @brief Editorの未保存状態を追跡するDocument種別
enum class EditorDocument {
	Camera,
	Scene,
	PostEffect,
	Audio,
	Light,
	Model,
	Sprite,
	Text,
	Particle,
	Cylinder,
	Ribbon,
	Beam,
	EffectSequence,
	StyleColor,
	Count,
};

/// @brief Editor共通ツールバーの操作状態とウィンドウ表示状態を管理するクラス
class EditorToolbar final {
public:
	/// @brief EditorToolbarのSingleton Instanceを取得
	/// @return EditorToolbarの参照
	static EditorToolbar& GetInstance();

	EditorToolbar(const EditorToolbar&) = delete;
	EditorToolbar& operator=(const EditorToolbar&) = delete;
	EditorToolbar(EditorToolbar&&) = delete;
	EditorToolbar& operator=(EditorToolbar&&) = delete;

	/// @brief Editor設定を初期化
	void Initialize();

	/// @brief Editor設定を保存して終了
	void Finalize();

	/// @brief Main MenuとEditor共通ツールバーを描画
	/// @return DockSpace上端から確保するツールバー高さ
	float Draw();

	/// @brief 実時間から今回のゲーム更新用時間を取得
	/// @param realDeltaTime 実時間のdeltaTime
	/// @param outGameDeltaTime ゲーム更新へ渡すdeltaTime
	/// @return 今回ゲーム更新を実行する場合はtrue
	bool ConsumeGameDeltaTime(float realDeltaTime, float& outGameDeltaTime);

	/// @brief 指定ウィンドウの表示状態を取得
	/// @param window 確認対象のウィンドウ種別
	/// @return 表示する場合はtrue
	bool IsWindowVisible(EditorWindow window) const;

	/// @brief Save All要求を取得して解除
	/// @return Save All要求がある場合はtrue
	bool ConsumeSaveAllRequest();

	/// @brief Reload All要求を取得して解除
	/// @return Reload All要求がある場合はtrue
	bool ConsumeReloadAllRequest();

	/// @brief レイアウト保存要求を取得して解除
	/// @return レイアウト保存要求がある場合はtrue
	bool ConsumeSaveLayoutRequest();

	/// @brief レイアウト初期化要求を取得して解除
	/// @return レイアウト初期化要求がある場合はtrue
	bool ConsumeResetLayoutRequest();

	/// @brief Document編集検出を開始
	/// @param document 編集状態を追跡するDocument種別
	void BeginDocumentCapture(EditorDocument document);

	/// @brief Document編集検出を終了
	void EndDocumentCapture();

	/// @brief 指定Documentを未保存状態へ設定
	/// @param document 未保存にするDocument種別
	void MarkDocumentDirty(EditorDocument document);

	/// @brief 指定Documentを保存済み状態へ設定
	/// @param document 保存済みにするDocument種別
	void MarkDocumentSaved(EditorDocument document);

	/// @brief 全Documentを保存済み状態へ設定
	void MarkAllDocumentsSaved();

	/// @brief Save AllまたはReload Allの結果を通知
	/// @param actionName 実行した操作名
	/// @param succeeded 全対象で成功した場合はtrue
	void NotifyDocumentOperationResult(const char* actionName, bool succeeded);

	/// @brief Document状態を変更しないEditor操作結果を通知
	/// @param actionName 実行した操作名
	/// @param succeeded 操作に成功した場合はtrue
	void NotifyOperationResult(const char* actionName, bool succeeded);

	/// @brief ゲーム更新が一時停止中か確認
	/// @return 一時停止中の場合はtrue
	bool IsPaused() const { return isPaused_; }

private:
	EditorToolbar();
	~EditorToolbar() = default;

	/// @brief Editorの既定状態を適用
	void ApplyDefaultSettings();

	/// @brief Editor設定をJsonから読み込み
	/// @return 読み込みに成功した場合はtrue
	bool LoadSettings();

	/// @brief Editor設定をJsonへ保存
	/// @return 保存に成功した場合はtrue
	bool SaveSettings() const;

	/// @brief Editor用ショートカット入力を処理
	void HandleShortcuts();

	/// @brief EditorのMain Menuを描画
	void DrawMainMenu();

	/// @brief 再生制御ツールバーを描画
	/// @return DockSpace上端から確保するツールバー高さ
	float DrawPlaybackToolbar();

	/// @brief Reload All確認Popupを描画
	void DrawReloadConfirmationPopup();

	/// @brief 未保存Documentが存在するか確認
	/// @return 未保存Documentが存在する場合はtrue
	bool HasDirtyDocument() const;

	static constexpr std::size_t kWindowCount = static_cast<std::size_t>(EditorWindow::Count);
	static constexpr std::size_t kDocumentCount = static_cast<std::size_t>(EditorDocument::Count);

	std::array<bool, kWindowCount> windowVisibility_{};
	std::array<bool, kDocumentCount> dirtyDocuments_{};
	EditorDocument capturedDocument_ = EditorDocument::Count;
	bool documentEditedAtCaptureStart_ = false;
	bool isDocumentCaptureActive_ = false;
	bool isPaused_ = false;
	bool stepRequested_ = false;
	bool saveAllRequested_ = false;
	bool reloadAllRequested_ = false;
	bool saveLayoutRequested_ = false;
	bool resetLayoutRequested_ = false;
	bool openReloadConfirmation_ = false;
	bool isInitialized_ = false;
	float timeScale_ = 1.0f;
	float fixedStepDeltaTime_ = 1.0f / 60.0f;
	std::string operationStatusMessage_;
	bool operationStatusSucceeded_ = true;
	double operationStatusExpireTime_ = 0.0;
};

} // namespace MadoEngine::Editor

#endif // USE_IMGUI
