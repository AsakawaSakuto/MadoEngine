#include "Shader/ShaderManager.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <fstream>
#include <format>
#include <cassert>

#pragma comment(lib, "dxcompiler.lib")

namespace MadoEngine {

	static constexpr const char* kShaderRoot = "Assets/Shader";
	static constexpr const char* kCacheDir   = "Assets/Shader/.cache";

	ShaderManager& ShaderManager::GetInstance() {
		static ShaderManager instance;
		return instance;
	}

	void ShaderManager::Initialize() {
		InitializeDxc();

		namespace fs = std::filesystem;

		// .cache ディレクトリが存在しない場合は作成
		if (!fs::exists(kCacheDir)) {
			fs::create_directories(kCacheDir);
			Logger::Output(
				std::format("キャッシュディレクトリを作成しました : {}", kCacheDir),
				Logger::Level::Assets
			);
		}

		const fs::path shaderRoot(kShaderRoot);
		const fs::path cacheRoot(kCacheDir);

		// Assets/Shader 以下を再帰スキャン
		for (const auto& entry : fs::recursive_directory_iterator(shaderRoot)) {
			if (!entry.is_regular_file()) { continue; }

			const fs::path& hlslPath = entry.path();

			// .hlsli はスキップ
			if (hlslPath.extension() != ".hlsl") { continue; }

			const wchar_t* profile = ResolveProfile(hlslPath.filename());
			if (!profile) {
				// VS/PS が判定できないファイルはスキップ
				continue;
			}

			// キー生成（Assets/Shader/ からの相対パス、拡張子なし）
			const fs::path relPath = fs::relative(hlslPath, shaderRoot);
			const std::string key  = MakeKey(relPath);

			// 対応する .cso のパスを組み立てる
			// サブディレクトリ構造をフラットに変換してキャッシュに配置
			std::string flatName = key;
			for (char& c : flatName) {
				if (c == '/') { c = '_'; }
			}
			const fs::path csoPath = cacheRoot / (flatName + ".cso");

			Microsoft::WRL::ComPtr<IDxcBlob> blob;

			bool needCompile = false;
			if (!fs::exists(csoPath)) {
				// .cso が存在しない → コンパイル必要
				Logger::Output(
					std::format("CSOが存在しないためコンパイルします : {}", key),
					Logger::Level::Assets
				);
				needCompile = true;
			} else {
				// タイムスタンプ比較 : HLSL が新しければ再コンパイル
				const auto hlslTime = fs::last_write_time(hlslPath);
				const auto csoTime  = fs::last_write_time(csoPath);
				if (hlslTime > csoTime) {
					Logger::Output(
						std::format("HLSLが更新されているため再コンパイルします : {}", key),
						Logger::Level::Assets
					);
					needCompile = true;
				}
			}

			if (needCompile) {
				blob = CompileAndSave(hlslPath, csoPath, profile);
				if (!blob) {
					Logger::Output(
						std::format("コンパイルに失敗しました : {}", key),
						Logger::Level::Error
					);
					continue;
				}
			} else {
				// .cso ファイルから読み込む
				blob = LoadCso(csoPath.wstring());
				if (!blob) {
					Logger::Output(
						std::format("CSOのBlob生成に失敗しました : {}", key),
						Logger::Level::Error
					);
					continue;
				}
			}

			shaderMap_[key] = std::move(blob);
			Logger::Output(
				std::format("シェーダーを登録しました : {}", key),
				Logger::Level::Assets
			);
		}

		Logger::Output(
			std::format("初期化完了。登録シェーダー数 : {}", shaderMap_.size()),
			Logger::Level::Engine
		);
	}

	D3D12_SHADER_BYTECODE ShaderManager::Get(const std::string& key) const {
		const auto it = shaderMap_.find(key);
		if (it == shaderMap_.end()) {
			Logger::Output(
				std::format("シェーダーキーが見つかりません : {}", key),
				Logger::Level::Warning
			);
			return { nullptr, 0 };
		}
		return {
			it->second->GetBufferPointer(),
			it->second->GetBufferSize()
		};
	}

	std::vector<std::string> ShaderManager::GetKeysByPrefix(const std::string& prefix) const {
		std::vector<std::string> keys;
		for (const auto& shaderPair : shaderMap_) {
			const std::string& key = shaderPair.first;
			if (key.rfind(prefix, 0) != 0) {
				continue;
			}

			keys.push_back(key);
		}

		std::sort(keys.begin(), keys.end());
		return keys;
	}

	void ShaderManager::Finalize() {
		shaderMap_.clear();
		Logger::Output("ShaderManager 終了しました", Logger::Level::Engine);
	}

	Microsoft::WRL::ComPtr<IDxcBlob> ShaderManager::CompileAndSave(
		const std::filesystem::path& hlslPath,
		const std::filesystem::path& csoPath,
		const wchar_t* profile)
	{
		auto blob = Compile(hlslPath.wstring(), profile);
		if (!blob) { return nullptr; }

		// .cso を書き出す
		std::ofstream ofs(csoPath, std::ios::binary);
		if (!ofs) {
			Logger::Output(
				std::format("CSOファイルの書き込みに失敗しました : {}", csoPath.string()),
				Logger::Level::Error
			);
			return blob; // blob は返す（メモリ上は有効）
		}
		ofs.write(
			static_cast<const char*>(blob->GetBufferPointer()),
			static_cast<std::streamsize>(blob->GetBufferSize())
		);
		Logger::Output(
			std::format("CSOを保存しました : {}", csoPath.string()),
			Logger::Level::Assets
		);
		return blob;
	}

	std::string ShaderManager::MakeKey(const std::filesystem::path& relPath) {
		// 拡張子を除いた相対パスを / 区切りで返す
		// 例: PostEffect\CopyImage.VS.hlsl → "PostEffect/CopyImage.VS"
		std::filesystem::path noExt = relPath;
		noExt.replace_extension(); // .hlsl を除去
		std::string key = noExt.generic_string();
		return key;
	}

	const wchar_t* ShaderManager::ResolveProfile(const std::filesystem::path& filename) {
		const std::string name = filename.string();
		// *.VS.hlsl → vs_6_0、*.PS.hlsl → ps_6_0
		if (name.find(".VS.") != std::string::npos) { return L"vs_6_0"; }
		if (name.find(".PS.") != std::string::npos) { return L"ps_6_0"; }
		if (name.find(".GS.") != std::string::npos) { return L"gs_6_0"; }
		if (name.find(".CS.") != std::string::npos) { return L"cs_6_0"; }
		return nullptr;
	}

	void ShaderManager::InitializeDxc() {
		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
		assert(SUCCEEDED(hr));
		Logger::Output("DxcUtils の生成が完了しました", Logger::Level::Engine);

		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
		assert(SUCCEEDED(hr));
		Logger::Output("DxcCompiler の生成が完了しました", Logger::Level::Engine);

		hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
		assert(SUCCEEDED(hr));
		Logger::Output("DXC の初期化が完了しました", Logger::Level::Engine);
	}

	Microsoft::WRL::ComPtr<IDxcBlob> ShaderManager::Compile(
		const std::wstring& filePath,
		const wchar_t* profile)
	{
		// wstring → string 変換ヘルパー
		auto toStr = [](const std::wstring& ws) -> std::string {
			int size = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
			std::string str(size - 1, '\0');
			WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &str[0], size, nullptr, nullptr);
			return str;
		};

		Logger::Output(
			std::format("コンパイルを開始します : {}", toStr(filePath)),
			Logger::Level::Assets
		);

		Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource;
		HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
		if (FAILED(hr)) {
			Logger::Output(
				std::format("シェーダーファイルの読み込みに失敗しました : {}", toStr(filePath)),
				Logger::Level::Error
			);
			assert(false);
			return nullptr;
		}

		DxcBuffer sourceBuffer{};
		sourceBuffer.Ptr      = shaderSource->GetBufferPointer();
		sourceBuffer.Size     = shaderSource->GetBufferSize();
		sourceBuffer.Encoding = DXC_CP_UTF8;

		LPCWSTR args[] = {
			filePath.c_str(),
			L"-E", L"main",
			L"-T", profile,
			L"-Zi",
			L"-Od",
			L"-Zpr",
		};

		Microsoft::WRL::ComPtr<IDxcResult> result;
		hr = dxcCompiler_->Compile(
			&sourceBuffer,
			args,
			_countof(args),
			includeHandler_.Get(),
			IID_PPV_ARGS(&result)
		);
		assert(SUCCEEDED(hr));

		Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0) {
			Logger::Output(
				std::format("コンパイルエラー : {}", errors->GetStringPointer()),
				Logger::Level::Error
			);
			assert(false);
			return nullptr;
		}

		HRESULT status = S_OK;
		result->GetStatus(&status);
		if (FAILED(status)) {
			Logger::Output(
				std::format("コンパイルに失敗しました : {}", toStr(filePath)),
				Logger::Level::Error
			);
			assert(false);
			return nullptr;
		}

		Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob;
		result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
		assert(shaderBlob);

		Logger::Output(
			std::format("コンパイルが完了しました : {}", toStr(filePath)),
			Logger::Level::Assets
		);
		return shaderBlob;
	}

	Microsoft::WRL::ComPtr<IDxcBlob> ShaderManager::LoadCso(const std::wstring& csoPath) {
		Microsoft::WRL::ComPtr<IDxcBlobEncoding> blob;
		HRESULT hr = dxcUtils_->LoadFile(csoPath.c_str(), nullptr, &blob);
		if (FAILED(hr)) {
			auto toStr = [](const std::wstring& ws) -> std::string {
				int size = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
				std::string str(size - 1, '\0');
				WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &str[0], size, nullptr, nullptr);
				return str;
			};
			Logger::Output(
				std::format("CSOファイルの読み込みに失敗しました : {}", toStr(csoPath)),
				Logger::Level::Error
			);
			return nullptr;
		}
		return blob;
	}

} // namespace MadoEngine
