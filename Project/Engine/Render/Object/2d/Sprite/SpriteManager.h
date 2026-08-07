#pragma once

#include "Sprite.h"
#include "SpriteSharedGeometry.h"
#include "Render/Object/ObjectHandle.h"
#include "Render/Object/RenderLayer.h"
#include "Utility/EditorManagementMode.h"
#include ".SceneManager/SceneType.h"
#include <cstddef>
#include <d3d12.h>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace MadoEngine {

/// @brief Sprite生成情報
struct SpriteCreateDesc {
	std::string name;
	std::string textureName;
	SceneType sceneType = SceneType::None;
	EditorManagementMode managementMode = EditorManagementMode::RuntimeOnly;
};

/// @brief Spriteの所有と世代付き参照を管理するManager
class SpriteManager {
public:
	/// @brief SpriteManagerのシングルトンを取得する
	/// @return SpriteManagerのインスタンス
	static SpriteManager& GetInstance();

	SpriteManager(const SpriteManager&) = delete;
	SpriteManager& operator=(const SpriteManager&) = delete;
	SpriteManager(SpriteManager&&) = delete;
	SpriteManager& operator=(SpriteManager&&) = delete;

	/// @brief SpriteManagerを初期化する
	/// @param device D3D12デバイス
	/// @param commandList コマンドリスト
	/// @param psoRegistry PSOレジストリ
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Render::PSORegistry* psoRegistry);

	/// @brief 全Spriteと共有リソースを解放する
	void Finalize();

	/// @brief 全Spriteへスクリーンサイズを設定する
	/// @param width スクリーン幅
	/// @param height スクリーン高さ
	void SetScreenSize(float width, float height);

	/// @brief Spriteを生成する
	/// @param name Sprite名
	/// @param textureName テクスチャ名
	/// @param sceneType 所属Scene
	/// @param managementMode 管理方式
	/// @return 生成したSpriteのHandle。失敗した場合は無効Handle
	[[nodiscard]] SpriteHandle Create(
		const std::string& name,
		const std::string& textureName,
		SceneType sceneType = SceneType::None,
		EditorManagementMode managementMode = EditorManagementMode::RuntimeOnly);

	/// @brief 指定した生成情報からSpriteを生成する
	/// @param desc Sprite生成情報
	/// @return 生成したSpriteのHandle。失敗した場合は無効Handle
	[[nodiscard]] SpriteHandle Create(const SpriteCreateDesc& desc);

	/// @brief 同じ条件のSpriteを取得し、存在しない場合だけ生成する
	/// @param desc Sprite生成情報
	/// @return 取得または生成したSpriteのHandle。条件不一致または生成失敗時は無効Handle
	[[nodiscard]] SpriteHandle FindOrCreate(const SpriteCreateDesc& desc);

	/// @brief JSONからEditor管理Spriteを生成または更新する
	/// @param json Sprite設定
	/// @return 生成または更新したSpriteのHandle。失敗した場合は無効Handle
	[[nodiscard]] SpriteHandle CreateFromJson(const nlohmann::json& json);

	/// @brief HandleからSpriteを一時参照として取得する
	/// @param handle 取得対象のHandle
	/// @return 有効な場合はSprite、無効な場合はnullptr
	Sprite* TryGet(SpriteHandle handle);

	/// @brief HandleからSpriteを読み取り専用の一時参照として取得する
	/// @param handle 取得対象のHandle
	/// @return 有効な場合はSprite、無効な場合はnullptr
	const Sprite* TryGet(SpriteHandle handle) const;

	/// @brief 名前からSpriteのHandleを検索する
	/// @param name 検索する名前
	/// @return 見つかったSpriteのHandle。見つからない場合は無効Handle
	[[nodiscard]] SpriteHandle Find(const std::string& name) const;

	/// @brief Handleが現在のSpriteを参照しているか確認する
	/// @param handle 確認するHandle
	/// @return 有効なSpriteを参照している場合はtrue
	[[nodiscard]] bool IsValid(SpriteHandle handle) const;

	/// @brief 名前からSpriteを一時参照として取得する互換API
	/// @param name Sprite名
	/// @return 見つかったSprite。見つからない場合はnullptr
	Sprite* Get(const std::string& name);

	/// @brief 名前からSpriteを読み取り専用の一時参照として取得する互換API
	/// @param name Sprite名
	/// @return 見つかったSprite。見つからない場合はnullptr
	const Sprite* Get(const std::string& name) const;

	/// @brief 名前を変更してもHandleを維持する
	/// @param handle 名前を変更するSpriteのHandle
	/// @param newName 新しい名前
	/// @return 変更に成功した場合はtrue
	bool Rename(SpriteHandle handle, const std::string& newName);

	/// @brief 名前を指定してSprite名を変更する互換API
	/// @param currentName 現在の名前
	/// @param newName 新しい名前
	/// @return 変更に成功した場合はtrue
	bool Rename(const std::string& currentName, const std::string& newName);

	/// @brief GPUが対象を使用していないことが保証された時点でSpriteを即時削除する
	/// @param handle 削除対象のHandle
	/// @return 削除できた場合はtrue
	bool Destroy(SpriteHandle handle);

	/// @brief 名前を指定してSpriteを即時削除する互換API
	/// @param name 削除対象の名前
	/// @return 削除できた場合はtrue
	bool Destroy(const std::string& name);

	/// @brief 描画中でも安全な時点までSpriteの削除を延期する
	/// @param handle 削除対象のHandle
	void RequestDestroy(SpriteHandle handle);

	/// @brief 名前を指定してSpriteの削除を延期する互換API
	/// @param name 削除対象の名前
	void RequestDestroy(const std::string& name);

	/// @brief 延期されているSprite削除を安全な時点で実行する
	void FlushPendingDestroys();

	/// @brief 指定Sceneに属するSpriteを即時削除する
	/// @param sceneType 削除対象のScene
	void DestroyByScene(SceneType sceneType);

	/// @brief 現在Sceneで有効な全Spriteを更新する
	/// @param currentSceneType 現在のScene
	void UpdateAll(SceneType currentSceneType);

	/// @brief 現在Sceneで有効な全Spriteを描画する
	/// @param currentSceneType 現在のScene
	void DrawAll(SceneType currentSceneType);

	/// @brief 指定描画LayerのSpriteを描画する
	/// @param currentSceneType 現在のScene
	/// @param layer 描画Layer
	void DrawLayer(SceneType currentSceneType, Render::RenderLayer layer);

	/// @brief 指定描画LayerMaskのSpriteを描画する
	/// @param currentSceneType 現在のScene
	/// @param layerMask 描画LayerMask
	void DrawLayerMask(SceneType currentSceneType, Render::RenderLayerMask layerMask);

	/// @brief Editor管理SpriteをJSONへ変換する
	/// @return Sprite一覧を含むJSON
	nlohmann::json ToJson() const;

	/// @brief JSONからEditor管理Spriteを復元する
	/// @param json Sprite一覧を含むJSON
	void FromJson(const nlohmann::json& json);

	/// @brief Editor管理SpriteをJSONファイルへ保存する
	/// @param filePath 保存先
	/// @return 保存に成功した場合はtrue
	bool SaveToFile(const std::filesystem::path& filePath) const;

	/// @brief JSONファイルからEditor管理Spriteを読み込む
	/// @param filePath 読み込み元
	/// @return 読み込みに成功した場合はtrue
	bool LoadFromFile(const std::filesystem::path& filePath);

	/// @brief 描画順でSprite名一覧を取得する
	/// @return Sprite名一覧
	std::vector<std::string> GetNames() const;

	/// @brief 管理中のSpriteインスタンス数を取得する
	/// @return 管理中のSpriteインスタンス数
	std::size_t GetSpriteCount() const;

	/// @brief 描画順でEditor管理Sprite名一覧を取得する
	/// @return Editor管理Sprite名一覧
	std::vector<std::string> GetEditorManagedNames() const;

private:
	struct SpriteSlot {
		std::unique_ptr<Sprite> sprite;
		std::string name;
		std::string textureName;
		EditorManagementMode managementMode = EditorManagementMode::RuntimeOnly;
		uint32_t generation = 1;
		bool active = false;
	};

	SpriteManager() = default;
	~SpriteManager() = default;

	ID3D12Device* device_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
	Render::PSORegistry* psoRegistry_ = nullptr;
	SpriteSharedGeometry sharedGeometry_;
	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;
	std::vector<SpriteSlot> slots_;
	std::vector<uint32_t> freeSlots_;
	std::unordered_map<std::string, SpriteHandle> nameToHandle_;
	std::vector<SpriteHandle> drawOrder_;
	std::vector<SpriteHandle> pendingDestroyHandles_;
};

} // namespace MadoEngine
