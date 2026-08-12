#pragma once
#include ".SceneManager/SceneType.h"
#include "Math/Transform.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Render/Object/3d/PrimitiveEffect/EffectTrack.h"
#include "Render/Object/RenderLayer.h"
#include "Render/PSO/PSODesc.h"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace MadoEngine::Ribbon {

	inline constexpr uint32_t kMinimumRibbonPointCount = 2;
	inline constexpr uint32_t kMaximumRibbonPointCount = 4096;
	inline constexpr uint32_t kMaximumRibbonSmoothingSubdivision = 32;
	inline constexpr uint32_t kDefaultRibbonCurveSubdivision = 8;
	inline constexpr std::size_t kMaximumRibbonEmitterCount = 64;

	enum class RibbonPointGenerationMode : uint32_t {
		TransformHistory,
		Manual,
	};

	enum class RibbonSimulationSpace : uint32_t {
		World,
		Local,
	};

	enum class RibbonInterpolationMode : uint32_t {
		Linear,
		CatmullRom,
	};

	enum class RibbonUvMode : uint32_t {
		Stretch,
		Tile,
	};

	enum class RibbonStopMode : uint32_t {
		Finish,
		Immediate,
	};

	enum class RibbonPlaybackMode : uint32_t {
		Full,
		Reveal,
		Sweep,
	};

	struct RibbonPlaybackModule {
		/// @brief 0から1へ進む既定トラックを持つ再生設定を構築
		RibbonPlaybackModule()
			: progress(0.0f) {
			progress.SetKeyframes({
				{ 0.0f, 0.0f, EaseType::Linear },
				{ 1.0f, 1.0f, EaseType::Linear },
			});
		}

		float duration = 1.0f;
		bool isLoop = false;
		RibbonPlaybackMode mode = RibbonPlaybackMode::Full;
		MadoEngine::Effect::EffectTrack<float> progress;
		float sweepLength = 1.0f;
	};

	struct RibbonTrailModule {
		float pointLifetime = 0.5f;
		float minPointDistance = 0.05f;
		uint32_t maxPointCount = 128;
		RibbonPointGenerationMode generationMode = RibbonPointGenerationMode::TransformHistory;
		RibbonSimulationSpace simulationSpace = RibbonSimulationSpace::World;
		std::vector<Vector3> defaultControlPoints = {
			{ -3.0f, 0.0f, 0.0f },
			{ -1.0f, 1.0f, 0.0f },
			{ 1.0f, -0.5f, 0.0f },
			{ 3.0f, 0.5f, 0.0f },
		};
	};

	struct RibbonGeometryModule {
		MadoEngine::Effect::EffectTrack<float> widthOverLifetime =
			MadoEngine::Effect::EffectTrack<float>{ 0.5f };
		RibbonInterpolationMode interpolation = RibbonInterpolationMode::Linear;
		uint32_t smoothingSubdivision = 0;
		bool cameraFacing = true;
	};

	struct RibbonMaterialModule {
		std::string textureName = "white2x2";
		MadoEngine::Render::BlendMode blendMode = MadoEngine::Render::BlendMode::Add;
		MadoEngine::Render::CullMode cullMode = MadoEngine::Render::CullMode::None;
		MadoEngine::Effect::EffectTrack<Vector4> colorOverLifetime =
			MadoEngine::Effect::EffectTrack<Vector4>{ Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } };
		MadoEngine::Effect::EffectTrack<float> globalAlpha =
			MadoEngine::Effect::EffectTrack<float>{ 1.0f };
		Vector2 uvScale = { 1.0f, 1.0f };
		Vector2 uvOffset{};
		Vector2 uvScroll{};
		RibbonUvMode uvMode = RibbonUvMode::Stretch;
		float tileLength = 1.0f;
	};

	struct RibbonEmitterConfig {
		std::string name = "Emitter";
		bool isEnabled = true;
		Vector3 translateOffset{};
		RibbonPlaybackModule playback;
		RibbonTrailModule trail;
		RibbonGeometryModule geometry;
		RibbonMaterialModule material;
	};

	using RibbonEffectConfig = RibbonEmitterConfig;

	struct RibbonEffectHandle {
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
		bool operator==(const RibbonEffectHandle& other) const = default;
	};

	struct RibbonEffectPlayDesc {
		Transform3D transform;
		SceneType sceneType = SceneType::None;
		MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::Effect;
		std::optional<bool> loopOverride;
	};

	struct RibbonPoint {
		Vector3 position{};
		float age = 0.0f;
		float lifetime = 1.0f;
	};

	struct RibbonRenderData {
		std::vector<RibbonPoint> points;
		MadoEngine::Effect::EffectTrack<float> widthOverLifetime;
		MadoEngine::Effect::EffectTrack<Vector4> colorOverLifetime;
		RibbonInterpolationMode interpolation = RibbonInterpolationMode::Linear;
		uint32_t smoothingSubdivision = 0;
		bool cameraFacing = true;
		std::string textureName = "white2x2";
		MadoEngine::Render::BlendMode blendMode = MadoEngine::Render::BlendMode::Add;
		MadoEngine::Render::CullMode cullMode = MadoEngine::Render::CullMode::None;
		float globalAlpha = 1.0f;
		float startAlphaFade = 0.0f;
		float endAlphaFade = 0.0f;
		Vector2 uvScale = { 1.0f, 1.0f };
		Vector2 uvOffset{};
		RibbonUvMode uvMode = RibbonUvMode::Stretch;
		float tileLength = 1.0f;
		RibbonPlaybackMode playbackMode = RibbonPlaybackMode::Full;
		float playbackProgress = 1.0f;
		float sweepLength = 1.0f;
		MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::Effect;
	};

} // namespace MadoEngine::Ribbon
