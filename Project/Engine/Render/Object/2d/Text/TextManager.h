#pragma once

#include "Render/Object/2d/Sprite/SpriteSharedGeometry.h"
#include "Render/Object/2d/Text/Text.h"
#include "Render/Object/ObjectHandle.h"
#include "Render/PSO/PSORegistry.h"
#include "Utility/EditorManagementMode.h"
#include ".SceneManager/SceneType.h"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace MadoEngine {

namespace Render {
class IRenderLayerBatchContext;
}

/// @brief Text生成情報
struct TextCreateDesc {
	std::string name;
	SceneType sceneType = SceneType::None;
	EditorManagementMode managementMode = EditorManagementMode::RuntimeOnly;
};

/// @brief Textの所有と世代付き参照を管理するManager
class TextManager {
public:
	/// @brief TextManagerのシングルトンを取得する
	/// @return TextManagerのインスタンス
	static TextManager& GetInstance();

	TextManager(const TextManager&) = delete;
	TextManager& operator=(const TextManager&) = delete;
	TextManager(TextManager&&) = delete;
	TextManager& operator=(TextManager&&) = delete;

	/// @brief TextManagerを初期化する
	/// @param device D3D12デバイス
	/// @param commandList コマンドリスト
	/// @param psoRegistry PSOレジストリ
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Render::PSORegistry* psoRegistry);

	/// @brief 全Textと共有リソースを解放する
	void Finalize();

	/// @brief 全Textへスクリーンサイズを設定する
	/// @param width スクリーン幅
	/// @param height スクリーン高さ
	void SetScreenSize(float width, float height);

	/// @brief Textを生成する
	/// @param name Text名
	/// @param sceneType 所属Scene
	/// @param managementMode 管理方式
	/// @return 生成したTextのHandle。失敗した場合は無効Handle
	[[nodiscard]] TextHandle Create(
		const std::string& name,
		SceneType sceneType = SceneType::None,
		EditorManagementMode managementMode = EditorManagementMode::RuntimeOnly);

	/// @brief 指定した生成情報からTextを生成する
	/// @param desc Text生成情報
	/// @return 生成したTextのHandle。失敗した場合は無効Handle
	[[nodiscard]] TextHandle Create(const TextCreateDesc& desc);

	/// @brief 同じ条件のTextを取得し、存在しない場合だけ生成する
	/// @param desc Text生成情報
	/// @return 取得または生成したTextのHandle。条件不一致または生成失敗時は無効Handle
	[[nodiscard]] TextHandle FindOrCreate(const TextCreateDesc& desc);

	/// @brief JSONからEditor管理Textを生成または更新する
	/// @param json Text設定
	/// @return 生成または更新したTextのHandle。失敗した場合は無効Handle
	[[nodiscard]] TextHandle CreateFromJson(const nlohmann::json& json);

	/// @brief HandleからTextを一時参照として取得する
	/// @param handle 取得対象のHandle
	/// @return 有効な場合はText、無効な場合はnullptr
	Text* TryGet(TextHandle handle);

	/// @brief HandleからTextを読み取り専用の一時参照として取得する
	/// @param handle 取得対象のHandle
	/// @return 有効な場合はText、無効な場合はnullptr
	const Text* TryGet(TextHandle handle) const;

	/// @brief 名前からTextのHandleを検索する
	/// @param name 検索する名前
	/// @return 見つかったTextのHandle。見つからない場合は無効Handle
	[[nodiscard]] TextHandle Find(const std::string& name) const;

	/// @brief Handleが現在のTextを参照しているか確認する
	/// @param handle 確認するHandle
	/// @return 有効なTextを参照している場合はtrue
	[[nodiscard]] bool IsValid(TextHandle handle) const;

	/// @brief 名前からTextを一時参照として取得する互換API
	/// @param name Text名
	/// @return 見つかったText。見つからない場合はnullptr
	Text* Get(const std::string& name);

	/// @brief 名前からTextを読み取り専用の一時参照として取得する互換API
	/// @param name Text名
	/// @return 見つかったText。見つからない場合はnullptr
	const Text* Get(const std::string& name) const;

	/// @brief 名前を変更してもHandleを維持する
	/// @param handle 名前を変更するTextのHandle
	/// @param newName 新しい名前
	/// @return 変更に成功した場合はtrue
	bool Rename(TextHandle handle, const std::string& newName);

	/// @brief 名前を指定してText名を変更する互換API
	/// @param currentName 現在の名前
	/// @param newName 新しい名前
	/// @return 変更に成功した場合はtrue
	bool Rename(const std::string& currentName, const std::string& newName);

	/// @brief GPUが対象を使用していないことが保証された時点でTextを即時削除する
	/// @param handle 削除対象のHandle
	/// @return 削除できた場合はtrue
	bool Destroy(TextHandle handle);

	/// @brief 名前を指定してTextを即時削除する互換API
	/// @param name 削除対象の名前
	/// @return 削除できた場合はtrue
	bool Destroy(const std::string& name);

	/// @brief 描画中でも安全な時点までTextの削除を延期する
	/// @param handle 削除対象のHandle
	void RequestDestroy(TextHandle handle);

	/// @brief 名前を指定してTextの削除を延期する互換API
	/// @param name 削除対象の名前
	void RequestDestroy(const std::string& name);

	/// @brief 延期されているText削除を安全な時点で実行する
	void FlushPendingDestroys();

	/// @brief 指定Sceneに属するTextを即時削除する
	/// @param sceneType 削除対象のScene
	void DestroyByScene(SceneType sceneType);

	/// @brief 現在Sceneで有効な全Textを更新する
	/// @param currentSceneType 現在のScene
	void UpdateAll(SceneType currentSceneType);

	/// @brief 現在Sceneで有効な全Textを描画する
	/// @param currentSceneType 現在のScene
	/// @param targetScreen 描画対象Screen。空文字の場合は絞り込まない
	void DrawAll(SceneType currentSceneType, const std::string& targetScreen = "");

	/// @brief 指定描画LayerのTextを描画する
	/// @param currentSceneType 現在のScene
	/// @param layer 描画Layer
	/// @param targetScreen 描画対象Screen。空文字の場合は絞り込まない
	void DrawLayer(SceneType currentSceneType, Render::RenderLayer layer, const std::string& targetScreen = "");

	/// @brief 指定描画LayerMaskのTextを描画する
	/// @param currentSceneType 現在のScene
	/// @param layerMask 描画LayerMask
	/// @param targetScreen 描画対象Screen。空文字の場合は絞り込まない
	void DrawLayerMask(SceneType currentSceneType, Render::RenderLayerMask layerMask, const std::string& targetScreen = "");

	/// @brief 現在SceneのTextを元の描画順の連続レイヤーバッチとして描画する
	/// @param currentSceneType 現在のScene
	/// @param batchContext バッチ前後の描画処理を受け取るContext
	/// @param targetScreen 描画対象Screen。空文字の場合は絞り込まない
	void DrawInOrder(
		SceneType currentSceneType,
		Render::IRenderLayerBatchContext& batchContext,
		const std::string& targetScreen = ""
	);

	/// @brief Editor管理TextをJSONへ変換する
	/// @return Text一覧を含むJSON
	nlohmann::json ToJson() const;

	/// @brief JSONからEditor管理Textを復元する
	/// @param json Text一覧を含むJSON
	void FromJson(const nlohmann::json& json);

	/// @brief JSONから指定シーン所属のEditor管理Textを復元する
	/// @param json Text一覧を含むJSON
	/// @param sceneType 復元対象のシーン。SceneType::None所属のTextも復元する
	void FromJson(const nlohmann::json& json, SceneType sceneType);

	/// @brief Editor管理TextをJSONファイルへ保存する
	/// @param filePath 保存先
	/// @return 保存に成功した場合はtrue
	bool SaveToFile(const std::filesystem::path& filePath) const;

	/// @brief 指定シーン所属のEditor管理TextをJSONファイルへ保存する
	/// @param filePath 保存先のファイルパス
	/// @param sceneType 保存対象のシーン。SceneType::None所属のTextも保存する
	/// @return 保存に成功した場合はtrue
	bool SaveToFile(const std::filesystem::path& filePath, SceneType sceneType) const;

	/// @brief JSONファイルからEditor管理Textを読み込む
	/// @param filePath 読み込み元
	/// @return 読み込みに成功した場合はtrue
	bool LoadFromFile(const std::filesystem::path& filePath);

	/// @brief JSONファイルから指定シーン所属のEditor管理Textを読み込む
	/// @param filePath 読み込み元のファイルパス
	/// @param sceneType 読み込み対象のシーン。SceneType::None所属のTextも読み込む
	/// @return 読み込みに成功した場合はtrue
	bool LoadFromFile(const std::filesystem::path& filePath, SceneType sceneType);

	/// @brief Text名一覧を取得する
	/// @return 名前順のText名一覧
	std::vector<std::string> GetNames() const;

	/// @brief 管理中のTextインスタンス数を取得する
	/// @return 管理中のTextインスタンス数
	std::size_t GetTextCount() const;

	/// @brief Editor管理Text名一覧を取得する
	/// @return 名前順のEditor管理Text名一覧
	std::vector<std::string> GetEditorManagedNames() const;

private:
	struct TextSlot {
		std::unique_ptr<Text> text;
		std::string name;
		EditorManagementMode managementMode = EditorManagementMode::RuntimeOnly;
		uint32_t generation = 1;
		bool active = false;
	};

	TextManager() = default;
	~TextManager() = default;

	/// @brief JSONからEditor管理Textを復元する
	/// @param json Text一覧を含むJSON
	/// @param sceneType 対象シーン。未指定の場合は全シーンを対象にする
	void FromJsonInternal(const nlohmann::json& json, std::optional<SceneType> sceneType);

	ID3D12Device* device_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
	Render::PSORegistry* psoRegistry_ = nullptr;
	SpriteSharedGeometry sharedGeometry_;
	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;
	std::vector<TextSlot> slots_;
	std::vector<uint32_t> freeSlots_;
	std::unordered_map<std::string, TextHandle> nameToHandle_;
	std::vector<TextHandle> pendingDestroyHandles_;
};

} // namespace MadoEngine
