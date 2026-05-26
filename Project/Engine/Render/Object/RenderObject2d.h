#pragma once
#include <string>
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Utility/Transform.h"
#include "Core/DxDevice/DxDevice.h"
#include "Core/Command/Command.h"
#include "Core/TextureManager/TextureManager.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include "Render/PSO/PSODesc.h"
#include "Render/PSO/PSORegistry.h"

/// @brief 2D描画オブジェクトの基底クラス
class RenderObject2d {
public:
	
	/// @brief デストラクタ
	virtual ~RenderObject2d() = default;

	/// @brief 初期化処理（派生クラスでオーバーライド）
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string textureName) = 0;

	/// @brief 更新処理（派生クラスでオーバーライド）
	virtual void Update() = 0;

	/// @brief 描画処理（派生クラスでオーバーライド）
	virtual void Draw() = 0;

	// ===== Transform関連 =====

	/// @brief 座標を設定
	/// @param pos 座標
	void SetPosition(const Vector2& pos) { transform_.translate = pos; }

	/// @brief 座標を取得
	/// @return 座標
	const Vector2& GetPosition() const { return transform_.translate; }

	/// @brief スケールを設定
	/// @param scale スケール
	void SetScale(const Vector2& scale) { transform_.scale = scale; }

	/// @brief スケールを取得
	/// @return スケール
	const Vector2& GetScale() const { return transform_.scale; }

	/// @brief 回転角度を設定（ラジアン）
	/// @param rotate 回転角度
	void SetRotation(float rotate) { transform_.rotate = rotate; }

	/// @brief 回転角度を取得（ラジアン）
	/// @return 回転角度
	float GetRotation() const { return transform_.rotate; }

	/// @brief Transform全体を取得
	/// @return Transform2D構造体
	const Transform2D& GetTransform() const { return transform_; }

	/// @brief Transform全体を設定
	/// @param transform Transform2D構造体
	void SetTransform(const Transform2D& transform) { transform_ = transform; }

	// ===== 色関連 =====

	/// @brief 色を設定（RGBA）
	/// @param color 色（0.0f～1.0f）
	void SetColor(const Vector4& color) { color_ = color; }

	/// @brief 色を取得
	/// @return 色（RGBA）
	const Vector4& GetColor() const { return color_; }

	// ===== 可視性関連 =====

	/// @brief 表示/非表示を設定
	/// @param visible true:表示、false:非表示
	void SetVisible(bool visible) { isVisible_ = visible; }

	/// @brief 表示状態を取得
	/// @return true:表示、false:非表示
	bool IsVisible() const { return isVisible_; }

	/// @brief PSORegistryを設定する
	/// @param registry PSORegistryポインタ
	void SetPSORegistry(MadoEngine::Render::PSORegistry* registry) { psoRegistry_ = registry; }

	/// @brief PSO記述子を取得する
	/// @return PSO記述子への定数参照
	const MadoEngine::Render::PSODesc& GetPSODesc() const { return psoDesc_; }

	/// @brief ルートシグネチャキーを取得する
	/// @return ルートシグネチャキー文字列への定数参照
	const std::string& GetRootSigKey() const { return psoDesc_.rootSigKey; }

	/// @brief スクリーンサイズを設定する（正射影行列の計算に使用）
	/// @param width スクリーン幅（ピクセル）
	/// @param height スクリーン高さ（ピクセル）
	void SetScreenSize(float width, float height) { screenWidth_ = width; screenHeight_ = height; }

protected:

	std::string objectName_; // オブジェクト名

	Transform2D transform_; // トランスフォーム（座標、スケール、回転）
	Vector4 color_ = {1.0f,1.0f,1.0f,1.0f}; // 色（RGBA）
	bool isVisible_ = true;                 // 表示フラグ

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
