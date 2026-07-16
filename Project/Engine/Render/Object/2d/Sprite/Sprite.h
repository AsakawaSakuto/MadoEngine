#pragma once
#include "SpriteData.h"
#include "SpriteSharedGeometry.h"
#include "Render/Object/IRenderObject2d.h"
#include ".SceneManager/SceneType.h"

namespace MadoEngine {
class SpriteManager;
}

class Sprite : public IRenderObject2d {
public:

	Sprite(std::string objectName = "default");

	/// @brief 単独使用時の初期化（自前でジオメトリバッファを生成する）
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName) override;

	/// @brief SpriteManager経由での初期化（外部の共有ジオメトリバッファを参照する）
	/// @param device D3D12デバイス
	/// @param commandList コマンドリスト
	/// @param textureName テクスチャ名
	/// @param sharedGeo SpriteManagerが所有する共有ジオメトリバッファ
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName, const SpriteSharedGeometry& sharedGeo);

	/// @brief 更新
	void Update() override;
	
	/// @brief 描画
	void Draw() override;

	/// @brief 使用するテクスチャを変更する
	/// @param textureName 変更先のテクスチャ名
	/// @return 変更に成功した場合はtrue
	bool SetTexture(const std::string& textureName);

	/// @brief 使用中のテクスチャ名を取得する
	/// @return 使用中のテクスチャ名
	const std::string& GetTextureName() const { return textureName_; }

	/// @brief テクスチャのピクセルサイズを取得する
	/// @return テクスチャのピクセルサイズ
	const Vector2& GetTextureSize() const { return size_; }

	/// @brief アンカーポイントを設定する
	/// @param anchorPoint 左上を0、右下を1とするアンカーポイント
	void SetAnchorPoint(const Vector2& anchorPoint);

	/// @brief アンカーポイントを取得する
	/// @return 現在のアンカーポイント
	const Vector2& GetAnchorPoint() const { return anchorPoint_; }

	/// @brief 描画対象シーンをセットする
	/// @param sceneType 描画を許可するシーンの種類（SceneType::None の場合は全シーンで描画）
	void SetSceneType(SceneType sceneType) { sceneType_ = sceneType; }

	/// @brief 描画対象シーンを取得する
	/// @return 登録されているシーンの種類
	SceneType GetSceneType() const { return sceneType_; }

	/// @brief 画面全体へ引き伸ばして描画するかを設定する
	/// @param isFitToScreen trueの場合は現在のスクリーンサイズに合わせて描画する
	void SetFitToScreen(bool isFitToScreen) { isFitToScreen_ = isFitToScreen; }

	/// @brief 画面全体へ引き伸ばして描画する設定かを取得する
	/// @return 画面全体へ引き伸ばす場合はtrue
	bool IsFitToScreen() const { return isFitToScreen_; }

	/// @brief JsonからSprite設定を復元する
	/// @param json 復元元のJson
	void FromJson(const nlohmann::json& json);

	/// @brief Sprite設定をJsonへ変換する
	/// @return Sprite設定を格納したJson
	nlohmann::json ToJson() const;

private:
	friend class MadoEngine::SpriteManager;

	/// @brief SpriteManagerが管理する識別名を更新する
	/// @param objectName 新しい識別名
	void SetObjectName(const std::string& objectName) { objectName_ = objectName; }

	/// @brief マテリアル・変換行列・PSOの共通初期化処理
	void InitializeCommonResources();

	// 実際に描画で使うVBV/IBVへのポインタ
	// SpriteManager経由: sharedGeo のバッファを指す
	// 単独使用時 : 自前の vertexResource_/indexResource_ のビューを指す
	const D3D12_VERTEX_BUFFER_VIEW* activeVBV_ = nullptr;
	const D3D12_INDEX_BUFFER_VIEW* activeIBV_ = nullptr;

	SpriteMaterial* materialData_ = nullptr;
	SpriteTransformationMatrix* transformationData_ = nullptr;

	Vector2     anchorPoint_ = { 0.0f, 0.0f };

	// 描画対象シーン（SceneType::None は全シーンで描画）
	SceneType sceneType_ = SceneType::None;

	// 画面全体へ引き伸ばして描画するかのフラグ
	bool isFitToScreen_ = false;
};
