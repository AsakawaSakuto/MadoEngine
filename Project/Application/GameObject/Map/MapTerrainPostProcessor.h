#pragma once
#include "MapBlockType.h"
#include "Utility/Collider/Shape/SlopeDirection.h"
#include "Utility/Random.h"
#include <cstdint>
#include <vector>

/// @brief 描画と衝突リソースの生成前に地形の凹凸と坂の接続を整理するクラス
class MapTerrainPostProcessor {
public:
	/// @brief リソースを所有しない生成途中の地形セル
	struct Cell {
		uint32_t height = 1;
		MapBlockType type = MapBlockType::Ground;
		SlopeDirection slopeDirection = SlopeDirection::PulsX;
	};

	using Grid = std::vector<std::vector<Cell>>;

	/// @brief 補正対象の地形格子を指定して初期化
	/// @param cells Z行とX列で構成する長方形の地形格子
	explicit MapTerrainPostProcessor(Grid& cells);

	/// @brief 連続した起伏と複数段の段差を維持して孤立した凹凸を補正
	/// @param passCount 局所的な平滑化の適用回数
	void SmoothHeights(int passCount);

	/// @brief 確定した高さを基準に坂を配置
	/// @param random 坂の採用判定に使用する乱数系列
	/// @param spawnRate 条件を満たすセルへの坂の配置確率
	/// @param ensureFlatLandings 坂の両端に通常地形を確保する場合はtrue
	void GenerateSlopes(Random& random, float spawnRate, bool ensureFlatLandings);

	/// @brief 坂配置後の閉じた窪みを補正し、地形と坂の接続を確定
	/// @param random 坂の採用判定に使用する乱数系列
	/// @param spawnRate 条件を満たすセルへの坂の配置確率
	void FinalizeTerrain(Random& random, float spawnRate);

private:
	/// @brief 四方に囲まれた窪みを埋め、同じ高さのつながりを維持した平滑化を一回適用
	/// @return 高さを変更した場合はtrue
	bool SmoothLocalHeights();

	/// @brief 坂の接地面を考慮して出口のない通常地形の連続領域を埋める補正
	/// @return 高さを変更した場合はtrue
	bool FillEnclosedGroundRegions();

	/// @brief セルの辺中央における地表高さを二倍の整数値で取得
	/// @param cell 高さを取得するセル
	/// @param offsetX セル中心から判定辺へ向かうX方向の差分
	/// @param offsetZ セル中心から判定辺へ向かうZ方向の差分
	/// @return 地表高さを二倍した整数値
	uint32_t GetEdgeHeightTwice(const Cell& cell, int offsetX, int offsetZ) const;

	/// @brief 指定セルが地形格子の範囲内か判定
	/// @param x 格子上のX座標
	/// @param z 格子上のZ座標
	/// @return 範囲内の場合はtrue
	bool IsInside(int x, int z) const;

	Grid& cells_;
	int width_ = 0;
	int depth_ = 0;
};
