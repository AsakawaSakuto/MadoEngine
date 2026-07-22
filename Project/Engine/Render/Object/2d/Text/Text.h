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
/// @return 対応するフォント種別。候補外の場合はTextFontFamilyType::Invalid
TextFontFamilyType GetTextFontFamilyTypeFromName(const std::string& fontFamily);

/// @brief DirectWriteで生成したテクスチャを2D空間へ描画するTextオブジェクト
class Text : public IRenderObject2d, public Json::IJsonSerializable {
public:
	explicit Text(std::string objectName = "Text");

	/// @brief 単体使用向けにTextを初期化
	/// @param device D3D12デバイス
	/// @param commandList コマンドリスト
	/// @param textureName 未使用です
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

	/// @brief 表示文字列を設定
	/// @param text 表示するUTF-8文字列
	void SetText(const std::string& text);

	/// @brief 表示文字列を取得
	/// @return 表示中のUTF-8文字列
	const std::string& GetText() const { return text_; }

	/// @brief 使用フォント名を設定
	/// @param fontFamily DirectWriteで使用するフォントファミリー名
	void SetFontFamily(const std::string& fontFamily);

	/// @brief 使用フォント名を取得
	/// @return フォントファミリー名
	const std::string& GetFontFamily() const { return fontFamily_; }

	/// @brief フォントサイズを設定
	/// @param fontSize フォントサイズ
	void SetFontSize(float fontSize);

	/// @brief フォントサイズを取得
	/// @return フォントサイズ
	float GetFontSize() const { return fontSize_; }

	/// @brief 行間倍率を設定
	/// @param lineSpacing 行間倍率。1.0fで標準
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
	/// @param size 表示領域のピクセルサイズ。0以下の場合は文字列から自動計測
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

	/// @brief TextManagerが管理する識別名と動的テクスチャキーを更新する
	/// @param objectName 新しい識別名
	void SetObjectName(const std::string& objectName);

	/// @brief 共通初期化を行う
	void InitializeCommonResources();

	/// @brief Textテクスチャの再生成が必要な状態にする
	void MarkDirty() { isTextureDirty_ = true; }

	/// @brief 必要であればTextテクスチャを再生成する
	void RebuildTextureIfNeeded();

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
