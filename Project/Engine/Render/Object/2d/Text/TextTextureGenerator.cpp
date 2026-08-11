#include "TextTextureGenerator.h"
#include "Utility/Logger/Logger.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <comdef.h>
#include <cwctype>
#include <format>

namespace MadoEngine {

namespace {
	/// @brief UTF-16文字列をUTF-8文字列へ変換
	/// @param text 変換するUTF-16文字列
	/// @return 変換されたUTF-8文字列
	std::string WideToUtf8(const std::wstring& text) {
		if (text.empty()) {
			return {};
		}

		const int requiredSize = WideCharToMultiByte(
			CP_UTF8,
			0,
			text.c_str(),
			static_cast<int>(text.size()),
			nullptr,
			0,
			nullptr,
			nullptr);
		if (requiredSize <= 0) {
			return {};
		}

		std::string result(static_cast<size_t>(requiredSize), '\0');
		WideCharToMultiByte(
			CP_UTF8,
			0,
			text.c_str(),
			static_cast<int>(text.size()),
			result.data(),
			requiredSize,
			nullptr,
			nullptr);
		return result;
	}

	/// @brief ローカライズ済み文字列から表示用文字列を取得
	/// @param localizedStrings DirectWriteのローカライズ済み文字列
	/// @return 日本語、英語、先頭要素の優先順で取得した文字列
	std::wstring GetLocalizedString(IDWriteLocalizedStrings* localizedStrings) {
		if (!localizedStrings || localizedStrings->GetCount() == 0) {
			return {};
		}

		UINT32 index = 0;
		BOOL exists = FALSE;
		localizedStrings->FindLocaleName(L"ja-jp", &index, &exists);
		if (!exists) {
			localizedStrings->FindLocaleName(L"en-us", &index, &exists);
		}
		if (!exists) {
			index = 0;
		}

		UINT32 length = 0;
		if (FAILED(localizedStrings->GetStringLength(index, &length))) {
			return {};
		}

		std::wstring result(static_cast<size_t>(length) + 1, L'\0');
		if (FAILED(localizedStrings->GetString(index, result.data(), length + 1))) {
			return {};
		}
		result.resize(length);
		return result;
	}

	/// @brief Textで使用可能なフォントファイル拡張子か判定
	/// @param extension 判定する拡張子
	/// @return 対応する拡張子の場合はtrue
	bool IsSupportedFontExtension(std::wstring extension) {
		std::transform(extension.begin(), extension.end(), extension.begin(), [](wchar_t character) {
			return static_cast<wchar_t>(std::towlower(character));
		});
		return extension == L".ttf" || extension == L".ttc" || extension == L".otf" || extension == L".otc";
	}

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

	// COM所有時だけFinalizeで解除し、既存Apartmentでは呼び出し側の所有を維持
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

	LoadFontAssets("Assets/Font");

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
	fontAssetCollections_.clear();
	fontAssetFamilyNames_.clear();
	fontAssets_.clear();
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
	IDWriteFontCollection* fontCollection = nullptr;
	const wchar_t* fontFamily = desc.fontFamily.c_str();

	// Asset Font指定時はSystem FontではなくFile単位のCollectionを選択
	if (!desc.fontFilePath.empty()) {
		for (size_t index = 0; index < fontAssets_.size(); ++index) {
			if (fontAssets_[index].filePath == desc.fontFilePath) {
				fontCollection = fontAssetCollections_[index].Get();
				fontFamily = fontAssetFamilyNames_[index].c_str();
				break;
			}
		}
	}

	Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
	HRESULT hr = writeFactory_->CreateTextFormat(
		fontFamily,
		fontCollection,
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

	// Layout計測後に必要最小限のBitmap Sizeへ確定
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

	// 透明背景へ白文字を描き色付けはSprite Material側へ委譲
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

	// WICのPremultiplied BGRAをEngine Texture用のStraight Alpha RGBAへ変換
	if (!ConvertPremultipliedBgraToRgba(bgraPixels, width, height, outPixels.pixels)) {
		return false;
	}

	outPixels.width = width;
	outPixels.height = height;
	return true;
}

void TextTextureGenerator::LoadFontAssets(const std::filesystem::path& fontDirectory) {
	fontAssetCollections_.clear();
	fontAssetFamilyNames_.clear();
	fontAssets_.clear();

	std::error_code error;
	if (!std::filesystem::is_directory(fontDirectory, error)) {
		Logger::Output("[Engine] フォントディレクトリが見つかりません: " + fontDirectory.string(), Logger::Level::Warning);
		return;
	}

	std::vector<std::filesystem::path> fontFilePaths;
	for (std::filesystem::directory_iterator iterator(fontDirectory, error), end; iterator != end; iterator.increment(error)) {
		if (error) {
			break;
		}
		if (!iterator->is_regular_file(error) || !IsSupportedFontExtension(iterator->path().extension().wstring())) {
			continue;
		}
		fontFilePaths.push_back(iterator->path());
	}

	// File Systemの列挙順に依存しない安定したEditor表示順へ整列
	std::sort(fontFilePaths.begin(), fontFilePaths.end());

	for (const std::filesystem::path& fontFilePath : fontFilePaths) {
		const std::filesystem::path absolutePath = std::filesystem::absolute(fontFilePath, error).lexically_normal();
		if (error) {
			Logger::Output("[Engine] フォントファイルの絶対パスを取得できませんでした: " + fontFilePath.string(), Logger::Level::Warning);
			error.clear();
			continue;
		}

		Microsoft::WRL::ComPtr<IDWriteFontCollection1> collection;
		std::wstring familyName;
		if (!CreateFontAssetCollection(absolutePath, collection, familyName)) {
			Logger::Output("[Engine] フォントファイルを読み込めませんでした: " + fontFilePath.string(), Logger::Level::Warning);
			continue;
		}

		std::filesystem::path relativePath = std::filesystem::relative(absolutePath, std::filesystem::current_path(), error);
		if (error) {
			relativePath = fontFilePath.lexically_normal();
			error.clear();
		}

		// Meta DataとDirectWrite Collectionを同じIndexで保持して寿命を同期
		fontAssets_.push_back({
			WideToUtf8(relativePath.generic_wstring()),
			WideToUtf8(fontFilePath.stem().wstring()),
			WideToUtf8(familyName),
		});
		fontAssetFamilyNames_.push_back(std::move(familyName));
		fontAssetCollections_.push_back(std::move(collection));
	}

	Logger::Output(
		"[Engine] Assets/Fontからフォントを読み込みました: " + std::to_string(fontAssets_.size()) + "件",
		Logger::Level::Assets);
}

bool TextTextureGenerator::CreateFontAssetCollection(
	const std::filesystem::path& filePath,
	Microsoft::WRL::ComPtr<IDWriteFontCollection1>& outCollection,
	std::wstring& outFamilyName) const {
	Microsoft::WRL::ComPtr<IDWriteFactory5> writeFactory5;
	HRESULT hr = writeFactory_.As(&writeFactory5);
	if (FAILED(hr)) {
		LogIfFailed("[Engine] DirectWrite Factory5の取得に失敗しました。", hr);
		return false;
	}

	Microsoft::WRL::ComPtr<IDWriteFontFile> fontFile;
	hr = writeFactory_->CreateFontFileReference(filePath.c_str(), nullptr, fontFile.GetAddressOf());
	if (FAILED(hr)) {
		LogIfFailed("[Engine] フォントファイル参照の作成に失敗しました。", hr);
		return false;
	}

	Microsoft::WRL::ComPtr<IDWriteFontSetBuilder1> fontSetBuilder;
	hr = writeFactory5->CreateFontSetBuilder(fontSetBuilder.GetAddressOf());
	if (FAILED(hr)) {
		LogIfFailed("[Engine] フォントセットBuilderの作成に失敗しました。", hr);
		return false;
	}

	hr = fontSetBuilder->AddFontFile(fontFile.Get());
	if (FAILED(hr)) {
		LogIfFailed("[Engine] フォントセットへのファイル追加に失敗しました。", hr);
		return false;
	}

	Microsoft::WRL::ComPtr<IDWriteFontSet> fontSet;
	hr = fontSetBuilder->CreateFontSet(fontSet.GetAddressOf());
	if (FAILED(hr)) {
		LogIfFailed("[Engine] フォントセットの作成に失敗しました。", hr);
		return false;
	}

	hr = writeFactory5->CreateFontCollectionFromFontSet(fontSet.Get(), outCollection.ReleaseAndGetAddressOf());
	if (FAILED(hr) || outCollection->GetFontFamilyCount() == 0) {
		LogIfFailed("[Engine] フォントコレクションの作成に失敗しました。", FAILED(hr) ? hr : E_FAIL);
		return false;
	}

	Microsoft::WRL::ComPtr<IDWriteFontFamily> fontFamily;
	hr = outCollection->GetFontFamily(0, fontFamily.GetAddressOf());
	if (FAILED(hr)) {
		LogIfFailed("[Engine] フォントファミリーの取得に失敗しました。", hr);
		return false;
	}

	Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> familyNames;
	hr = fontFamily->GetFamilyNames(familyNames.GetAddressOf());
	if (FAILED(hr)) {
		LogIfFailed("[Engine] フォントファミリー名の取得に失敗しました。", hr);
		return false;
	}

	outFamilyName = GetLocalizedString(familyNames.Get());
	return !outFamilyName.empty();
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
