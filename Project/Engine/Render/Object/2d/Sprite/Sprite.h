#pragma once
#include "SpriteData.h"
#include "SpriteSharedGeometry.h"
#include "Render/Object/IRenderObject2d.h"
#include ".SceneManager/SceneType.h"

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

private:

	/// @brief マテリアル・変換行列・PSOの共通初期化処理
	void InitializeCommonResources(const std::string& textureName);

	// 実際に描画で使うVBV/IBVへのポインタ
	// SpriteManager経由: sharedGeo のバッファを指す
	// 単独使用時 : 自前の vertexResource_/indexResource_ のビューを指す
	const D3D12_VERTEX_BUFFER_VIEW* activeVBV_ = nullptr;
	const D3D12_INDEX_BUFFER_VIEW* activeIBV_ = nullptr;

	SpriteMaterial* materialData_ = nullptr;
	SpriteTransformationMatrix* transformationData_ = nullptr;

	Vector2     anchorPoint_ = { 0.0f, 0.0f };
	AnchorPoint anchorType_ = AnchorPoint::TopLeft;

	// 描画対象シーン（SceneType::None は全シーンで描画）
	SceneType sceneType_ = SceneType::None;

	// 画面全体へ引き伸ばして描画するかのフラグ
	bool isFitToScreen_ = false;
};
