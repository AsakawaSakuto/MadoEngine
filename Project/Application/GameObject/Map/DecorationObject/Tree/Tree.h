#pragma once
#include "../MapDecorationObjectBase.h"

/// @brief 木のModelを使用するMap装飾オブジェクト
class Tree final : public MapDecorationObjectBase {
public:
	static constexpr const char* kBatchName = "MapDecoration.Tree";

	/// @brief 木の描画インスタンスを初期化
	/// @param desc 描画インスタンスの共通初期化設定
	/// @return 描画インスタンスを登録した場合はtrue
	bool Initialize(const InitializeDesc& desc) override;

	/// @brief 木の共有描画Batchを破棄
	static void DestroyInstancedBatches();
};
