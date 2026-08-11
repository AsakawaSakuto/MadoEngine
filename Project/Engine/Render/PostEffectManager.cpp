#include "Render/PostEffectManager.h"
#include "Render/PostEffect/PostEffectDefinitionRegistry.h"
#include "Utility/Handle/GenerationalHandle.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cassert>
#include <limits>

namespace MadoEngine::Render {

PostEffectManager& PostEffectManager::GetInstance() {
	static PostEffectManager instance;
	return instance;
}

void PostEffectManager::Initialize(const PSODesc& basePostEffectDesc, ID3D12Device* device) {
	assert(device && "PostEffectManagerのD3D12Deviceが空です");
	if (isInitialized_) {
		Logger::Output("PostEffectManagerは既に初期化されています", Logger::Level::Warning);
		return;
	}

	basePostEffectDesc_ = basePostEffectDesc;
	device_ = device;
	isInitialized_ = true;
}

void PostEffectManager::Finalize() {
	if (!isInitialized_) {
		return;
	}

	pendingDestroyHandles_.clear();
	parameterWarningKeys_.clear();
	keyToHandle_.clear();
	layerPassOrder_.clear();
	screenPassOrder_.clear();
	freeSlots_.clear();
	freeSlots_.reserve(slots_.size());

	// 外部に残ったHandleを無効化するため全Slotの世代を更新
	for (uint32_t index = 0; index < slots_.size(); ++index) {
		PostEffectPassSlot& slot = slots_[index];
		slot.pass.reset();
		slot.key.clear();
		slot.scope = PostEffectPassScope::Layer;
		slot.active = false;
		slot.generation = NextGeneration(slot.generation);
		freeSlots_.push_back(index);
	}

	basePostEffectDesc_ = {};
	device_ = nullptr;
	isInitialized_ = false;
	Logger::Output("PostEffectManagerを終了しました", Logger::Level::Engine);
}

PostEffectPassHandle PostEffectManager::CreateLayerPass(const LayerPostEffectPassCreateDesc& desc) {
	PostEffectPass::Desc passDesc{};
	passDesc.key = desc.key;
	passDesc.name = desc.name;
	passDesc.targetLayerMask = desc.targetLayerMask;
	passDesc.layerEffectStage = desc.layerEffectStage;
	passDesc.effectShaderKey = desc.effectShaderKey;
	passDesc.enabled = desc.enabled;
	passDesc.ignoreDepthForMask = desc.ignoreDepthForMask;
	return CreatePass(passDesc, PostEffectPassScope::Layer);
}

PostEffectPassHandle PostEffectManager::CreateScreenPass(const ScreenPostEffectPassCreateDesc& desc) {
	PostEffectPass::Desc passDesc{};
	passDesc.key = desc.key;
	passDesc.name = desc.name;
	passDesc.targetLayerMask = kAllRenderLayers;
	passDesc.effectShaderKey = desc.effectShaderKey;
	passDesc.enabled = desc.enabled;
	passDesc.ignoreDepthForMask = false;
	passDesc.screenEffectStage = desc.screenEffectStage;
	return CreatePass(passDesc, PostEffectPassScope::Screen);
}

PostEffectPassHandle PostEffectManager::Find(const std::string& key) const {
	const auto it = keyToHandle_.find(key);
	if (it == keyToHandle_.end() || !IsValid(it->second)) {
		return {};
	}

	return it->second;
}

PostEffectPass* PostEffectManager::TryGet(PostEffectPassHandle handle) {
	return const_cast<PostEffectPass*>(std::as_const(*this).TryGet(handle));
}

const PostEffectPass* PostEffectManager::TryGet(PostEffectPassHandle handle) const {
	if (!handle.IsValid() || handle.index >= slots_.size()) {
		return nullptr;
	}

	const PostEffectPassSlot& slot = slots_[handle.index];
	if (!slot.active || slot.generation != handle.generation || !slot.pass) {
		return nullptr;
	}

	return slot.pass.get();
}

bool PostEffectManager::IsValid(PostEffectPassHandle handle) const {
	return TryGet(handle) != nullptr;
}

PostEffectPassScope PostEffectManager::GetScope(PostEffectPassHandle handle) const {
	if (!IsValid(handle)) {
		return PostEffectPassScope::Count;
	}

	return slots_[handle.index].scope;
}

bool PostEffectManager::Destroy(PostEffectPassHandle handle) {
	PostEffectPass* pass = TryGet(handle);
	if (!pass) {
		return false;
	}

	PostEffectPassSlot& slot = slots_[handle.index];
	const std::string key = slot.key;
	const std::string name = pass->GetName();

	// 遅延削除と警告履歴から対象Handleを外して再利用後のSlotへ状態を持ち越さない構成
	pendingDestroyHandles_.erase(
		std::remove(pendingDestroyHandles_.begin(), pendingDestroyHandles_.end(), handle),
		pendingDestroyHandles_.end()
	);
	parameterWarningKeys_.erase(
		std::remove_if(
			parameterWarningKeys_.begin(),
			parameterWarningKeys_.end(),
			[handle](const ParameterWarningKey& warning) { return warning.handle == handle; }
		),
		parameterWarningKeys_.end()
	);
	layerPassOrder_.erase(std::remove(layerPassOrder_.begin(), layerPassOrder_.end(), handle), layerPassOrder_.end());
	screenPassOrder_.erase(std::remove(screenPassOrder_.begin(), screenPassOrder_.end(), handle), screenPassOrder_.end());
	const auto keyIt = keyToHandle_.find(key);
	if (keyIt != keyToHandle_.end() && keyIt->second == handle) {
		keyToHandle_.erase(keyIt);
	}

	const PostEffectPassScope scope = slot.scope;

	// 世代更新後に空きSlotへ戻して破棄済みHandleからの参照を拒否
	slot.pass.reset();
	slot.key.clear();
	slot.scope = PostEffectPassScope::Layer;
	slot.active = false;
	slot.generation = NextGeneration(slot.generation);
	freeSlots_.push_back(handle.index);

	Logger::Output(
		(scope == PostEffectPassScope::Layer ?
			"レイヤーポストエフェクトPassを削除しました: " :
			"フルスクリーンポストエフェクトPassを削除しました: ") + name,
		Logger::Level::Engine
	);
	return true;
}

void PostEffectManager::RequestDestroy(PostEffectPassHandle handle) {
	if (!IsValid(handle)) {
		return;
	}

	if (std::find(pendingDestroyHandles_.begin(), pendingDestroyHandles_.end(), handle) == pendingDestroyHandles_.end()) {
		pendingDestroyHandles_.push_back(handle);
	}
}

void PostEffectManager::FlushPendingDestroys() {
	if (pendingDestroyHandles_.empty()) {
		return;
	}

	std::vector<PostEffectPassHandle> handles = std::move(pendingDestroyHandles_);
	pendingDestroyHandles_.clear();
	for (PostEffectPassHandle handle : handles) {
		Destroy(handle);
	}
}

void PostEffectManager::ClearLayerPasses() {
	const std::vector<PostEffectPassHandle> handles = layerPassOrder_;
	for (PostEffectPassHandle handle : handles) {
		Destroy(handle);
	}
}

void PostEffectManager::ClearScreenPasses() {
	const std::vector<PostEffectPassHandle> handles = screenPassOrder_;
	for (PostEffectPassHandle handle : handles) {
		Destroy(handle);
	}
}

const std::vector<PostEffectPassHandle>& PostEffectManager::GetLayerPassHandles() const {
	return layerPassOrder_;
}

const std::vector<PostEffectPassHandle>& PostEffectManager::GetScreenPassHandles() const {
	return screenPassOrder_;
}

bool PostEffectManager::MoveLayerPass(PostEffectPassHandle handle, std::size_t newIndex) {
	if (GetScope(handle) != PostEffectPassScope::Layer) {
		return false;
	}

	return MovePass(layerPassOrder_, handle, newIndex);
}

bool PostEffectManager::MoveScreenPass(PostEffectPassHandle handle, std::size_t newIndex) {
	if (GetScope(handle) != PostEffectPassScope::Screen) {
		return false;
	}

	return MovePass(screenPassOrder_, handle, newIndex);
}

bool PostEffectManager::SetEffectType(PostEffectPassHandle handle, PostEffectType effectType) {
	PostEffectPass* pass = TryGet(handle);
	const PostEffectDefinition* definition = PostEffectDefinitionRegistry::Find(effectType);
	if (!pass || !definition) {
		Logger::Output("ポストエフェクト種別を変更できませんでした", Logger::Level::Warning);
		return false;
	}

	parameterWarningKeys_.erase(
		std::remove_if(
			parameterWarningKeys_.begin(),
			parameterWarningKeys_.end(),
			[handle](const ParameterWarningKey& warning) { return warning.handle == handle; }
		),
		parameterWarningKeys_.end()
	);
	pass->ApplyEffectDefinition(*definition);
	return true;
}

bool PostEffectManager::SetEnabled(PostEffectPassHandle handle, bool enabled) {
	PostEffectPass* pass = TryGet(handle);
	if (!pass) {
		return false;
	}

	pass->SetEnabled(enabled);
	return true;
}

bool PostEffectManager::TryGetEnabled(PostEffectPassHandle handle, bool& outEnabled) const {
	const PostEffectPass* pass = TryGet(handle);
	if (!pass) {
		return false;
	}

	outEnabled = pass->IsEnabled();
	return true;
}

bool PostEffectManager::SetFloatParameter(
	PostEffectPassHandle handle,
	const std::string& parameterKey,
	float value)
{
	PostEffectPass* pass = TryGet(handle);
	if (!pass) {
		return false;
	}

	return pass->SetFloatParameter(parameterKey, value);
}

bool PostEffectManager::TryGetFloatParameter(
	PostEffectPassHandle handle,
	const std::string& parameterKey,
	float& outValue) const
{
	const PostEffectPass* pass = TryGet(handle);
	if (!pass) {
		return false;
	}

	return pass->TryGetFloatParameter(parameterKey, outValue);
}

bool PostEffectManager::SetEnabled(const std::string& key, bool enabled) {
	return SetEnabled(Find(key), enabled);
}

bool PostEffectManager::TryGetEnabled(const std::string& key, bool& outEnabled) const {
	return TryGetEnabled(Find(key), outEnabled);
}

bool PostEffectManager::SetFloatParameter(
	const std::string& passKey,
	const std::string& parameterKey,
	float value)
{
	return SetFloatParameter(Find(passKey), parameterKey, value);
}

bool PostEffectManager::TryGetFloatParameter(
	const std::string& passKey,
	const std::string& parameterKey,
	float& outValue) const
{
	return TryGetFloatParameter(Find(passKey), parameterKey, outValue);
}

RenderLayerMask PostEffectManager::GetEnabledLayerTargetMask() const {
	RenderLayerMask layerMask = 0;
	for (PostEffectPassHandle handle : layerPassOrder_) {
		const PostEffectPass* pass = TryGet(handle);
		if (pass && pass->IsEnabled()) {
			layerMask |= pass->GetTargetLayerMask();
		}
	}

	return layerMask;
}

RenderLayerMask PostEffectManager::GetEnabledLayerTargetMask(LayerEffectStage stage) const {
	assert(IsValidLayerEffectStage(stage) && "LayerEffectStageが範囲外です");

	RenderLayerMask layerMask = 0;
	for (PostEffectPassHandle handle : layerPassOrder_) {
		const PostEffectPass* pass = TryGet(handle);
		if (pass && pass->IsEnabled() && pass->GetLayerEffectStage() == stage) {
			layerMask |= pass->GetTargetLayerMask();
		}
	}

	return layerMask;
}

bool PostEffectManager::NeedsIgnoreDepthMask(RenderLayerMask layerMask) const {
	for (PostEffectPassHandle handle : layerPassOrder_) {
		const PostEffectPass* pass = TryGet(handle);
		if (!pass || !pass->IsEnabled()) {
			continue;
		}

		if (pass->GetTargetLayerMask() == layerMask && pass->IsIgnoreDepthForMask()) {
			return true;
		}
	}

	return false;
}

bool PostEffectManager::NeedsIgnoreDepthMask(RenderLayerMask layerMask, LayerEffectStage stage) const {
	assert(IsValidLayerEffectStage(stage) && "LayerEffectStageが範囲外です");

	for (PostEffectPassHandle handle : layerPassOrder_) {
		const PostEffectPass* pass = TryGet(handle);
		if (!pass || !pass->IsEnabled() || pass->GetLayerEffectStage() != stage) {
			continue;
		}

		if (pass->GetTargetLayerMask() == layerMask && pass->IsIgnoreDepthForMask()) {
			return true;
		}
	}

	return false;
}

PostEffectPassHandle PostEffectManager::CreatePass(const PostEffectPass::Desc& desc, PostEffectPassScope scope) {
	assert(isInitialized_ && device_ && "PostEffectManagerが初期化されていません");
	if (desc.key.empty()) {
		Logger::Output("ポストエフェクトPassの内部キーが空です", Logger::Level::Warning);
		return {};
	}
	if (desc.name.empty()) {
		Logger::Output("ポストエフェクトPassの表示名が空です", Logger::Level::Warning);
		return {};
	}
	if (scope != PostEffectPassScope::Layer && scope != PostEffectPassScope::Screen) {
		Logger::Output("ポストエフェクトPassの適用先が不正です", Logger::Level::Warning);
		return {};
	}
	if (keyToHandle_.contains(desc.key)) {
		Logger::Output("同じ内部キーのポストエフェクトPassが既に存在します: " + desc.key, Logger::Level::Warning);
		return {};
	}
	if (!PostEffectDefinitionRegistry::FindByShaderKey(desc.effectShaderKey)) {
		Logger::Output("未登録のポストエフェクトShaderキーです: " + desc.effectShaderKey, Logger::Level::Warning);
		return {};
	}

	// 破棄済みSlotを優先的に再利用してHandle Indexの増加を抑制
	uint32_t index = 0;
	if (freeSlots_.empty()) {
		if (slots_.size() >= static_cast<std::size_t>(kInvalidGenerationalHandleIndex)) {
			Logger::Output("ポストエフェクトPassのSlot上限へ到達しました", Logger::Level::Error);
			return {};
		}
		index = static_cast<uint32_t>(slots_.size());
		slots_.emplace_back();
	} else {
		index = freeSlots_.back();
		freeSlots_.pop_back();
	}

	PostEffectPassSlot& slot = slots_[index];
	if (slot.generation == 0) {
		slot.generation = 1;
	}
	slot.pass = std::make_unique<PostEffectPass>();
	slot.pass->Initialize(desc, basePostEffectDesc_, device_);
	slot.key = desc.key;
	slot.scope = scope;
	slot.active = true;

	const PostEffectPassHandle handle{ index, slot.generation };
	keyToHandle_.emplace(slot.key, handle);

	// Layer用とScreen用で描画順を独立管理
	if (scope == PostEffectPassScope::Layer) {
		layerPassOrder_.push_back(handle);
	} else {
		screenPassOrder_.push_back(handle);
	}
	return handle;
}

bool PostEffectManager::MovePass(
	std::vector<PostEffectPassHandle>& order,
	PostEffectPassHandle handle,
	std::size_t newIndex)
{
	if (newIndex >= order.size()) {
		return false;
	}

	const auto currentIt = std::find(order.begin(), order.end(), handle);
	if (currentIt == order.end()) {
		return false;
	}

	const PostEffectPassHandle movingHandle = *currentIt;
	order.erase(currentIt);
	order.insert(order.begin() + static_cast<std::ptrdiff_t>(newIndex), movingHandle);
	return true;
}

bool PostEffectManager::SetTypedParameterData(
	PostEffectPassHandle handle,
	PostEffectType expectedType,
	const void* data,
	std::size_t sizeInBytes)
{
	PostEffectPass* pass = TryGet(handle);
	if (!pass) {
		return false;
	}

	// 型とBuffer Sizeの双方を検証して誤った構造体のGPU転送を防止
	const std::optional<PostEffectType> effectType = pass->GetPostEffectType();
	if (!effectType || *effectType != expectedType) {
		LogParameterWarningOnce(
			handle,
			expectedType,
			ParameterWarningReason::TypeMismatch,
			"型付きパラメータとPassのEffect種別が一致しません: " + pass->GetKey()
		);
		return false;
	}
	if (pass->GetParameterDataSize() != sizeInBytes) {
		LogParameterWarningOnce(
			handle,
			expectedType,
			ParameterWarningReason::SizeMismatch,
			"型付きパラメータとConstantBufferのサイズが一致しません: " + pass->GetKey()
		);
		return false;
	}

	pass->SetParameterData(data, sizeInBytes);
	return true;
}

bool PostEffectManager::TryGetTypedParameterData(
	PostEffectPassHandle handle,
	PostEffectType expectedType,
	void* outData,
	std::size_t sizeInBytes) const
{
	const PostEffectPass* pass = TryGet(handle);
	if (!pass) {
		return false;
	}
	const std::optional<PostEffectType> effectType = pass->GetPostEffectType();
	if (!effectType || *effectType != expectedType) {
		LogParameterWarningOnce(
			handle,
			expectedType,
			ParameterWarningReason::TypeMismatch,
			"型付きパラメータとPassのEffect種別が一致しません: " + pass->GetKey()
		);
		return false;
	}
	if (pass->GetParameterDataSize() != sizeInBytes) {
		LogParameterWarningOnce(
			handle,
			expectedType,
			ParameterWarningReason::SizeMismatch,
			"型付きパラメータとConstantBufferのサイズが一致しません: " + pass->GetKey()
		);
		return false;
	}

	return pass->TryCopyParameterData(outData, sizeInBytes);
}

void PostEffectManager::LogParameterWarningOnce(
	PostEffectPassHandle handle,
	PostEffectType expectedType,
	ParameterWarningReason reason,
	const std::string& message) const
{
	const ParameterWarningKey warning{ handle, expectedType, reason };

	// 毎フレーム呼ばれるParameter設定失敗を同一条件につき一度だけ通知
	if (std::find(parameterWarningKeys_.begin(), parameterWarningKeys_.end(), warning) != parameterWarningKeys_.end()) {
		return;
	}

	parameterWarningKeys_.push_back(warning);
	Logger::Output(message, Logger::Level::Warning);
}

} // namespace MadoEngine::Render
