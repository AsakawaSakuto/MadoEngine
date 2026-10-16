#pragma once
#include <chrono>

namespace MadoEngine
{
    class DeltaTime
    {
    public:
        DeltaTime();
        ~DeltaTime() = default;

        void Update();

		/// @brief 目標FPSを設定
        /// @param targetFPS 目標FPS
        void SetTargetFPS(double targetFPS = 60.0);

		/// @brief FPS制限の有効状態を設定
		/// @param enable trueで有効、falseで無効
        void EnableFPSLimit(bool enable = true);

		/// @return 前フレームからの経過時間（秒）
        double GetDeltaTime() const { return deltaTime_; }

		/// @return 現在のFPS
        double GetFPS() const { return fps_; }

		/// @return 目標FPS
        double GetTargetFPS() const { return targetFPS_; }

		/// @return FPS制限が有効な場合はtrue
        bool IsFPSLimitEnabled() const { return enableFPSLimit_; }

    private:
        void WaitForTargetFrameTime();

        std::chrono::high_resolution_clock::time_point lastTime_;
        std::chrono::high_resolution_clock::time_point currentTime_;

        double deltaTime_;
        double fps_;
        double targetFPS_;
        double targetFrameTime_;
        bool enableFPSLimit_;
    };
}
