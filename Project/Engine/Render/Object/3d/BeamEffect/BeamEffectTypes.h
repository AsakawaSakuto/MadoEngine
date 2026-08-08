#pragma once
#include ".SceneManager/SceneType.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Render/Object/3d/PrimitiveEffect/EffectTrack.h"
#include "Render/Object/3d/RibbonEffect/RibbonEffectTypes.h"
#include "Render/Object/RenderLayer.h"
#include "Render/PSO/PSODesc.h"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace MadoEngine::Beam {

	inline constexpr uint32_t kMinimumBeamSegmentCount = 1;
	inline constexpr uint32_t kMaximumBeamSegmentCount = 1024;
	inline constexpr std::size_t kMaximumBeamEmitterCount = 64;

	enum class BeamStopMode : uint32_t {
		Finish,
		Immediate,
	};

	struct BeamPlaybackModule {
		/// @brief 始点から終点まで伸びる既定Trackを構築する
		BeamPlaybackModule()
			: extensionOverTime(0.0f) {
			extensionOverTime.SetKeyframes({
				{ 0.0f, 0.0f, EaseType::Linear },
				{ 1.0f, 1.0f, EaseType::Linear },
			});
		}

		float duration = 1.0f;
		bool isLoop = false;
		MadoEngine::Effect::EffectTrack<float> extensionOverTime;
	};

	struct BeamGeometryModule {
		MadoEngine::Effect::EffectTrack<float> widthOverTime =
			MadoEngine::Effect::EffectTrack<float>{ 0.25f };
		uint32_t segmentCount = 16;
		bool cameraFacing = true;
		float startFade = 0.05f;
		float endFade = 0.05f;
	};

	struct BeamNoiseModule {
		float amplitude = 0.0f;
		float frequency = 3.0f;
		float scrollSpeed = 1.0f;
		uint32_t seed = 0;
	};

	struct BeamMaterialModule {
		std::string textureName = "white2x2";
		MadoEngine::Render::BlendMode blendMode = MadoEngine::Render::BlendMode::Add;
		MadoEngine::Render::CullMode cullMode = MadoEngine::Render::CullMode::None;
		MadoEngine::Effect::EffectTrack<Vector4> colorOverTime =
			MadoEngine::Effect::EffectTrack<Vector4>{ Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } };
		MadoEngine::Effect::EffectTrack<Vector4> colorOverLength =
			MadoEngine::Effect::EffectTrack<Vector4>{ Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } };
		MadoEngine::Effect::EffectTrack<float> globalAlphaOverTime =
			MadoEngine::Effect::EffectTrack<float>{ 1.0f };
		Vector2 uvScale = { 1.0f, 1.0f };
		Vector2 uvOffset{};
		Vector2 uvScroll{};
		MadoEngine::Ribbon::RibbonUvMode uvMode = MadoEngine::Ribbon::RibbonUvMode::Stretch;
		float tileLength = 1.0f;
	};

	struct BeamEmitterConfig {
		std::string name = "Emitter";
		bool isEnabled = true;
		BeamPlaybackModule playback;
		BeamGeometryModule geometry;
		BeamNoiseModule noise;
		BeamMaterialModule material;
	};

	using BeamEffectConfig = BeamEmitterConfig;

	struct BeamEffectHandle {
		uint32_t index = (std::numeric_limits<uint32_t>::max)();
		uint32_t generation = 0;

		/// @brief Handleが有効値を保持しているか確認する
		/// @return 有効値を保持している場合はtrue
		bool HasValue() const {
			return index != (std::numeric_limits<uint32_t>::max)() && generation != 0;
		}

		/// @brief 2つのHandleが同じInstanceを指すか比較する
		/// @param other 比較対象Handle
		/// @return 同じ値の場合はtrue
		bool operator==(const BeamEffectHandle& other) const = default;
	};

	struct BeamEffectPlayDesc {
		Vector3 startPosition{};
		Vector3 endPosition = { 0.0f, 1.0f, 0.0f };
		SceneType sceneType = SceneType::None;
		MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::Effect;
		std::optional<bool> loopOverride;
	};

} // namespace MadoEngine::Beam
