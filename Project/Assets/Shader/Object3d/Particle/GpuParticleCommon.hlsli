#ifndef MADO_ENGINE_GPU_PARTICLE_COMMON_HLSLI
#define MADO_ENGINE_GPU_PARTICLE_COMMON_HLSLI

static const uint kGpuParticleShapePoint = 0;
static const uint kGpuParticleShapeLine = 1;
static const uint kGpuParticleShapeSphere = 2;
static const uint kGpuParticleShapeBox = 3;
static const uint kGpuParticleShapePlane = 4;
static const uint kGpuParticleShapeRing = 5;
static const uint kGpuParticleShapeCount = 6;

static const uint kGpuParticleSimulationSpaceLocal = 0;
static const uint kGpuParticleSimulationSpaceWorld = 1;
static const uint kGpuParticleSimulationSpaceCount = 2;
static const uint kGpuParticleDirectionConfigured = 0;
static const uint kGpuParticleDirectionShapeOutward = 1;
static const uint kGpuParticleDirectionModeCount = 2;
static const uint kGpuParticleFlagEmitFromSurfaceOrEdge = 1u << 0;
static const uint kGpuParticleThreadGroupSize = 64;
static const uint kGpuParticleEmitFreeBaseOffset = 20;
static const uint kGpuParticleEmitAliveBaseOffset = 24;
static const uint kGpuParticleEmitCountOffset = 28;
static const float kGpuParticleEpsilon = 0.000001f;
static const float kGpuParticleTwoPi = 6.28318530717958647692f;

struct GpuParticleState {
	float3 position;
	float rotation;
	float3 velocity;
	float angularVelocity;
	float2 scale;
	float2 startScale;
	float2 endScale;
	float age;
	float lifeTime;
	float4 color;
	float4 startColor;
	float4 endColor;
};

struct GpuParticleDrawInstance {
	float3 position;
	float rotation;
	float2 scale;
	float2 padding;
	float4 color;
};

#ifndef MADO_ENGINE_GPU_PARTICLE_GRAPHICS

cbuffer GpuParticleEmitterParameters : register(b0) {
	// x: 最大数、y: 形状、z: SimulationSpace、w: DirectionMode
	uint4 gGpuParticleEmitterMetadata;
	// x: 形状フラグ、y: Seed
	uint4 gGpuParticleEmitterFlagsAndSeed;
	float4 gGpuParticleShape0;
	float4 gGpuParticleShape1;
	float4 gGpuParticleDirectionMin;
	float4 gGpuParticleDirectionMax;
	// x: LifeTime最小、y: LifeTime最大、z: Speed最小、w: Speed最大
	float4 gGpuParticleLifeTimeSpeed;
	// x: Rotation最小、y: Rotation最大、z: AngularVelocity最小、w: AngularVelocity最大
	float4 gGpuParticleRotationRange;
	// xyz: Gravity、w: Drag
	float4 gGpuParticleGravityDrag;
	float4 gGpuParticleAcceleration;
	// xy: 最小、zw: 最大
	float4 gGpuParticleStartScaleMinMax;
	float4 gGpuParticleEndScaleMinMax;
	float4 gGpuParticleStartColorMin;
	float4 gGpuParticleStartColorMax;
	float4 gGpuParticleEndColorMin;
	float4 gGpuParticleEndColorMax;
};

cbuffer GpuParticlePerFrameParameters : register(b1) {
	row_major float4x4 gGpuParticleEmitterMatrix;
	row_major float4x4 gGpuParticleEmitterRotationMatrix;
	// xy: Emitter Scale、z: Emitter Z Rotation
	float4 gGpuParticleEmitterScaleRotation;
	float gGpuParticleDeltaTime;
	uint gGpuParticleEmitCount;
	uint gGpuParticleEmitSequenceBase;
	uint gGpuParticlePerFramePadding;
};

RWStructuredBuffer<GpuParticleState> gGpuParticleStates : register(u0);
RWStructuredBuffer<GpuParticleDrawInstance> gGpuParticleDrawInstances : register(u1);
StructuredBuffer<uint> gGpuParticleAliveInput : register(t0);
RWStructuredBuffer<uint> gGpuParticleAliveOutput : register(u2);
RWByteAddressBuffer gGpuParticleCurrentCounter : register(u3);
RWByteAddressBuffer gGpuParticleNextCounter : register(u4);
RWStructuredBuffer<uint> gGpuParticleFreeIndices : register(u5);
RWByteAddressBuffer gGpuParticleFreeCounter : register(u6);
RWByteAddressBuffer gGpuParticleIndirectArguments : register(u7);

/// @brief ゼロに近いVectorを既定方向へ置き換えて正規化する
/// @param value 正規化するVector
/// @param fallback ゼロに近い場合に使用する既定方向
/// @return 正規化したVector
float3 NormalizeGpuParticleDirection(float3 value, float3 fallback) {
	const float lengthSquared = dot(value, value);
	if (lengthSquared <= kGpuParticleEpsilon) {
		return fallback;
	}
	return value * rsqrt(lengthSquared);
}

/// @brief GPU Particle Stateから描画用Instanceを生成する
/// @param state 描画するParticle State
/// @return Emitter Transformを反映した描画用Instance
GpuParticleDrawInstance BuildGpuParticleDrawInstance(GpuParticleState state) {
	GpuParticleDrawInstance instance;
	instance.position = state.position;
	instance.rotation = state.rotation;
	instance.scale = state.scale;
	instance.padding = float2(0.0f, 0.0f);
	instance.color = state.color;

	if (gGpuParticleEmitterMetadata.z == kGpuParticleSimulationSpaceLocal) {
		instance.position = mul(
			float4(state.position, 1.0f),
			gGpuParticleEmitterMatrix
		).xyz;
		instance.rotation += gGpuParticleEmitterScaleRotation.z;
		instance.scale *= abs(gGpuParticleEmitterScaleRotation.xy);
	}
	return instance;
}

/// @brief Alive Outputへ安全にParticle Indexを追加する
/// @param particleIndex 追加するParticle Index
/// @return 追加できた場合はtrue
bool AppendGpuParticleAliveIndex(uint particleIndex) {
	const uint maxParticles = gGpuParticleEmitterMetadata.x;
	[loop]
	while (true) {
		const uint currentCount = gGpuParticleNextCounter.Load(0);
		if (currentCount >= maxParticles) {
			return false;
		}

		uint originalCount = 0;
		gGpuParticleNextCounter.InterlockedCompareExchange(
			0,
			currentCount,
			currentCount + 1,
			originalCount
		);
		if (originalCount == currentCount) {
			gGpuParticleAliveOutput[currentCount] = particleIndex;
			return true;
		}
	}
}

/// @brief Free Listへ安全にParticle Indexを追加する
/// @param particleIndex 解放するParticle Index
/// @return 追加できた場合はtrue
bool PushGpuParticleFreeIndex(uint particleIndex) {
	const uint maxParticles = gGpuParticleEmitterMetadata.x;
	[loop]
	while (true) {
		const uint currentCount = gGpuParticleFreeCounter.Load(0);
		if (currentCount >= maxParticles) {
			return false;
		}

		uint originalCount = 0;
		gGpuParticleFreeCounter.InterlockedCompareExchange(
			0,
			currentCount,
			currentCount + 1,
			originalCount
		);
		if (originalCount == currentCount) {
			gGpuParticleFreeIndices[currentCount] = particleIndex;
			return true;
		}
	}
}

/// @brief Free Listから安全にParticle Indexを取得する
/// @param particleIndex 取得したParticle Index
/// @return 取得できた場合はtrue
bool PopGpuParticleFreeIndex(out uint particleIndex) {
	particleIndex = 0;
	[loop]
	while (true) {
		const uint currentCount = gGpuParticleFreeCounter.Load(0);
		if (currentCount == 0) {
			return false;
		}

		uint originalCount = 0;
		gGpuParticleFreeCounter.InterlockedCompareExchange(
			0,
			currentCount,
			currentCount - 1,
			originalCount
		);
		if (originalCount == currentCount) {
			particleIndex = gGpuParticleFreeIndices[currentCount - 1];
			return particleIndex < gGpuParticleEmitterMetadata.x;
		}
	}
}

/// @brief 32bit値を疑似乱数用にHash化する
/// @param value Hash化する値
/// @return Hash化した値
uint HashGpuParticleValue(uint value) {
	value ^= value >> 16;
	value *= 0x7feb352du;
	value ^= value >> 15;
	value *= 0x846ca68bu;
	value ^= value >> 16;
	return value;
}

/// @brief GPU Particle用疑似乱数列の次の値を生成する
/// @param state 疑似乱数列の内部状態
/// @return 生成した32bit疑似乱数
uint NextGpuParticleRandom(inout uint state) {
	state = state * 747796405u + 2891336453u;
	const uint word =
		((state >> ((state >> 28) + 4)) ^ state) * 277803737u;
	return (word >> 22) ^ word;
}

/// @brief 0以上1未満のGPU Particle用疑似乱数を生成する
/// @param state 疑似乱数列の内部状態
/// @return 0以上1未満の疑似乱数
float NextGpuParticleRandom01(inout uint state) {
	return (float)(NextGpuParticleRandom(state) >> 8) *
		(1.0f / 16777216.0f);
}

/// @brief float範囲からGPU Particle用疑似乱数を生成する
/// @param minimum 範囲の最小値
/// @param maximum 範囲の最大値
/// @param state 疑似乱数列の内部状態
/// @return 範囲内の疑似乱数
float SampleGpuParticleRange(
	float minimum,
	float maximum,
	inout uint state) {
	return lerp(minimum, maximum, NextGpuParticleRandom01(state));
}

/// @brief float2範囲からGPU Particle用疑似乱数を生成する
/// @param minimum 範囲の最小値
/// @param maximum 範囲の最大値
/// @param state 疑似乱数列の内部状態
/// @return 範囲内の疑似乱数
float2 SampleGpuParticleRange(
	float2 minimum,
	float2 maximum,
	inout uint state) {
	return float2(
		SampleGpuParticleRange(minimum.x, maximum.x, state),
		SampleGpuParticleRange(minimum.y, maximum.y, state)
	);
}

/// @brief float3範囲からGPU Particle用疑似乱数を生成する
/// @param minimum 範囲の最小値
/// @param maximum 範囲の最大値
/// @param state 疑似乱数列の内部状態
/// @return 範囲内の疑似乱数
float3 SampleGpuParticleRange(
	float3 minimum,
	float3 maximum,
	inout uint state) {
	return float3(
		SampleGpuParticleRange(minimum.x, maximum.x, state),
		SampleGpuParticleRange(minimum.y, maximum.y, state),
		SampleGpuParticleRange(minimum.z, maximum.z, state)
	);
}

/// @brief float4範囲からGPU Particle用疑似乱数を生成する
/// @param minimum 範囲の最小値
/// @param maximum 範囲の最大値
/// @param state 疑似乱数列の内部状態
/// @return 範囲内の疑似乱数
float4 SampleGpuParticleRange(
	float4 minimum,
	float4 maximum,
	inout uint state) {
	return float4(
		SampleGpuParticleRange(minimum.x, maximum.x, state),
		SampleGpuParticleRange(minimum.y, maximum.y, state),
		SampleGpuParticleRange(minimum.z, maximum.z, state),
		SampleGpuParticleRange(minimum.w, maximum.w, state)
	);
}

#endif
#endif
