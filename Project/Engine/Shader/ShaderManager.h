#pragma once
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace MadoEngine {

	/// @brief HLSLシェーダーのコンパイル・キャッシュ・取得を管理するシングルトンクラス
	class ShaderManager {
	public:
		/// @brief シングルトンインスタンスを取得する
		/// @return ShaderManagerの唯一のインスタンス
		static ShaderManager* GetInstance();

		// コピー・ムーブ禁止
		ShaderManager(const ShaderManager&) = delete;
		ShaderManager& operator=(const ShaderManager&) = delete;
		ShaderManager(ShaderManager&&) = delete;
		ShaderManager& operator=(ShaderManager&&) = delete;

		/// @brief 初期化。DXCの準備と "Assets/Shader" 内の全 .hlsl を走査しコンパイル・キャッシュを行う
		void Initialize();

		/// @brief キーに対応するシェーダーバイトコードを取得する
		/// @param key シェーダーキー（例: "PostEffect/CopyImage.VS"）
		/// @return D3D12_SHADER_BYTECODE（見つからない場合はサイズ0）
		D3D12_SHADER_BYTECODE Get(const std::string& key) const;

		/// @brief 全シェーダーを解放する
		void Finalize();

	private:
		ShaderManager() = default;
		~ShaderManager() = default;

		/// @brief DXCライブラリを初期化する
		void InitializeDxc();

		/// @brief HLSLファイルをコンパイルしてバイトコードを返す
		/// @param filePath HLSLファイルパス（ワイド文字列）
		/// @param profile シェーダープロファイル（例: L"vs_6_0"）
		/// @return コンパイル済みバイトコード（失敗時はnullptr）
		Microsoft::WRL::ComPtr<IDxcBlob> Compile(
			const std::wstring& filePath,
			const wchar_t* profile
		);

		/// @brief コンパイル済み .cso ファイルをBlobとして読み込む
		/// @param csoPath CSOファイルパス（ワイド文字列）
		/// @return 読み込んだ IDxcBlob（失敗時はnullptr）
		Microsoft::WRL::ComPtr<IDxcBlob> LoadCso(const std::wstring& csoPath);

		/// @brief .hlsl ファイルをコンパイルし .cso を .cache/ に保存する
		/// @param hlslPath HLSLファイルのパス
		/// @param csoPath  出力 CSOファイルのパス
		/// @param profile  シェーダープロファイル（例: L"vs_6_0"）
		/// @return コンパイル済み IDxcBlob（失敗時 nullptr）
		Microsoft::WRL::ComPtr<IDxcBlob> CompileAndSave(
			const std::filesystem::path& hlslPath,
			const std::filesystem::path& csoPath,
			const wchar_t* profile
		);

		/// @brief ファイルパスからシェーダーキー文字列を生成する
		/// @param relPath HLSLファイルパス（Assets/Shader/ からの相対）
		/// @return キー文字列（例: "PostEffect/CopyImage.VS"）
		static std::string MakeKey(const std::filesystem::path& relPath);

		/// @brief ファイル名からシェーダープロファイルを判定する
		/// @param filename ファイル名
		/// @return プロファイル文字列ポインタ（例: L"vs_6_0"）。対象外なら nullptr
		static const wchar_t* ResolveProfile(const std::filesystem::path& filename);

		// DXC
		Microsoft::WRL::ComPtr<IDxcUtils>          dxcUtils_;
		Microsoft::WRL::ComPtr<IDxcCompiler3>      dxcCompiler_;
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;

		// キー → バイトコード blob
		std::unordered_map<std::string, Microsoft::WRL::ComPtr<IDxcBlob>> shaderMap_;
	};

} // namespace MadoEngine
