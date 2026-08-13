#pragma once
#include "ParticleTypes.h"
#include "Math/Matrix4x4.h"
#include <cstdint>
#include <d3d12.h>

namespace MadoEngine::Particle {

	enum class GpuParticleShapeType : uint32_t {
		Point,
		Line,
		Sphere,
		Box,
		Plane,
		Ring,
		Count,
	};

	struct alignas(16) GpuParticleState {
		Vector3 position{};
		float rotation = 0.0f;
		Vector3 velocity{};
		float angularVelocity = 0.0f;
		Vector2 scale = { 1.0f, 1.0f };
		Vector2 startScale = { 1.0f, 1.0f };
		Vector2 endScale = { 1.0f, 1.0f };
		float age = 0.0f;
		float lifeTime = 1.0f;
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 startColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 endColor = { 1.0f, 1.0f, 1.0f, 0.0f };
		uint32_t identifier = 0;
		uint32_t padding[3]{};
	};
	static_assert(sizeof(GpuParticleState) == 128, "GPU Particle StateのLayoutがHLSLと一致していません。");

	struct alignas(16) GpuParticleTrailSample {
		Vector3 position{};
		uint32_t identifier = 0;
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};
	static_assert(sizeof(GpuParticleTrailSample) == 32, "GPU Particle Trail SampleのLayoutがHLSLと一致していません。");

	struct alignas(16) GpuParticleDrawInstance {
		Vector3 position{};
		float rotation = 0.0f;
		Vector2 scale = { 1.0f, 1.0f };
		Vector2 padding{};
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};
	static_assert(sizeof(GpuParticleDrawInstance) == 48, "GPU Particle Draw InstanceのLayoutがHLSLと一致していません。");

	struct alignas(16) GpuParticleEmitterParameters {
		uint32_t maxParticles = 1;
		uint32_t shapeType = 0;
		uint32_t simulationSpace = 0;
		uint32_t directionMode = 0;
		uint32_t shapeFlags = 0;
		uint32_t randomSeed = 0;
		uint32_t padding0 = 0;
		uint32_t padding1 = 0;
		Vector4 shapeData0{};
		Vector4 shapeData1{};
		Vector4 directionMin{};
		Vector4 directionMax{};
		Vector4 lifeTimeSpeedRange{};
		Vector4 rotationRange{};
		Vector4 gravityDrag{};
		Vector4 acceleration{};
		Vector4 startScaleMinMax{};
		Vector4 endScaleMinMax{};
		Vector4 startColorMin{};
		Vector4 startColorMax{};
		Vector4 endColorMin{};
		Vector4 endColorMax{};
	};
	static_assert(sizeof(GpuParticleEmitterParameters) == 256, "GPU Particle Emitter ParameterのLayoutがHLSLと一致していません。");

	struct alignas(16) GpuParticlePerFrameParameters {
		Matrix4x4 emitterMatrix{};
		Matrix4x4 emitterRotationMatrix{};
		Vector4 emitterScaleRotation{};
		float deltaTime = 0.0f;
		uint32_t emitCount = 0;
		uint32_t emitSequenceBase = 0;
		uint32_t padding = 0;
	};
	static_assert(sizeof(GpuParticlePerFrameParameters) == 160, "GPU Particle PerFrame ParameterのLayoutがHLSLと一致していません。");

	struct alignas(16) GpuParticleDrawArguments {
		uint32_t indexCountPerInstance = 6;
		uint32_t instanceCount = 0;
		uint32_t startIndexLocation = 0;
		int32_t baseVertexLocation = 0;
		uint32_t startInstanceLocation = 0;
		uint32_t padding[3]{};
	};
	static_assert(sizeof(GpuParticleDrawArguments) == 32, "GPU Particle Draw ArgumentのLayoutが不正です。");

	struct GpuParticleRenderData {
		D3D12_GPU_VIRTUAL_ADDRESS drawInstanceBufferAddress = 0;
		D3D12_GPU_VIRTUAL_ADDRESS aliveIndexBufferAddress = 0;
		ID3D12Resource* indirectArgumentBuffer = nullptr;
		uint32_t maxParticleCount = 0;
		uint64_t bufferCapacityBytes = 0;
	};

} // MadoEngine::Particle名前空間
