#include "GpuParticleCommon.hlsli"

/// @brief 次フレーム用Alive Counterをゼロへ初期化
/// @param dispatchThreadId Dispatch全体のThread ID
[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
	if (dispatchThreadId.x == 0) {

		// UpdateとEmitが共有する次Frame用Alive Listの書き込み位置を初期化
		gGpuParticleNextCounter.Store(0, 0);
	}
}
