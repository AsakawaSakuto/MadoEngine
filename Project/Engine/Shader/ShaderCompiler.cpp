#include "ShaderCompiler.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include <format>

#pragma comment(lib, "dxcompiler.lib")

namespace MadoEngine {

    void ShaderCompiler::Initialize() {
        // DXCユーティリティの生成
        HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
        assert(SUCCEEDED(hr));
        Logger::Output("DxcUtils の生成が完了しました", Logger::Level::Engine);

        // DXCコンパイラの生成
        hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
        assert(SUCCEEDED(hr));
        Logger::Output("DxcCompiler の生成が完了しました", Logger::Level::Engine);

        // デフォルトのインクルードハンドラを生成
        hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
        assert(SUCCEEDED(hr));
        Logger::Output("ShaderCompiler の初期化が完了しました", Logger::Level::Engine);
    }

    Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::Compile(
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

        // HLSLファイルを読み込む
        Logger::Output(
            std::format("シェーダーのコンパイルを開始します : {}", toStr(filePath)),
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

        // ソースバッファを設定
        DxcBuffer sourceBuffer{};
        sourceBuffer.Ptr      = shaderSource->GetBufferPointer();
        sourceBuffer.Size     = shaderSource->GetBufferSize();
        sourceBuffer.Encoding = DXC_CP_UTF8;

        // コンパイル引数を設定
        LPCWSTR args[] = {
            filePath.c_str(),
            L"-E", L"main",
            L"-T", profile,
            L"-Zi",             // デバッグ情報を含める
            L"-Od",             // 最適化を無効化
			L"-Zpr",            // メモリレイアウトは行優先
        };

        // コンパイル実行
        Microsoft::WRL::ComPtr<IDxcResult> result;
        hr = dxcCompiler_->Compile(
            &sourceBuffer,
            args,
            _countof(args),
            includeHandler_.Get(),
            IID_PPV_ARGS(&result)
        );
        assert(SUCCEEDED(hr));

        // コンパイルエラーの確認
        Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
        result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
        if (errors && errors->GetStringLength() > 0) {
            Logger::Output(
                std::format("シェーダーのコンパイルエラー : {}", errors->GetStringPointer()),
                Logger::Level::Error
            );
            assert(false);
            return nullptr;
        }

        // コンパイル結果のステータス確認
        HRESULT status = S_OK;
        result->GetStatus(&status);
        if (FAILED(status)) {
            Logger::Output(
                std::format("シェーダーのコンパイルに失敗しました : {}", toStr(filePath)),
                Logger::Level::Error
            );
            assert(false);
            return nullptr;
        }

        // バイトコードを取得
        Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob;
        result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
        assert(shaderBlob);

        Logger::Output(
            std::format("シェーダーのコンパイルが完了しました : {}", toStr(filePath)),
            Logger::Level::Assets
        );

        return shaderBlob;
    }

}
