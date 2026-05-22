#pragma once
#include <string>
#include "MathHeaders.h"
#include "RenderHeaders.h"
#include "UtilityHeaders.h"
#include "ImGuiHeaders.h"

/// @brief シーンの基底インターフェース
/// @details 各シーン(Title, Game, Resultなど)はこのインターフェースを実装する
class IScene
{
public:

	/// @brief 仮想デストラクタ
	virtual ~IScene() = default;

	/// @brief シーンの初期化処理
	virtual void Initialize() = 0;

	/// @brief シーンの更新処理
	/// @return 次に遷移するシーン名（遷移しない場合は自身のシーン名、または空文字）
	virtual std::string Update() = 0;

	/// @brief シーンの描画処理
	virtual void Draw() = 0;

	/// @brief シーン固有のImGui描画処理
	virtual void DrawImGui() = 0;

	/// @brief シーンの終了処理
	virtual void Finalize() = 0;
};