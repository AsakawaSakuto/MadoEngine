#include "MapDecorationObjectBase.h"
#include "Render/Object/3d/Model/MyInstancedModel.h"
#include "Utility/Collider/CollisionFunction.h"
#include "Render/Object/3d/Line/MyDebugLine.h"

namespace {
constexpr float kCameraOcclusionAlpha = 0.3f;
constexpr const char* kOccludedBatchSuffix = ".Occluded";
}

MapDecorationObjectBase::~MapDecorationObjectBase() {
	SetVisible(false);
}

bool MapDecorationObjectBase::InitializeInstancedDraw(const InitializeDesc& desc,
	const std::string& batchName, const std::string& modelName) {

	// 再初期化時の旧Instanceを隠し、共有Batchの寿命をMap側へ委譲
	SetVisible(false);
	instancedModel_ = {};
	occludedInstancedModel_ = {};
	instanceHandle_ = UINT32_MAX;
	occludedInstanceHandle_ = UINT32_MAX;
	const MadoEngine::InstancedModelHandle batchHandle = MyInstancedModel::GetOrCreate(
		batchName, modelName, SceneType::Game,
		MadoEngine::Render::RenderLayer::MapDecorationObject);
	InstancedModel* batch = MyInstancedModel::TryGet(batchHandle);
	if (!batch) {
		return false;
	}

	// EventObjectと同じ固定オフセットを使い、描画の回転やScaleによる範囲変更を回避
	MapDecorationObjectSettings validatedSettings = desc.settings;
	validatedSettings.Clamp();
	cameraOcclusionBounds_ = validatedSettings.CreateCollider(desc.transform.translate);

	// 不透明Instanceの深度書き込みを維持するため半透明用のBatchを分離
	const MadoEngine::InstancedModelHandle occludedBatchHandle = MyInstancedModel::GetOrCreate(
		batchName + kOccludedBatchSuffix, modelName, SceneType::Game,
		MadoEngine::Render::RenderLayer::MapDecorationObject);
	InstancedModel* occludedBatch = MyInstancedModel::TryGet(occludedBatchHandle);
	if (!occludedBatch) {
		return false;
	}
	occludedBatch->SetColor({ 1.0f, 1.0f, 1.0f, kCameraOcclusionAlpha });
	occludedBatch->SetCastShadow(false);

	// 同じ配置と色のInstanceを両Batchへ登録し、遮蔽状態に応じて片方だけ表示
	InstancedModel::InstanceDesc instance;
	instance.transform = desc.transform;
	instance.color = desc.color;
	instance.isVisible = false;
	instancedModel_ = batchHandle;
	instanceHandle_ = batch->AddInstance(instance);
	occludedInstancedModel_ = occludedBatchHandle;
	occludedInstanceHandle_ = occludedBatch->AddInstance(instance);
	isVisible_ = desc.isVisible;
	isCameraOccluded_ = false;
	ApplyVisibility();
	return true;
}

void MapDecorationObjectBase::SetVisible(bool isVisible) {
	isVisible_ = isVisible;
	ApplyVisibility();
}

void MapDecorationObjectBase::UpdateCameraOcclusion(const Segment& sightLine) {
	const bool isOccluded = Collision::IsHit(sightLine, cameraOcclusionBounds_);

	// 遮蔽状態が変化したInstanceだけを同期して毎Frameの描画設定更新を抑制
	if (isCameraOccluded_ == isOccluded) {
		return;
	}
	isCameraOccluded_ = isOccluded;
	ApplyVisibility();
}

void MapDecorationObjectBase::DrawDebugLine() const {
	MyDebugLine::AddShape(cameraOcclusionBounds_, { 0.2f, 0.7f, 1.0f, 1.0f });
}

void MapDecorationObjectBase::DestroyInstancedDrawBatches(const std::string& batchName) {

	// 再生成時に半透明側のInstanceも破棄し、非表示Instanceの蓄積を防止
	MyInstancedModel::Destroy(batchName);
	MyInstancedModel::Destroy(batchName + kOccludedBatchSuffix);
}

void MapDecorationObjectBase::ApplyVisibility() {
	const bool isVisible = isVisible_ && !isCameraOccluded_;
	const bool isOccludedVisible = isVisible_ && isCameraOccluded_;

	// Scene破棄などでBatchが先に消えた場合も世代付きHandleで安全に参照
	if (InstancedModel* batch = MyInstancedModel::TryGet(instancedModel_)) {
		batch->SetInstanceVisible(instanceHandle_, isVisible);
	}

	// F1や破棄時の非表示を半透明側にも反映し、両Batchの同時表示を抑止
	if (InstancedModel* batch = MyInstancedModel::TryGet(occludedInstancedModel_)) {
		batch->SetInstanceVisible(occludedInstanceHandle_, isOccludedVisible);
	}
}
