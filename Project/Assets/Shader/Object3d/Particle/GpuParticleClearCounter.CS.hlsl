#include "GpuParticleCommon.hlsli"

/// @brief 次フレーム用Alive Counterをゼロへ戻す
/// @param dispatchThreadId Dispatch全体のThread ID
[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
	if (dispatchThreadId.x == 0) {
		gGpuParticleNextCounter.Store(0, 0);
	}
}
