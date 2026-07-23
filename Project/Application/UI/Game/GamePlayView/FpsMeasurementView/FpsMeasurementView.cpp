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
		fpsText_ = MyText::Create(
			kTextObjectName,
			"FPS : 0.0",
			SceneType::Game,
			MadoEngine::EditorManagementMode::EditorManaged,
			MadoEngine::Render::RenderLayer::UI);
		sampleTime_ = 0.0f;
		sampleFrameCount_ = 0;
	}

	void FpsMeasurementView::Update(float deltaTime) {
		if (!fpsText_) {
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
		fpsText_->SetText(std::format("FPS : {:.1f}", fps));
		sampleTime_ = 0.0f;
		sampleFrameCount_ = 0;
	}

	void FpsMeasurementView::Finalize() {
		MyText::Destroy(kTextObjectName);
		fpsText_ = nullptr;
		sampleTime_ = 0.0f;
		sampleFrameCount_ = 0;
	}
}
