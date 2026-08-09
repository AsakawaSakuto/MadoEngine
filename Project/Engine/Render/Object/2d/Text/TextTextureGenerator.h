#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <dwrite_1.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include "Math/Vector2.h"

namespace MadoEngine {

enum class TextHorizontalAlign {
	Left,
	Center,
	Right,
};

enum class TextVerticalAlign {
	Top,
	Center,
	Bottom,
};

/// @brief Assets/Fontから読み込んだフォント情報
struct TextFontAsset {
	std::string filePath;
	std::string displayName;
	std::string familyName;
};

/// @brief DirectWriteで生成するTextテクスチャの設定
struct TextTextureDesc {
	std::wstring text = L"Text";
	std::wstring fontFamily = L"Yu Gothic UI";
	std::string fontFilePath;
	float fontSize = 24.0f;
	float lineSpacing = 1.0f;
	float characterSpacing = 0.0f;
	Vector2 areaSize = { 0.0f, 0.0f };
	TextHorizontalAlign horizontalAlign = TextHorizontalAlign::Left;
	TextVerticalAlign verticalAlign = TextVerticalAlign::Top;
	bool wordWrap = true;
};

/// @brief DirectWriteで描画した文字列をRGBAピクセルへ変換した結果
struct TextTexturePixels {
	std::vector<uint8_t> pixels;
	uint32_t width = 1;
	uint32_t height = 1;
};

/// @brief DirectWrite、Direct2D、WICを使ってText用のRGBAテクスチャデータを生成
class TextTextureGenerator {
public:
	/// @brief シングルトンインスタンスを取得
	/// @return TextTextureGeneratorのインスタンス
	static TextTextureGenerator& GetInstance();

	/// @brief DirectWrite関連リソースを初期化
	/// @return 初期化に成功した場合はtrue
	bool Initialize();

	/// @brief DirectWrite関連リソースを解放
	void Finalize();

	/// @brief Text設定からRGBAピクセルを生成
	/// @param desc 生成に使用するText設定
	/// @param outPixels 生成されたRGBAピクセル
	/// @return 生成に成功した場合はtrue
	bool Generate(const TextTextureDesc& desc, TextTexturePixels& outPixels);

	/// @brief Assets/Fontから読み込んだフォント一覧を取得
	/// @return 選択可能なフォント情報一覧
	const std::vector<TextFontAsset>& GetFontAssets() const { return fontAssets_; }

private:
	TextTextureGenerator() = default;
	~TextTextureGenerator() = default;

	TextTextureGenerator(const TextTextureGenerator&) = delete;
	TextTextureGenerator& operator=(const TextTextureGenerator&) = delete;

	/// @brief 指定ディレクトリ内のフォントファイルを読み込む
	/// @param fontDirectory フォントを検索するディレクトリ
	void LoadFontAssets(const std::filesystem::path& fontDirectory);

	/// @brief フォントファイルから専用DirectWriteコレクションを生成する
	/// @param filePath 読み込むフォントファイルの絶対パス
	/// @param outCollection 生成されたフォントコレクション
	/// @param outFamilyName フォントファミリー名
	/// @return 読み込みに成功した場合はtrue
	bool CreateFontAssetCollection(
		const std::filesystem::path& filePath,
		Microsoft::WRL::ComPtr<IDWriteFontCollection1>& outCollection,
		std::wstring& outFamilyName) const;

	/// @brief TextLayoutから必要なテクスチャサイズを計算
	/// @param textLayout 計測対象のTextLayout
	/// @param requestedSize JsonやEditorから指定された表示領域
	/// @param outWidth 出力するテクスチャ幅
	/// @param outHeight 出力するテクスチャ高さ
	void ResolveTextureSize(
		IDWriteTextLayout* textLayout,
		const Vector2& requestedSize,
		uint32_t& outWidth,
		uint32_t& outHeight) const;

	/// @brief WICのPBGRAピクセルをSprite描画向けのストレートRGBAへ変換
	/// @param premultipliedBGRA WICから取得したPBGRAピクセル
	/// @param width ピクセル幅
	/// @param height ピクセル高さ
	/// @param outRGBA 変換後のRGBAピクセル
	bool ConvertPremultipliedBgraToRgba(
		const std::vector<uint8_t>& premultipliedBGRA,
		uint32_t width,
		uint32_t height,
		std::vector<uint8_t>& outRGBA) const;

	Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory_;
	Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;
	Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory_;
	std::vector<TextFontAsset> fontAssets_;
	std::vector<std::wstring> fontAssetFamilyNames_;
	std::vector<Microsoft::WRL::ComPtr<IDWriteFontCollection1>> fontAssetCollections_;
	bool isInitialized_ = false;
	bool didInitializeCom_ = false;
};

} // namespace MadoEngine
