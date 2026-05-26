#pragma once
#include "SpriteData.h"
#include "SpriteSharedGeometry.h"
#include "Render/Object/RenderObject2d.h"

class Sprite : public RenderObject2d {
public:

	Sprite(std::string objectName);

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

	/// @brief 描画対象シーン名をセットする
	/// @param sceneName 描画を許可するシーン名
	void SetSceneName(const std::string& sceneName) { sceneName_ = sceneName; }

	/// @brief 描画対象シーン名を取得する
	/// @return 登録されているシーン名
	const std::string& GetSceneName() const { return sceneName_; }

private:

	/// @brief マテリアル・変換行列・PSOの共通初期化処理
	void InitializeCommonResources(const std::string& textureName);

	// 実際に描画で使うVBV/IBVへのポインタ
	// SpriteManager経由: sharedGeo のバッファを指す
	// 単独使用時     : 自前の vertexResource_/indexResource_ のビューを指す
	const D3D12_VERTEX_BUFFER_VIEW* activeVBV_ = nullptr;
	const D3D12_INDEX_BUFFER_VIEW* activeIBV_ = nullptr;

	SpriteMaterial* materialData_ = nullptr;
	SpriteTransformationMatrix* transformationData_ = nullptr;

	Vector2     anchorPoint_ = { 0.0f, 0.0f };
	AnchorPoint anchorType_ = AnchorPoint::TopLeft;

	// 描画対象シーン名（空文字は全シーンで描画）
	std::string sceneName_;
};