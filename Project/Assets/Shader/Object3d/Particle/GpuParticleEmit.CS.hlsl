#include "GpuParticleCommon.hlsli"

struct GpuParticleShapeSample {
	float3 position;
	float3 outward;
};

/// @brief 単位球面上の一様な方向を生成
/// @param randomState 疑似乱数列の内部状態
/// @return 単位球面上の方向
float3 SampleGpuParticleUnitSphere(inout uint randomState) {

	// zと方位角から面積密度が偏らない単位球面方向を生成
	const float z = SampleGpuParticleRange(-1.0f, 1.0f, randomState);
	const float angle = SampleGpuParticleRange(
		0.0f,
		kGpuParticleTwoPi,
		randomState
	);
	const float radialLength = sqrt(max(0.0f, 1.0f - z * z));
	float sine = 0.0f;
	float cosine = 1.0f;
	sincos(angle, sine, cosine);
	return float3(
		radialLength * cosine,
		z,
		radialLength * sine
	);
}

/// @brief PlaneとRingで使用する直交基底を生成
/// @param normal 基底の法線
/// @param tangent 生成した接線
/// @param bitangent 生成した従法線
void BuildGpuParticlePlaneBasis(
	float3 normal,
	out float3 tangent,
	out float3 bitangent) {
	const float3 normalizedNormal = NormalizeGpuParticleDirection(
		normal,
		float3(0.0f, 1.0f, 0.0f)
	);
	const float3 reference = abs(normalizedNormal.y) < 0.999f
		? float3(0.0f, 1.0f, 0.0f)
		: float3(1.0f, 0.0f, 0.0f);
	tangent = NormalizeGpuParticleDirection(
		cross(reference, normalizedNormal),
		float3(1.0f, 0.0f, 0.0f)
	);
	bitangent = NormalizeGpuParticleDirection(
		cross(normalizedNormal, tangent),
		float3(0.0f, 0.0f, 1.0f)
	);
}

/// @brief Emitter形状から発生位置と外向き方向を生成
/// @param randomState 疑似乱数列の内部状態
/// @return 生成した形状Sample
GpuParticleShapeSample SampleGpuParticleShape(inout uint randomState) {
	GpuParticleShapeSample sample;
	sample.position = float3(0.0f, 0.0f, 0.0f);
	sample.outward = float3(0.0f, 0.0f, 0.0f);

	const uint shapeType = gGpuParticleEmitterMetadata.y;
	const bool emitFromSurfaceOrEdge =
		(gGpuParticleEmitterFlagsAndSeed.x &
			kGpuParticleFlagEmitFromSurfaceOrEdge) != 0;
	if (shapeType == kGpuParticleShapeLine) {
		const float interpolation = NextGpuParticleRandom01(randomState);
		sample.position = lerp(
			gGpuParticleShape0.xyz,
			gGpuParticleShape1.xyz,
			interpolation
		);
	} else if (shapeType == kGpuParticleShapeSphere) {
		const float3 direction = SampleGpuParticleUnitSphere(randomState);

		// 体積内発生では半径に立方根を適用して中心への密度偏りを補正
		const float radiusScale = emitFromSurfaceOrEdge
			? 1.0f
			: pow(NextGpuParticleRandom01(randomState), 1.0f / 3.0f);
		sample.position =
			direction * gGpuParticleShape0.x * radiusScale;
		sample.outward = direction;
	} else if (shapeType == kGpuParticleShapeBox) {
		const float3 halfExtents = gGpuParticleShape0.xyz;
		sample.position = SampleGpuParticleRange(
			-halfExtents,
			halfExtents,
			randomState
		);
		if (emitFromSurfaceOrEdge) {

			// 面発生では一軸を境界へ固定して必ずBox表面上へ配置
			const uint axis = min(
				(uint)(NextGpuParticleRandom01(randomState) * 3.0f),
				2u
			);
			const float sign =
				NextGpuParticleRandom01(randomState) < 0.5f ? -1.0f : 1.0f;
			sample.position[axis] = halfExtents[axis] * sign;
			sample.outward[axis] = sign;
		} else {
			sample.outward = NormalizeGpuParticleDirection(
				sample.position,
				float3(0.0f, 0.0f, 0.0f)
			);
		}
	} else if (shapeType == kGpuParticleShapePlane) {
		float3 tangent;
		float3 bitangent;
		BuildGpuParticlePlaneBasis(
			gGpuParticleShape1.xyz,
			tangent,
			bitangent
		);
		const float x = SampleGpuParticleRange(
			-gGpuParticleShape0.x,
			gGpuParticleShape0.x,
			randomState
		);
		const float y = SampleGpuParticleRange(
			-gGpuParticleShape0.y,
			gGpuParticleShape0.y,
			randomState
		);
		sample.position = tangent * x + bitangent * y;
		sample.outward = NormalizeGpuParticleDirection(
			gGpuParticleShape1.xyz,
			float3(0.0f, 1.0f, 0.0f)
		);
	} else if (shapeType == kGpuParticleShapeRing) {
		float3 tangent;
		float3 bitangent;
		BuildGpuParticlePlaneBasis(
			gGpuParticleShape1.xyz,
			tangent,
			bitangent
		);
		const float angle = SampleGpuParticleRange(
			0.0f,
			kGpuParticleTwoPi,
			randomState
		);
		float sine = 0.0f;
		float cosine = 1.0f;
		sincos(angle, sine, cosine);

		// 面積一様な環状分布にするため二乗半径を補間して平方根へ復元
		const float radius = emitFromSurfaceOrEdge
			? gGpuParticleShape0.y
			: sqrt(SampleGpuParticleRange(
				gGpuParticleShape0.x * gGpuParticleShape0.x,
				gGpuParticleShape0.y * gGpuParticleShape0.y,
				randomState
			));
		const float3 radial = tangent * cosine + bitangent * sine;
		sample.position = radial * radius;
		sample.outward = radial;
	} else {
		sample.position = gGpuParticleShape0.xyz;
	}
	return sample;
}

/// @brief 新規Particle Stateを生成
/// @param randomState 疑似乱数列の内部状態
/// @param identifier Particle固有ID
/// @return 初期値を設定したParticle State
GpuParticleState CreateGpuParticleState(inout uint randomState, uint identifier) {
	const GpuParticleShapeSample shapeSample =
		SampleGpuParticleShape(randomState);
	const float3 configuredDirection = NormalizeGpuParticleDirection(
		SampleGpuParticleRange(
			gGpuParticleDirectionMin.xyz,
			gGpuParticleDirectionMax.xyz,
			randomState
		),
		float3(0.0f, 1.0f, 0.0f)
	);
	const bool useShapeOutward =
		gGpuParticleEmitterMetadata.w ==
			kGpuParticleDirectionShapeOutward &&
		dot(shapeSample.outward, shapeSample.outward) >
			kGpuParticleEpsilon;
	const float3 direction = useShapeOutward
		? NormalizeGpuParticleDirection(
			shapeSample.outward,
			configuredDirection
		)
		: configuredDirection;

	GpuParticleState state;
	state.position = shapeSample.position;
	state.rotation = SampleGpuParticleRange(
		gGpuParticleRotationRange.x,
		gGpuParticleRotationRange.y,
		randomState
	);
	state.velocity = direction * SampleGpuParticleRange(
		gGpuParticleLifeTimeSpeed.z,
		gGpuParticleLifeTimeSpeed.w,
		randomState
	);
	state.angularVelocity = SampleGpuParticleRange(
		gGpuParticleRotationRange.z,
		gGpuParticleRotationRange.w,
		randomState
	);
	state.startScale = SampleGpuParticleRange(
		gGpuParticleStartScaleMinMax.xy,
		gGpuParticleStartScaleMinMax.zw,
		randomState
	);
	state.endScale = SampleGpuParticleRange(
		gGpuParticleEndScaleMinMax.xy,
		gGpuParticleEndScaleMinMax.zw,
		randomState
	);
	state.scale = state.startScale;
	state.age = 0.0f;
	state.lifeTime = max(
		SampleGpuParticleRange(
			gGpuParticleLifeTimeSpeed.x,
			gGpuParticleLifeTimeSpeed.y,
			randomState
		),
		kGpuParticleEpsilon
	);
	state.startColor = SampleGpuParticleRange(
		gGpuParticleStartColorMin,
		gGpuParticleStartColorMax,
		randomState
	);
	state.endColor = SampleGpuParticleRange(
		gGpuParticleEndColorMin,
		gGpuParticleEndColorMax,
		randomState
	);
	state.color = state.startColor;
	state.identifier = identifier;
	state.padding = uint3(0, 0, 0);

	if (gGpuParticleEmitterMetadata.z ==
		kGpuParticleSimulationSpaceWorld) {

		// World Simulationは生成時だけEmitter Transformを焼き付けて以後の移動から分離
		state.position = mul(
			float4(state.position, 1.0f),
			gGpuParticleEmitterMatrix
		).xyz;
		state.velocity = mul(
			float4(state.velocity, 0.0f),
			gGpuParticleEmitterRotationMatrix
		).xyz;
		state.rotation += gGpuParticleEmitterScaleRotation.z;
		const float2 absoluteScale =
			abs(gGpuParticleEmitterScaleRotation.xy);
		state.startScale *= absoluteScale;
		state.endScale *= absoluteScale;
		state.scale = state.startScale;
	}
	return state;
}

/// @brief 発生要求に従ってFree Slotへ新規Particleを生成
/// @param dispatchThreadId Dispatch全体のThread ID
[numthreads(kGpuParticleThreadGroupSize, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
	const uint emitIndex = dispatchThreadId.x;
	const uint reservedEmitCount =
		gGpuParticleIndirectArguments.Load(kGpuParticleEmitCountOffset);

	// PrepareEmit Passで確保済みの範囲だけを生成対象としてBuffer競合を回避
	if (emitIndex >= reservedEmitCount) {
		return;
	}

	const uint maxParticles = gGpuParticleEmitterMetadata.x;
	const uint freeBase =
		gGpuParticleIndirectArguments.Load(kGpuParticleEmitFreeBaseOffset);
	const uint aliveBase =
		gGpuParticleIndirectArguments.Load(kGpuParticleEmitAliveBaseOffset);
	const uint freeListIndex = freeBase + emitIndex;
	const uint aliveListIndex = aliveBase + emitIndex;
	if (freeListIndex >= maxParticles || aliveListIndex >= maxParticles) {
		return;
	}
	const uint particleIndex = gGpuParticleFreeIndices[freeListIndex];
	if (particleIndex >= maxParticles) {
		return;
	}

	const uint sequence = gGpuParticleEmitSequenceBase + emitIndex;

	// Emitter Seedと通算Sequenceを混ぜてFrame分割に依存しない乱数列を生成
	uint randomState = HashGpuParticleValue(
		gGpuParticleEmitterFlagsAndSeed.y ^
		HashGpuParticleValue(sequence)
	);
	const GpuParticleState state =
		CreateGpuParticleState(randomState, sequence);
	gGpuParticleStates[particleIndex] = state;
	gGpuParticleAliveOutput[aliveListIndex] = particleIndex;
	gGpuParticleDrawInstances[particleIndex] =
		BuildGpuParticleDrawInstance(state);
	gGpuParticleTrailSamples[particleIndex] =
		BuildGpuParticleTrailSample(state);
}
