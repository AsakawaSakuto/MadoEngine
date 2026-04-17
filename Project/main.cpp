#include <Windows.h>
#include "Engine/Render/Screen/WindowsAPI.h"
#include "Engine/Core/DxDevice/DxDevice.h"
#include "Engine/UtilityHeaders.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

    Logger::Initialize();

    MadoEngine::WindowsAPI windowsAPI;

    MadoEngine::WindowsAPI::WindowDesc desc;
    desc.title = "MadoEngine";
    desc.width = 1280;
    desc.height = 720;
    desc.iconPath = "Assets/.EngineResource/icon.png";

    windowsAPI.Initialize(desc, hInstance);

	// ログのテスト
    Logger::Info("情報メッセージ");
    Logger::Warning("警告メッセージ");
    Logger::Error("エラーメッセージ");
    Logger::Debug("デバッグメッセージ");

    // DxDeviceの初期化
    MadoEngine::DxDevice dxDevice;
    dxDevice.Initialize();

    // メインループ
    while (windowsAPI.ProcessMessage()) {
        // ゲームの処理
    }

    Logger::Finalize();

    return 0;
}