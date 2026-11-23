#include "Rock.h"
#include <string>

Rock::Rock(int modelIndex) : modelIndex_(modelIndex) {
}

bool Rock::Initialize(const InitializeDesc& desc) {

	// 未用意のModel名を生成しないよう末尾番号の有効範囲を確認
	if (modelIndex_ < 0 || modelIndex_ >= kModelCount) {
		return false;
	}

	// 同じ末尾番号でModelとBatchを対応させ、異なるMeshの混在を防止
	const std::string suffix = std::to_string(modelIndex_);
	return InitializeInstancedDraw(desc, kBatchNamePrefix + suffix, "Rock" + suffix);
}

void Rock::DestroyInstancedBatches() {

	// 生成時と同じ末尾番号の範囲を使用し、全ModelのBatchを一括破棄
	for (int modelIndex = 0; modelIndex < kModelCount; ++modelIndex) {
		DestroyInstancedDrawBatches(kBatchNamePrefix + std::to_string(modelIndex));
	}
}
