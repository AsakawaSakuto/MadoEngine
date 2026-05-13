#pragma once
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <string>

namespace MadoEngine {

    /// @brief HLSLシェーダーのコンパイルを担当するクラス
    class ShaderCompiler {
    public:
        /// @brief DXCライブラリを初期化する
        void Initialize();

        /// @brief HLSLファイルをコンパイルしてバイトコードを返す
        /// @param filePath HLSLファイルパス（ワイド文字列）
        /// @param profile シェーダープロファイル（例: L"vs_6_0", L"ps_6_0"）
        /// @return コンパイル済みバイトコード（失敗時はnullptr）
        Microsoft::WRL::ComPtr<IDxcBlob> Compile(
            const std::wstring& filePath,
            const wchar_t* profile
        );

    private:
        Microsoft::WRL::ComPtr<IDxcUtils>          dxcUtils_;
        Microsoft::WRL::ComPtr<IDxcCompiler3>      dxcCompiler_;
        Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
    };

}
