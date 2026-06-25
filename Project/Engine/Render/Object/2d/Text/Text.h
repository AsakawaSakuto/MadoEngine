#pragma once
#include "Render/Object/IRenderObject2d.h"
#include "Render/Object/2d/Sprite/SpriteData.h"
#include "Render/Object/2d/Sprite/SpriteSharedGeometry.h"
#include "Render/Object/2d/Text/TextTextureGenerator.h"
#include "Utility/Json/Core/IJsonSerializable.h"
#include ".SceneManager/SceneType.h"
#include "TextFont.h"
#include <string>

namespace MadoEngine {

/// @brief Textフォント種別の表示名を取得します。
/// @param type フォント種別。
/// @return Editor表示用のフォント名。
const char* GetTextFontDisplayName(TextFontFamilyType type);

/// @brief Textフォント種別に対応するDirectWriteフォントファミリー名を取得します。
/// @param type フォント種別。
/// @return DirectWriteへ渡すフォントファミリー名。
const char* GetTextFontFamilyName(TextFontFamilyType type);

/// @brief フォントファミリー名からTextフォント種別を取得します。
/// @param fontFamily DirectWriteフォントファミリー名。
/// @return 対応するフォント種別。候補外の場合はTextFontFamilyType::Count。
TextFontFamilyType GetTextFontFamilyTypeFromName(const std::string& fontFamily);

/// @brief DirectWriteで生成したテクスチャを2D空間へ描画するTextオブジェクトです。
class Text : public IRenderObject2d, public Json::IJsonSerializable {
public:
	explicit Text(std::string objectName = "Text");

	/// @brief 単体使用向けにTextを初期化します。
	/// @param device D3D12デバイス。
	/// @param commandList コマンドリスト。
	/// @param textureName 未使用です。
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName) override;

	/// @brief TextManager経由の共有ジオメトリでTextを初期化します。
	/// @param device D3D12デバイス。
	/// @param commandList コマンドリスト。
	/// @param sharedGeo Spriteと共通形状の頂点/インデックスバッファ。
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const SpriteSharedGeometry& sharedGeo);

	/// @brief Textの状態を更新します。
	void Update() override;

	/// @brief Textを描画します。
	void Draw() override;

	/// @brief Text用に生成した動的テクスチャを解放します。
	void ReleaseTexture();

	/// @brief 表示文字列を設定します。
	/// @param text 表示するUTF-8文字列。
	void SetText(const std::string& text);

	/// @brief 表示文字列を取得します。
	/// @return 表示中のUTF-8文字列。
	const std::string& GetText() const { return text_; }

	/// @brief 使用フォント名を設定します。
	/// @param fontFamily DirectWriteで使用するフォントファミリー名。
	void SetFontFamily(const std::string& fontFamily);

	/// @brief 使用フォント名を取得します。
	/// @return フォントファミリー名。
	const std::string& GetFontFamily() const { return fontFamily_; }

	/// @brief フォントサイズを設定します。
	/// @param fontSize フォントサイズ。
	void SetFontSize(float fontSize);

	/// @brief フォントサイズを取得します。
	/// @return フォントサイズ。
	float GetFontSize() const { return fontSize_; }

	/// @brief 行間倍率を設定します。
	/// @param lineSpacing 行間倍率。1.0fで標準です。
	void SetLineSpacing(float lineSpacing);

	/// @brief 行間倍率を取得します。
	/// @return 行間倍率。
	float GetLineSpacing() const { return lineSpacing_; }

	/// @brief 文字間隔を設定します。
	/// @param characterSpacing 文字間隔の追加量。ピクセル単位です。
	void SetCharacterSpacing(float characterSpacing);

	/// @brief 文字間隔を取得します。
	/// @return 文字間隔の追加量。ピクセル単位です。
	float GetCharacterSpacing() const { return characterSpacing_; }

	/// @brief Textの表示領域を設定します。
	/// @param size 表示領域のピクセルサイズ。0以下の場合は文字列から自動計測します。
	void SetAreaSize(const Vector2& size);

	/// @brief Textの表示領域を取得します。
	/// @return 表示領域のピクセルサイズ。
	const Vector2& GetAreaSize() const { return areaSize_; }

	/// @brief Textのアンカーポイントを設定します。
	/// @param anchorPoint 正規化されたアンカーポイントです。左上が{0, 0}、中央が{0.5, 0.5}、右下が{1, 1}です。
	void SetAnchorPoint(const Vector2& anchorPoint);

	/// @brief Textのアンカーポイントを取得します。
	/// @return 正規化されたアンカーポイントです。
	const Vector2& GetAnchorPoint() const { return anchorPoint_; }

	/// @brief 水平方向の配置を設定します。
	/// @param align 水平方向の配置。
	void SetHorizontalAlign(TextHorizontalAlign align);

	/// @brief 水平方向の配置を取得します。
	/// @return 水平方向の配置。
	TextHorizontalAlign GetHorizontalAlign() const { return horizontalAlign_; }

	/// @brief 垂直方向の配置を設定します。
	/// @param align 垂直方向の配置。
	void SetVerticalAlign(TextVerticalAlign align);

	/// @brief 垂直方向の配置を取得します。
	/// @return 垂直方向の配置。
	TextVerticalAlign GetVerticalAlign() const { return verticalAlign_; }

	/// @brief 自動折り返しを設定します。
	/// @param enabled 有効にする場合はtrue。
	void SetWordWrap(bool enabled);

	/// @brief 自動折り返しが有効か取得します。
	/// @return 有効な場合はtrue。
	bool IsWordWrapEnabled() const { return wordWrap_; }

	/// @brief 描画対象Sceneを設定します。
	/// @param sceneType 描画対象Scene。
	void SetSceneType(SceneType sceneType) { sceneType_ = sceneType; }

	/// @brief 描画対象Sceneを取得します。
	/// @return 描画対象Scene。
	SceneType GetSceneType() const { return sceneType_; }

	/// @brief 描画先Screen名を設定します。
	/// @param targetScreen 描画先Screen名。
	void SetTargetScreen(const std::string& targetScreen) { targetScreen_ = targetScreen; }

	/// @brief 描画先Screen名を取得します。
	/// @return 描画先Screen名。
	const std::string& GetTargetScreen() const { return targetScreen_; }

	/// @brief JsonからTextの状態を復元します。
	/// @param json 読み込むJson。
	void FromJson(const nlohmann::json& json) override;

	/// @brief Textの状態をJsonへ変換します。
	/// @return 変換されたJson。
	nlohmann::json ToJson() const override;

private:
	/// @brief 共通初期化を行います。
	void InitializeCommonResources();

	/// @brief Textテクスチャの再生成が必要な状態にします。
	void MarkDirty() { isTextureDirty_ = true; }

	/// @brief 必要であればTextテクスチャを再生成します。
	void RebuildTextureIfNeeded();

	/// @brief UTF-8文字列をUTF-16文字列へ変換します。
	/// @param text UTF-8文字列。
	/// @return UTF-16文字列。
	static std::wstring Utf8ToWide(const std::string& text);

	/// @brief 水平方向配置を文字列へ変換します。
	/// @param align 水平方向配置。
	/// @return 文字列化された配置名。
	static std::string HorizontalAlignToString(TextHorizontalAlign align);

	/// @brief 文字列から水平方向配置へ変換します。
	/// @param value 文字列化された配置名。
	/// @return 水平方向配置。
	static TextHorizontalAlign HorizontalAlignFromString(const std::string& value);

	/// @brief 垂直方向配置を文字列へ変換します。
	/// @param align 垂直方向配置。
	/// @return 文字列化された配置名。
	static std::string VerticalAlignToString(TextVerticalAlign align);

	/// @brief 文字列から垂直方向配置へ変換します。
	/// @param value 文字列化された配置名。
	/// @return 垂直方向配置。
	static TextVerticalAlign VerticalAlignFromString(const std::string& value);

	/// @brief RenderLayerを文字列へ変換します。
	/// @param layer RenderLayer。
	/// @return 文字列化されたRenderLayer名。
	static std::string RenderLayerToString(Render::RenderLayer layer);

	/// @brief 文字列からRenderLayerへ変換します。
	/// @param value 文字列化されたRenderLayer名。
	/// @return RenderLayer。
	static Render::RenderLayer RenderLayerFromString(const std::string& value);

	const D3D12_VERTEX_BUFFER_VIEW* activeVBV_ = nullptr;
	const D3D12_INDEX_BUFFER_VIEW* activeIBV_ = nullptr;

	SpriteMaterial* materialData_ = nullptr;
	SpriteTransformationMatrix* transformationData_ = nullptr;

	std::string text_ = "Text";
	std::string fontFamily_ = "Yu Gothic UI";
	float fontSize_ = 24.0f;
	float lineSpacing_ = 1.0f;
	float characterSpacing_ = 0.0f;
	Vector2 areaSize_ = { 0.0f, 0.0f };
	Vector2 anchorPoint_ = { 0.0f, 0.0f };
	TextHorizontalAlign horizontalAlign_ = TextHorizontalAlign::Left;
	TextVerticalAlign verticalAlign_ = TextVerticalAlign::Top;
	bool wordWrap_ = true;
	bool isTextureDirty_ = true;

	std::string textureKey_;
	std::string targetScreen_ = "BackBuffer";
	SceneType sceneType_ = SceneType::None;
};

} // namespace MadoEngine
