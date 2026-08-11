#include "GpuParticleCommon.hlsli"

/// @brief Alive Particleを更新し、生存Indexを次のAlive Listへ追加
/// @param dispatchThreadId Dispatch全体のThread ID
[numthreads(kGpuParticleThreadGroupSize, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
	const uint aliveListIndex = dispatchThreadId.x;
	const uint maxParticles = gGpuParticleEmitterMetadata.x;
	const uint aliveCount = min(gGpuParticleCurrentCounter.Load(0), maxParticles);

	// DispatchをThread Group単位へ切り上げた余剰ThreadをBuffer範囲外へ出さない制限
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

		// 寿命終了SlotをFree Listへ戻して次回Emitで再利用
		PushGpuParticleFreeIndex(particleIndex);
		return;
	}

	const float3 totalAcceleration =
		gGpuParticleGravityDrag.xyz + gGpuParticleAcceleration.xyz;
	state.velocity += totalAcceleration * gGpuParticleDeltaTime;
	const float dragFactor =
		exp(-gGpuParticleGravityDrag.w * gGpuParticleDeltaTime);

	// Frame Rateに依存しない指数減衰でDragを適用
	state.velocity *= dragFactor;
	state.position += state.velocity * gGpuParticleDeltaTime;
	state.rotation += state.angularVelocity * gGpuParticleDeltaTime;

	const float normalizedAge = saturate(
		state.age / max(state.lifeTime, kGpuParticleEpsilon)
	);

	// 正規化寿命を共通係数にしてScaleとColorの終端到達を同期
	state.scale = lerp(state.startScale, state.endScale, normalizedAge);
	state.color = lerp(state.startColor, state.endColor, normalizedAge);
	gGpuParticleStates[particleIndex] = state;

	if (!AppendGpuParticleAliveIndex(particleIndex)) {

		// Alive Listが満杯の場合は孤立Slotを残さずFree Listへ回収
		PushGpuParticleFreeIndex(particleIndex);
		return;
	}
	gGpuParticleDrawInstances[particleIndex] =
		BuildGpuParticleDrawInstance(state);
}
