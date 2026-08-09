#pragma once
#include "Render/Object/RenderLayer.h"
#include "Render/PostEffect/PostEffectDefinitionRegistry.h"
#include "Render/PostEffect/PostEffectType.h"
#include "Render/PSO/PSODesc.h"
#include <cstddef>
#include <cstdint>
#include <d3d12.h>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>
#include <wrl/client.h>

namespace MadoEngine::Render {
	/// @brief 個別描画レイヤー向けポストエフェクトの適用段階
	enum class LayerEffectStage : uint32_t {
		Scene,
		Transparent,
		Overlay,
	};

	/// @brief レイヤーポストエフェクトの適用段階が有効か判定する
	/// @param stage 判定する適用段階
	/// @return 有効な適用段階の場合はtrue
	constexpr bool IsValidLayerEffectStage(LayerEffectStage stage) {
		return stage == LayerEffectStage::Scene ||
			stage == LayerEffectStage::Transparent ||
			stage == LayerEffectStage::Overlay;
	}

	/// @brief フルスクリーンポストエフェクトの適用段階
	enum class ScreenEffectStage : uint32_t {
		Scene,
		Final,
	};

	/// @brief フルスクリーンポストエフェクトの適用段階が有効か判定する
	/// @param stage 判定する適用段階
	/// @return 有効な適用段階の場合はtrue
	constexpr bool IsValidScreenEffectStage(ScreenEffectStage stage) {
		return stage == ScreenEffectStage::Scene || stage == ScreenEffectStage::Final;
	}

	/// @brief LayerとScreenの両方へ適用できるポストエフェクトPass
	class PostEffectPass {
	public:
		/// @brief PostEffectPassの生成設定
		struct Desc {
			std::string key;
			std::string name = "LayerEffect";
			RenderLayerMask targetLayerMask = ToRenderLayerMask(RenderLayer::Default);
			LayerEffectStage layerEffectStage = LayerEffectStage::Scene;
			std::string effectShaderKey = "PostEffect/CopyImage.PS";
			bool enabled = true;
			bool ignoreDepthForMask = false;
			ScreenEffectStage screenEffectStage = ScreenEffectStage::Final;
		};

		/// @brief ImGuiで編集するfloatパラメータの設定
		struct FloatParameterControl {
			std::string key;
			std::string label;
			std::size_t offset = 0;
			float minValue = 0.0f;
			float maxValue = 1.0f;
			float speed = 0.01f;
		};

		/// @brief PostEffectPassを初期化する
		/// @param desc PostEffectPassの生成設定
		/// @param basePostEffectDesc ポストエフェクト用の基本PSO設定
		/// @param device D3D12デバイス
		void Initialize(const Desc& desc, const PSODesc& basePostEffectDesc, ID3D12Device* device);

		/// @brief 有効状態を設定する
		/// @param enabled 有効にする場合はtrue
		void SetEnabled(bool enabled);

		/// @brief 有効状態を取得する
		/// @return 有効な場合はtrue
		bool IsEnabled() const;

		/// @brief マスク描画時にDepthを無視するか設定する
		/// @param ignoreDepth Depthを無視する場合はtrue
		void SetIgnoreDepthForMask(bool ignoreDepth);

		/// @brief マスク描画時にDepthを無視するか取得する
		/// @return Depthを無視する場合はtrue
		bool IsIgnoreDepthForMask() const;

		/// @brief フルスクリーンポストエフェクトの適用段階を設定する
		/// @param stage 設定する適用段階
		void SetScreenEffectStage(ScreenEffectStage stage);

		/// @brief フルスクリーンポストエフェクトの適用段階を取得する
		/// @return 設定されている適用段階
		ScreenEffectStage GetScreenEffectStage() const;

		/// @brief パス名を設定する
		/// @param name 設定するパス名
		void SetName(const std::string& name);

		/// @brief パス名を取得する
		/// @return パス名
		const std::string& GetName() const;

		/// @brief コードから参照するためのパスキーを取得する
		/// @return パスキー
		const std::string& GetKey() const;

		/// @brief 対象Layerを単体で設定する
		/// @param layer 対象Layer
		void SetTargetLayer(RenderLayer layer);

		/// @brief 対象Layerマスクを設定する
		/// @param layerMask 対象Layerマスク
		void SetTargetLayerMask(RenderLayerMask layerMask);

		/// @brief 対象Layerマスクを取得する
		/// @return 対象Layerマスク
		RenderLayerMask GetTargetLayerMask() const;

		/// @brief 個別描画レイヤー向けポストエフェクトの適用段階を設定する
		/// @param stage 設定する適用段階
		void SetLayerEffectStage(LayerEffectStage stage);

		/// @brief 個別描画レイヤー向けポストエフェクトの適用段階を取得する
		/// @return 設定されている適用段階
		LayerEffectStage GetLayerEffectStage() const;

		/// @brief 対象Layerを除外した描画Layerマスクを取得する
		/// @param sourceLayerMask 元になるLayerマスク
		/// @return 対象Layerを除外したLayerマスク
		RenderLayerMask GetBaseLayerMask(RenderLayerMask sourceLayerMask) const;

		/// @brief 適用するPixelShaderキーを設定する
		/// @param shaderKey PixelShaderキー
		void SetEffectShaderKey(const std::string& shaderKey);

		/// @brief Registry定義を適用してEffect種別と既定パラメータを置き換える
		/// @param definition 適用するEffect定義
		void ApplyEffectDefinition(const PostEffectDefinition& definition);

		/// @brief 適用中のポストエフェクト種別を取得する
		/// @return 適用中のポストエフェクト種別。未登録Shaderの場合はstd::nullopt
		std::optional<PostEffectType> GetPostEffectType() const;

		/// @brief 適用するPixelShaderキーを取得する
		/// @return PixelShaderキー
		const std::string& GetEffectShaderKey() const;

		/// @brief ポストエフェクト描画に使用するPSO設定を取得する
		/// @return PSO設定
		const PSODesc& GetEffectPSODesc() const;

		/// @brief ポストエフェクト用パラメータをConstantBufferへ書き込む
		/// @tparam T 書き込むパラメータ構造体
		/// @param parameter 書き込むパラメータ
		template<typename T>
		void SetParameterData(const T& parameter) {
			static_assert(std::is_trivially_copyable_v<T>, "ConstantBufferに書き込む型は単純コピー可能である必要があります");
			SetParameterData(&parameter, sizeof(T));
		}

		/// @brief ポストエフェクト用パラメータをConstantBufferへ書き込む
		/// @param data 書き込むデータ先頭アドレス
		/// @param sizeInBytes 書き込むデータサイズ
		void SetParameterData(const void* data, std::size_t sizeInBytes);

		/// @brief CPU側パラメータを指定領域へ安全にコピーする
		/// @param outData コピー先
		/// @param sizeInBytes コピーするサイズ
		/// @return 保存サイズと指定サイズが一致してコピーできた場合はtrue
		bool TryCopyParameterData(void* outData, std::size_t sizeInBytes) const;

		/// @brief CPU側に保存しているパラメータサイズを取得する
		/// @return パラメータサイズ
		std::size_t GetParameterDataSize() const;

		/// @brief ImGui編集用のfloatパラメータを追加する
		/// @param label UIに表示する名前
		/// @param offset ConstantBuffer内のfloat先頭位置
		/// @param minValue 最小値
		/// @param maxValue 最大値
		/// @param speed ドラッグ操作時の変化量
		void AddFloatParameterControl(
			const std::string& label,
			std::size_t offset,
			float minValue,
			float maxValue,
			float speed = 0.01f
		);

		/// @brief ImGui表示名とは別の内部キーを持つfloatパラメータを追加する
		/// @param key コードから参照するためのパラメータキー
		/// @param label UIに表示する名前
		/// @param offset ConstantBuffer内のfloat先頭位置
		/// @param minValue 最小値
		/// @param maxValue 最大値
		/// @param speed ドラッグ操作時の変化量
		void AddFloatParameterControl(
			const std::string& key,
			const std::string& label,
			std::size_t offset,
			float minValue,
			float maxValue,
			float speed = 0.01f
		);

		/// @brief ImGui編集用のfloatパラメータ一覧を取得する
		/// @return floatパラメータ設定の配列
		const std::vector<FloatParameterControl>& GetFloatParameterControls() const;

		/// @brief ImGui編集用のfloatパラメータ一覧を削除する
		void ClearFloatParameterControls();

		/// @brief ポストエフェクト用パラメータデータを削除する
		void ClearParameterData();

		/// @brief ConstantBuffer内のfloat値を取得する
		/// @param offset ConstantBuffer内のfloat先頭位置
		/// @param outValue 取得した値の出力先
		/// @return 取得できた場合はtrue
		bool TryGetFloatParameter(std::size_t offset, float& outValue) const;

		/// @brief 内部キーからfloatパラメータを取得する
		/// @param key パラメータキー
		/// @param outValue 取得した値の出力先
		/// @return 取得できた場合はtrue
		bool TryGetFloatParameter(const std::string& key, float& outValue) const;

		/// @brief ConstantBuffer内のfloat値を更新する
		/// @param offset ConstantBuffer内のfloat先頭位置
		/// @param value 書き込む値
		void SetFloatParameter(std::size_t offset, float value);

		/// @brief 内部キーからfloatパラメータを更新する
		/// @param key パラメータキー
		/// @param value 設定する値
		/// @return 更新できた場合はtrue
		bool SetFloatParameter(const std::string& key, float value);

		/// @brief パラメータ用ConstantBufferを保持しているかを取得する
		/// @return ConstantBufferを保持している場合はtrue
		bool HasParameterBuffer() const;

		/// @brief パラメータ用ConstantBufferのGPU仮想アドレスを取得する
		/// @return GPU仮想アドレス。未作成の場合は0
		D3D12_GPU_VIRTUAL_ADDRESS GetParameterGPUVirtualAddress() const;

	private:
		/// @brief ConstantBufferサイズへアラインする
		/// @param sizeInBytes 元のサイズ
		/// @return 256バイト境界へ切り上げたサイズ
		static std::size_t AlignConstantBufferSize(std::size_t sizeInBytes);

		/// @brief パラメータ用ConstantBufferを必要に応じて作成する
		/// @param sizeInBytes 必要なデータサイズ
		void EnsureParameterBuffer(std::size_t sizeInBytes);

		/// @brief CPU側パラメータデータをConstantBufferへ反映する
		void UploadParameterData();

		Desc desc_;
		std::optional<PostEffectType> effectType_ = PostEffectType::CopyImage;
		PSODesc effectDesc_;
		ID3D12Device* device_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> parameterResource_;
		std::uint8_t* mappedParameter_ = nullptr;
		std::vector<std::uint8_t> parameterData_;
		std::vector<FloatParameterControl> floatParameterControls_;
		std::size_t parameterSizeInBytes_ = 0;
		std::size_t parameterBufferSizeInBytes_ = 0;
	};

} // namespace MadoEngine::Render
