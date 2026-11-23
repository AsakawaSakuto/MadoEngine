#pragma once
#include "MapDecorationObjectSettings.h"
#include "Math/Transform.h"
#include "Math/Vector4.h"
#include "Render/Object/ObjectHandle.h"
#include "Utility/Collider/Shape/AABB.h"
#include "Utility/Collider/Shape/Segment.h"
#include <cstdint>
#include <string>

/// @brief 遮蔽判定用AABBと描画インスタンスを管理するMap装飾オブジェクトの基底クラス
class MapDecorationObjectBase {
public:
	struct InitializeDesc {
		Transform3D transform;
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		MapDecorationObjectSettings settings;
		bool isVisible = true;
	};

	/// @brief 共有Batch内の描画インスタンスを非表示化して破棄
	virtual ~MapDecorationObjectBase();

	/// @brief 描画インスタンスの所有権の複製を禁止
	/// @param other 複製元の装飾オブジェクト
	MapDecorationObjectBase(const MapDecorationObjectBase& other) = delete;

	/// @brief 描画インスタンスの所有権のコピー代入を禁止
	/// @param other 代入元の装飾オブジェクト
	/// @return 代入先の装飾オブジェクト
	MapDecorationObjectBase& operator=(const MapDecorationObjectBase& other) = delete;

	/// @brief 種別に応じた装飾モデルを初期化
	/// @param desc 描画インスタンスの共通初期化設定
	/// @return 描画インスタンスを登録した場合はtrue
	virtual bool Initialize(const InitializeDesc& desc) = 0;

	/// @brief 描画インスタンスの表示状態を設定
	/// @param isVisible 表示する場合はtrue
	void SetVisible(bool isVisible);

	/// @brief CameraとPlayerを結ぶ視線に応じた遮蔽表示を更新
	/// @param sightLine CameraからPlayerへのワールド座標の線分
	void UpdateCameraOcclusion(const Segment& sightLine);

	/// @brief 設定済みの遮蔽判定用AABBをデバッグラインで描画
	void DrawDebugLine() const;

protected:
	/// @brief 未登録の装飾オブジェクトを構築
	MapDecorationObjectBase() = default;

	/// @brief 派生クラスが指定したモデルを共有Batchへ登録
	/// @param desc 描画インスタンスの共通初期化設定
	/// @param batchName 共有描画Batchの登録名
	/// @param modelName 描画に使用するModelアセット名
	/// @return 描画インスタンスを登録した場合はtrue
	bool InitializeInstancedDraw(const InitializeDesc& desc,
		const std::string& batchName, const std::string& modelName);

	/// @brief 装飾の不透明用と半透明用の共有描画Batchを破棄
	/// @param batchName 不透明用の共有描画Batch名
	static void DestroyInstancedDrawBatches(const std::string& batchName);

private:
	/// @brief 表示設定とCamera遮蔽に応じて不透明表示と半透明表示を切り替え
	void ApplyVisibility();

	AABB cameraOcclusionBounds_;
	bool isVisible_ = true;
	bool isCameraOccluded_ = false;
	MadoEngine::InstancedModelHandle instancedModel_{};
	MadoEngine::InstancedModelHandle occludedInstancedModel_{};
	uint32_t instanceHandle_ = UINT32_MAX;
	uint32_t occludedInstanceHandle_ = UINT32_MAX;
};
