#include "DeltaTime.h"
#include <thread>

namespace MadoEngine
{
    DeltaTime::DeltaTime()
        : m_deltaTime(0.0)
        , m_fps(0.0)
        , m_targetFPS(60.0)
        , m_targetFrameTime(1.0 / 60.0)
        , m_enableFPSLimit(false)
    {
        m_lastTime = std::chrono::high_resolution_clock::now();
        m_currentTime = m_lastTime;
    }

    void DeltaTime::Update()
    {
        if (m_enableFPSLimit)
        {
            WaitForTargetFrameTime();
        }

        m_currentTime = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> elapsed = m_currentTime - m_lastTime;
        m_deltaTime = elapsed.count();

        if (m_enableFPSLimit)
        {
            m_deltaTime = m_targetFrameTime;
        }

        if (m_deltaTime > 0.0)
        {
            m_fps = 1.0 / m_deltaTime;
        }

        m_lastTime = m_currentTime;
    }

    void DeltaTime::SetTargetFPS(double targetFPS)
    {
        if (targetFPS > 0.0)
        {
            m_targetFPS = targetFPS;
            m_targetFrameTime = 1.0 / targetFPS;
        }
    }

    void DeltaTime::EnableFPSLimit(bool enable)
    {
        m_enableFPSLimit = enable;
        if (enable)
        {
            m_lastTime = std::chrono::high_resolution_clock::now();
        }
    }

    void DeltaTime::WaitForTargetFrameTime()
    {
        auto targetTime = m_lastTime + std::chrono::duration<double>(m_targetFrameTime);
        auto now = std::chrono::high_resolution_clock::now();

        if (now < targetTime)
        {
            auto sleepDuration = std::chrono::duration_cast<std::chrono::microseconds>(targetTime - now);
            std::this_thread::sleep_for(sleepDuration);
        }
    }
}
