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
        double GetDeltaTime() const { return m_deltaTime; }

		/// @return 現在のFPS
        double GetFPS() const { return m_fps; }

		/// @return 目標FPS
        double GetTargetFPS() const { return m_targetFPS; }

		/// @return FPS制限が有効な場合はtrue
        bool IsFPSLimitEnabled() const { return m_enableFPSLimit; }

    private:
        void WaitForTargetFrameTime();

        std::chrono::high_resolution_clock::time_point m_lastTime;
        std::chrono::high_resolution_clock::time_point m_currentTime;

        double m_deltaTime;
        double m_fps;
        double m_targetFPS;
        double m_targetFrameTime;
        bool m_enableFPSLimit;
    };
}
