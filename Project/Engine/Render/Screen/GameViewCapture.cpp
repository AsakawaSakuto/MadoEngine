#include "Render/Screen/GameViewCapture.h"
#include "Render/Screen/RenderTexture.h"
#include "DirectXTex/DirectXTex.h"
#include "Utility/Logger/Logger.h"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <format>
#include <iomanip>
#include <sstream>

namespace MadoEngine::Render {

	void GameViewCapture::Initialize(
		ID3D12CommandQueue* commandQueue,
		const std::filesystem::path& outputDirectory
	) {
		assert(commandQueue != nullptr && "commandQueueはnullptrにできません");
		assert(!outputDirectory.empty() && "出力先ディレクトリを指定してください");

		commandQueue_ = commandQueue;
		outputDirectory_ = outputDirectory;
	}

	bool GameViewCapture::Capture(const RenderTexture& sourceTexture) const {
		if (commandQueue_ == nullptr || sourceTexture.GetResource() == nullptr) {
			Logger::Output("GameViewのスクリーンショット機能が初期化されていません", Logger::Level::Error);
			return false;
		}

		std::error_code directoryError;
		std::filesystem::create_directories(outputDirectory_, directoryError);
		if (directoryError) {
			Logger::Output(
				"スクリーンショットの出力先ディレクトリを作成できませんでした: " +
				outputDirectory_.string(),
				Logger::Level::Error
			);
			return false;
		}

		DirectX::ScratchImage capturedImage;
		const HRESULT captureResult = DirectX::CaptureTexture(
			commandQueue_,
			sourceTexture.GetResource(),
			false,
			capturedImage,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		if (FAILED(captureResult)) {
			Logger::Output(
				std::format(
					"GameViewの画像をGPUから読み戻せませんでした (HRESULT: 0x{:08X})",
					static_cast<std::uint32_t>(captureResult)
				),
				Logger::Level::Error
			);
			return false;
		}

		const DirectX::Image* image = capturedImage.GetImage(0, 0, 0);
		if (image == nullptr) {
			Logger::Output("読み戻したGameViewの画像データが空です", Logger::Level::Error);
			return false;
		}

		const std::filesystem::path outputPath = CreateOutputPath();
		const HRESULT saveResult = DirectX::SaveToWICFile(
			*image,
			DirectX::WIC_FLAGS_FORCE_SRGB,
			DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
			outputPath.c_str()
		);
		if (FAILED(saveResult)) {
			Logger::Output(
				std::format(
					"GameViewのPNG保存に失敗しました (HRESULT: 0x{:08X})",
					static_cast<std::uint32_t>(saveResult)
				),
				Logger::Level::Error
			);
			return false;
		}

		Logger::Output(
			"GameViewのスクリーンショットを保存しました: " + outputPath.string(),
			Logger::Level::Engine
		);
		return true;
	}

	std::filesystem::path GameViewCapture::CreateOutputPath() const {
		const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		const std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
		std::tm localTime{};
		localtime_s(&localTime, &currentTime);

		const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch()
		) % 1000;

		std::ostringstream fileNameStream;
		fileNameStream
			<< "GameView_"
			<< std::put_time(&localTime, "%Y%m%d_%H%M%S")
			<< '_'
			<< std::setw(3)
			<< std::setfill('0')
			<< milliseconds.count();

		const std::string baseFileName = fileNameStream.str();
		std::filesystem::path outputPath = outputDirectory_ / (baseFileName + ".png");
		std::error_code existsError;
		for (std::uint32_t suffix = 1; std::filesystem::exists(outputPath, existsError) && !existsError; ++suffix) {
			outputPath = outputDirectory_ / (baseFileName + "_" + std::to_string(suffix) + ".png");
		}

		return outputPath;
	}

} // namespace MadoEngine::Render
