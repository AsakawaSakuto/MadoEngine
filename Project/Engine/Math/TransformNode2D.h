#pragma once

#include "Math/Matrix4x4.h"
#include "Math/Transform.h"
#include <memory>
#include <optional>

namespace MadoEngine {

class UIGroup;

/// @brief 親子の変換・表示状態・透明度を合成する2Dノード
class TransformNode2D {
public:
	/// @brief 実行時の変更から分離した編集用ローカル設定
	struct AuthoringState {
		Transform2D transform;
		bool visible = true;
		float opacity = 1.0f;
	};

	/// @brief 編集用設定の有無を取得
	/// @return 編集用設定を保持する場合はtrue
	bool HasAuthoringState() const { return authoringState_.has_value(); }
	/// @brief 保存・編集の基準となるローカル設定を取得
	/// @return 編集用設定、未設定の場合は現在の実行時設定
	AuthoringState GetAuthoringState() const;
	/// @brief 有限な編集用設定を保存して実行時状態へ反映
	/// @param state 編集用ローカル設定
	/// @return 全ての数値が有限で反映できた場合はtrue
	bool SetAuthoringState(const AuthoringState& state);
	/// @brief 編集用設定を実行時状態へ戻した後に編集用保持を解除
	void ClearAuthoringState();
	/// @brief 編集用設定の保持中だけ明示的な読込結果を編集用設定へ同期
	void CommitLoadedAuthoringState();

	/// @brief 親子参照の寿命情報を初期化
	TransformNode2D();
	/// @brief 子からの親参照を無効化
	virtual ~TransformNode2D() = default;
	TransformNode2D(const TransformNode2D&) = delete;
	TransformNode2D& operator=(const TransformNode2D&) = delete;
	TransformNode2D(TransformNode2D&&) = delete;
	TransformNode2D& operator=(TransformNode2D&&) = delete;

	/// @brief 親を基準とした座標を設定
	/// @param pos ローカル座標
	void SetPosition(const Vector2& pos) { transform_.translate = pos; }
	/// @brief 親を基準とした座標を取得
	/// @return ローカル座標
	const Vector2& GetPosition() const { return transform_.translate; }
	/// @brief ローカル拡縮率を設定
	/// @param scale ローカル拡縮率
	void SetScale(const Vector2& scale) { transform_.scale = scale; }
	/// @brief ローカル拡縮率を取得
	/// @return ローカル拡縮率
	const Vector2& GetScale() const { return transform_.scale; }
	/// @brief ローカル回転角を設定
	/// @param rotate 回転角、ラジアン単位
	void SetRotation(float rotate) { transform_.rotate = rotate; }
	/// @brief ローカル回転角を取得
	/// @return 回転角、ラジアン単位
	float GetRotation() const { return transform_.rotate; }
	/// @brief ローカルTransformを設定
	/// @param transform ローカルTransform
	void SetTransform(const Transform2D& transform) { transform_ = transform; }
	/// @brief ローカルTransformを取得
	/// @return ローカルTransform
	const Transform2D& GetTransform() const { return transform_; }

	/// @brief ローカル値を維持した親を設定
	/// @param parent 設定する親ノード
	/// @return 未所属かつ循環を発生させず登録できた場合はtrue
	bool SetParent(const TransformNode2D& parent);
	/// @brief ローカル値を維持した親子関係の解除
	void ClearParent();
	/// @brief 有効な親の有無を取得
	/// @return 有効な親が存在する場合はtrue
	bool HasParent() const;
	/// @brief 指定したノードが直接の親か確認
	/// @param parent 比較対象の親ノード
	/// @return 直接の親が一致する場合はtrue
	bool IsParent(const TransformNode2D& parent) const;
	/// @brief ローカル変換行列を取得
	/// @return 拡縮・回転・移動の順で合成した行列
	Matrix4x4 GetLocalMatrix() const;
	/// @brief 親のワールド変換行列を取得
	/// @return 親のワールド変換行列、親が存在しない場合は単位行列
	Matrix4x4 GetParentWorldMatrix() const;
	/// @brief 全祖先を含めたワールド変換行列を取得
	/// @return ローカル行列へ祖先の変換を順に合成した行列
	Matrix4x4 GetWorldMatrix() const;

	/// @brief 個別の表示状態を設定
	/// @param visible 表示する場合はtrue
	void SetVisible(bool visible) { isVisible_ = visible; }
	/// @brief 個別の表示状態を取得
	/// @return 個別設定で表示する場合はtrue
	bool IsVisible() const { return isVisible_; }
	/// @brief 全祖先を含めた表示状態を取得
	/// @return 自身と全祖先が表示中の場合はtrue
	bool IsVisibleInHierarchy() const;
	/// @brief 個別の透明度を設定
	/// @param opacity 0から1までの不透明度
	void SetOpacity(float opacity);
	/// @brief 個別の透明度を取得
	/// @return 0から1までの不透明度
	float GetOpacity() const { return opacity_; }
	/// @brief 全祖先を含めた透明度を取得
	/// @return 自身と全祖先の不透明度の積
	float GetWorldOpacity() const;

protected:
	Transform2D transform_;
	bool isVisible_ = true;

private:
	friend class UIGroup;

	struct LifetimeState {
		const TransformNode2D* node = nullptr;
	};

	std::shared_ptr<const LifetimeState> lifetime_;
	std::weak_ptr<const LifetimeState> parent_;
	float opacity_ = 1.0f;
	std::optional<AuthoringState> authoringState_;
};

} // namespace MadoEngine
