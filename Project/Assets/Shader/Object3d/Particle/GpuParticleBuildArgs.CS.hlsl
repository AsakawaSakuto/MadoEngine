#include "GpuParticleCommon.hlsli"

/// @brief Alive数からDrawIndexed用Indirect Argumentsを生成する
/// @param dispatchThreadId Dispatch全体のThread ID
[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
	if (dispatchThreadId.x != 0) {
		return;
	}

	const uint aliveCount = min(
		gGpuParticleNextCounter.Load(0),
		gGpuParticleEmitterMetadata.x
	);
	gGpuParticleIndirectArguments.Store(0, 6);
	gGpuParticleIndirectArguments.Store(4, aliveCount);
	gGpuParticleIndirectArguments.Store(8, 0);
	gGpuParticleIndirectArguments.Store(12, 0);
	gGpuParticleIndirectArguments.Store(16, 0);
}
