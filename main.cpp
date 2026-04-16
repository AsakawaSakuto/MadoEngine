#include <Windows.h>
#include "Engine/Render/Screen/WindowsAPI.h"
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

    Logger::Info("情報メッセージ");
    Logger::Warning("警告メッセージ");
    Logger::Error("エラーメッセージ");
    Logger::Debug("デバッグメッセージ");

    // メインループ
    while (windowsAPI.ProcessMessage()) {
        // ゲームの処理
    }

    Logger::Finalize();

    return 0;
}