#pragma once
#include "Sprite.h"
#include "SpriteSharedGeometry.h"
#include "Render/Object/RenderLayer.h"
#include ".SceneManager/SceneType.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <d3d12.h>

namespace MadoEngine {

/// @brief Spriteの生成・ライフサイクル管理・一括更新/描画を担当するシングルトン
class SpriteManager {
public:

	/// @brief シングルトンインスタンスを取得する
	/// @return SpriteManagerの唯一のインスタンス
	static SpriteManager& GetInstance();

	// コピー・ムーブ禁止
	SpriteManager(const SpriteManager&)            = delete;
	SpriteManager& operator=(const SpriteManager&) = delete;
	SpriteManager(SpriteManager&&)                 = delete;
	SpriteManager& operator=(SpriteManager&&)      = delete;

	/// @brief 初期化（共有ジオメトリバッファの生成）
	/// @param device D3D12デバイス
	/// @param commandList コマンドリスト
	/// @param psoRegistry PSOレジストリ
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, MadoEngine::Render::PSORegistry* psoRegistry);

	/// @brief 全リソースを解放する
	void Finalize();

	/// @brief Spriteを生成して管理下に登録する
	/// @param name Spriteの識別名
	/// @param textureName 使用するテクスチャ名
	/// @param sceneType 描画を許可するシーンの種類（SceneType::None の場合は全シーンで描画）
	/// @return 生成したSpriteのポインタ（所有権はSpriteManagerが持つ）
	Sprite* Create(const std::string& name, const std::string& textureName, SceneType sceneType = SceneType::None);

	/// @brief 識別名でSpriteを取得する
	/// @param name Spriteの識別名
	/// @return Spriteのポインタ（存在しない場合はnullptr）
	Sprite* Get(const std::string& name) const;

	/// @brief 指定したSpriteを破棄する
	/// @param name Spriteの識別名
	void Destroy(const std::string& name);

	/// @brief 指定したシーンに属するSpriteインスタンスをすべて破棄する
	/// @param sceneType 破棄対象のシーン種別
	void DestroyByScene(SceneType sceneType);

	/// @brief 管理下の全Spriteを更新する
	void UpdateAll(SceneType currentSceneType);

	/// @brief 管理下の全Spriteを描画する（IsVisible() == false またはシーン不一致はスキップ）
	/// @param currentSceneType 現在実行中のシーンの種類
	void DrawAll(SceneType currentSceneType);

	/// @brief 指定した描画レイヤーのSpriteのみを描画する
	/// @param currentSceneType 現在のシーン種別
	/// @param layer 描画対象のレイヤー
	void DrawLayer(SceneType currentSceneType, MadoEngine::Render::RenderLayer layer);

	/// @brief 指定したレイヤーマスクに含まれるSpriteを描画する
	/// @param currentSceneType 現在のシーン種別
	/// @param layerMask 描画対象のレイヤーマスク
	void DrawLayerMask(SceneType currentSceneType, MadoEngine::Render::RenderLayerMask layerMask);

private:

	SpriteManager()  = default;
	~SpriteManager() = default;

	ID3D12Device*              device_      = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;

	MadoEngine::Render::PSORegistry* psoRegistry_ = nullptr; // PSOレジストリ（外部からセット）

	// 共有ジオメトリバッファ（このクラスが唯一の所有者）
	SpriteSharedGeometry sharedGeometry_;

	// Sprite管理マップ（識別名 → unique_ptr）
	std::unordered_map<std::string, std::unique_ptr<Sprite>> sprites_;
};

} // namespace MadoEngine
