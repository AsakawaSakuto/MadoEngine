#pragma once

#include "Render/PostEffect/PostEffectPass.h"
#include "Render/PostEffect/PostEffectParameters.h"
#include "Render/PostEffect/PostEffectPassHandle.h"
#include <cstddef>
#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace MadoEngine::Render {

/// @brief 個別描画レイヤー向けPassの生成設定
struct LayerPostEffectPassCreateDesc {
	std::string key;
	std::string name;
	RenderLayerMask targetLayerMask = ToRenderLayerMask(RenderLayer::Default);
	LayerEffectStage layerEffectStage = LayerEffectStage::Scene;
	std::string effectShaderKey = "PostEffect/CopyImage.PS";
	bool enabled = true;
	bool ignoreDepthForMask = false;
};

/// @brief フルスクリーン向けPassの生成設定
struct ScreenPostEffectPassCreateDesc {
	std::string key;
	std::string name;
	ScreenEffectStage screenEffectStage = ScreenEffectStage::Final;
	std::string effectShaderKey = "PostEffect/CopyImage.PS";
	bool enabled = true;
};

/// @brief ポストエフェクトPassを世代付きSlotで一元管理するクラス
class PostEffectManager {
public:
	/// @brief PostEffectManagerのシングルトンインスタンスを取得する
	/// @return PostEffectManagerの参照
	static PostEffectManager& GetInstance();

	PostEffectManager(const PostEffectManager&) = delete;
	PostEffectManager& operator=(const PostEffectManager&) = delete;
	PostEffectManager(PostEffectManager&&) = delete;
	PostEffectManager& operator=(PostEffectManager&&) = delete;

	/// @brief ポストエフェクト管理を初期化する
	/// @param basePostEffectDesc ポストエフェクト用の基本PSO設定
	/// @param device D3D12デバイス
	void Initialize(const PSODesc& basePostEffectDesc, ID3D12Device* device);

	/// @brief GPU完了待機後に登録済みPassとデバイス参照を解放する
	void Finalize();

	/// @brief 個別描画レイヤー向けPassを生成する
	/// @param desc 生成設定
	/// @return 生成したPassのHandle。key重複や設定不正の場合は無効Handle
	[[nodiscard]] PostEffectPassHandle CreateLayerPass(const LayerPostEffectPassCreateDesc& desc);

	/// @brief フルスクリーン向けPassを生成する
	/// @param desc 生成設定
	/// @return 生成したPassのHandle。key重複や設定不正の場合は無効Handle
	[[nodiscard]] PostEffectPassHandle CreateScreenPass(const ScreenPostEffectPassCreateDesc& desc);

	/// @brief 内部キーからPassを検索する
	/// @param key 検索する不変の内部キー
	/// @return 見つかったPassのHandle。存在しない場合は無効Handle
	[[nodiscard]] PostEffectPassHandle Find(const std::string& key) const;

	/// @brief HandleからPassを一時参照として取得する
	/// @param handle 取得対象のHandle
	/// @return 有効な場合はPass、無効な場合はnullptr
	PostEffectPass* TryGet(PostEffectPassHandle handle);

	/// @brief HandleからPassを読み取り専用の一時参照として取得する
	/// @param handle 取得対象のHandle
	/// @return 有効な場合はPass、無効な場合はnullptr
	const PostEffectPass* TryGet(PostEffectPassHandle handle) const;

	/// @brief Handleが現在のPassを参照しているか確認する
	/// @param handle 確認するHandle
	/// @return 有効なPassを参照している場合はtrue
	[[nodiscard]] bool IsValid(PostEffectPassHandle handle) const;

	/// @brief Handleが参照するPassの適用先を取得する
	/// @param handle 確認するHandle
	/// @return 適用先。無効Handleの場合はCount
	[[nodiscard]] PostEffectPassScope GetScope(PostEffectPassHandle handle) const;

	/// @brief GPUが対象を使用していないことが保証された時点でPassを即時削除する
	/// @param handle 削除対象のHandle
	/// @return 削除できた場合はtrue
	bool Destroy(PostEffectPassHandle handle);

	/// @brief 描画中でも安全なGPU完了待機後までPassの削除を延期する
	/// @param handle 削除対象のHandle
	void RequestDestroy(PostEffectPassHandle handle);

	/// @brief GPU完了待機後に延期されているPass削除を実行する
	void FlushPendingDestroys();

	/// @brief 個別描画レイヤー向けPassをGPU安全時点で即時全削除する
	void ClearLayerPasses();

	/// @brief フルスクリーン向けPassをGPU安全時点で即時全削除する
	void ClearScreenPasses();

	/// @brief 個別描画レイヤー向けPassの実行順Handle一覧を取得する
	/// @return 実行順Handle一覧
	const std::vector<PostEffectPassHandle>& GetLayerPassHandles() const;

	/// @brief フルスクリーン向けPassの実行順Handle一覧を取得する
	/// @return 実行順Handle一覧
	const std::vector<PostEffectPassHandle>& GetScreenPassHandles() const;

	/// @brief 個別描画レイヤー向けPassの実行位置を変更する
	/// @param handle 移動対象のHandle
	/// @param newIndex 移動先index
	/// @return 移動できた場合はtrue
	bool MoveLayerPass(PostEffectPassHandle handle, std::size_t newIndex);

	/// @brief フルスクリーン向けPassの実行位置を変更する
	/// @param handle 移動対象のHandle
	/// @param newIndex 移動先index
	/// @return 移動できた場合はtrue
	bool MoveScreenPass(PostEffectPassHandle handle, std::size_t newIndex);

	/// @brief Handleを維持したままEffect種別とParameterを既定値へ置き換える
	/// @param handle 変更対象のHandle
	/// @param effectType 新しいEffect種別
	/// @return 変更できた場合はtrue
	bool SetEffectType(PostEffectPassHandle handle, PostEffectType effectType);

	/// @brief Passの有効状態を変更する
	/// @param handle 対象PassのHandle
	/// @param enabled 有効にする場合はtrue
	/// @return 変更できた場合はtrue
	bool SetEnabled(PostEffectPassHandle handle, bool enabled);

	/// @brief Passの有効状態を取得する
	/// @param handle 対象PassのHandle
	/// @param outEnabled 取得した有効状態の出力先
	/// @return 取得できた場合はtrue
	bool TryGetEnabled(PostEffectPassHandle handle, bool& outEnabled) const;

	/// @brief Passのfloatパラメータを変更する
	/// @param handle 対象PassのHandle
	/// @param parameterKey 対象パラメータキー
	/// @param value 設定する値
	/// @return 変更できた場合はtrue
	bool SetFloatParameter(PostEffectPassHandle handle, const std::string& parameterKey, float value);

	/// @brief Passのfloatパラメータを取得する
	/// @param handle 対象PassのHandle
	/// @param parameterKey 対象パラメータキー
	/// @param outValue 取得した値の出力先
	/// @return 取得できた場合はtrue
	bool TryGetFloatParameter(PostEffectPassHandle handle, const std::string& parameterKey, float& outValue) const;

	/// @brief 内部キー指定でPassの有効状態を変更する互換API
	/// @param key 対象Passの内部キー
	/// @param enabled 有効にする場合はtrue
	/// @return 変更できた場合はtrue
	bool SetEnabled(const std::string& key, bool enabled);

	/// @brief 内部キー指定でPassの有効状態を取得する互換API
	/// @param key 対象Passの内部キー
	/// @param outEnabled 取得した有効状態の出力先
	/// @return 取得できた場合はtrue
	bool TryGetEnabled(const std::string& key, bool& outEnabled) const;

	/// @brief 内部キー指定でfloatパラメータを変更する互換API
	/// @param passKey 対象Passの内部キー
	/// @param parameterKey 対象パラメータキー
	/// @param value 設定する値
	/// @return 変更できた場合はtrue
	bool SetFloatParameter(const std::string& passKey, const std::string& parameterKey, float value);

	/// @brief 内部キー指定でfloatパラメータを取得する互換API
	/// @param passKey 対象Passの内部キー
	/// @param parameterKey 対象パラメータキー
	/// @param outValue 取得した値の出力先
	/// @return 取得できた場合はtrue
	bool TryGetFloatParameter(const std::string& passKey, const std::string& parameterKey, float& outValue) const;

	/// @brief Effect型を検証してParameterを設定する
	/// @tparam T PostEffectParameterTraitsへ登録済みのParameter型
	/// @param handle 対象PassのHandle
	/// @param parameters 設定するParameter
	/// @return 型とサイズが一致して設定できた場合はtrue
	template<class T>
	bool SetParameters(PostEffectPassHandle handle, const T& parameters) {
		static_assert(PostEffectParameterTraits<T>::kIsSupported, "未登録のPostEffect Parameter型です");
		static_assert(std::is_trivially_copyable_v<T>, "PostEffect Parameter型は単純コピー可能である必要があります");
		return SetTypedParameterData(
			handle,
			PostEffectParameterTraits<T>::kEffectType,
			&parameters,
			sizeof(T)
		);
	}

	/// @brief Effect型を検証してParameterを取得する
	/// @tparam T PostEffectParameterTraitsへ登録済みのParameter型
	/// @param handle 対象PassのHandle
	/// @param outParameters 取得先
	/// @return 型とサイズが一致して取得できた場合はtrue
	template<class T>
	bool TryGetParameters(PostEffectPassHandle handle, T& outParameters) const {
		static_assert(PostEffectParameterTraits<T>::kIsSupported, "未登録のPostEffect Parameter型です");
		static_assert(std::is_trivially_copyable_v<T>, "PostEffect Parameter型は単純コピー可能である必要があります");
		return TryGetTypedParameterData(
			handle,
			PostEffectParameterTraits<T>::kEffectType,
			&outParameters,
			sizeof(T)
		);
	}

	/// @brief 型付きParameterを短時間だけ編集して書き戻す
	/// @tparam T PostEffectParameterTraitsへ登録済みのParameter型
	/// @tparam Callback T&を受け取るcallback型
	/// @param handle 対象PassのHandle
	/// @param callback Parameterを変更するcallback
	/// @return 型とサイズが一致して更新できた場合はtrue
	template<class T, class Callback>
	bool UpdateParameters(PostEffectPassHandle handle, Callback&& callback) {
		T parameters{};
		if (!TryGetParameters(handle, parameters)) {
			return false;
		}

		std::forward<Callback>(callback)(parameters);
		return SetParameters(handle, parameters);
	}

	/// @brief 有効な個別描画レイヤー向けPassの対象LayerMaskをまとめて取得する
	/// @return 有効なPassの対象LayerMask
	RenderLayerMask GetEnabledLayerTargetMask() const;

	/// @brief 指定段階で有効な個別描画レイヤー向けPassの対象LayerMaskをまとめて取得する
	/// @param stage 対象の適用段階
	/// @return 指定段階で有効なPassの対象LayerMask
	RenderLayerMask GetEnabledLayerTargetMask(LayerEffectStage stage) const;

	/// @brief 指定LayerMaskの描画にDepth無視マスクが必要か判定する
	/// @param layerMask 判定するLayerMask
	/// @return Depth無視が必要な場合はtrue
	bool NeedsIgnoreDepthMask(RenderLayerMask layerMask) const;

	/// @brief 指定段階とLayerMaskの描画にDepth無視マスクが必要か判定する
	/// @param layerMask 判定するLayerMask
	/// @param stage 対象の適用段階
	/// @return Depth無視が必要な場合はtrue
	bool NeedsIgnoreDepthMask(RenderLayerMask layerMask, LayerEffectStage stage) const;

private:
	struct PostEffectPassSlot {
		std::unique_ptr<PostEffectPass> pass;
		std::string key;
		PostEffectPassScope scope = PostEffectPassScope::Layer;
		uint32_t generation = 1;
		bool active = false;
	};

	/// @brief 型付きParameter APIの重複警告を識別する理由
	enum class ParameterWarningReason : uint32_t {
		TypeMismatch,
		SizeMismatch,
	};

	/// @brief 同じ不一致を毎フレーム出力しないための警告識別子
	struct ParameterWarningKey {
		PostEffectPassHandle handle{};
		PostEffectType expectedType = PostEffectType::CopyImage;
		ParameterWarningReason reason = ParameterWarningReason::TypeMismatch;

		bool operator==(const ParameterWarningKey&) const = default;
	};

	PostEffectManager() = default;

	/// @brief 共通設定から指定scopeのPassを生成する
	/// @param desc PostEffectPass互換の生成設定
	/// @param scope Passの適用先
	/// @return 生成したPassのHandle
	[[nodiscard]] PostEffectPassHandle CreatePass(const PostEffectPass::Desc& desc, PostEffectPassScope scope);

	/// @brief 指定実行順配列内でHandleを移動する
	/// @param order 対象の実行順配列
	/// @param handle 移動対象のHandle
	/// @param newIndex 移動先index
	/// @return 移動できた場合はtrue
	static bool MovePass(std::vector<PostEffectPassHandle>& order, PostEffectPassHandle handle, std::size_t newIndex);

	/// @brief 型とサイズを検証してParameterデータを設定する
	/// @param handle 対象PassのHandle
	/// @param expectedType 期待するEffect種別
	/// @param data 設定するデータ
	/// @param sizeInBytes データサイズ
	/// @return 設定できた場合はtrue
	bool SetTypedParameterData(
		PostEffectPassHandle handle,
		PostEffectType expectedType,
		const void* data,
		std::size_t sizeInBytes
	);

	/// @brief 型とサイズを検証してParameterデータを取得する
	/// @param handle 対象PassのHandle
	/// @param expectedType 期待するEffect種別
	/// @param outData 取得先
	/// @param sizeInBytes データサイズ
	/// @return 取得できた場合はtrue
	bool TryGetTypedParameterData(
		PostEffectPassHandle handle,
		PostEffectType expectedType,
		void* outData,
		std::size_t sizeInBytes
	) const;

	/// @brief 同一Handleと型のParameter不一致を一度だけLoggerへ出力する
	/// @param handle 対象PassのHandle
	/// @param expectedType APIが期待したEffect種別
	/// @param reason 不一致理由
	/// @param message 出力する日本語メッセージ
	void LogParameterWarningOnce(
		PostEffectPassHandle handle,
		PostEffectType expectedType,
		ParameterWarningReason reason,
		const std::string& message
	) const;

	PSODesc basePostEffectDesc_{};
	ID3D12Device* device_ = nullptr;
	std::vector<PostEffectPassSlot> slots_;
	std::vector<uint32_t> freeSlots_;
	std::unordered_map<std::string, PostEffectPassHandle> keyToHandle_;
	std::vector<PostEffectPassHandle> layerPassOrder_;
	std::vector<PostEffectPassHandle> screenPassOrder_;
	std::vector<PostEffectPassHandle> pendingDestroyHandles_;
	mutable std::vector<ParameterWarningKey> parameterWarningKeys_;
	bool isInitialized_ = false;
};

} // namespace MadoEngine::Render
