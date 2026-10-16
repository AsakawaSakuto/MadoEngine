#pragma once
#include "Render/Object/IRenderObject2d.h"
#include "Render/Object/2d/Sprite/SpriteData.h"
#include "Render/Object/2d/Sprite/SpriteSharedGeometry.h"
#include "Render/Object/2d/Text/TextTextureGenerator.h"
#include "Utility/Json/Core/IJsonSerializable.h"
#include ".SceneManager/SceneType.h"
#include "TextFont.h"
#include "TextData.h"
#include <span>
#include <string>

namespace MadoEngine {

struct TextAtlasResource;

/// @brief Textの文字画像の生成と描画の方式
enum class TextRenderMode {
	Rasterized,
	NumericAtlas,
};

/// @brief Textフォント種別の表示名を取得
/// @param type フォント種別
/// @return Editor表示用のフォント名
const char* GetTextFontDisplayName(TextFontFamilyType type);

/// @brief Textフォント種別に対応するDirectWriteフォントファミリー名を取得
/// @param type フォント種別
/// @return DirectWriteへ渡すフォントファミリー名
const char* GetTextFontFamilyName(TextFontFamilyType type);

/// @brief フォントファミリー名からTextフォント種別を取得
/// @param fontFamily DirectWriteフォントファミリー名
/// @return 対応するフォント種別、候補外の場合はTextFontFamilyType::Invalid
TextFontFamilyType GetTextFontFamilyTypeFromName(const std::string& fontFamily);

/// @brief DirectWriteで生成したテクスチャを2D空間へ描画するTextオブジェクト
class Text : public IRenderObject2d, public Json::IJsonSerializable {
public:
	explicit Text(std::string objectName = "Text");

	/// @brief 単体使用向けにTextを初期化
	/// @param device D3D12デバイス
	/// @param commandList コマンドリスト
	/// @param textureName 未使用
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName) override;

	/// @brief TextManager経由の共有ジオメトリでTextを初期化
	/// @param device D3D12デバイス
	/// @param commandList コマンドリスト
	/// @param sharedGeo Spriteと共通形状の頂点/インデックスバッファ
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const SpriteSharedGeometry& sharedGeo);

	/// @brief Textの状態を更新
	void Update() override;

	/// @brief Textを描画
	void Draw() override;

	/// @brief Text用に生成した動的テクスチャを解放
	void ReleaseTexture();

	/// @brief 文字列の描画方式を設定
	/// @param mode 数字アトラスまたは全文字列画像の描画方式
	void SetRenderMode(TextRenderMode mode);

	/// @brief 指定された文字列の描画方式を取得
	/// @return 現在指定されている描画方式
	TextRenderMode GetRenderMode() const { return renderMode_; }

	/// @brief アトラスを画像化する基準フォントサイズを設定
	/// @param fontSize 共有画像の一文字を生成する基準サイズ
	void SetAtlasRasterSize(float fontSize);

	/// @brief アトラスを画像化する基準フォントサイズを取得
	/// @return 共有画像の基準フォントサイズ
	float GetAtlasRasterSize() const { return atlasRasterSize_; }

	/// @brief 通常文字列と数字アトラスのSDF描画を設定
	/// @param enabled 単一チャンネルSDFで描画する場合はtrue
	void SetSdfEnabled(bool enabled);

	/// @brief SDF描画の指定状態を取得
	/// @return SDF描画を指定している場合はtrue
	bool IsSdfEnabled() const { return sdfEnabled_; }

	/// @brief SDFアウトラインの色を設定
	/// @param color 縁のRGBA色、Alphaへ文字全体の透明度を乗算
	void SetOutlineColor(const Vector4& color) { outlineColor_ = color; }

	/// @brief SDFアウトラインの色を取得
	/// @return 縁のRGBA色
	const Vector4& GetOutlineColor() const { return outlineColor_; }

	/// @brief SDFアウトラインの画面上の太さを設定
	/// @param width 画面Pixel単位の縁幅、生成済み距離範囲内に制限
	void SetOutlineWidth(float width);

	/// @brief SDFアウトラインの指定幅を取得
	/// @return 画面Pixel単位の縁幅
	float GetOutlineWidth() const { return outlineWidth_; }

	/// @brief 非表示中も描画リソースを事前構築
	/// @return 描画用Textureの準備に成功した場合はtrue
	bool PrepareRendering();

	/// @brief 表示文字列を設定
	/// @param text 表示するUTF-8文字列
	void SetText(const std::string& text);

	/// @brief 表示文字列を取得
	/// @return 表示中のUTF-8文字列
	const std::string& GetText() const { return text_; }

	/// @brief SDF用の余白を除くTextの表示サイズを取得
	/// @return 表示倍率適用前の文字列の表示領域サイズ
	const Vector2& GetTextureSize() const { return size_; }

	/// @brief 使用フォント名を設定
	/// @param fontFamily DirectWriteで使用するフォントファミリー名
	void SetFontFamily(const std::string& fontFamily);

	/// @brief Assets/Fontから読み込んだフォントを設定
	/// @param fontFilePath Assets/Fontを基準としたフォントファイルパス
	/// @param fontFamily DirectWriteで使用するフォントファミリー名
	void SetFontAsset(const std::string& fontFilePath, const std::string& fontFamily);

	/// @brief 使用フォント名を取得
	/// @return フォントファミリー名
	const std::string& GetFontFamily() const { return fontFamily_; }

	/// @brief 使用中のアセットフォントファイルパスを取得
	/// @return アセットフォントを使用しない場合は空文字列
	const std::string& GetFontFilePath() const { return fontFilePath_; }

	/// @brief フォントサイズを設定
	/// @param fontSize フォントサイズ
	void SetFontSize(float fontSize);

	/// @brief フォントサイズを取得
	/// @return フォントサイズ
	float GetFontSize() const { return fontSize_; }

	/// @brief 行間倍率を設定
	/// @param lineSpacing 行間倍率、1.0fで標準
	void SetLineSpacing(float lineSpacing);

	/// @brief 行間倍率を取得
	/// @return 行間倍率
	float GetLineSpacing() const { return lineSpacing_; }

	/// @brief 文字間隔を設定
	/// @param characterSpacing 文字間隔の追加量ピクセル単位
	void SetCharacterSpacing(float characterSpacing);

	/// @brief 文字間隔を取得
	/// @return 文字間隔の追加量ピクセル単位
	float GetCharacterSpacing() const { return characterSpacing_; }

	/// @brief Textの表示領域を設定
	/// @param size 表示領域のピクセルサイズ、0以下の場合は文字列から自動計測
	void SetAreaSize(const Vector2& size);

	/// @brief Textの表示領域を取得
	/// @return 表示領域のピクセルサイズ
	const Vector2& GetAreaSize() const { return areaSize_; }

	/// @brief Textのアンカーポイントを設定
	/// @param anchorPoint 正規化されたアンカーポイント 左上が{0, 0}、中央が{0.5, 0.5}、右下が{1, 1}
	void SetAnchorPoint(const Vector2& anchorPoint);

	/// @brief Textのアンカーポイントを取得
	/// @return 正規化されたアンカーポイント 左上が{0, 0}、中央が{0.5, 0.5}、右下が{1, 1}
	const Vector2& GetAnchorPoint() const { return anchorPoint_; }

	/// @brief 水平方向の配置を設定
	/// @param align 水平方向の配置
	void SetHorizontalAlign(TextHorizontalAlign align);

	/// @brief 水平方向の配置を取得
	/// @return 水平方向の配置
	TextHorizontalAlign GetHorizontalAlign() const { return horizontalAlign_; }

	/// @brief 垂直方向の配置を設定
	/// @param align 垂直方向の配置
	void SetVerticalAlign(TextVerticalAlign align);

	/// @brief 垂直方向の配置を取得
	/// @return 垂直方向の配置
	TextVerticalAlign GetVerticalAlign() const { return verticalAlign_; }

	/// @brief 自動折り返しを設定
	/// @param enabled 有効にする場合はtrue
	void SetWordWrap(bool enabled);

	/// @brief 自動折り返しが有効か取得
	/// @return 有効な場合はtrue
	bool IsWordWrapEnabled() const { return wordWrap_; }

	/// @brief 描画対象Sceneを設定
	/// @param sceneType 描画対象Scene
	void SetSceneType(SceneType sceneType) { sceneType_ = sceneType; }

	/// @brief 描画対象Sceneを取得
	/// @return 描画対象Scene
	SceneType GetSceneType() const { return sceneType_; }

	/// @brief 描画先Screen名を設定
	/// @param targetScreen 描画先Screen名
	void SetTargetScreen(const std::string& targetScreen) { targetScreen_ = targetScreen; }

	/// @brief 描画先Screen名を取得
	/// @return 描画先Screen名
	const std::string& GetTargetScreen() const { return targetScreen_; }

	/// @brief JsonからTextの状態を復元
	/// @param json 読み込むJson
	void FromJson(const nlohmann::json& json) override;

	/// @brief Textの状態をJsonへ変換
	/// @return 変換されたJson
	nlohmann::json ToJson() const override;

private:
	friend class TextManager;

	/// @brief TextManagerが管理する識別名と動的テクスチャキーを更新
	/// @param objectName 新しい識別名
	/// @return 識別名と動的テクスチャキーを更新できた場合はtrue
	bool SetObjectName(const std::string& objectName);

	/// @brief 共通初期化
	void InitializeCommonResources();

	/// @brief Textテクスチャの再生成要求を設定
	void MarkDirty() { isTextureDirty_ = true; }

	/// @brief 必要であればTextテクスチャを再生成
	void RebuildTextureIfNeeded();

	/// @brief 共有アトラスと確保済み頂点から文字列全体の描画を準備
	/// @return 数字アトラス方式で準備できた場合はtrue
	bool RebuildAtlasGeometry();

	/// @brief 余白付きSDF文字列画像の描画頂点を構築
	/// @param textureSize 余白込みの画像Pixel数
	/// @param rasterScale 画像化時の拡大率
	void RebuildSdfQuad(const Vector2& textureSize, float rasterScale);

	/// @brief 固定頂点領域を初回確保して描画頂点を更新
	/// @param vertices 更新する文字列の頂点
	void UpdateTextVertices(std::span<const SpriteVertexData> vertices);

	/// @brief UTF-8文字列をUTF-16文字列へ変換
	/// @param text UTF-8文字列
	/// @return UTF-16文字列
	static std::wstring Utf8ToWide(const std::string& text);

	/// @brief 水平方向配置を文字列へ変換
	/// @param align 水平方向配置
	/// @return 文字列化された配置名
	static std::string HorizontalAlignToString(TextHorizontalAlign align);

	/// @brief 文字列から水平方向配置へ変換
	/// @param value 文字列化された配置名
	/// @return 水平方向配置
	static TextHorizontalAlign HorizontalAlignFromString(const std::string& value);

	/// @brief 垂直方向配置を文字列へ変換
	/// @param align 垂直方向配置
	/// @return 文字列化された配置名
	static std::string VerticalAlignToString(TextVerticalAlign align);

	/// @brief 文字列から垂直方向配置へ変換
	/// @param value 文字列化された配置名
	/// @return 垂直方向配置
	static TextVerticalAlign VerticalAlignFromString(const std::string& value);

	const D3D12_VERTEX_BUFFER_VIEW* activeVBV_ = nullptr;
	const D3D12_INDEX_BUFFER_VIEW* activeIBV_ = nullptr;

	TextMaterial* materialData_ = nullptr;
	SpriteTransformationMatrix* transformationData_ = nullptr;

	std::string text_ = "Text";
	std::string fontFamily_ = "Yu Gothic UI";
	std::string fontFilePath_;
	float fontSize_ = 24.0f;
	float lineSpacing_ = 1.0f;
	float characterSpacing_ = 0.0f;
	Vector2 areaSize_ = { 0.0f, 0.0f };
	Vector2 anchorPoint_ = { 0.0f, 0.0f };
	TextHorizontalAlign horizontalAlign_ = TextHorizontalAlign::Left;
	TextVerticalAlign verticalAlign_ = TextVerticalAlign::Top;
	bool wordWrap_ = true;
	bool isTextureDirty_ = true;
	bool hasOwnedTexture_ = false;
	bool usesAtlas_ = false;
	bool sdfEnabled_ = false;
	bool usesSdf_ = false;
	Vector4 outlineColor_ = { 0.0f, 0.0f, 0.0f, 1.0f };
	float outlineWidth_ = 0.0f;
	Vector2 texturePixelSize_ = {};
	TextRenderMode renderMode_ = TextRenderMode::Rasterized;
	float atlasRasterSize_ = 128.0f;
	const TextAtlasResource* atlas_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> textVertexResource_;
	SpriteVertexData* textVertices_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW textVertexBufferView_{};
	uint32_t textVertexCount_ = 0;

	std::string textureKey_;
	std::string targetScreen_ = "BackBuffer";
	SceneType sceneType_ = SceneType::None;
};

} // namespace MadoEngine
