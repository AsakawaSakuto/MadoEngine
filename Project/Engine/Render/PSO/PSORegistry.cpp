#include "Render/PSO/PSORegistry.h"
#include "Core/DxDevice/DxDevice.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>
#include <windows.h>

namespace MadoEngine::Render {

	/// @brief ワイド文字列をUTF-8文字列に変換する
	/// @param wstr 変換元のワイド文字列
	/// @return UTF-8文字列
	static std::string WStringToString(const std::wstring& wstr) {
		if (wstr.empty()) {
			return {};
		}

		const int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::string result(static_cast<size_t>(size - 1), '\0');
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);
		return result;
	}

	/// @brief PSO生成失敗時の詳細をログ出力する
	/// @param hr 失敗したHRESULT
	/// @param desc 生成に使用したPSODesc
	static void LogPipelineStateError(HRESULT hr, const PSODesc& desc) {
		std::ostringstream message;
		message
			<< "[Engine] GraphicsPipelineStateの生成に失敗しました。HRESULT=0x"
			<< std::hex << static_cast<uint32_t>(hr)
			<< std::dec
			<< " VS=" << desc.vsKey
			<< " PS=" << desc.psKey
			<< " RootSig=" << desc.rootSigKey
			<< " RTVCount=" << desc.renderTargetCount
			<< " RTVFormat=" << static_cast<int>(desc.rtvFormat)
			<< " DSVFormat=" << static_cast<int>(desc.dsvFormat);
		Logger::Output(message.str(), Logger::Level::Error);
	}

	void PSORegistry::Initialize(Core::DxDevice* device, PSOFactory* factory) {
		assert(device);
		assert(factory);
		device_ = device;
		factory_ = factory;
		Logger::Output("[Engine] PSORegistryを初期化しました。", Logger::Level::Engine);
	}

	void PSORegistry::Finalize() {
		while (!IsPrewarmComplete()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		prewarmTasks_.clear();

		if (isDirty_ && !cachePath_.empty()) {
			SavePipelineLibrary(cachePath_);
		}

		Logger::Output("[Engine] PSORegistryを終了しました。", Logger::Level::Engine);
	}

	ID3D12PipelineState* PSORegistry::Get(const PSODesc& desc) {
		std::lock_guard<std::mutex> lock(cacheMutex_);

		auto it = psoCache_.find(desc);
		if (it != psoCache_.end()) {
			return it->second.Get();
		}

		return CreateAndCache(desc);
	}

	void PSORegistry::Prewarm(const std::vector<PSODesc>& descs) {
		Logger::Output("[Engine] PSOのプリウォームを開始します。件数: " + std::to_string(descs.size()), Logger::Level::Engine);

		for (const auto& desc : descs) {
			std::lock_guard<std::mutex> lock(cacheMutex_);
			if (!psoCache_.contains(desc)) {
				CreateAndCache(desc);
			}
		}

		Logger::Output("[Engine] PSOのプリウォームが完了しました。", Logger::Level::Engine);
	}

	void PSORegistry::PrewarmAsync(const std::vector<PSODesc>& descs) {
		Logger::Output("[Engine] PSOの非同期プリウォームを開始します。件数: " + std::to_string(descs.size()), Logger::Level::Engine);

		for (const auto& desc : descs) {
			auto task = std::async(std::launch::async, [this, desc]() {
				std::lock_guard<std::mutex> lock(cacheMutex_);
				if (!psoCache_.contains(desc)) {
					CreateAndCache(desc);
				}
			});
			prewarmTasks_.push_back(std::move(task));
		}
	}

	bool PSORegistry::IsPrewarmComplete() const {
		for (const auto& task : prewarmTasks_) {
			if (!task.valid()) {
				continue;
			}
			if (task.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
				return false;
			}
		}
		return true;
	}

	void PSORegistry::LoadPipelineLibrary(const std::wstring& cachePath) {
		cachePath_ = cachePath;

		std::ifstream file(cachePath, std::ios::binary | std::ios::ate);
		if (!file.is_open()) {
			Logger::Output("[Engine] PipelineLibraryキャッシュが見つからないため新規作成します。", Logger::Level::Assets);
			Microsoft::WRL::ComPtr<ID3D12Device1> device1;
			device_->GetDevice()->QueryInterface(IID_PPV_ARGS(&device1));
			if (device1) {
				const HRESULT hr = device1->CreatePipelineLibrary(nullptr, 0, IID_PPV_ARGS(&pipelineLibrary_));
				if (FAILED(hr)) {
					Logger::Output("[Engine] PipelineLibraryの新規作成に失敗しました。", Logger::Level::Error);
				}
			}
			return;
		}

		const std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		std::vector<char> buffer(static_cast<size_t>(size));
		file.read(buffer.data(), size);
		file.close();

		Microsoft::WRL::ComPtr<ID3D12Device1> device1;
		device_->GetDevice()->QueryInterface(IID_PPV_ARGS(&device1));
		if (!device1) {
			return;
		}

		HRESULT hr = device1->CreatePipelineLibrary(buffer.data(), static_cast<SIZE_T>(size), IID_PPV_ARGS(&pipelineLibrary_));
		if (FAILED(hr)) {
			Logger::Output("[Engine] PipelineLibraryキャッシュが無効なため再作成します。", Logger::Level::Assets);
			device1->CreatePipelineLibrary(nullptr, 0, IID_PPV_ARGS(&pipelineLibrary_));
			return;
		}

		Logger::Output("[Engine] PipelineLibraryをロードしました: " + WStringToString(cachePath), Logger::Level::Assets);
	}

	void PSORegistry::SavePipelineLibrary(const std::wstring& cachePath) {
		if (!pipelineLibrary_) {
			Logger::Output("[Engine] PipelineLibraryが存在しないため保存をスキップします。", Logger::Level::Assets);
			return;
		}

		const SIZE_T serializedSize = pipelineLibrary_->GetSerializedSize();
		std::vector<char> buffer(serializedSize);
		const HRESULT hr = pipelineLibrary_->Serialize(buffer.data(), serializedSize);
		if (FAILED(hr)) {
			Logger::Output("[Engine] PipelineLibraryのシリアライズに失敗しました。", Logger::Level::Error);
			return;
		}

		std::ofstream outFile(cachePath, std::ios::binary);
		if (!outFile.is_open()) {
			Logger::Output("[Engine] PipelineLibraryキャッシュファイルを開けませんでした。", Logger::Level::Error);
			return;
		}

		outFile.write(buffer.data(), static_cast<std::streamsize>(serializedSize));
		outFile.close();
		Logger::Output("[Engine] PipelineLibraryを保存しました: " + WStringToString(cachePath), Logger::Level::Assets);
	}

	ID3D12PipelineState* PSORegistry::CreateAndCache(const PSODesc& desc) {
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = factory_->Build(desc);

		if (pipelineLibrary_) {
			const std::wstring key = MakeLibraryKey(desc);
			Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
			const HRESULT hr = pipelineLibrary_->LoadGraphicsPipeline(key.c_str(), &psoDesc, IID_PPV_ARGS(&pso));
			if (SUCCEEDED(hr)) {
				auto* raw = pso.Get();
				psoCache_[desc] = std::move(pso);
				return raw;
			}
		}

		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
		const HRESULT hr = device_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
		if (FAILED(hr)) {
			LogPipelineStateError(hr, desc);
			assert(false && "PSORegistry::CreateAndCache: PipelineStateの生成に失敗しました");
			return nullptr;
		}

		if (pipelineLibrary_) {
			const std::wstring key = MakeLibraryKey(desc);
			pipelineLibrary_->StorePipeline(key.c_str(), pso.Get());
		}

		isDirty_ = true;
		auto* raw = pso.Get();
		psoCache_[desc] = std::move(pso);
		return raw;
	}

	std::wstring PSORegistry::MakeLibraryKey(const PSODesc& desc) {
		std::wostringstream oss;
		oss << static_cast<int>(desc.blendMode) << L"_"
			<< static_cast<int>(desc.depthMode) << L"_"
			<< static_cast<int>(desc.cullMode) << L"_"
			<< static_cast<int>(desc.fillMode) << L"_"
			<< static_cast<int>(desc.topology) << L"_"
			<< static_cast<int>(desc.inputLayout) << L"_"
			<< desc.renderTargetCount << L"_"
			<< static_cast<int>(desc.rtvFormat) << L"_"
			<< static_cast<int>(desc.dsvFormat) << L"_"
			<< desc.depthBias << L"_"
			<< desc.depthBiasClamp << L"_"
			<< desc.slopeScaledDepthBias << L"_"
			<< std::wstring(desc.vsKey.begin(), desc.vsKey.end()) << L"_"
			<< std::wstring(desc.psKey.begin(), desc.psKey.end()) << L"_"
			<< std::wstring(desc.rootSigKey.begin(), desc.rootSigKey.end());
		return oss.str();
	}

} // namespace MadoEngine::Render
