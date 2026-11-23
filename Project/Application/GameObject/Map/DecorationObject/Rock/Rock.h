#pragma once
#include "../MapDecorationObjectBase.h"

/// @brief 指定した岩のModelを使用するMap装飾オブジェクト
class Rock final : public MapDecorationObjectBase {
public:
	static constexpr int kModelCount = 3;

	/// @brief Model名の末尾番号を指定して岩の装飾オブジェクトを構築
	/// @param modelIndex Model名の末尾へ付ける0始まりの番号
	explicit Rock(int modelIndex);

	/// @brief 種別に応じた岩の描画インスタンスを初期化
	/// @param desc 描画インスタンスの共通初期化設定
	/// @return 描画インスタンスを登録した場合はtrue
	bool Initialize(const InitializeDesc& desc) override;

	/// @brief 岩の全Modelに対応する共有描画Batchを破棄
	static void DestroyInstancedBatches();

private:
	static constexpr const char* kBatchNamePrefix = "MapDecoration.Rock";
	const int modelIndex_;
};
