#include <Windows.h>
#include "Engine/Render/Screen/WindowsAPI.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

    MadoEngine::WindowsAPI windowsAPI;

    MadoEngine::WindowsAPI::WindowDesc desc;
    desc.title = "MadoEngine";
    desc.width = 1280;
    desc.height = 720;

    windowsAPI.Initialize(desc, hInstance);

    // メインループ
    while (windowsAPI.ProcessMessage()) {
        // ゲームの処理
    }

    return 0;
}