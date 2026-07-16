#pragma once
#include "Sprite.h"
#include "SpriteSharedGeometry.h"
#include "Utility/EditorManagementMode.h"
#include "Render/Object/RenderLayer.h"
#include ".SceneManager/SceneType.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <d3d12.h>
#include <filesystem>

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

	/// @brief 管理中のSpriteへスクリーンサイズを設定する
	/// @param width スクリーン幅
	/// @param height スクリーン高さ
	void SetScreenSize(float width, float height);

	/// @brief Spriteを生成して管理下に登録する
	/// @param name Spriteの識別名
	/// @param textureName 使用するテクスチャ名
	/// @param sceneType 描画を許可するシーンの種類（SceneType::None の場合は全シーンで描画）
	/// @param managementMode Spriteの管理方法
	/// @return 生成したSpriteのポインタ（所有権はSpriteManagerが持つ）
	Sprite* Create(
		const std::string& name,
		const std::string& textureName,
		SceneType sceneType = SceneType::None,
		EditorManagementMode managementMode = EditorManagementMode::RuntimeOnly);

	/// @brief JsonからSpriteを生成してEditor管理対象へ登録する
	/// @param json Sprite設定を格納したJson
	/// @return 生成または更新したSprite、失敗した場合はnullptr
	Sprite* CreateFromJson(const nlohmann::json& json);

	/// @brief 識別名でSpriteを取得する
	/// @param name Spriteの識別名
	/// @return Spriteのポインタ（存在しない場合はnullptr）
	Sprite* Get(const std::string& name) const;

	/// @brief Spriteの識別名を変更する
	/// @param currentName 現在の識別名
	/// @param newName 新しい識別名
	/// @return 変更に成功した場合はtrue
	bool Rename(const std::string& currentName, const std::string& newName);

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

	/// @brief Editor管理対象のSprite一覧をJsonへ変換する
	/// @return Editor管理対象のSprite一覧を格納したJson
	nlohmann::json ToJson() const;

	/// @brief JsonからEditor管理対象のSprite一覧を復元する
	/// @param json 復元元のJson
	void FromJson(const nlohmann::json& json);

	/// @brief Editor管理対象のSprite一覧をJsonファイルへ保存する
	/// @param filePath 保存先のファイルパス
	/// @return 保存に成功した場合はtrue
	bool SaveToFile(const std::filesystem::path& filePath) const;

	/// @brief JsonファイルからEditor管理対象のSprite一覧を読み込む
	/// @param filePath 読み込み元のファイルパス
	/// @return 読み込みに成功した場合はtrue
	bool LoadFromFile(const std::filesystem::path& filePath);

	/// @brief 管理中のSprite名一覧を取得する
	/// @return 描画順に並んだSprite名一覧
	std::vector<std::string> GetNames() const;

	/// @brief Editor管理対象のSprite名一覧を取得する
	/// @return 描画順に並んだEditor管理対象のSprite名一覧
	std::vector<std::string> GetEditorManagedNames() const;

private:
	struct SpriteEntry {
		std::unique_ptr<Sprite> sprite;
		EditorManagementMode managementMode = EditorManagementMode::RuntimeOnly;
	};

	SpriteManager()  = default;
	~SpriteManager() = default;

	ID3D12Device*              device_      = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;

	MadoEngine::Render::PSORegistry* psoRegistry_ = nullptr; // PSOレジストリ（外部からセット）

	// 共有ジオメトリバッファ（このクラスが唯一の所有者）
	SpriteSharedGeometry sharedGeometry_;

	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;

	// Sprite管理マップ（識別名 → Spriteと管理方法）
	std::unordered_map<std::string, SpriteEntry> sprites_;
	std::vector<std::string> drawOrder_;
};

} // namespace MadoEngine
