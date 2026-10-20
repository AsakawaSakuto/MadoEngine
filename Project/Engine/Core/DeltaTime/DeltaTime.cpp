#include "DeltaTime.h"
#include <thread>

namespace MadoEngine
{
    DeltaTime::DeltaTime()
        : deltaTime_(0.0)
        , fps_(0.0)
        , targetFPS_(60.0)
        , targetFrameTime_(1.0 / 60.0)
        , enableFPSLimit_(false)
    {
        Reset();
    }

    void DeltaTime::Update()
    {

        // 計測前に前Frame基準の目標時刻まで待機してFrame Rateを制限
        if (enableFPSLimit_)
        {
            WaitForTargetFrameTime();
        }

        currentTime_ = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> elapsed = currentTime_ - lastTime_;
        deltaTime_ = elapsed.count();

        if (enableFPSLimit_)
        {

            // Sleep精度の揺らぎをGame側へ伝えず固定Stepとして扱う補正
            deltaTime_ = targetFrameTime_;
        }

        if (deltaTime_ > 0.0)
        {
            fps_ = 1.0 / deltaTime_;
        }

        lastTime_ = currentTime_;
    }

    void DeltaTime::Reset() {
        lastTime_ = std::chrono::high_resolution_clock::now();
        currentTime_ = lastTime_;
        deltaTime_ = 0.0;
        fps_ = 0.0;
    }

    void DeltaTime::SetTargetFPS(double targetFPS)
    {
        if (targetFPS > 0.0)
        {
            targetFPS_ = targetFPS;
            targetFrameTime_ = 1.0 / targetFPS;
        }
    }

    void DeltaTime::EnableFPSLimit(bool enable)
    {
        enableFPSLimit_ = enable;
        if (enable)
        {
            lastTime_ = std::chrono::high_resolution_clock::now();
        }
    }

    void DeltaTime::WaitForTargetFrameTime()
    {
        auto targetTime = lastTime_ + std::chrono::duration<double>(targetFrameTime_);
        auto now = std::chrono::high_resolution_clock::now();

        if (now < targetTime)
        {
            auto sleepDuration = std::chrono::duration_cast<std::chrono::microseconds>(targetTime - now);
            std::this_thread::sleep_for(sleepDuration);
        }
    }
}
