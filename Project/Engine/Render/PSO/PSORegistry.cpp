#include "Render/PSO/PSORegistry.h"
#include "Core/DxDevice/DxDevice.h"
#include "Utility/Logger/Logger.h"
#include <fstream>
#include <cassert>
#include <thread>
#include <chrono>
#include <sstream>
#include <windows.h>

namespace MadoEngine::Render {

	/// @brief ワイド文字列をUTF-8文字列に変換する
	/// @param wstr 変換元ワイド文字列
	/// @return UTF-8文字列
	static std::string WStringToString(const std::wstring& wstr) {
		if (wstr.empty()) return {};
		int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::string result(size - 1, '\0');
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);
		return result;
	}

	void PSORegistry::Initialize(Core::DxDevice* device, PSOFactory* factory) {
		assert(device);
		assert(factory);
		device_  = device;
		factory_ = factory;
		Logger::Output("[PSORegistry] 初期化完了", Logger::Level::Engine);
	}

	void PSORegistry::Finalize() {
		// プリウォーム完了まで待機
		while (!IsPrewarmComplete()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		prewarmTasks_.clear();

		// 変更があればPipelineLibraryを保存
		if (isDirty_ && !cachePath_.empty()) {
			SavePipelineLibrary(cachePath_);
		}

		Logger::Output("[PSORegistry] 終了処理完了", Logger::Level::Engine);
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
		Logger::Output("[PSORegistry] プリウォーム開始 count=" + std::to_string(descs.size()),
					   Logger::Level::Engine);

		for (const auto& desc : descs) {
			std::lock_guard<std::mutex> lock(cacheMutex_);
			auto it = psoCache_.find(desc);
			if (it == psoCache_.end()) {
				CreateAndCache(desc);
			}
		}

		Logger::Output("[PSORegistry] プリウォーム完了", Logger::Level::Engine);
	}

	void PSORegistry::PrewarmAsync(const std::vector<PSODesc>& descs) {
		Logger::Output("[PSORegistry] 非同期プリウォーム開始 count=" + std::to_string(descs.size()),
					   Logger::Level::Engine);

		for (const auto& desc : descs) {
			auto fut = std::async(std::launch::async, [this, desc]() {
				std::lock_guard<std::mutex> lock(cacheMutex_);
				auto it = psoCache_.find(desc);
				if (it == psoCache_.end()) {
					CreateAndCache(desc);
				}
			});
			prewarmTasks_.push_back(std::move(fut));
		}
	}

	bool PSORegistry::IsPrewarmComplete() const {
		for (const auto& task : prewarmTasks_) {
			if (!task.valid()) continue;
			auto status = task.wait_for(std::chrono::seconds(0));
			if (status != std::future_status::ready) {
				return false;
			}
		}
		return true;
	}

	void PSORegistry::LoadPipelineLibrary(const std::wstring& cachePath) {
		cachePath_ = cachePath;

		std::ifstream file(cachePath, std::ios::binary | std::ios::ate);
		if (!file.is_open()) {
			// ファイルが存在しない場合は空のPipelineLibraryを新規作成
			Logger::Output("[PSORegistry] PipelineLibraryキャッシュが見つからないため新規作成します",
						   Logger::Level::Assets);
			Microsoft::WRL::ComPtr<ID3D12Device1> device1;
			device_->GetDevice()->QueryInterface(IID_PPV_ARGS(&device1));
			if (device1) {
				HRESULT hr = device1->CreatePipelineLibrary(
					nullptr, 0, IID_PPV_ARGS(&pipelineLibrary_));
				if (FAILED(hr)) {
					Logger::Output("[PSORegistry] PipelineLibraryの新規作成に失敗しました",
								   Logger::Level::Error);
				}
			}
			return;
		}

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		std::vector<char> buffer(static_cast<size_t>(size));
		file.read(buffer.data(), size);
		file.close();

		{
			Microsoft::WRL::ComPtr<ID3D12Device1> device1;
			device_->GetDevice()->QueryInterface(IID_PPV_ARGS(&device1));
			if (!device1) return;

			HRESULT hr = device1->CreatePipelineLibrary(
				buffer.data(), static_cast<SIZE_T>(size), IID_PPV_ARGS(&pipelineLibrary_));

			if (FAILED(hr)) {
				// 無効なキャッシュの場合は空のライブラリを作成し直す
				Logger::Output("[PSORegistry] PipelineLibraryキャッシュが無効なため再作成します",
							   Logger::Level::Assets);
				device1->CreatePipelineLibrary(
					nullptr, 0, IID_PPV_ARGS(&pipelineLibrary_));
			} else {
				Logger::Output("[PSORegistry] PipelineLibraryをロードしました: " +
							   WStringToString(cachePath),
							   Logger::Level::Assets);
			}
		}
	}

	void PSORegistry::SavePipelineLibrary(const std::wstring& cachePath) {
		if (!pipelineLibrary_) {
			Logger::Output("[PSORegistry] PipelineLibraryが存在しないため保存をスキップします",
						   Logger::Level::Assets);
			return;
		}

		SIZE_T serializedSize = pipelineLibrary_->GetSerializedSize();
		std::vector<char> buffer(serializedSize);
		HRESULT hr = pipelineLibrary_->Serialize(buffer.data(), serializedSize);
		if (FAILED(hr)) {
			Logger::Output("[PSORegistry] PipelineLibraryのシリアライズに失敗しました",
						   Logger::Level::Error);
			return;
		}

		std::ofstream outFile(cachePath, std::ios::binary);
		if (!outFile.is_open()) {
			Logger::Output("[PSORegistry] PipelineLibraryキャッシュファイルのオープンに失敗しました",
						   Logger::Level::Error);
			return;
		}
		outFile.write(buffer.data(), static_cast<std::streamsize>(serializedSize));
		outFile.close();

		Logger::Output("[PSORegistry] PipelineLibraryを保存しました: " +
					   WStringToString(cachePath),
					   Logger::Level::Assets);
	}

	ID3D12PipelineState* PSORegistry::CreateAndCache(const PSODesc& desc) {
		// PipelineLibraryにキーが存在すれば LoadPipeline で生成
		if (pipelineLibrary_) {
			std::wstring key = MakeLibraryKey(desc);
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = factory_->Build(desc);

			Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
			HRESULT hr = pipelineLibrary_->LoadGraphicsPipeline(
				key.c_str(), &psoDesc, IID_PPV_ARGS(&pso));

			if (SUCCEEDED(hr)) {
				Logger::Output("[PSORegistry] PipelineLibraryからPSOをロードしました",
							   Logger::Level::Assets);
				auto* raw = pso.Get();
				psoCache_[desc] = std::move(pso);
				return raw;
			}
		}

		// PipelineLibraryに存在しない場合は PSOFactory::Build() で生成
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = factory_->Build(desc);

		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
		HRESULT hr = device_->GetDevice()->CreateGraphicsPipelineState(
			&psoDesc, IID_PPV_ARGS(&pso));

		if (FAILED(hr)) {
			Logger::Output("[PSORegistry] GraphicsPipelineStateの生成に失敗しました",
						   Logger::Level::Error);
			assert(false && "PSORegistry::CreateAndCache: PipelineStateの生成に失敗しました");
			return nullptr;
		}

		// PipelineLibraryに登録
		if (pipelineLibrary_) {
			std::wstring key = MakeLibraryKey(desc);
			pipelineLibrary_->StorePipeline(key.c_str(), pso.Get());
			Logger::Output("[PSORegistry] PipelineLibraryにPSOを登録します", Logger::Level::Assets);
		}

		isDirty_ = true;

		auto* raw = pso.Get();
		psoCache_[desc] = std::move(pso);
		return raw;
	}

	std::wstring PSORegistry::MakeLibraryKey(const PSODesc& desc) {
		std::wostringstream oss;
		oss << static_cast<int>(desc.blendMode)    << L"_"
			<< static_cast<int>(desc.depthMode)    << L"_"
			<< static_cast<int>(desc.cullMode)     << L"_"
			<< static_cast<int>(desc.fillMode)     << L"_"
			<< static_cast<int>(desc.topology)     << L"_"
			<< static_cast<int>(desc.inputLayout)  << L"_"
			<< static_cast<int>(desc.rtvFormat)    << L"_"
			<< static_cast<int>(desc.dsvFormat)    << L"_"
			<< std::wstring(desc.vsKey.begin(), desc.vsKey.end())           << L"_"
			<< std::wstring(desc.psKey.begin(), desc.psKey.end())           << L"_"
			<< std::wstring(desc.rootSigKey.begin(), desc.rootSigKey.end());
		return oss.str();
	}

} // namespace MadoEngine::Render
