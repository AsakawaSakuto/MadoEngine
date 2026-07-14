#pragma once
#include "Render/Object/2d/Text/Text.h"
#include "Render/Object/2d/Sprite/SpriteSharedGeometry.h"
#include "Render/PSO/PSORegistry.h"
#include ".SceneManager/SceneType.h"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace MadoEngine {

/// @brief Textの生成、更新、描画、Json保存を管理するシングルトン
class TextManager {
public:
	/// @brief TextManagerのインスタンスを取得
	/// @return TextManagerのインスタンス
	static TextManager& GetInstance();

	TextManager(const TextManager&) = delete;
	TextManager& operator=(const TextManager&) = delete;
	TextManager(TextManager&&) = delete;
	TextManager& operator=(TextManager&&) = delete;

	/// @brief TextManagerを初期化
	/// @param device D3D12デバイス
	/// @param commandList コマンドリスト
	/// @param psoRegistry PSOレジストリ
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, Render::PSORegistry* psoRegistry);

	/// @brief TextManagerのリソースを解放
	void Finalize();

	/// @brief 管理中のTextへスクリーンサイズを設定
	/// @param width スクリーン幅
	/// @param height スクリーン高さ
	void SetScreenSize(float width, float height);

	/// @brief Textを作成して管理下へ登録
	/// @param name Textの識別名
	/// @param sceneType 描画対象Scene
	/// @return 作成または取得されたText
	Text* Create(const std::string& name, SceneType sceneType = SceneType::None);

	/// @brief JsonからTextを作成して管理下へ登録
	/// @param json Text設定Json
	/// @return 作成または復元されたText
	Text* CreateFromJson(const nlohmann::json& json);

	/// @brief 指定名のTextを取得
	/// @param name Textの識別名
	/// @return Text。存在しない場合はnullptr
	Text* Get(const std::string& name) const;

	/// @brief Textを破棄
	/// @param name 破棄するText名
	void Destroy(const std::string& name);

	/// @brief Sceneに属するTextを破棄
	/// @param sceneType 破棄対象Scene
	void DestroyByScene(SceneType sceneType);

	/// @brief 管理下のTextを更新
	/// @param currentSceneType 現在のScene
	void UpdateAll(SceneType currentSceneType);

	/// @brief 管理下のTextを描画
	/// @param currentSceneType 現在のScene
	/// @param targetScreen 描画対象Screen。空文字列の場合はScreenで絞り込みません
	void DrawAll(SceneType currentSceneType, const std::string& targetScreen = "");

	/// @brief 指定LayerのTextを描画
	/// @param currentSceneType 現在のScene
	/// @param layer 描画対象Layer
	/// @param targetScreen 描画対象Screen。空文字列の場合はScreenで絞り込みません
	void DrawLayer(SceneType currentSceneType, Render::RenderLayer layer, const std::string& targetScreen = "");

	/// @brief 指定LayerMaskに含まれるTextを描画
	/// @param currentSceneType 現在のScene
	/// @param layerMask 描画対象LayerMask
	/// @param targetScreen 描画対象Screen。空文字列の場合はScreenで絞り込みません
	void DrawLayerMask(SceneType currentSceneType, Render::RenderLayerMask layerMask, const std::string& targetScreen = "");

	/// @brief 管理下のTextをJsonへ変換
	/// @return Text一覧を含むJson
	nlohmann::json ToJson() const;

	/// @brief JsonからText一覧を復元
	/// @param json 復元元Json
	void FromJson(const nlohmann::json& json);

	/// @brief Text一覧をJsonファイルへ保存
	/// @param filePath 保存先パス
	/// @return 保存に成功した場合はtrue
	bool SaveToFile(const std::filesystem::path& filePath) const;

	/// @brief Text一覧をJsonファイルから読み込み
	/// @param filePath 読み込み元パス
	/// @return 読み込みに成功した場合はtrue
	bool LoadFromFile(const std::filesystem::path& filePath);

	/// @brief 管理中のText名一覧を取得
	/// @return Text名一覧
	std::vector<std::string> GetNames() const;

private:
	TextManager() = default;
	~TextManager() = default;

	ID3D12Device* device_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
	Render::PSORegistry* psoRegistry_ = nullptr;
	SpriteSharedGeometry sharedGeometry_;
	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;
	std::unordered_map<std::string, std::unique_ptr<Text>> texts_;
};

} // namespace MadoEngine
