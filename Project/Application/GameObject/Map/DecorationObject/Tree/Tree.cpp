#include "Tree.h"

bool Tree::Initialize(const InitializeDesc& desc) {
	return InitializeInstancedDraw(desc, kBatchName, "Tree");
}

void Tree::DestroyInstancedBatches() {
	DestroyInstancedDrawBatches(kBatchName);
}
