#pragma once
#include ".SceneManager/SceneType.h"
#include "EffectTrack.h"
#include "Math/Transform.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Render/Object/RenderLayer.h"
#include "Render/PSO/PSODesc.h"
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace MadoEngine::Effect {

	inline constexpr uint32_t kMaximumCylinderGradientStops = 8;

	enum class CylinderUvDirection : uint32_t {
		TopToBottom,
		BottomToTop,
		Clockwise,
		CounterClockwise,
	};

	enum class CylinderPivot : uint32_t {
		Bottom,
		Center,
		Top,
	};

	enum class PrimitiveEffectStopMode {
		Finish,
		Immediate,
	};

	struct CylinderColorStop {
		float position = 0.0f;
		EffectTrack<Vector4> color = EffectTrack<Vector4>{ Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } };
	};

	struct CylinderGeometryModule {
		uint32_t radialSegments = 32;
		uint32_t heightSegments = 1;
		CylinderPivot pivot = CylinderPivot::Bottom;
		EffectTrack<Vector2> bottomRadii = EffectTrack<Vector2>{ Vector2{ 1.0f, 1.0f } };
		EffectTrack<Vector2> topRadii = EffectTrack<Vector2>{ Vector2{ 1.0f, 1.0f } };
		EffectTrack<float> height = EffectTrack<float>{ 1.0f };
		EffectTrack<float> startAngleDegrees = EffectTrack<float>{ 0.0f };
		EffectTrack<float> arcAngleDegrees = EffectTrack<float>{ 360.0f };
	};

	struct CylinderUvModule {
		CylinderUvDirection direction = CylinderUvDirection::TopToBottom;
		EffectTrack<Vector2> scale = EffectTrack<Vector2>{ Vector2{ 1.0f, 1.0f } };
		EffectTrack<Vector2> offset = EffectTrack<Vector2>{ Vector2{} };
		EffectTrack<float> rotationDegrees = EffectTrack<float>{ 0.0f };
	};

	struct CylinderMaterialModule {
		std::string textureName = "white2x2";
		MadoEngine::Render::BlendMode blendMode = MadoEngine::Render::BlendMode::Add;
		MadoEngine::Render::CullMode cullMode = MadoEngine::Render::CullMode::None;
		CylinderUvModule uv;
		EffectTrack<float> globalAlpha = EffectTrack<float>{ 1.0f };
		EffectTrack<float> bottomFadeRange = EffectTrack<float>{ 0.0f };
		EffectTrack<float> topFadeRange = EffectTrack<float>{ 0.0f };
		std::vector<CylinderColorStop> gradient;
	};

	struct CylinderEffectConfig {
		float duration = 1.0f;
		bool isLoop = false;
		CylinderGeometryModule geometry;
		CylinderMaterialModule material;
	};

	struct PrimitiveEffectHandle {
		uint32_t index = (std::numeric_limits<uint32_t>::max)();
		uint32_t generation = 0;

		/// @brief ハンドルが有効値を保持しているか確認する
		/// @return 有効値を保持している場合はtrue
		bool HasValue() const {
			return index != (std::numeric_limits<uint32_t>::max)() && generation != 0;
		}

		/// @brief 2つのHandleが同じInstanceを指すか比較する
		/// @param other 比較対象Handle
		/// @return 同じ値の場合はtrue
		bool operator==(const PrimitiveEffectHandle& other) const = default;
	};

	struct PrimitiveEffectPlayDesc {
		Transform3D transform;
		SceneType sceneType = SceneType::None;
		MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::Effect;
		std::optional<bool> loopOverride;
	};

	struct CylinderGradientValue {
		float position = 0.0f;
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct CylinderRenderData {
		Transform3D transform;
		uint32_t radialSegments = 32;
		uint32_t heightSegments = 1;
		CylinderPivot pivot = CylinderPivot::Bottom;
		Vector2 bottomRadii = { 1.0f, 1.0f };
		Vector2 topRadii = { 1.0f, 1.0f };
		float height = 1.0f;
		float startAngleRadians = 0.0f;
		float arcAngleRadians = 0.0f;
		CylinderUvDirection uvDirection = CylinderUvDirection::TopToBottom;
		Vector2 uvScale = { 1.0f, 1.0f };
		Vector2 uvOffset{};
		float uvRotationRadians = 0.0f;
		float globalAlpha = 1.0f;
		float bottomFadeRange = 0.0f;
		float topFadeRange = 0.0f;
		std::array<CylinderGradientValue, kMaximumCylinderGradientStops> gradient{};
		uint32_t gradientCount = 0;
		std::string textureName = "white2x2";
		MadoEngine::Render::BlendMode blendMode = MadoEngine::Render::BlendMode::Add;
		MadoEngine::Render::CullMode cullMode = MadoEngine::Render::CullMode::None;
		MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::Effect;
	};

} // namespace MadoEngine::Effect
