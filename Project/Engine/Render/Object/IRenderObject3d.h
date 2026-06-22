#pragma once
#include <algorithm>
#include <string>
#include "MathHeaders.h"
#include "UtilityHeaders.h"
#include "Core/DxDevice/DxDevice.h"
#include "Core/Command/Command.h"
#include "Core/TextureManager/TextureManager.h"
#include "Render/Object/RenderLayer.h"
#include "Render/PSO/PSODesc.h"
#include "Render/PSO/PSORegistry.h"

/// @brief 3D描画オブジェクトの基底クラス
class IRenderObject3d {
public:
	
	/// @brief デストラクタ
	virtual ~IRenderObject3d() = default;

	/// @brief 初期化処理（派生クラスでオーバーライド）
	/// @param device D3D12デバイス
	/// @param commandList グラフィクスコマンドリスト
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::string modelPath) = 0;

	/// @brief 更新処理（派生クラスでオーバーライド）
	virtual void Update() = 0;

	/// @brief 描画処理（派生クラスでオーバーライド）
	virtual void Draw(Camera& useCamera) = 0;

	// ===== Transform関連 =====

	/// @brief 座標を設定
	/// @param pos ワールド座標
	void SetPosition(const Vector3& pos) { transform_.translate = pos; }

	/// @brief 座標を取得
	/// @return ワールド座標
	const Vector3& GetPosition() const { return transform_.translate; }

	/// @brief スケールを設定
	/// @param scale スケール
	void SetScale(const Vector3& scale) { transform_.scale = scale; }

	/// @brief スケールを取得
	/// @return スケール
	const Vector3& GetScale() const { return transform_.scale; }

	/// @brief 回転角度を設定（ラジアン・XYZ）
	/// @param rotate 回転角度
	void SetRotation(const Vector3& rotate) { transform_.rotate = rotate; }

	/// @brief 回転角度を取得（ラジアン・XYZ）
	/// @return 回転角度
	const Vector3& GetRotation() const { return transform_.rotate; }

	/// @brief Transform全体を取得
	/// @return Transform3D構造体
	const Transform3D& GetTransform() const { return transform_; }

	/// @brief Transform全体を設定
	/// @param transform Transform3D構造体
	void SetTransform(const Transform3D& transform) { transform_ = transform; }

	// ===== 色関連 =====

	/// @brief 色を設定（RGBA）
	/// @param color 色（0.0f〜1.0f）
	void SetColor(const Vector4& color) { color_ = color; }

	/// @brief 色を取得
	/// @return 色（RGBA）
	const Vector4& GetColor() const { return color_; }

	// ===== テクスチャ関連 =====

	/// @brief オブジェクトのテクスチャを変更する
	/// @param textureName TextureManagerに登録されているテクスチャ名
	/// @return テクスチャの変更に成功した場合はtrue
	bool SetTexture(const std::string& textureName) {
		const uint32_t textureIndex = MadoEngine::TextureManager::GetInstance().GetTextureIndex(textureName);
		if (textureIndex == UINT32_MAX) {
			Logger::Output(objectName_ + " のテクスチャ変更に失敗しました。テクスチャが見つかりません: " + textureName, Logger::Level::Warning);
			return false;
		}

		textureName_ = textureName;
		textureIndex_ = textureIndex;

		if (!textureNames_.empty()) {
			std::fill(textureNames_.begin(), textureNames_.end(), textureName);
		}
		if (!textureIndices_.empty()) {
			std::fill(textureIndices_.begin(), textureIndices_.end(), textureIndex);
		}

		Logger::Output(objectName_ + " のテクスチャを変更しました: " + textureName, Logger::Level::Debug);
		return true;
	}

	// ===== 可視性関連 =====

	/// @brief 表示/非表示を設定
	/// @param visible true:表示、false:非表示
	void SetVisible(bool visible) { isVisible_ = visible; }

	/// @brief 表示状態を取得
	/// @return true:表示、false:非表示
	bool IsVisible() const { return isVisible_; }

	/// @brief 描画レイヤーを設定する
	/// @param layer 設定する描画レイヤー
	void SetRenderLayer(MadoEngine::Render::RenderLayer layer) { renderLayer_ = layer; }

	/// @brief 描画レイヤーを取得する
	/// @return 現在の描画レイヤー
	MadoEngine::Render::RenderLayer GetRenderLayer() const { return renderLayer_; }

	/// @brief 指定したレイヤーマスクに自身の描画レイヤーが含まれているか確認する
	/// @param layerMask 判定対象のレイヤーマスク
	/// @return 含まれている場合はtrue
	bool IsRenderLayerIncluded(MadoEngine::Render::RenderLayerMask layerMask) const {
		return MadoEngine::Render::ContainsRenderLayer(layerMask, renderLayer_);
	}

	/// @brief PSORegistryを設定する
	/// @param registry PSORegistryポインタ
	void SetPSORegistry(MadoEngine::Render::PSORegistry* registry) { psoRegistry_ = registry; }

protected:
	std::string objectName_; // オブジェクト名

	Transform3D transform_; // トランスフォーム（座標、スケール、回転）
	Vector4 color_;         // 色（RGBA）
	bool isVisible_;        // 表示フラグ
	MadoEngine::Render::RenderLayer renderLayer_ = MadoEngine::Render::RenderLayer::Default;

	Camera camera_; // カメラ

	std::string textureName_;
	uint32_t textureIndex_ = 0;

	std::vector<std::string> textureNames_;
	std::vector<uint32_t> textureIndices_;

	MadoEngine::Render::PSODesc psoDesc_;                    // PSO記述子
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
	Microsoft::WRL::ComPtr<ID3D12Resource> lightGpuDataResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ = {};
	D3D12_INDEX_BUFFER_VIEW indexBufferView_ = {};
};
