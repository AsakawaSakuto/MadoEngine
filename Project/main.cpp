#include "Application/.Core/Terminal.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	auto terminal = std::make_unique<Terminal>(hInstance);
	terminal->Run();
	return 0;
}