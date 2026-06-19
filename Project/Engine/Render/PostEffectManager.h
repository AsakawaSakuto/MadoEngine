#pragma once
#include "Render/LayerEffectPass.h"
#include <cstddef>
#include <d3d12.h>
#include <string>
#include <vector>

namespace MadoEngine::Render {

	/// @brief ポストエフェクトの登録状態と実行時パラメータを管理するクラス
	class PostEffectManager {
	public:
		/// @brief ポストエフェクト管理を初期化する
		/// @param basePostEffectDesc ポストエフェクト用の基本PSO設定
		/// @param device D3D12デバイス
		void Initialize(const PSODesc& basePostEffectDesc, ID3D12Device* device);

		/// @brief レイヤー対象のポストエフェクトPassを追加する
		/// @param desc 追加するPassの生成設定
		/// @return 追加したPassのポインタ
		LayerEffectPass* AddLayerPass(const LayerEffectPass::Desc& desc);

		/// @brief 画面全体のポストエフェクトPassを追加する
		/// @param desc 追加するPassの生成設定
		/// @return 追加したPassのポインタ
		LayerEffectPass* AddScreenPass(const LayerEffectPass::Desc& desc);

		/// @brief レイヤー対象のPassを指定indexで削除する
		/// @param index 削除するPassのindex
		/// @return 削除できた場合はtrue
		bool RemoveLayerPass(std::size_t index);

		/// @brief 画面全体のPassを指定indexで削除する
		/// @param index 削除するPassのindex
		/// @return 削除できた場合はtrue
		bool RemoveScreenPass(std::size_t index);

		/// @brief Pass名または内部キーからPassを削除する
		/// @param name 削除するPass名または内部キー
		/// @return 削除できた場合はtrue
		bool RemovePass(const std::string& name);

		/// @brief レイヤー対象のPassをすべて削除する
		void ClearLayerPasses();

		/// @brief 画面全体のPassをすべて削除する
		void ClearScreenPasses();

		/// @brief レイヤー対象のPass一覧を取得する
		/// @return レイヤー対象のPass配列
		std::vector<LayerEffectPass>& GetLayerPasses();

		/// @brief レイヤー対象のPass一覧を取得する
		/// @return レイヤー対象のPass配列
		const std::vector<LayerEffectPass>& GetLayerPasses() const;

		/// @brief 画面全体のPass一覧を取得する
		/// @return 画面全体のPass配列
		std::vector<LayerEffectPass>& GetScreenPasses();

		/// @brief 画面全体のPass一覧を取得する
		/// @return 画面全体のPass配列
		const std::vector<LayerEffectPass>& GetScreenPasses() const;

		/// @brief Pass名からPassを検索する
		/// @param name 検索するPass名
		/// @return 見つかったPass。存在しない場合はnullptr
		LayerEffectPass* FindPass(const std::string& name);

		/// @brief Pass名からPassを検索する
		/// @param name 検索するPass名
		/// @return 見つかったPass。存在しない場合はnullptr
		const LayerEffectPass* FindPass(const std::string& name) const;

		/// @brief Passの有効状態を変更する
		/// @param name 対象Pass名
		/// @param enabled 有効にする場合はtrue
		/// @return 変更できた場合はtrue
		bool SetEnabled(const std::string& name, bool enabled);

		/// @brief Passの有効状態を取得する
		/// @param name 対象Pass名
		/// @param outEnabled 取得した有効状態の出力先
		/// @return 取得できた場合はtrue
		bool TryGetEnabled(const std::string& name, bool& outEnabled) const;

		/// @brief Passのfloatパラメータを変更する
		/// @param passName 対象Pass名
		/// @param parameterKey 対象パラメータキー
		/// @param value 設定する値
		/// @return 変更できた場合はtrue
		bool SetFloatParameter(const std::string& passName, const std::string& parameterKey, float value);

		/// @brief Passのfloatパラメータを取得する
		/// @param passName 対象Pass名
		/// @param parameterKey 対象パラメータキー
		/// @param outValue 取得した値の出力先
		/// @return 取得できた場合はtrue
		bool TryGetFloatParameter(const std::string& passName, const std::string& parameterKey, float& outValue) const;

		/// @brief 最初に有効なレイヤー対象Passを取得する
		/// @return 有効なPass。存在しない場合はnullptr
		const LayerEffectPass* GetFirstEnabledLayerPass() const;

		/// @brief 有効なレイヤー対象Passの対象LayerMaskをまとめて取得する
		/// @return 有効なPassの対象LayerMask
		RenderLayerMask GetEnabledLayerTargetMask() const;

		/// @brief 指定LayerMaskの描画にDepth無視マスクが必要か判定する
		/// @param layerMask 判定するLayerMask
		/// @return Depth無視が必要な場合はtrue
		bool NeedsIgnoreDepthMask(RenderLayerMask layerMask) const;

	private:
		/// @brief 指定配列からPass名を検索する
		/// @param passes 検索対象のPass配列
		/// @param name 検索するPass名
		/// @return 見つかったPass。存在しない場合はnullptr
		static LayerEffectPass* FindPassIn(std::vector<LayerEffectPass>& passes, const std::string& name);

		/// @brief 指定配列からPass名を検索する
		/// @param passes 検索対象のPass配列
		/// @param name 検索するPass名
		/// @return 見つかったPass。存在しない場合はnullptr
		static const LayerEffectPass* FindPassIn(const std::vector<LayerEffectPass>& passes, const std::string& name);

		/// @brief 指定配列からPass名または内部キーに一致するPassを削除する
		/// @param passes 削除対象のPass配列
		/// @param name 削除するPass名または内部キー
		/// @return 削除できた場合はtrue
		static bool RemovePassIn(std::vector<LayerEffectPass>& passes, const std::string& name);

		PSODesc basePostEffectDesc_{};
		ID3D12Device* device_ = nullptr;
		std::vector<LayerEffectPass> layerPasses_;
		std::vector<LayerEffectPass> screenPasses_;
	};

} // namespace MadoEngine::Render
