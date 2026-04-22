#include <Windows.h>
#include "Engine/Render/Screen/WindowsAPI.h"
#include "Engine/Core/DxDevice/DxDevice.h"
#include "Engine/Audio/AudioManager.h"
#include "Engine/Input/InputManager.h"
#include "Engine/UtilityHeaders.h"
#include "Engine/Core/DeltaTime/DeltaTime.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	Logger::Initialize();

	MadoEngine::WindowsAPI windowsAPI;
	MadoEngine::DeltaTime deltaTime;

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

	// AudioManagerの初期化（Assets/Audio内の全ファイルを自動ロード）
	MadoEngine::AudioManager::GetInstance()->Initialize();

	MadoEngine::InputManager::GetInstance()->Initialize();

	Input::SetInputKeys("Jump", DIK_SPACE, GAMEPAD_STICK_R, GAMEPAD_STICK_L, GAMEPAD_L, GAMEPAD_R, GAMEPAD_A);

	// メインループ
	while (windowsAPI.ProcessMessage()) {
		// デルタタイムを計算
		deltaTime.Update();
		float dt = static_cast<float>(deltaTime.GetDeltaTime());

		// ゲームの処理
		MadoEngine::AudioManager::GetInstance()->Update();

		MadoEngine::InputManager::GetInstance()->Update(windowsAPI.GetHWnd(), dt);

		if (Input::Trigger("jump")) {
			//Input::GetGamePad()->SetVibration(1.0f, 1.0f, 2.f, EaseType::None);
			//Audio::Play("jump");
		}

		if (Input::Press("jump")) {}

		if (Input::Release("jump")) {}
	}

	MadoEngine::AudioManager::GetInstance()->Finalize();

	MadoEngine::InputManager::GetInstance()->Finalize();

	Logger::Finalize();

	return 0;
}