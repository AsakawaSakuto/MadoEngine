#include "EffectSequenceInstance.h"
#include "Math/Function/MatrixFunction.h"
#include <algorithm>
#include <cmath>

namespace {

	using namespace MadoEngine::EffectSequence;

	constexpr float kSequenceTimeEpsilon = 0.000001f;

	/// @brief Vector3が有限値だけで構成されているか確認
	/// @param value 確認対象Vector
	/// @return 全要素が有限値の場合はtrue
	bool IsFiniteVector3(const Vector3& value) {
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
	}

	/// @brief 外部から渡されたRoot Transformを安全な範囲へ補正
	/// @param transform 補正対象Transform
	/// @return 補正後Transform
	Transform3D NormalizeRootTransform(Transform3D transform) {
		if (!IsFiniteVector3(transform.scale)) {
			transform.scale = { 1.0f, 1.0f, 1.0f };
		}
		if (!IsFiniteVector3(transform.rotate)) {
			transform.rotate = {};
		}
		if (!IsFiniteVector3(transform.translate)) {
			transform.translate = {};
		}
		transform.scale.x = std::clamp(transform.scale.x, 0.001f, 10000.0f);
		transform.scale.y = std::clamp(transform.scale.y, 0.001f, 10000.0f);
		transform.scale.z = std::clamp(transform.scale.z, 0.001f, 10000.0f);
		transform.rotate.x = std::clamp(transform.rotate.x, -10000.0f, 10000.0f);
		transform.rotate.y = std::clamp(transform.rotate.y, -10000.0f, 10000.0f);
		transform.rotate.z = std::clamp(transform.rotate.z, -10000.0f, 10000.0f);
		transform.translate.x = std::clamp(transform.translate.x, -1000000.0f, 1000000.0f);
		transform.translate.y = std::clamp(transform.translate.y, -1000000.0f, 1000000.0f);
		transform.translate.z = std::clamp(transform.translate.z, -1000000.0f, 1000000.0f);
		return transform;
	}

	/// @brief Affine MatrixをTransform3Dへ分解
	/// @param matrix 分解対象Matrix
	/// @return 分解後Transform
	Transform3D DecomposeTransform(const Matrix4x4& matrix) {
		Transform3D result;
		result.translate = { matrix.m[3][0], matrix.m[3][1], matrix.m[3][2] };
		result.scale = {
			std::sqrt(matrix.m[0][0] * matrix.m[0][0] + matrix.m[0][1] * matrix.m[0][1] + matrix.m[0][2] * matrix.m[0][2]),
			std::sqrt(matrix.m[1][0] * matrix.m[1][0] + matrix.m[1][1] * matrix.m[1][1] + matrix.m[1][2] * matrix.m[1][2]),
			std::sqrt(matrix.m[2][0] * matrix.m[2][0] + matrix.m[2][1] * matrix.m[2][1] + matrix.m[2][2] * matrix.m[2][2]),
		};
		result.scale.x = (std::max)(result.scale.x, 0.001f);
		result.scale.y = (std::max)(result.scale.y, 0.001f);
		result.scale.z = (std::max)(result.scale.z, 0.001f);

		const float rotation[3][3] = {
			{ matrix.m[0][0] / result.scale.x, matrix.m[0][1] / result.scale.x, matrix.m[0][2] / result.scale.x },
			{ matrix.m[1][0] / result.scale.y, matrix.m[1][1] / result.scale.y, matrix.m[1][2] / result.scale.y },
			{ matrix.m[2][0] / result.scale.z, matrix.m[2][1] / result.scale.z, matrix.m[2][2] / result.scale.z },
		};
		const float sinY = std::clamp(-rotation[0][2], -1.0f, 1.0f);
		result.rotate.y = std::asin(sinY);
		const float cosY = std::cos(result.rotate.y);
		if (std::abs(cosY) > 0.00001f) {
			result.rotate.x = std::atan2(rotation[1][2], rotation[2][2]);
			result.rotate.z = std::atan2(rotation[0][1], rotation[0][0]);
		} else {
			result.rotate.x = std::atan2(
				sinY >= 0.0f ? rotation[1][0] : -rotation[1][0],
				rotation[1][1]
			);
			result.rotate.z = 0.0f;
		}
		return NormalizeRootTransform(result);
	}

	/// @brief ParentからLocalへの順序でTransformを合成
	/// @param parentTransform 親World Transform
	/// @param localTransform 子Local Transform
	/// @return 子World Transform
	Transform3D ComposeTransform(
		const Transform3D& parentTransform,
		const Transform3D& localTransform) {
		const Matrix4x4 parentMatrix = Matrix::MakeAffine(
			parentTransform.scale,
			parentTransform.rotate,
			parentTransform.translate
		);
		const Matrix4x4 localMatrix = Matrix::MakeAffine(
			localTransform.scale,
			localTransform.rotate,
			localTransform.translate
		);
		return DecomposeTransform(Matrix::Multiply(localMatrix, parentMatrix));
	}

} // namespace

namespace MadoEngine::EffectSequence {

	void EffectSequenceInstance::Initialize(
		std::shared_ptr<const EffectSequenceAsset> asset,
		const EffectSequencePlayDesc& desc,
		const EffectSequenceNodeDispatcher& dispatcher) {
		asset_ = std::move(asset);
		dispatcher_ = &dispatcher;
		activeChildren_.clear();
		firedNodeIds_.clear();
		nodeIndices_.clear();
		worldTransforms_.clear();
		rootTransform_ = NormalizeRootTransform(desc.rootTransform);
		sceneType_ = desc.sceneType;
		defaultRenderLayer_ = MadoEngine::Render::IsValidRenderLayer(desc.renderLayer)
			? desc.renderLayer
			: MadoEngine::Render::RenderLayer::Effect;
		context_ = desc.context;
		finishReason_ = EffectSequenceFinishReason::Natural;
		playbackTime_ = 0.0f;
		isPaused_ = false;
		isWaitingForChildren_ = false;
		isFinished_ = !asset_;
		if (!asset_) {
			return;
		}

		const EffectSequenceConfig& config = asset_->GetConfig();
		isLoop_ = desc.loopOverride.value_or(config.isLoop);
		playbackSpeed_ = desc.playbackSpeedOverride.value_or(config.playbackSpeed);
		if (!std::isfinite(playbackSpeed_) || playbackSpeed_ <= 0.0f) {
			playbackSpeed_ = 1.0f;
		}
		playbackSpeed_ = std::clamp(
			playbackSpeed_,
			kMinimumEffectSequencePlaybackSpeed,
			kMaximumEffectSequencePlaybackSpeed
		);
		RebuildWorldTransforms();
		FireNodes(0.0f, 0.0f, true);
	}

	void EffectSequenceInstance::Update(float deltaTime) {
		if (isFinished_ || !asset_ || !dispatcher_) {
			return;
		}
		RebuildWorldTransforms();
		ApplyWorldTransformsToChildren();
		RemoveFinishedChildren();

		// 親Timeline終了後もFinish停止したChildが完了するまでSequenceを保持
		if (isWaitingForChildren_) {
			TryFinishWaitingSequence();
			return;
		}
		if (isPaused_) {
			return;
		}

		const float safeDeltaTime = std::clamp(
			std::isfinite(deltaTime) ? deltaTime : 0.0f,
			0.0f,
			kMaximumEffectSequenceDuration
		);
		float remainingTime = safeDeltaTime * playbackSpeed_;
		const float duration = asset_->GetConfig().duration;

		// 非Loop再生は終端を一度だけ通過して未発火Nodeを確実にDispatch
		if (!isLoop_) {
			const float previousTime = playbackTime_;
			playbackTime_ = (std::min)(playbackTime_ + remainingTime, duration);
			FireNodes(previousTime, playbackTime_, false);
			if (playbackTime_ + kSequenceTimeEpsilon >= duration) {
				playbackTime_ = duration;
				finishReason_ = EffectSequenceFinishReason::Natural;
				isWaitingForChildren_ = true;
				StopChildren(EffectSequenceStopMode::Finish);
				TryFinishWaitingSequence();
			}
			return;
		}

		uint32_t processedLoopCount = 0;

		// 一Frame内のLoop回数へ上限を設けて極端なDelta Timeでの無制限処理を防止
		while (remainingTime > 0.0f && processedLoopCount < kMaximumEffectSequenceLoopsPerUpdate) {
			const float timeToLoopEnd = duration - playbackTime_;
			if (remainingTime + kSequenceTimeEpsilon < timeToLoopEnd) {
				const float previousTime = playbackTime_;
				playbackTime_ += remainingTime;
				FireNodes(previousTime, playbackTime_, false);
				remainingTime = 0.0f;
				break;
			}

			FireNodes(playbackTime_, duration, false);
			remainingTime = (std::max)(0.0f, remainingTime - timeToLoopEnd);
			BeginNextLoop();
			++processedLoopCount;
		}

		if (remainingTime > 0.0f) {
			if (remainingTime >= duration) {

				// 上限を超えたLoop分はChild状態を破棄して時間だけ高速に繰り越し
				StopChildren(EffectSequenceStopMode::Immediate);
				activeChildren_.clear();
				firedNodeIds_.clear();
				playbackTime_ = 0.0f;
				remainingTime = std::fmod(remainingTime, duration);
				FireNodes(0.0f, 0.0f, true);
			}
			const float previousTime = playbackTime_;
			playbackTime_ = (std::min)(playbackTime_ + remainingTime, duration);
			FireNodes(previousTime, playbackTime_, false);
		}
	}

	void EffectSequenceInstance::Stop(EffectSequenceStopMode mode) {
		if (isFinished_) {
			return;
		}
		isPaused_ = false;
		if (mode == EffectSequenceStopMode::Immediate) {
			finishReason_ = EffectSequenceFinishReason::StopImmediate;
			StopChildren(EffectSequenceStopMode::Immediate);
			activeChildren_.clear();
			isWaitingForChildren_ = false;
			isFinished_ = true;
			return;
		}

		finishReason_ = EffectSequenceFinishReason::StopFinish;
		isWaitingForChildren_ = true;
		for (ActiveChild& child : activeChildren_) {
			dispatcher_->Resume(child.handle);
		}
		StopChildren(EffectSequenceStopMode::Finish);
		TryFinishWaitingSequence();
	}

	void EffectSequenceInstance::Destroy(EffectSequenceFinishReason reason) {
		if (isFinished_) {
			return;
		}
		finishReason_ = reason;
		StopChildren(EffectSequenceStopMode::Immediate);
		activeChildren_.clear();
		isWaitingForChildren_ = false;
		isFinished_ = true;
	}

	void EffectSequenceInstance::Pause() {
		if (isFinished_ || isPaused_) {
			return;
		}
		isPaused_ = true;
		for (ActiveChild& child : activeChildren_) {
			dispatcher_->Pause(child.handle);
		}
	}

	void EffectSequenceInstance::Resume() {
		if (isFinished_ || !isPaused_) {
			return;
		}
		isPaused_ = false;
		for (ActiveChild& child : activeChildren_) {
			dispatcher_->Resume(child.handle);
		}
	}

	void EffectSequenceInstance::SetTransform(const Transform3D& transform) {
		rootTransform_ = NormalizeRootTransform(transform);
		RebuildWorldTransforms();
		ApplyWorldTransformsToChildren();
	}

	bool EffectSequenceInstance::SetPlaybackSpeed(float playbackSpeed) {
		if (
			!std::isfinite(playbackSpeed) ||
			playbackSpeed < kMinimumEffectSequencePlaybackSpeed ||
			playbackSpeed > kMaximumEffectSequencePlaybackSpeed) {
			return false;
		}
		playbackSpeed_ = playbackSpeed;
		ApplyPlaybackSpeedToChildren();
		return true;
	}

	const std::string& EffectSequenceInstance::GetAssetName() const {
		static const std::string emptyName;
		return asset_ ? asset_->GetName() : emptyName;
	}

	void EffectSequenceInstance::FireNodes(
		float previousTime,
		float currentTime,
		bool includePrevious) {
		if (!asset_ || !dispatcher_) {
			return;
		}
		for (const EffectSequenceNode& node : asset_->GetConfig().nodes) {
			if (!node.isEnabled || firedNodeIds_.contains(node.nodeId)) {
				continue;
			}
			const bool isAfterStart = includePrevious
				? node.startTime >= previousTime
				: node.startTime > previousTime;
			if (isAfterStart && node.startTime <= currentTime + kSequenceTimeEpsilon) {
				FireNode(node);
			}
		}
	}

	void EffectSequenceInstance::FireNode(const EffectSequenceNode& node) {
		firedNodeIds_.insert(node.nodeId);
		const auto worldTransform = worldTransforms_.find(node.nodeId);
		if (worldTransform == worldTransforms_.end()) {
			return;
		}
		const float childPlaybackSpeed = std::clamp(
			playbackSpeed_ * node.playbackSpeed,
			kMinimumEffectSequencePlaybackSpeed,
			256.0f
		);
		const std::optional<EffectSequenceChildHandle> child = dispatcher_->Play(
			node,
			worldTransform->second,
			sceneType_,
			defaultRenderLayer_,
			childPlaybackSpeed
		);
		if (child.has_value()) {
			activeChildren_.push_back({ node.nodeId, child.value() });
			if (isPaused_) {
				dispatcher_->Pause(activeChildren_.back().handle);
			}
		}
	}

	void EffectSequenceInstance::RebuildWorldTransforms() {
		if (!asset_) {
			return;
		}
		nodeIndices_.clear();
		worldTransforms_.clear();
		const std::vector<EffectSequenceNode>& nodes = asset_->GetConfig().nodes;
		for (std::size_t index = 0; index < nodes.size(); ++index) {
			nodeIndices_[nodes[index].nodeId] = index;
		}
		std::vector<uint8_t> visitStates(nodes.size(), 0);
		for (std::size_t index = 0; index < nodes.size(); ++index) {
			ResolveWorldTransform(index, visitStates);
		}
	}

	Transform3D EffectSequenceInstance::ResolveWorldTransform(
		std::size_t nodeIndex,
		std::vector<uint8_t>& visitStates) {
		const std::vector<EffectSequenceNode>& nodes = asset_->GetConfig().nodes;
		const EffectSequenceNode& node = nodes[nodeIndex];
		if (visitStates[nodeIndex] == 2) {
			return worldTransforms_.at(node.nodeId);
		}
		if (visitStates[nodeIndex] == 1) {
			const Transform3D safeTransform = ComposeTransform(rootTransform_, node.localTransform);
			worldTransforms_[node.nodeId] = safeTransform;
			return safeTransform;
		}

		visitStates[nodeIndex] = 1;
		Transform3D parentTransform = rootTransform_;
		if (node.parentNodeId.has_value()) {
			const auto parent = nodeIndices_.find(node.parentNodeId.value());
			if (parent != nodeIndices_.end() && parent->second != nodeIndex) {
				parentTransform = ResolveWorldTransform(parent->second, visitStates);
			}
		}
		const Transform3D worldTransform = ComposeTransform(parentTransform, node.localTransform);
		worldTransforms_[node.nodeId] = worldTransform;
		visitStates[nodeIndex] = 2;
		return worldTransform;
	}

	void EffectSequenceInstance::ApplyWorldTransformsToChildren() {
		if (!asset_ || !dispatcher_) {
			return;
		}
		const std::vector<EffectSequenceNode>& nodes = asset_->GetConfig().nodes;
		std::erase_if(activeChildren_, [&](ActiveChild& child) {
			const auto nodeIndex = nodeIndices_.find(child.nodeId);
			const auto worldTransform = worldTransforms_.find(child.nodeId);
			if (nodeIndex == nodeIndices_.end() || worldTransform == worldTransforms_.end()) {
				dispatcher_->Stop(child.handle, EffectSequenceStopMode::Immediate);
				return true;
			}
			dispatcher_->SetTransform(nodes[nodeIndex->second], child.handle, worldTransform->second);
			return false;
		});
	}

	void EffectSequenceInstance::ApplyPlaybackSpeedToChildren() {
		if (!asset_ || !dispatcher_) {
			return;
		}
		const std::vector<EffectSequenceNode>& nodes = asset_->GetConfig().nodes;
		for (ActiveChild& child : activeChildren_) {
			const auto nodeIndex = nodeIndices_.find(child.nodeId);
			if (nodeIndex == nodeIndices_.end()) {
				continue;
			}
			const float childPlaybackSpeed = std::clamp(
				playbackSpeed_ * nodes[nodeIndex->second].playbackSpeed,
				kMinimumEffectSequencePlaybackSpeed,
				256.0f
			);
			dispatcher_->SetPlaybackSpeed(child.handle, childPlaybackSpeed);
		}
	}

	void EffectSequenceInstance::StopChildren(EffectSequenceStopMode mode) {
		if (!dispatcher_) {
			return;
		}
		for (ActiveChild& child : activeChildren_) {
			dispatcher_->Stop(child.handle, mode);
		}
	}

	void EffectSequenceInstance::RemoveFinishedChildren() {
		if (!dispatcher_) {
			return;
		}
		std::erase_if(activeChildren_, [&](const ActiveChild& child) {
			return !dispatcher_->IsAlive(child.handle);
		});
	}

	void EffectSequenceInstance::BeginNextLoop() {
		StopChildren(EffectSequenceStopMode::Immediate);
		activeChildren_.clear();
		firedNodeIds_.clear();
		playbackTime_ = 0.0f;
		FireNodes(0.0f, 0.0f, true);
	}

	void EffectSequenceInstance::TryFinishWaitingSequence() {
		RemoveFinishedChildren();
		if (isWaitingForChildren_ && activeChildren_.empty()) {
			isWaitingForChildren_ = false;
			isFinished_ = true;
		}
	}

} // namespace MadoEngine::EffectSequence
