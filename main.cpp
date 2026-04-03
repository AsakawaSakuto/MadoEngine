#include <Windows.h>

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // 出力ウィンドウへの文字出力
    OutputDebugStringA("Hello,DirectX!\n");

    return 0;
}