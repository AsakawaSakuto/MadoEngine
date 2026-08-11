#include "GpuParticleCommon.hlsli"

/// @brief Particle BufferとFree Listを初期化
/// @param dispatchThreadId Dispatch全体のThread ID
[numthreads(kGpuParticleThreadGroupSize, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
	const uint particleIndex = dispatchThreadId.x;
	const uint maxParticles = gGpuParticleEmitterMetadata.x;

	if (particleIndex == 0) {

		// Global CounterとIndirect Argumentsは競合を避けるため先頭Threadだけで初期化
		gGpuParticleCurrentCounter.Store(0, 0);
		gGpuParticleNextCounter.Store(0, 0);
		gGpuParticleFreeCounter.Store(0, maxParticles);
		gGpuParticleIndirectArguments.Store(0, 6);
		gGpuParticleIndirectArguments.Store(4, 0);
		gGpuParticleIndirectArguments.Store(8, 0);
		gGpuParticleIndirectArguments.Store(12, 0);
		gGpuParticleIndirectArguments.Store(16, 0);
	}

	if (particleIndex >= maxParticles) {
		return;
	}

	// 全Slotを未使用状態に揃えてFree Listへ一対一で登録
	GpuParticleState state = (GpuParticleState)0;
	state.lifeTime = 1.0f;
	gGpuParticleStates[particleIndex] = state;

	GpuParticleDrawInstance drawInstance = (GpuParticleDrawInstance)0;
	gGpuParticleDrawInstances[particleIndex] = drawInstance;
	gGpuParticleFreeIndices[particleIndex] = particleIndex;
}
