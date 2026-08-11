#include "GpuParticleCommon.hlsli"

/// @brief Emit Passが使用するFree ListとAlive Listの範囲を予約
/// @param dispatchThreadId Dispatch全体のThread ID
[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {

	// Counter間の予約を一括処理してEmit Thread同士のAtomic競合を回避
	if (dispatchThreadId.x != 0) {
		return;
	}

	const uint maxParticles = gGpuParticleEmitterMetadata.x;
	const uint aliveCount = min(gGpuParticleNextCounter.Load(0), maxParticles);
	const uint freeCount = min(gGpuParticleFreeCounter.Load(0), maxParticles);
	const uint availableAliveCount = maxParticles - aliveCount;
	const uint reservedEmitCount = min(
		gGpuParticleEmitCount,
		min(freeCount, availableAliveCount)
	);
	const uint freeBase = freeCount - reservedEmitCount;

	// Free末尾とAlive末尾に重ならない連続領域を確保して後続Passへ受け渡し
	gGpuParticleFreeCounter.Store(0, freeBase);
	gGpuParticleNextCounter.Store(0, aliveCount + reservedEmitCount);
	gGpuParticleIndirectArguments.Store(
		kGpuParticleEmitFreeBaseOffset,
		freeBase
	);
	gGpuParticleIndirectArguments.Store(
		kGpuParticleEmitAliveBaseOffset,
		aliveCount
	);
	gGpuParticleIndirectArguments.Store(
		kGpuParticleEmitCountOffset,
		reservedEmitCount
	);
}
