#pragma once
#include <Windows.h>
#include "Math/Vector2.h"

// 左クリック
#define MOUSE_L 0x10000
// 右クリック
#define MOUSE_R 0x10001
// 中クリック
#define MOUSE_M 0x10002

namespace MadoEngine::InputDevice
{
	class Mouse
	{
	public:
		Mouse();
		~Mouse();

		void Update(HWND hwnd);

		bool IsPress(int button) const;
		bool IsTrigger(int button) const;
		bool IsRelease(int button) const;

		Vector2 GetPosition() const;
		Vector2 GetDelta() const;
		float GetWheelDelta() const;
		void AddWheelDelta(float delta);
		/// @brief マウスをウィンドウ内に固定して相対移動量を取得するモードを設定する
		/// @param enable trueで相対入力モードを有効にする
		void SetRelativeMode(bool enable);
		/// @brief 相対入力モードが有効かどうかを取得する
		/// @return 有効ならtrue
		bool IsRelativeMode() const;

	private:
		static constexpr int BUTTON_COUNT = 3;
		bool currentState_[BUTTON_COUNT];
		bool previousState_[BUTTON_COUNT];

		Vector2 currentPosition_;
		Vector2 previousPosition_;
		Vector2 relativeDelta_;
		float wheelDelta_;
		float currentWheelDelta_;
		bool isRelativeMode_;
		bool isRelativeCenterInitialized_;

		/// @brief ウィンドウのクライアント領域中心をスクリーン座標で取得する
		/// @param hwnd 対象ウィンドウハンドル
		/// @param center 取得した中心座標の格納先
		/// @return 取得できた場合はtrue
		bool TryGetClientCenter(HWND hwnd, POINT& center) const;

		/// @brief ウィンドウのクライアント領域をスクリーン座標で取得する
		/// @param hwnd 対象ウィンドウハンドル
		/// @param rect 取得した矩形の格納先
		/// @return 取得できた場合はtrue
		bool TryGetClientScreenRect(HWND hwnd, RECT& rect) const;
	};
}
