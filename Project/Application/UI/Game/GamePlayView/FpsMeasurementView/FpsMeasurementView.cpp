#include "FpsMeasurementView.h"
#include ".SceneManager/SceneType.h"
#include "Render/Object/2d/Text/MyText.h"
#include <format>

namespace {
	constexpr float kTextUpdateInterval = 0.25f;
	constexpr const char* kTextObjectName = "FpsText";
}

namespace UI::Game {

	void FpsMeasurementView::Initialize() {
		sampleTime_ = 0.0f;
		sampleFrameCount_ = 0;
	}

	void FpsMeasurementView::Update(float deltaTime) {
		auto fpsTextHandle = MyText::Find(kTextObjectName);
		MadoEngine::Text* fpsText = MyText::TryGet(fpsTextHandle);
		if (!fpsText) {
			return;
		}

		sampleTime_ += deltaTime;
		++sampleFrameCount_;
		if (sampleTime_ < kTextUpdateInterval) {
			return;
		}

		const float fps = sampleTime_ > 0.0f
			? static_cast<float>(sampleFrameCount_) / sampleTime_
			: 0.0f;
		fpsText->SetText(std::format("FPS : {:.1f}", fps));
		sampleTime_ = 0.0f;
		sampleFrameCount_ = 0;
	}

	void FpsMeasurementView::Finalize() {
		sampleTime_ = 0.0f;
		sampleFrameCount_ = 0;
	}
}
