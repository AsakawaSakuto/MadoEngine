#pragma once
#include <string>
#include "MathHeaders.h"
#include "Math/TransformNode2D.h"
#include "UtilityHeaders.h"
#include "Core/DxDevice/DxDevice.h"
#include "Core/Command/Command.h"
#include "Core/TextureManager/TextureManager.h"
#include "Render/Object/RenderLayer.h"
#include "Render/PSO/PSODesc.h"
#include "Render/PSO/PSORegistry.h"

/// @brief 2D描画オブジェクトの基底クラス
class IRenderObject2d : public MadoEngine::TransformNode2D {
public:
	
	/// @brief デストラクタ
	virtual ~IRenderObject2d() = default;

	/// @brief 初期化処理（派生クラスでオーバーライド）
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName) = 0;

	/// @brief 更新処理（派生クラスでオーバーライド）
	virtual void Update() = 0;

	/// @brief 描画処理（派生クラスでオーバーライド）
	virtual void Draw() = 0;

	// ===== 色関連 =====

	/// @brief 色を設定（RGBA）
	/// @param color 色（0.0f～1.0f）
	void SetColor(const Vector4& color) { color_ = color; }

	/// @brief 色を取得
	/// @return 色（RGBA）
	const Vector4& GetColor() const { return color_; }

	/// @brief 階層の透明度を反映した描画色を取得
	/// @return 個別の色に自身と祖先の不透明度を乗算した描画色
	Vector4 GetColorForRendering() const {
		Vector4 color = color_;
		color.w *= GetWorldOpacity();
		return color;
	}

	// ===== 可視性関連 =====

	/// @brief 保存対象の表示設定を維持した実行時の表示許可を設定
	/// @param visible 実行時の表示を許可する場合はtrue
	void SetRuntimeVisible(bool visible) { isRuntimeVisible_ = visible; }

	/// @brief 描画に使用する最終表示状態を取得
	/// @return 自身と祖先の表示設定および実行時の表示許可が有効な場合はtrue
	bool IsVisibleForRendering() const { return IsVisibleInHierarchy() && isRuntimeVisible_; }

	/// @brief 描画レイヤーを設定
	/// @param layer 設定する描画レイヤー
	void SetRenderLayer(MadoEngine::Render::RenderLayer layer) { renderLayer_ = layer; }

	/// @brief 描画レイヤーを取得
	/// @return 現在の描画レイヤー
	MadoEngine::Render::RenderLayer GetRenderLayer() const { return renderLayer_; }

	/// @brief 指定したレイヤーマスクに自身の描画レイヤーが含まれているか確認
	/// @param layerMask 判定対象のレイヤーマスク
	/// @return 含まれている場合はtrue
	bool IsRenderLayerIncluded(MadoEngine::Render::RenderLayerMask layerMask) const {
		return MadoEngine::Render::ContainsRenderLayer(layerMask, renderLayer_);
	}

	/// @brief PSORegistryを設定
	/// @param registry PSORegistryポインタ
	void SetPSORegistry(MadoEngine::Render::PSORegistry* registry) { psoRegistry_ = registry; }

	/// @brief PSO記述子を取得
	/// @return PSO記述子への定数参照
	const MadoEngine::Render::PSODesc& GetPSODesc() const { return psoDesc_; }

	/// @brief ルートシグネチャキーを取得
	/// @return ルートシグネチャキー文字列への定数参照
	const std::string& GetRootSigKey() const { return psoDesc_.rootSigKey; }

	/// @brief スクリーンサイズを設定する（正射影行列の計算に使用）
	/// @param width スクリーン幅（ピクセル）
	/// @param height スクリーン高さ（ピクセル）
	void SetScreenSize(float width, float height) { screenWidth_ = width; screenHeight_ = height; }

protected:

	std::string objectName_; // オブジェクト名

	Vector4 color_ = {1.0f,1.0f,1.0f,1.0f}; // 色（RGBA）
	bool isRuntimeVisible_ = true;          // 保存対象へ含めない実行時の表示許可
	MadoEngine::Render::RenderLayer renderLayer_ = MadoEngine::Render::RenderLayer::Default;

	uint32_t textureIndex_ = 0;
	Vector2 size_ = {};
	std::string textureName_;

	MadoEngine::Render::PSODesc psoDesc_;           // PSO記述子
	MadoEngine::Render::PSORegistry* psoRegistry_ = nullptr; // PSOレジストリ（外部からセット）

	// デバイス
	Microsoft::WRL::ComPtr<ID3D12Device> device_;

	// コマンドリスト
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

	// リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource_;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ = {};
	D3D12_INDEX_BUFFER_VIEW indexBufferView_ = {};

	// スクリーンサイズ（正射影行列計算用）
	float screenWidth_  = 1280.0f;
	float screenHeight_ = 720.0f;
};
