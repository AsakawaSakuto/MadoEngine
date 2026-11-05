#include "TransformNode2D.h"
#include "Math/Function/MatrixFunction.h"
#include <algorithm>
#include <cmath>

namespace MadoEngine {

TransformNode2D::AuthoringState TransformNode2D::GetAuthoringState() const {
	if (authoringState_) {
		return *authoringState_;
	}
	return { transform_, isVisible_, opacity_ };
}

bool TransformNode2D::SetAuthoringState(const AuthoringState& state) {
	const auto& transform = state.transform;

	// 保存可能な値だけを受け入れ、壊れた設定のGPUへの伝播を防止
	if (!std::isfinite(transform.translate.x) || !std::isfinite(transform.translate.y) ||
		!std::isfinite(transform.scale.x) || !std::isfinite(transform.scale.y) ||
		!std::isfinite(transform.rotate) || !std::isfinite(state.opacity)) {
		return false;
	}
	authoringState_ = state;
	authoringState_->opacity = std::clamp(state.opacity, 0.0f, 1.0f);
	transform_ = authoringState_->transform;
	isVisible_ = authoringState_->visible;
	opacity_ = authoringState_->opacity;
	return true;
}

void TransformNode2D::ClearAuthoringState() {

	// 登録解除時にアニメーション途中の値を通常設定として残さないための復元
	if (authoringState_) {
		transform_ = authoringState_->transform;
		isVisible_ = authoringState_->visible;
		opacity_ = authoringState_->opacity;
		authoringState_.reset();
	}
}

void TransformNode2D::CommitLoadedAuthoringState() {

	// 通常のSetterによるアニメーションは保存せず、明示的な読込だけを同期
	if (authoringState_) {
		SetAuthoringState({ transform_, isVisible_, opacity_ });
	}
}

TransformNode2D::TransformNode2D()
	: lifetime_(std::make_shared<LifetimeState>(LifetimeState{ this })) {
}

bool TransformNode2D::SetParent(const TransformNode2D& parent) {

	// 別名での多重所属と、祖先を子へ接続する循環の防止
	if (HasParent()) {
		return false;
	}
	const TransformNode2D* ancestor = &parent;
	std::shared_ptr<const LifetimeState> ancestorLifetime;
	while (ancestor) {
		if (ancestor == this) {
			return false;
		}
		ancestorLifetime = ancestor->parent_.lock();
		ancestor = nullptr;
		if (ancestorLifetime) {
			ancestor = ancestorLifetime->node;
		}
	}

	parent_ = parent.lifetime_;
	return true;
}

void TransformNode2D::ClearParent() {
	parent_.reset();
}

bool TransformNode2D::HasParent() const {
	return !parent_.expired();
}

bool TransformNode2D::IsParent(const TransformNode2D& parent) const {
	return parent_.lock() == parent.lifetime_;
}

Matrix4x4 TransformNode2D::GetLocalMatrix() const {
	return Matrix::MakeScale({ transform_.scale.x, transform_.scale.y, 1.0f })
		* Matrix::MakeRotateZ(transform_.rotate)
		* Matrix::MakeTranslate({ transform_.translate.x, transform_.translate.y, 0.0f });
}

Matrix4x4 TransformNode2D::GetParentWorldMatrix() const {
	const auto parent = parent_.lock();

	// 親の破棄後はルートとして扱い、解放済みノードへの参照を回避
	if (!parent) {
		return Matrix::MakeIdentity();
	}
	return parent->node->GetWorldMatrix();
}

Matrix4x4 TransformNode2D::GetWorldMatrix() const {
	Matrix4x4 result = GetLocalMatrix();

	// 行ベクトル規約で祖先を順に乗算し、非等方拡縮と回転の合成も維持
	for (auto parent = parent_.lock(); parent; parent = parent->node->parent_.lock()) {
		result = result * parent->node->GetLocalMatrix();
	}
	return result;
}

bool TransformNode2D::IsVisibleInHierarchy() const {
	if (!isVisible_) {
		return false;
	}

	// 子の個別設定を変更せず祖先の表示許可だけを合成
	for (auto parent = parent_.lock(); parent; parent = parent->node->parent_.lock()) {
		if (!parent->node->isVisible_) {
			return false;
		}
	}
	return true;
}

void TransformNode2D::SetOpacity(float opacity) {

	// NaNや無限大のAlphaが描画へ伝播することを防止
	if (!std::isfinite(opacity)) {
		return;
	}
	opacity_ = std::clamp(opacity, 0.0f, 1.0f);
}

float TransformNode2D::GetWorldOpacity() const {
	float result = opacity_;

	// 子のAlphaを上書きせず階層ごとのフェードを乗算
	for (auto parent = parent_.lock(); parent; parent = parent->node->parent_.lock()) {
		result *= parent->node->opacity_;
	}
	return result;
}

} // namespace MadoEngine
