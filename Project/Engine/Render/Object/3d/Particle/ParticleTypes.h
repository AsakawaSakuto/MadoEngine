#pragma once
#include ".SceneManager/SceneType.h"
#include "Math/Transform.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Render/Object/RenderLayer.h"
#include "Render/PSO/PSODesc.h"
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace MadoEngine::Particle {

	template<class T>
	struct ValueRange {
		T min{};
		T max{};
	};

	enum class SimulationSpace {
		Local,
		World,
	};

	enum class DirectionMode {
		Configured,
		ShapeOutward,
	};

	enum class SortMode {
		None,
		BackToFront,
	};

	enum class StopMode {
		Finish,
		Immediate,
	};

	struct PointShape {
		Vector3 offset{};
	};

	struct LineShape {
		Vector3 start{};
		Vector3 end = { 0.0f, 1.0f, 0.0f };
	};

	struct SphereShape {
		float radius = 1.0f;
		bool emitFromSurface = false;
	};

	struct BoxShape {
		Vector3 halfExtents = { 0.5f, 0.5f, 0.5f };
		bool emitFromSurface = false;
	};

	struct PlaneShape {
		Vector2 halfExtents = { 0.5f, 0.5f };
		Vector3 normal = { 0.0f, 1.0f, 0.0f };
	};

	struct RingShape {
		float innerRadius = 0.0f;
		float outerRadius = 1.0f;
		Vector3 normal = { 0.0f, 1.0f, 0.0f };
		bool emitFromEdge = false;
	};

	using ParticleShape = std::variant<
		PointShape,
		LineShape,
		SphereShape,
		BoxShape,
		PlaneShape,
		RingShape
	>;

	struct BurstConfig {
		float time = 0.0f;
		uint32_t count = 1;
	};

	struct EmissionModule {
		uint32_t maxParticles = 256;
		float ratePerSecond = 10.0f;
		float duration = 1.0f;
		float startDelay = 0.0f;
		bool isLoop = false;
		std::vector<BurstConfig> bursts;
	};

	struct InitialParticleModule {
		ValueRange<float> lifeTime = { 1.0f, 1.0f };
		ValueRange<float> speed = { 1.0f, 1.0f };
		ValueRange<Vector3> direction = {
			{ 0.0f, 1.0f, 0.0f },
			{ 0.0f, 1.0f, 0.0f },
		};
		ValueRange<float> rotation = { 0.0f, 0.0f };
		ValueRange<float> angularVelocity = { 0.0f, 0.0f };
		DirectionMode directionMode = DirectionMode::Configured;
	};

	struct MotionModule {
		Vector3 gravity{};
		Vector3 acceleration{};
		float drag = 0.0f;
	};

	struct SizeOverLifetimeModule {
		ValueRange<Vector2> start = {
			{ 1.0f, 1.0f },
			{ 1.0f, 1.0f },
		};
		ValueRange<Vector2> end = {
			{ 1.0f, 1.0f },
			{ 1.0f, 1.0f },
		};
	};

	struct ColorOverLifetimeModule {
		ValueRange<Vector4> start = {
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			{ 1.0f, 1.0f, 1.0f, 1.0f },
		};
		ValueRange<Vector4> end = {
			{ 1.0f, 1.0f, 1.0f, 0.0f },
			{ 1.0f, 1.0f, 1.0f, 0.0f },
		};
	};

	struct ParticleRendererModule {
		std::string textureName = "white2x2";
		MadoEngine::Render::BlendMode blendMode = MadoEngine::Render::BlendMode::Add;
		SortMode sortMode = SortMode::None;
	};

	struct EmitterConfig {
		std::string name = "Emitter";
		EmissionModule emission;
		ParticleShape shape = PointShape{};
		SimulationSpace simulationSpace = SimulationSpace::World;
		InitialParticleModule initial;
		MotionModule motion;
		SizeOverLifetimeModule sizeOverLifetime;
		ColorOverLifetimeModule colorOverLifetime;
		ParticleRendererModule renderer;
	};

	struct ParticleState {
		Vector3 position{};
		Vector3 velocity{};
		Vector2 scale = { 1.0f, 1.0f };
		Vector2 startScale = { 1.0f, 1.0f };
		Vector2 endScale = { 1.0f, 1.0f };
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 startColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 endColor = { 1.0f, 1.0f, 1.0f, 0.0f };
		float rotation = 0.0f;
		float angularVelocity = 0.0f;
		float age = 0.0f;
		float lifeTime = 1.0f;
	};

	struct EffectHandle {
		uint32_t index = (std::numeric_limits<uint32_t>::max)();
		uint32_t generation = 0;

		/// @brief ハンドルが値を保持しているか確認する
		/// @return 値を保持している場合はtrue
		bool HasValue() const {
			return index != (std::numeric_limits<uint32_t>::max)() && generation != 0;
		}

		bool operator==(const EffectHandle&) const = default;
	};

	struct PlayDesc {
		Transform3D transform;
		SceneType sceneType = SceneType::None;
		MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::Effect;
		std::optional<bool> loopOverride;
		uint32_t randomSeed = 0;
	};

} // namespace MadoEngine::Particle
