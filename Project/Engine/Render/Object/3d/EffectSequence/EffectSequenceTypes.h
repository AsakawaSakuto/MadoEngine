#pragma once
#include ".SceneManager/SceneType.h"
#include "Math/Transform.h"
#include "Math/Vector3.h"
#include "Render/Object/3d/BeamEffect/BeamEffectTypes.h"
#include "Render/Object/3d/Particle/ParticleTypes.h"
#include "Render/Object/3d/PrimitiveEffect/PrimitiveEffectTypes.h"
#include "Render/Object/3d/RibbonEffect/RibbonEffectTypes.h"
#include "Render/Object/RenderLayer.h"
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace MadoEngine::EffectSequence {

	inline constexpr std::size_t kMaximumEffectSequenceNodeCount = 512;
	inline constexpr float kMinimumEffectSequenceDuration = 0.001f;
	inline constexpr float kMaximumEffectSequenceDuration = 3600.0f;
	inline constexpr float kMinimumEffectSequencePlaybackSpeed = 0.01f;
	inline constexpr float kMaximumEffectSequencePlaybackSpeed = 16.0f;
	inline constexpr uint32_t kMaximumEffectSequenceLoopsPerUpdate = 64;

	enum class EffectSequenceNodeType : uint32_t {
		Particle,
		PrimitiveEffect,
		Ribbon,
		Beam,
		Count,
	};

	enum class PrimitiveEffectNodeKind : uint32_t {
		Cylinder,
		Count,
	};

	enum class EffectSequenceStopMode : uint32_t {
		Finish,
		Immediate,
	};

	enum class EffectSequenceFinishReason : uint32_t {
		Natural,
		StopFinish,
		StopImmediate,
		SceneCleared,
	};

	enum class EffectSequencePlaybackContext : uint32_t {
		Game,
		EditorPreview,
	};

	struct ParticleNodeSettings {
	};

	struct PrimitiveEffectNodeSettings {
		PrimitiveEffectNodeKind kind = PrimitiveEffectNodeKind::Cylinder;
	};

	struct RibbonNodeSettings {
		bool overrideManualControlPoints = false;
		std::vector<Vector3> controlPoints;
	};

	struct BeamNodeSettings {
		Vector3 startPosition{};
		Vector3 endPosition = { 0.0f, 1.0f, 0.0f };
	};

	using EffectSequenceNodeSettings = std::variant<
		ParticleNodeSettings,
		PrimitiveEffectNodeSettings,
		RibbonNodeSettings,
		BeamNodeSettings
	>;

	struct EffectSequenceNode {
		uint32_t nodeId = 0;
		std::string displayName = "Effect Node";
		EffectSequenceNodeType nodeType = EffectSequenceNodeType::Particle;
		std::string effectAssetName;
		bool isEnabled = true;
		float startTime = 0.0f;
		float playbackSpeed = 1.0f;
		Transform3D localTransform;
		std::optional<uint32_t> parentNodeId;
		std::optional<MadoEngine::Render::RenderLayer> renderLayer;
		EffectSequenceNodeSettings settings = ParticleNodeSettings{};
	};

	struct EffectSequenceConfig {
		float duration = 1.0f;
		bool isLoop = false;
		float playbackSpeed = 1.0f;
		std::vector<EffectSequenceNode> nodes;
	};

	struct EffectSequenceHandle {
		uint32_t index = (std::numeric_limits<uint32_t>::max)();
		uint32_t generation = 0;

		/// @brief Handleが有効値を保持しているか確認
		/// @return 有効値を保持している場合はtrue
		bool HasValue() const {
			return index != (std::numeric_limits<uint32_t>::max)() && generation != 0;
		}

		/// @brief 2つのHandleが同じInstanceを指すか比較
		/// @param other 比較対象Handle
		/// @return 同じ値の場合はtrue
		bool operator==(const EffectSequenceHandle& other) const = default;
	};

	using EffectSequenceChildHandle = std::variant<
		MadoEngine::Particle::EffectHandle,
		MadoEngine::Effect::PrimitiveEffectHandle,
		MadoEngine::Ribbon::RibbonEffectHandle,
		MadoEngine::Beam::BeamEffectHandle
	>;

	struct EffectSequencePlayDesc {
		Transform3D rootTransform;
		SceneType sceneType = SceneType::None;
		MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::Effect;
		std::optional<bool> loopOverride;
		std::optional<float> playbackSpeedOverride;
		EffectSequencePlaybackContext context = EffectSequencePlaybackContext::Game;
	};

	struct EffectSequenceFinishedEvent {
		EffectSequenceHandle handle;
		std::string assetName;
		SceneType sceneType = SceneType::None;
		EffectSequenceFinishReason reason = EffectSequenceFinishReason::Natural;
		EffectSequencePlaybackContext context = EffectSequencePlaybackContext::Game;
	};

} // namespace MadoEngine::EffectSequence
