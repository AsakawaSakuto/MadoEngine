#include "TextTextureGenerator.h"
#include "Utility/Logger/Logger.h"
#include <algorithm>
#include <cmath>
#include <comdef.h>
#include <format>

namespace MadoEngine {

namespace {

	/// @brief 水平方向の配置をDirectWriteの値へ変換
	/// @param align Text側の水平配置
	/// @return DirectWriteの水平配置
	DWRITE_TEXT_ALIGNMENT ToDWriteTextAlignment(TextHorizontalAlign align) {
		switch (align) {
		case TextHorizontalAlign::Center:
			return DWRITE_TEXT_ALIGNMENT_CENTER;
		case TextHorizontalAlign::Right:
			return DWRITE_TEXT_ALIGNMENT_TRAILING;
		case TextHorizontalAlign::Left:
		default:
			return DWRITE_TEXT_ALIGNMENT_LEADING;
		}
	}

	/// @brief 垂直方向の配置をDirectWriteの値へ変換
	/// @param align Text側の垂直配置
	/// @return DirectWriteの垂直配置
	DWRITE_PARAGRAPH_ALIGNMENT ToDWriteParagraphAlignment(TextVerticalAlign align) {
		switch (align) {
		case TextVerticalAlign::Center:
			return DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
		case TextVerticalAlign::Bottom:
			return DWRITE_PARAGRAPH_ALIGNMENT_FAR;
		case TextVerticalAlign::Top:
		default:
			return DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		}
	}

	/// @brief HRESULT失敗時にログを出力
	/// @param message 出力するメッセージ
	/// @param hr HRESULT
	void LogIfFailed(const std::string& message, HRESULT hr) {
		if (FAILED(hr)) {
			Logger::Output(message + " HRESULT: 0x" + std::format("{:08X}", static_cast<uint32_t>(hr)), Logger::Level::Error);
		}
	}

} // namespace

TextTextureGenerator& TextTextureGenerator::GetInstance() {
	static TextTextureGenerator instance;
	return instance;
}

bool TextTextureGenerator::Initialize() {
	if (isInitialized_) {
		return true;
	}

	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (SUCCEEDED(hr)) {
		didInitializeCom_ = true;
	} else if (hr != RPC_E_CHANGED_MODE) {
		LogIfFailed("[Engine] COMの初期化に失敗しました。", hr);
		return false;
	}

	hr = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(writeFactory_.GetAddressOf()));
	if (FAILED(hr)) {
		LogIfFailed("[Engine] DirectWrite Factoryの作成に失敗しました。", hr);
		return false;
	}

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory_.GetAddressOf());
	if (FAILED(hr)) {
		LogIfFailed("[Engine] Direct2D Factoryの作成に失敗しました。", hr);
		return false;
	}

	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(wicFactory_.GetAddressOf()));
	if (FAILED(hr)) {
		LogIfFailed("[Engine] WIC Imaging Factoryの作成に失敗しました。", hr);
		return false;
	}

	isInitialized_ = true;
	Logger::Output("[Engine] TextTextureGeneratorを初期化しました。", Logger::Level::Engine);
	return true;
}

void TextTextureGenerator::Finalize() {
	wicFactory_.Reset();
	d2dFactory_.Reset();
	writeFactory_.Reset();
	isInitialized_ = false;

	if (didInitializeCom_) {
		CoUninitialize();
		didInitializeCom_ = false;
	}

	Logger::Output("[Engine] TextTextureGeneratorを終了しました。", Logger::Level::Engine);
}

bool TextTextureGenerator::Generate(const TextTextureDesc& desc, TextTexturePixels& outPixels) {
	if (!Initialize()) {
		return false;
	}

	const std::wstring text = desc.text.empty() ? L" " : desc.text;
	const float layoutWidth = desc.areaSize.x > 0.0f ? desc.areaSize.x : 4096.0f;
	const float layoutHeight = desc.areaSize.y > 0.0f ? desc.areaSize.y : 4096.0f;

	Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
	HRESULT hr = writeFactory_->CreateTextFormat(
		desc.fontFamily.c_str(),
		nullptr,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		desc.fontSize,
		L"ja-jp",
		textFormat.GetAddressOf());
	if (FAILED(hr)) {
		LogIfFailed("[Engine] TextFormatの作成に失敗しました。", hr);
		return false;
	}

	textFormat->SetTextAlignment(ToDWriteTextAlignment(desc.horizontalAlign));
	textFormat->SetParagraphAlignment(ToDWriteParagraphAlignment(desc.verticalAlign));
	textFormat->SetWordWrapping(desc.wordWrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
	if (desc.lineSpacing > 0.0f && desc.lineSpacing != 1.0f) {
		const float lineSpacing = desc.fontSize * desc.lineSpacing;
		const float baseline = lineSpacing * 0.8f;
		hr = textFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, lineSpacing, baseline);
		if (FAILED(hr)) {
			LogIfFailed("[Engine] Textの行間設定に失敗しました。", hr);
			return false;
		}
	}

	Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
	hr = writeFactory_->CreateTextLayout(
		text.c_str(),
		static_cast<UINT32>(text.size()),
		textFormat.Get(),
		layoutWidth,
		layoutHeight,
		textLayout.GetAddressOf());
	if (FAILED(hr)) {
		LogIfFailed("[Engine] TextLayoutの作成に失敗しました。", hr);
		return false;
	}

	if (desc.characterSpacing != 0.0f) {
		Microsoft::WRL::ComPtr<IDWriteTextLayout1> textLayout1;
		hr = textLayout.As(&textLayout1);
		if (FAILED(hr)) {
			LogIfFailed("[Engine] Textの文字間隔設定に必要なTextLayout1の取得に失敗しました。", hr);
			return false;
		}

		DWRITE_TEXT_RANGE range{};
		range.startPosition = 0;
		range.length = static_cast<UINT32>(text.size());
		hr = textLayout1->SetCharacterSpacing(0.0f, desc.characterSpacing, 0.0f, range);
		if (FAILED(hr)) {
			LogIfFailed("[Engine] Textの文字間隔設定に失敗しました。", hr);
			return false;
		}
	}

	uint32_t width = 1;
	uint32_t height = 1;
	ResolveTextureSize(textLayout.Get(), desc.areaSize, width, height);
	textLayout->SetMaxWidth(static_cast<float>(width));
	textLayout->SetMaxHeight(static_cast<float>(height));

	Microsoft::WRL::ComPtr<IWICBitmap> bitmap;
	hr = wicFactory_->CreateBitmap(
		width,
		height,
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapCacheOnLoad,
		bitmap.GetAddressOf());
	if (FAILED(hr)) {
		LogIfFailed("[Engine] Text用WIC Bitmapの作成に失敗しました。", hr);
		return false;
	}

	D2D1_RENDER_TARGET_PROPERTIES renderTargetProps = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	Microsoft::WRL::ComPtr<ID2D1RenderTarget> renderTarget;
	hr = d2dFactory_->CreateWicBitmapRenderTarget(
		bitmap.Get(),
		renderTargetProps,
		renderTarget.GetAddressOf());
	if (FAILED(hr)) {
		LogIfFailed("[Engine] Text用RenderTargetの作成に失敗しました。", hr);
		return false;
	}

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
	hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), brush.GetAddressOf());
	if (FAILED(hr)) {
		LogIfFailed("[Engine] Text用Brushの作成に失敗しました。", hr);
		return false;
	}

	renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	renderTarget->BeginDraw();
	renderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
	renderTarget->DrawTextLayout(D2D1::Point2F(0.0f, 0.0f), textLayout.Get(), brush.Get());
	hr = renderTarget->EndDraw();
	if (FAILED(hr)) {
		LogIfFailed("[Engine] Text描画に失敗しました。", hr);
		return false;
	}

	const uint32_t stride = width * 4;
	std::vector<uint8_t> bgraPixels(static_cast<size_t>(stride) * height);
	hr = bitmap->CopyPixels(nullptr, stride, static_cast<UINT>(bgraPixels.size()), bgraPixels.data());
	if (FAILED(hr)) {
		LogIfFailed("[Engine] Textピクセルの取得に失敗しました。", hr);
		return false;
	}

	if (!ConvertPremultipliedBgraToRgba(bgraPixels, width, height, outPixels.pixels)) {
		return false;
	}

	outPixels.width = width;
	outPixels.height = height;
	return true;
}

void TextTextureGenerator::ResolveTextureSize(
	IDWriteTextLayout* textLayout,
	const Vector2& requestedSize,
	uint32_t& outWidth,
	uint32_t& outHeight) const {
	DWRITE_TEXT_METRICS metrics{};
	textLayout->GetMetrics(&metrics);

	const float measuredWidth = (std::max)(metrics.widthIncludingTrailingWhitespace, metrics.width);
	const float measuredHeight = metrics.height;

	const float resolvedWidth = requestedSize.x > 0.0f ? requestedSize.x : measuredWidth;
	const float resolvedHeight = requestedSize.y > 0.0f ? requestedSize.y : measuredHeight;

	outWidth = std::max<uint32_t>(1, static_cast<uint32_t>(std::ceil(resolvedWidth)));
	outHeight = std::max<uint32_t>(1, static_cast<uint32_t>(std::ceil(resolvedHeight)));
}

bool TextTextureGenerator::ConvertPremultipliedBgraToRgba(
	const std::vector<uint8_t>& premultipliedBGRA,
	uint32_t width,
	uint32_t height,
	std::vector<uint8_t>& outRGBA) const {
	const size_t pixelCount = static_cast<size_t>(width) * height;
	if (premultipliedBGRA.size() < pixelCount * 4) {
		Logger::Output("[Engine] Textピクセル変換に失敗しました。入力サイズが不足しています。", Logger::Level::Error);
		return false;
	}

	outRGBA.resize(pixelCount * 4);
	for (size_t i = 0; i < pixelCount; ++i) {
		const uint8_t b = premultipliedBGRA[i * 4 + 0];
		const uint8_t g = premultipliedBGRA[i * 4 + 1];
		const uint8_t r = premultipliedBGRA[i * 4 + 2];
		const uint8_t a = premultipliedBGRA[i * 4 + 3];

		if (a == 0) {
			outRGBA[i * 4 + 0] = 255;
			outRGBA[i * 4 + 1] = 255;
			outRGBA[i * 4 + 2] = 255;
			outRGBA[i * 4 + 3] = 0;
			continue;
		}

		const float invAlpha = 255.0f / static_cast<float>(a);
		outRGBA[i * 4 + 0] = static_cast<uint8_t>((std::min)(255.0f, std::round(static_cast<float>(r) * invAlpha)));
		outRGBA[i * 4 + 1] = static_cast<uint8_t>((std::min)(255.0f, std::round(static_cast<float>(g) * invAlpha)));
		outRGBA[i * 4 + 2] = static_cast<uint8_t>((std::min)(255.0f, std::round(static_cast<float>(b) * invAlpha)));
		outRGBA[i * 4 + 3] = a;
	}

	return true;
}

} // namespace MadoEngine
