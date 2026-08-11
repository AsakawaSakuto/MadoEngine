#include "GpuParticleCommon.hlsli"

/// @brief Alive数からDrawIndexed用Indirect Argumentsを生成
/// @param dispatchThreadId Dispatch全体のThread ID
[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {

	// Indirect Arguments全体を一度だけ更新するため先頭Thread以外を除外
	if (dispatchThreadId.x != 0) {
		return;
	}

	const uint aliveCount = min(
		gGpuParticleNextCounter.Load(0),
		gGpuParticleEmitterMetadata.x
	);

	// D3D12_DRAW_INDEXED_ARGUMENTSのField順へQuadの描画条件を書き込み
	gGpuParticleIndirectArguments.Store(0, 6);
	gGpuParticleIndirectArguments.Store(4, aliveCount);
	gGpuParticleIndirectArguments.Store(8, 0);
	gGpuParticleIndirectArguments.Store(12, 0);
	gGpuParticleIndirectArguments.Store(16, 0);
}
