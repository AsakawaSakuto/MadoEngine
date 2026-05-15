#pragma once
#include <string>
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Utility/Transform.h"
#include "Core/DxDevice/DxDevice.h"
#include "Core/Command/Command.h"
#include "Core/TextureManager/TextureManager.h"

/// @brief 2D描画オブジェクトの基底クラス
class RenderObject2d {
public:
	/// @brief コンストラクタ
	RenderObject2d();

	/// @brief デストラクタ
	virtual ~RenderObject2d() = default;

	/// @brief 初期化処理（派生クラスでオーバーライド）
	virtual void Initialize() = 0;

	/// @brief 更新処理（派生クラスでオーバーライド）
	virtual void Update() = 0;

	/// @brief 描画処理（派生クラスでオーバーライド）
	virtual void Draw() = 0;

	// ===== Transform関連 =====

	/// @brief 座標を設定
	/// @param pos 座標
	void SetPosition(const Vector2& pos) { transform_.translate = pos; }

	/// @brief 座標を取得
	/// @return 座標
	const Vector2& GetPosition() const { return transform_.translate; }

	/// @brief スケールを設定
	/// @param scale スケール
	void SetScale(const Vector2& scale) { transform_.scale = scale; }

	/// @brief スケールを取得
	/// @return スケール
	const Vector2& GetScale() const { return transform_.scale; }

	/// @brief 回転角度を設定（ラジアン）
	/// @param rotate 回転角度
	void SetRotation(float rotate) { transform_.rotate = rotate; }

	/// @brief 回転角度を取得（ラジアン）
	/// @return 回転角度
	float GetRotation() const { return transform_.rotate; }

	/// @brief Transform全体を取得
	/// @return Transform2D構造体
	const Transform2D& GetTransform() const { return transform_; }

	/// @brief Transform全体を設定
	/// @param transform Transform2D構造体
	void SetTransform(const Transform2D& transform) { transform_ = transform; }

	// ===== 色関連 =====

	/// @brief 色を設定（RGBA）
	/// @param color 色（0.0f～1.0f）
	void SetColor(const Vector4& color) { color_ = color; }

	/// @brief 色を取得
	/// @return 色（RGBA）
	const Vector4& GetColor() const { return color_; }

	// ===== 可視性関連 =====

	/// @brief 表示/非表示を設定
	/// @param visible true:表示、false:非表示
	void SetVisible(bool visible) { isVisible_ = visible; }

	/// @brief 表示状態を取得
	/// @return true:表示、false:非表示
	bool IsVisible() const { return isVisible_; }

protected:
	Transform2D transform_; // トランスフォーム（座標、スケール、回転）
	Vector4 color_;         // 色（RGBA）
	bool isVisible_;        // 表示フラグ
};
