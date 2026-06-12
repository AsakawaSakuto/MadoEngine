#include "Render/Screen/RenderTargetManager.h"
#include "Core/DxDevice/DxDevice.h"
#include "Core/View/RTVManager.h"
#include "Core/View/SRVManager.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include <stdexcept>

namespace MadoEngine::Render {

	void RenderTargetManager::Initialize(
		MadoEngine::Core::DxDevice* device,
		MadoEngine::Core::RTVManager* rtvManager,
		MadoEngine::Core::SRVManager* srvManager
	) {
		assert(device != nullptr && "DxDevice が nullptr です");
		assert(rtvManager != nullptr && "RTVManager が nullptr です");
		assert(srvManager != nullptr && "SRVManager が nullptr です");

		device_ = device;
		rtvManager_ = rtvManager;
		srvManager_ = srvManager;
		renderTargets_.clear();

		Logger::Output("RenderTargetManagerを初期化しました。", Logger::Level::Engine);
	}

	RenderTexture* RenderTargetManager::Create(const std::string& name, const Desc& desc) {
		assert(device_ != nullptr && "RenderTargetManager が未初期化です");
		assert(!name.empty() && "RenderTarget名が空です");
		assert(desc.width > 0 && "RenderTargetの幅が0です");
		assert(desc.height > 0 && "RenderTargetの高さが0です");

		if (auto it = renderTargets_.find(name); it != renderTargets_.end()) {
			Logger::Output("既に存在するRenderTargetを返します: " + name, Logger::Level::Warning);
			return it->second.texture.get();
		}

		Entry entry{};
		entry.desc = desc;
		entry.texture = std::make_unique<RenderTexture>();
		entry.texture->Initialize(device_, rtvManager_, srvManager_, desc.width, desc.height, desc.format);

		RenderTexture* created = entry.texture.get();
		renderTargets_.emplace(name, std::move(entry));

		Logger::Output(
			"RenderTargetを作成しました: " + name + " (" +
			std::to_string(desc.width) + "x" + std::to_string(desc.height) + ")",
			Logger::Level::Engine
		);
		return created;
	}

	RenderTexture* RenderTargetManager::Create(
		const std::string& name,
		uint32_t width,
		uint32_t height,
		DXGI_FORMAT format,
		bool resizeWithWindow
	) {
		Desc desc{};
		desc.width = width;
		desc.height = height;
		desc.format = format;
		desc.resizeWithWindow = resizeWithWindow;
		return Create(name, desc);
	}

	bool RenderTargetManager::Contains(const std::string& name) const {
		return renderTargets_.contains(name);
	}

	RenderTexture* RenderTargetManager::Get(const std::string& name) {
		return GetEntry(name).texture.get();
	}

	const RenderTexture* RenderTargetManager::Get(const std::string& name) const {
		return GetEntry(name).texture.get();
	}

	void RenderTargetManager::Begin(
		const std::string& name,
		ID3D12GraphicsCommandList* commandList,
		const D3D12_CPU_DESCRIPTOR_HANDLE* depthStencilHandle
	) {
		Entry& entry = GetEntry(name);
		entry.texture->BeginRender(commandList, depthStencilHandle, entry.desc.clearColor.data());
	}

	void RenderTargetManager::Begin(
		const std::string& name,
		ID3D12GraphicsCommandList* commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle
	) {
		Begin(name, commandList, &depthStencilHandle);
	}

	void RenderTargetManager::End(const std::string& name, ID3D12GraphicsCommandList* commandList) {
		GetEntry(name).texture->EndRender(commandList);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE RenderTargetManager::GetSRVGPUHandle(const std::string& name) const {
		return GetEntry(name).texture->GetSRVGPUHandle();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetManager::GetSRVCPUHandle(const std::string& name) const {
		return GetEntry(name).texture->GetSRVCPUHandle();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetManager::GetRTVCPUHandle(const std::string& name) const {
		return GetEntry(name).texture->GetRTVCPUHandle();
	}

	void RenderTargetManager::Resize(const std::string& name, uint32_t width, uint32_t height) {
		assert(width > 0 && "RenderTargetの幅が0です");
		assert(height > 0 && "RenderTargetの高さが0です");

		Entry& entry = GetEntry(name);
		entry.desc.width = width;
		entry.desc.height = height;
		entry.texture->Resize(width, height);
	}

	void RenderTargetManager::ResizeAll(uint32_t width, uint32_t height) {
		assert(width > 0 && "RenderTargetの幅が0です");
		assert(height > 0 && "RenderTargetの高さが0です");

		for (auto& [name, entry] : renderTargets_) {
			if (!entry.desc.resizeWithWindow) {
				continue;
			}

			entry.desc.width = width;
			entry.desc.height = height;
			entry.texture->Resize(width, height);
		}

		Logger::Output(
			"RenderTargetManagerのリサイズを反映しました: " +
			std::to_string(width) + "x" + std::to_string(height),
			Logger::Level::Engine
		);
	}

	void RenderTargetManager::Clear() {
		const size_t count = renderTargets_.size();
		renderTargets_.clear();
		Logger::Output("RenderTargetをすべて解放しました。数: " + std::to_string(count), Logger::Level::Engine);
	}

	RenderTargetManager::Entry& RenderTargetManager::GetEntry(const std::string& name) {
		auto it = renderTargets_.find(name);
		if (it == renderTargets_.end()) {
			Logger::Output("RenderTargetが見つかりません: " + name, Logger::Level::Error);
			assert(false && "指定したRenderTargetが見つかりません");
			throw std::runtime_error("指定したRenderTargetが見つかりません: " + name);
		}
		return it->second;
	}

	const RenderTargetManager::Entry& RenderTargetManager::GetEntry(const std::string& name) const {
		auto it = renderTargets_.find(name);
		if (it == renderTargets_.end()) {
			Logger::Output("RenderTargetが見つかりません: " + name, Logger::Level::Error);
			assert(false && "指定したRenderTargetが見つかりません");
			throw std::runtime_error("指定したRenderTargetが見つかりません: " + name);
		}
		return it->second;
	}

} // namespace MadoEngine::Render
