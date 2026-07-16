#pragma once
#include "DirectionalLight.h"
#include "LightLayer.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Utility/EditorManagementMode.h"
#include ".SceneManager/SceneType.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

/// @brief ライトの種類を表す列挙型
enum class LightType {
	None,
	Directional,
	Point,
	Spot,
};

static constexpr uint32_t kInvalidLightIndex = 0xffffffffu;
static constexpr uint32_t kMaxDirectionalLights = 1;
static constexpr uint32_t kMaxPointLights = 8;
static constexpr uint32_t kMaxSpotLights = 8;

/// @brief LightManager内のライトを参照するハンドル
struct LightHandle {
	LightType type = LightType::None;
	uint32_t index = kInvalidLightIndex;
	uint32_t generation = 0;

	/// @brief ハンドルが初期値ではないか確認する
	/// @return 初期値ではない場合はtrue
	bool IsValid() const {
		return type != LightType::None && index != kInvalidLightIndex;
	}
};

/// @brief ライト共通の登録情報
struct LightMetaData {
	std::string name;
	SceneType sceneType = SceneType::None;
	LightLayerMask layerMask = ToLightLayerMask(LightLayer::World);
	MadoEngine::EditorManagementMode managementMode = MadoEngine::EditorManagementMode::RuntimeOnly;
	bool enabled = true;
};

/// @brief 平行光源の登録情報
struct DirectionalLightEntry {
	DirectionalLight light;
	LightMetaData meta;
};

/// @brief 点光源の登録情報
struct PointLightEntry {
	PointLight light;
	LightMetaData meta;
};

/// @brief スポットライトの登録情報
struct SpotLightEntry {
	SpotLight light;
	LightMetaData meta;
};

/// @brief GPU送信用の固定長ライト配列データ
struct LightGpuData {
	uint32_t directionalLightCount = 0;
	uint32_t pointLightCount = 0;
	uint32_t spotLightCount = 0;
	uint32_t padding = 0;
	std::array<DirectionalLight, kMaxDirectionalLights> directionalLights = {};
	std::array<PointLight, kMaxPointLights> pointLights = {};
	std::array<SpotLight, kMaxSpotLights> spotLights = {};
};

/// @brief ライトを名前とハンドルで管理するクラス
class LightManager {
public:
	static constexpr const char* kDefaultLightJsonPath = "Assets/Json/LightManager.json";

	/// @brief LightManagerのシングルトンインスタンスを取得する
	/// @return LightManagerの参照
	static LightManager& GetInstance();

	LightManager(const LightManager&) = delete;
	LightManager& operator=(const LightManager&) = delete;
	LightManager(LightManager&&) = delete;
	LightManager& operator=(LightManager&&) = delete;

	/// @brief 平行光源を登録する
	/// @param name ライト名
	/// @param light 登録する平行光源
	/// @param sceneType ライトを使用するシーン
	/// @param layerMask ライトの影響先レイヤーマスク
	/// @param managementMode ライトの管理方法
	/// @return 登録したライトのハンドル
	LightHandle CreateDirectionalLight(
		const std::string& name,
		const DirectionalLight& light,
		SceneType sceneType = SceneType::None,
		LightLayerMask layerMask = ToLightLayerMask(LightLayer::World),
		MadoEngine::EditorManagementMode managementMode = MadoEngine::EditorManagementMode::RuntimeOnly);

	/// @brief 点光源を登録する
	/// @param name ライト名
	/// @param light 登録する点光源
	/// @param sceneType ライトを使用するシーン
	/// @param layerMask ライトの影響先レイヤーマスク
	/// @param managementMode ライトの管理方法
	/// @return 登録したライトのハンドル
	LightHandle CreatePointLight(
		const std::string& name,
		const PointLight& light,
		SceneType sceneType = SceneType::None,
		LightLayerMask layerMask = ToLightLayerMask(LightLayer::World),
		MadoEngine::EditorManagementMode managementMode = MadoEngine::EditorManagementMode::RuntimeOnly);

	/// @brief スポットライトを登録する
	/// @param name ライト名
	/// @param light 登録するスポットライト
	/// @param sceneType ライトを使用するシーン
	/// @param layerMask ライトの影響先レイヤーマスク
	/// @param managementMode ライトの管理方法
	/// @return 登録したライトのハンドル
	LightHandle CreateSpotLight(
		const std::string& name,
		const SpotLight& light,
		SceneType sceneType = SceneType::None,
		LightLayerMask layerMask = ToLightLayerMask(LightLayer::World),
		MadoEngine::EditorManagementMode managementMode = MadoEngine::EditorManagementMode::RuntimeOnly);

	/// @brief ライト名からハンドルを取得する
	/// @param name 検索するライト名
	/// @return 見つかったライトのハンドル。見つからない場合は無効なハンドル
	LightHandle Find(const std::string& name) const;

	/// @brief 登録済み平行光源のハンドル一覧を取得する
	/// @return 登録済み平行光源のハンドル配列
	std::vector<LightHandle> GetDirectionalLightHandles() const;

	/// @brief 登録済み点光源のハンドル一覧を取得する
	/// @return 登録済み点光源のハンドル配列
	std::vector<LightHandle> GetPointLightHandles() const;

	/// @brief 登録済みスポットライトのハンドル一覧を取得する
	/// @return 登録済みスポットライトのハンドル配列
	std::vector<LightHandle> GetSpotLightHandles() const;

	/// @brief Editor管理対象の平行光源ハンドル一覧を取得する
	/// @return Editor管理対象の平行光源ハンドル配列
	std::vector<LightHandle> GetEditorManagedDirectionalLightHandles() const;

	/// @brief Editor管理対象の点光源ハンドル一覧を取得する
	/// @return Editor管理対象の点光源ハンドル配列
	std::vector<LightHandle> GetEditorManagedPointLightHandles() const;

	/// @brief Editor管理対象のスポットライトハンドル一覧を取得する
	/// @return Editor管理対象のスポットライトハンドル配列
	std::vector<LightHandle> GetEditorManagedSpotLightHandles() const;

	/// @brief ハンドルが現在も有効か確認する
	/// @param handle 確認するハンドル
	/// @return 有効なライトを指す場合はtrue
	bool IsValid(LightHandle handle) const;

	/// @brief ライトを削除する
	/// @param handle 削除するライトのハンドル
	/// @return 削除できた場合はtrue
	bool Destroy(LightHandle handle);

	/// @brief ライト名を指定してライトを削除する
	/// @param name 削除するライト名
	/// @return 削除できた場合はtrue
	bool Destroy(const std::string& name);

	/// @brief 指定したシーンに属するライトをすべて削除する
	/// @param sceneType 削除対象のシーン種別
	void DestroyByScene(SceneType sceneType);

	/// @brief ライト名を変更する
	/// @param handle 対象ライトのハンドル
	/// @param newName 新しいライト名
	/// @return 変更できた場合はtrue
	bool RenameLight(LightHandle handle, const std::string& newName);

	/// @brief 登録済みライトをすべて削除する
	void Clear();

	/// @brief Editor管理対象のライトをJsonファイルへ保存する
	/// @param filePath 保存先のJsonファイルパス
	/// @return 保存に成功した場合はtrue
	bool SaveToJson(const std::filesystem::path& filePath = kDefaultLightJsonPath) const;

	/// @brief JsonファイルからEditor管理対象のライトを読み込む
	/// @param filePath 読み込み元のJsonファイルパス
	/// @return 実行時専用ライトを維持して読み込みに成功した場合はtrue
	bool LoadFromJson(const std::filesystem::path& filePath = kDefaultLightJsonPath);

	/// @brief ライトの有効状態を設定する
	/// @param handle 対象ライトのハンドル
	/// @param enabled 有効にする場合はtrue
	/// @return 設定できた場合はtrue
	bool SetEnabled(LightHandle handle, bool enabled);

	/// @brief ライトの有効状態を取得する
	/// @param handle 対象ライトのハンドル
	/// @return 有効なライトの場合はtrue
	bool IsEnabled(LightHandle handle) const;

	/// @brief ライトのシーンを設定する
	/// @param handle 対象ライトのハンドル
	/// @param sceneType 設定するシーン
	/// @return 設定できた場合はtrue
	bool SetSceneType(LightHandle handle, SceneType sceneType);

	/// @brief ライトのシーンを取得する
	/// @param handle 対象ライトのハンドル
	/// @return 設定されているシーン。無効な場合はSceneType::None
	SceneType GetSceneType(LightHandle handle) const;

	/// @brief ライトの影響先レイヤーマスクを設定する
	/// @param handle 対象ライトのハンドル
	/// @param layerMask 設定するレイヤーマスク
	/// @return 設定できた場合はtrue
	bool SetLayerMask(LightHandle handle, LightLayerMask layerMask);

	/// @brief ライトの影響先レイヤーマスクを取得する
	/// @param handle 対象ライトのハンドル
	/// @return 設定されているレイヤーマスク。無効な場合はLightLayer::None
	LightLayerMask GetLayerMask(LightHandle handle) const;

	/// @brief ライト名を取得する
	/// @param handle 対象ライトのハンドル
	/// @return ライト名。無効な場合は空文字列
	const std::string& GetName(LightHandle handle) const;

	/// @brief 平行光源本体を取得する
	/// @param handle 対象ライトのハンドル
	/// @return 平行光源本体。無効な場合、または種類が違う場合はnullptr
	const DirectionalLight* GetDirectionalLightData(LightHandle handle) const;

	/// @brief 点光源本体を取得する
	/// @param handle 対象ライトのハンドル
	/// @return 点光源本体。無効な場合、または種類が違う場合はnullptr
	const PointLight* GetPointLightData(LightHandle handle) const;

	/// @brief スポットライト本体を取得する
	/// @param handle 対象ライトのハンドル
	/// @return スポットライト本体。無効な場合、または種類が違う場合はnullptr
	const SpotLight* GetSpotLightData(LightHandle handle) const;

	/// @brief 平行光源本体を書き換える
	/// @param handle 対象ライトのハンドル
	/// @param light 設定する平行光源
	/// @return 書き換えに成功した場合はtrue
	bool SetDirectionalLight(LightHandle handle, const DirectionalLight& light);

	/// @brief 点光源本体を書き換える
	/// @param handle 対象ライトのハンドル
	/// @param light 設定する点光源
	/// @return 書き換えに成功した場合はtrue
	bool SetPointLight(LightHandle handle, const PointLight& light);

	/// @brief スポットライト本体を書き換える
	/// @param handle 対象ライトのハンドル
	/// @param light 設定するスポットライト
	/// @return 書き換えに成功した場合はtrue
	bool SetSpotLight(LightHandle handle, const SpotLight& light);

	/// @brief 平行光源本体の可変ポインタを取得する
	/// @param handle 対象ライトのハンドル
	/// @return 平行光源本体の可変ポインタ。無効な場合、または種類が違う場合はnullptr
	DirectionalLight* MutableDirectionalLight(LightHandle handle);

	/// @brief 点光源本体の可変ポインタを取得する
	/// @param handle 対象ライトのハンドル
	/// @return 点光源本体の可変ポインタ。無効な場合、または種類が違う場合はnullptr
	PointLight* MutablePointLight(LightHandle handle);

	/// @brief スポットライト本体の可変ポインタを取得する
	/// @param handle 対象ライトのハンドル
	/// @return スポットライト本体の可変ポインタ。無効な場合、または種類が違う場合はnullptr
	SpotLight* MutableSpotLight(LightHandle handle);

	/// @brief 平行光源の登録情報を取得する
	/// @param handle 対象ライトのハンドル
	/// @return 平行光源の登録情報。無効な場合はnullptr
	DirectionalLightEntry* GetDirectionalLight(LightHandle handle);

	/// @brief 平行光源の登録情報を取得する
	/// @param handle 対象ライトのハンドル
	/// @return 平行光源の登録情報。無効な場合はnullptr
	const DirectionalLightEntry* GetDirectionalLight(LightHandle handle) const;

	/// @brief 点光源の登録情報を取得する
	/// @param handle 対象ライトのハンドル
	/// @return 点光源の登録情報。無効な場合はnullptr
	PointLightEntry* GetPointLight(LightHandle handle);

	/// @brief 点光源の登録情報を取得する
	/// @param handle 対象ライトのハンドル
	/// @return 点光源の登録情報。無効な場合はnullptr
	const PointLightEntry* GetPointLight(LightHandle handle) const;

	/// @brief スポットライトの登録情報を取得する
	/// @param handle 対象ライトのハンドル
	/// @return スポットライトの登録情報。無効な場合はnullptr
	SpotLightEntry* GetSpotLight(LightHandle handle);

	/// @brief スポットライトの登録情報を取得する
	/// @param handle 対象ライトのハンドル
	/// @return スポットライトの登録情報。無効な場合はnullptr
	const SpotLightEntry* GetSpotLight(LightHandle handle) const;

	/// @brief 登録中の平行光源数を取得する
	/// @return 有効スロットの平行光源数
	size_t GetDirectionalLightCount() const;

	/// @brief 登録中の点光源数を取得する
	/// @return 有効スロットの点光源数
	size_t GetPointLightCount() const;

	/// @brief 登録中のスポットライト数を取得する
	/// @return 有効スロットのスポットライト数
	size_t GetSpotLightCount() const;

	/// @brief 条件に一致する平行光源配列を取得する
	/// @param sceneType 使用するシーン
	/// @param receiveLightMask 受け取るライトレイヤーマスク
	/// @return 条件に一致する平行光源配列
	std::vector<DirectionalLight> GetFilteredDirectionalLights(SceneType sceneType, LightLayerMask receiveLightMask) const;

	/// @brief 条件に一致する点光源配列を取得する
	/// @param sceneType 使用するシーン
	/// @param receiveLightMask 受け取るライトレイヤーマスク
	/// @return 条件に一致する点光源配列
	std::vector<PointLight> GetFilteredPointLights(SceneType sceneType, LightLayerMask receiveLightMask) const;

	/// @brief 条件に一致するスポットライト配列を取得する
	/// @param sceneType 使用するシーン
	/// @param receiveLightMask 受け取るライトレイヤーマスク
	/// @return 条件に一致するスポットライト配列
	std::vector<SpotLight> GetFilteredSpotLights(SceneType sceneType, LightLayerMask receiveLightMask) const;

	/// @brief 条件に一致するライトをGPU送信用データへ詰める
	/// @param sceneType 使用するシーン
	/// @param receiveLightMask 受け取るライトレイヤーマスク
	/// @return GPU送信用の固定長ライト配列データ
	LightGpuData BuildGpuData(SceneType sceneType, LightLayerMask receiveLightMask) const;

	/// @brief キャッシュ済みのGPU送信用ライトデータを取得する
	/// @param sceneType 使用するシーン
	/// @param receiveLightMask 受け取るライトレイヤーマスク
	/// @return GPU送信用ライトデータ
	const LightGpuData& GetCachedGpuData(SceneType sceneType, LightLayerMask receiveLightMask);

private:
	LightManager() = default;
	~LightManager() = default;

	struct LightGpuDataCache {
		uint64_t revision = 0;
		LightGpuData gpuData;
	};

	template <typename TEntry>
	struct LightSlot {
		using entry_type = TEntry;

		TEntry entry;
		uint32_t generation = 1;
		bool active = false;
	};

	using DirectionalSlot = LightSlot<DirectionalLightEntry>;
	using PointSlot = LightSlot<PointLightEntry>;
	using SpotSlot = LightSlot<SpotLightEntry>;

	/// @brief ライトスロットへライトを登録する
	/// @param slots 登録先のライトスロット配列
	/// @param type 登録するライト種別
	/// @param name ライト名
	/// @param entry 登録するライト情報
	/// @return 登録したライトのハンドル
	template <typename TSlot>
	LightHandle CreateLight(
		std::vector<TSlot>& slots,
		LightType type,
		const std::string& name,
		const typename TSlot::entry_type& entry);

	/// @brief ハンドルから共通メタ情報を取得する
	/// @param handle 対象ライトのハンドル
	/// @return 共通メタ情報。無効な場合はnullptr
	LightMetaData* GetMetaData(LightHandle handle);

	/// @brief ハンドルから共通メタ情報を取得する
	/// @param handle 対象ライトのハンドル
	/// @return 共通メタ情報。無効な場合はnullptr
	const LightMetaData* GetMetaData(LightHandle handle) const;

	/// @brief ライト構造体内のuseLightへ有効状態を反映する
	/// @param handle 対象ライトのハンドル
	/// @param enabled 有効にする場合はtrue
	void ApplyEnabledToLight(LightHandle handle, bool enabled);

	/// @brief ハンドルに紐づくライト名を検索マップから削除する
	/// @param handle 削除対象ライトのハンドル
	void EraseName(LightHandle handle);

	/// @brief ライト状態の更新世代を進める
	void AdvanceRevision();

	/// @brief GPUライトデータキャッシュのキーを作成する
	/// @param sceneType 使用するシーン
	/// @param receiveLightMask 受け取るライトレイヤーマスク
	/// @return キャッシュ検索用キー
	uint64_t MakeGpuDataCacheKey(SceneType sceneType, LightLayerMask receiveLightMask) const;

	/// @brief ライトスロット配列を無効化して世代を進める
	/// @param slots 無効化するライトスロット配列
	template <typename TSlot>
	void ClearSlots(std::vector<TSlot>& slots);

	/// @brief Editor管理対象のライトをすべて削除する
	void ClearEditorManagedLights();

	/// @brief ライトスロット配列からEditor管理対象を無効化して世代を進める
	/// @param slots 無効化するライトスロット配列
	template <typename TSlot>
	void ClearEditorManagedSlots(std::vector<TSlot>& slots);

	/// @brief ライトスロット配列から有効なハンドル一覧を取得する
	/// @param slots 検索対象のライトスロット配列
	/// @param type ライト種別
	/// @param editorManagedOnly Editor管理対象のみに絞り込む場合はtrue
	/// @return 有効なハンドル配列
	template <typename TSlot>
	std::vector<LightHandle> GetActiveHandles(
		const std::vector<TSlot>& slots,
		LightType type,
		bool editorManagedOnly = false) const;

	/// @brief ライトが抽出条件に一致するか判定する
	/// @param meta 判定するライトの共通メタ情報
	/// @param sceneType 使用するシーン
	/// @param receiveLightMask 受け取るライトレイヤーマスク
	/// @return 条件に一致する場合はtrue
	bool IsLightMatched(const LightMetaData& meta, SceneType sceneType, LightLayerMask receiveLightMask) const;

	/// @brief 条件に一致するライト配列を取得する
	/// @param slots 検索対象のライトスロット配列
	/// @param sceneType 使用するシーン
	/// @param receiveLightMask 受け取るライトレイヤーマスク
	/// @return 条件に一致するライト配列
	template <typename TLight, typename TSlot>
	std::vector<TLight> GetFilteredLights(const std::vector<TSlot>& slots, SceneType sceneType, LightLayerMask receiveLightMask) const;

	/// @brief 条件に一致するライトを固定長配列へ詰める
	/// @param slots 検索対象のライトスロット配列
	/// @param sceneType 使用するシーン
	/// @param receiveLightMask 受け取るライトレイヤーマスク
	/// @param outLights 出力先の固定長ライト配列
	/// @return 出力したライト数
	template <typename TLight, typename TSlot, size_t MaxLightCount>
	uint32_t FillGpuLights(
		const std::vector<TSlot>& slots,
		SceneType sceneType,
		LightLayerMask receiveLightMask,
		std::array<TLight, MaxLightCount>& outLights) const;

	std::vector<DirectionalSlot> directionalLights_;
	std::vector<PointSlot> pointLights_;
	std::vector<SpotSlot> spotLights_;
	std::unordered_map<std::string, LightHandle> nameToHandle_;
	std::unordered_map<uint64_t, LightGpuDataCache> gpuDataCache_;
	uint64_t revision_ = 1;
};
