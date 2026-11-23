#pragma once
#include "MapGenerationSettings.h"
#include "MapBlock.h"
#include "DecorationObject/MapDecorationObjectBase.h"
#include <memory>
#include <vector>

/// @brief 生成済み地形への装飾オブジェクト配置を担当するクラス
class MapDecorationObjectSpawner final {
public:
	/// @brief 一回の生成に使用する設定と配置先を接続
	/// @param settings 適用済みの生成設定
	/// @param blocks 配置先の読み取り専用地形
	/// @param decorationObjects 生成した装飾オブジェクトの格納先
	MapDecorationObjectSpawner(const MapGenerationSettings& settings,
		const std::vector<std::vector<MapBlock>>& blocks,
		std::vector<std::unique_ptr<MapDecorationObjectBase>>& decorationObjects);

	/// @brief 指定シードから生成した一つの乱数系列を全装飾で共有して配置
	/// @param isVisible 生成直後から表示する場合はtrue
	void Generate(bool isVisible);

	/// @brief Map再生成時に装飾用の共有描画Batchを破棄
	static void DestroyInstancedBatches();

private:
	const MapGenerationSettings& settings_;
	const std::vector<std::vector<MapBlock>>& mapBlocks_;
	std::vector<std::unique_ptr<MapDecorationObjectBase>>& decorationObjects_;
};
