#pragma once

#include "UIGroup.h"
#include "SceneManager/SceneType.h"
#include <filesystem>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

namespace MadoEngine {

/// @brief 永続化する子要素の種類
enum class UIGroupElementType { Sprite, Text, Group };

/// @brief Editor表示と再接続に使用する子要素の情報
struct UIGroupElementInfo {
	std::string name;
	UIGroupElementType type = UIGroupElementType::Sprite;
	std::string target;
	TransformNode2D::AuthoringState state;
	SpriteHandle sprite;
	TextHandle text;
	UIGroupHandle group;
	bool resolved = false;
};

/// @brief Editor管理UIGroupの寿命・永続ID・子要素の再接続を管理するクラス
class UIGroupManager final {
public:
	/// @brief Managerの唯一のインスタンスを取得
	/// @return Managerの参照
	static UIGroupManager& GetInstance();
	UIGroupManager(const UIGroupManager&) = delete;
	UIGroupManager& operator=(const UIGroupManager&) = delete;

	/// @brief シーンに所属する編集用Groupを生成
	/// @param name 同じシーン内で一意な表示名
	/// @param sceneType 所属シーン
	/// @return 生成したハンドル、失敗時は無効ハンドル
	UIGroupHandle Create(const std::string& name, SceneType sceneType);
	/// @brief 世代が一致するGroupを一時取得
	/// @param handle 対象のハンドル
	/// @return 有効なGroup、無効な場合はnullptr
	UIGroup* TryGet(UIGroupHandle handle);
	/// @brief 世代が一致するGroupを定数参照として一時取得
	/// @param handle 対象のハンドル
	/// @return 有効なGroup、無効な場合はnullptr
	const UIGroup* TryGet(UIGroupHandle handle) const;
	/// @brief 表示名からハンドルを取得
	/// @param name Groupの表示名
	/// @return 対応するハンドル、未登録または複数シーンで同名の場合は無効ハンドル
	UIGroupHandle Find(const std::string& name) const;
	/// @brief シーンと表示名からハンドルを取得
	/// @param name Groupの表示名
	/// @param sceneType 所属シーン
	/// @return 対応するハンドル、未登録の場合は無効ハンドル
	UIGroupHandle Find(const std::string& name, SceneType sceneType) const;
	/// @brief 永続IDからハンドルを取得
	/// @param id 保存用の永続ID
	/// @return 対応するハンドル、未登録の場合は無効ハンドル
	UIGroupHandle FindById(const std::string& id) const;
	/// @brief 表示名を取得
	/// @param handle 対象のハンドル
	/// @return 表示名、無効な場合は空文字列
	std::string GetName(UIGroupHandle handle) const;
	/// @brief 保存用の永続IDを取得
	/// @param handle 対象のハンドル
	/// @return 永続ID、無効な場合は空文字列
	std::string GetId(UIGroupHandle handle) const;
	/// @brief 所属シーンを取得
	/// @param handle 対象のハンドル
	/// @return 所属シーン、無効な場合はSceneType::None
	SceneType GetSceneType(UIGroupHandle handle) const;
	/// @brief 表示名順で全ハンドルを取得
	/// @return 有効なGroupのハンドル一覧
	std::vector<UIGroupHandle> GetHandles() const;
	/// @brief 永続IDと親子関係を維持した表示名の変更
	/// @param handle 対象のハンドル
	/// @param name 変更後の表示名
	/// @return 名前を変更できた場合はtrue
	bool Rename(UIGroupHandle handle, const std::string& name);
	/// @brief 描画要素を残したGroupの削除
	/// @param handle 削除対象のハンドル
	/// @return 有効なGroupを削除できた場合はtrue
	bool Destroy(UIGroupHandle handle);
	/// @brief 指定シーンのGroupだけを削除
	/// @param sceneType 削除対象のシーン
	void DestroyByScene(SceneType sceneType);
	/// @brief 描画要素を残した全Groupの解放
	void Finalize();

	/// @brief 同じシーンのEditor管理要素を名前付きで接続
	/// @param handle 接続先Group
	/// @param name Group内の登録名
	/// @param type 接続する要素の種類
	/// @param target Sprite・Textの管理名、Groupの場合は永続ID
	/// @return 未所属の有効な要素を循環なしで接続できた場合はtrue
	bool AddElement(UIGroupHandle handle, const std::string& name, UIGroupElementType type, const std::string& target);
	/// @brief Spriteを新規生成してGroupへ直接追加
	/// @param handle 追加先のGroup
	/// @param name Group内の要素名
	/// @param textureName 使用するテクスチャ名
	/// @param displaySize 親座標での長辺の初期表示サイズ、ピクセル単位、省略時はScaleを1で維持
	/// @return 追加したSpriteのハンドル、失敗した場合は無効ハンドル
	SpriteHandle CreateSpriteElement(UIGroupHandle handle, const std::string& name, const std::string& textureName, std::optional<float> displaySize = std::nullopt);
	/// @brief Textを新規生成してGroupへ直接追加
	/// @param handle 追加先のGroup
	/// @param name Group内の要素名
	/// @param text 初期状態の本文
	/// @return 追加したTextのハンドル、失敗した場合は無効ハンドル
	TextHandle CreateTextElement(UIGroupHandle handle, const std::string& name, const std::string& text);
	/// @brief 子Groupを新規生成して直接追加
	/// @param handle 追加先のGroup
	/// @param name 親Group内の要素名
	/// @return 追加した子Groupのハンドル、失敗した場合は無効ハンドル
	UIGroupHandle CreateGroupElement(UIGroupHandle handle, const std::string& name);
	/// @brief 接続先と編集用設定を維持した登録名の変更
	/// @param handle 対象のGroup
	/// @param name 現在の登録名
	/// @param newName 新しい登録名
	/// @return 登録名を変更できた場合はtrue
	bool RenameElement(UIGroupHandle handle, const std::string& name, const std::string& newName);
	/// @brief 描画要素と編集用のローカル値を残した接続の解除
	/// @param handle 対象のGroup
	/// @param name 解除する登録名
	/// @return 登録を解除できた場合はtrue
	bool RemoveElement(UIGroupHandle handle, const std::string& name);
	/// @brief 登録と接続中の要素を削除し、子Groupの配下だけを維持
	/// @param handle 対象のGroup
	/// @param name 削除する要素名
	/// @return 登録を削除できた場合はtrue
	bool DestroyElement(UIGroupHandle handle, const std::string& name);
	/// @brief ローカル設定と登録名を維持した親Groupの変更
	/// @param source 現在の親Group
	/// @param name 移動する登録名
	/// @param destination 移動先の親Group
	/// @return 移動に成功した場合はtrue、失敗時は元の構成
	bool MoveElement(UIGroupHandle source, const std::string& name, UIGroupHandle destination);
	/// @brief 型・名前・接続状態を含む子要素一覧を取得
	/// @param handle 対象のGroup
	/// @return 登録順の子要素一覧
	std::vector<UIGroupElementInfo> GetElements(UIGroupHandle handle) const;
	/// @brief 有効な直接の子ノードを一時取得
	/// @param handle 対象のGroup
	/// @param name 子要素の登録名
	/// @return 接続中のノード、未解決の場合はnullptr
	TransformNode2D* TryGetElement(UIGroupHandle handle, const std::string& name);
	/// @brief Undoや再読込で再生成された子要素を永続参照から再接続
	void RefreshBindings();

	/// @brief 実行時の変更を除外した編集用設定をJsonへ変換
	/// @return 全Groupと子要素の編集用設定
	nlohmann::json ToJson() const;
	/// @brief 検証済みJsonで対象シーンの構成を置換
	/// @param json 復元元のJson
	/// @param sceneType 対象シーン、未指定の場合は全シーン
	/// @return 構成を復元できた場合はtrue
	bool FromJson(const nlohmann::json& json, std::optional<SceneType> sceneType = std::nullopt);
	/// @brief 他シーンの保存内容を維持したJson保存
	/// @param path 保存先のパス
	/// @param sceneType 保存対象のシーン、共通Groupも対象
	/// @return 保存に成功した場合はtrue
	bool SaveToFile(const std::filesystem::path& path, SceneType sceneType) const;
	/// @brief Jsonから対象シーンと共通Groupを復元
	/// @param path 読込元のパス
	/// @param sceneType 読込対象のシーン
	/// @return 読込と復元に成功した場合はtrue
	bool LoadFromFile(const std::filesystem::path& path, SceneType sceneType);

private:
	struct Slot {
		std::unique_ptr<UIGroup> group;
		std::string id;
		std::string name;
		SceneType scene = SceneType::None;
		uint32_t generation = 1;
		std::vector<UIGroupElementInfo> elements;
	};
	UIGroupManager() = default;
	~UIGroupManager() = default;

	/// @brief 未解決の参照も含めて要素名の使用可否を確認
	/// @param handle 追加先のGroup
	/// @param name 確認する要素名
	/// @return 有効かつ未使用の要素名の場合はtrue
	bool CanCreateElement(UIGroupHandle handle, const std::string& name) const;

	/// @brief 指定スロットの子情報を現在の参照状態へ同期
	/// @param slot 対象のスロット
	/// @param element 同期する子情報
	/// @return 接続中のノード、未解決の場合はnullptr
	TransformNode2D* Resolve(Slot& slot, UIGroupElementInfo& element);

	std::vector<Slot> slots_;
	std::vector<uint32_t> freeSlots_;
	std::unordered_map<std::string, UIGroupHandle> names_;
	std::unordered_map<std::string, UIGroupHandle> ids_;
};

} // namespace MadoEngine
