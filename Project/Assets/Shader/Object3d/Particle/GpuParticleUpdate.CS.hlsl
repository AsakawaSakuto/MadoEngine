#include "GpuParticleCommon.hlsli"

/// @brief Alive Particleを更新し、生存Indexを次のAlive Listへ追加する
/// @param dispatchThreadId Dispatch全体のThread ID
[numthreads(kGpuParticleThreadGroupSize, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
	const uint aliveListIndex = dispatchThreadId.x;
	const uint maxParticles = gGpuParticleEmitterMetadata.x;
	const uint aliveCount = min(gGpuParticleCurrentCounter.Load(0), maxParticles);
	if (aliveListIndex >= aliveCount) {
		return;
	}

	const uint particleIndex = gGpuParticleAliveInput[aliveListIndex];
	if (particleIndex >= maxParticles) {
		return;
	}

	GpuParticleState state = gGpuParticleStates[particleIndex];
	state.age += gGpuParticleDeltaTime;
	if (state.age >= state.lifeTime) {
		PushGpuParticleFreeIndex(particleIndex);
		return;
	}

	const float3 totalAcceleration =
		gGpuParticleGravityDrag.xyz + gGpuParticleAcceleration.xyz;
	state.velocity += totalAcceleration * gGpuParticleDeltaTime;
	const float dragFactor =
		exp(-gGpuParticleGravityDrag.w * gGpuParticleDeltaTime);
	state.velocity *= dragFactor;
	state.position += state.velocity * gGpuParticleDeltaTime;
	state.rotation += state.angularVelocity * gGpuParticleDeltaTime;

	const float normalizedAge = saturate(
		state.age / max(state.lifeTime, kGpuParticleEpsilon)
	);
	state.scale = lerp(state.startScale, state.endScale, normalizedAge);
	state.color = lerp(state.startColor, state.endColor, normalizedAge);
	gGpuParticleStates[particleIndex] = state;

	if (!AppendGpuParticleAliveIndex(particleIndex)) {
		PushGpuParticleFreeIndex(particleIndex);
		return;
	}
	gGpuParticleDrawInstances[particleIndex] =
		BuildGpuParticleDrawInstance(state);
}
