#pragma once
#include <string>

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
	virtual void Update() = 0;

	/// @brief シーンの描画処理
	virtual void Draw() = 0;

protected:
	std::string sceneName_; // シーン名
};