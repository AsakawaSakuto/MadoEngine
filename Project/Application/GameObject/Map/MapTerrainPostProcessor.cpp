#include "MapTerrainPostProcessor.h"
#include <algorithm>
#include <array>

namespace {
/// @brief 坂の上り方向と隣接セルへの差分
struct SlopeCandidate {
	int offsetX;
	int offsetZ;
	SlopeDirection direction;
};

constexpr std::array<SlopeCandidate, 4> kSlopeCandidates = { {
	{ 1, 0, SlopeDirection::PulsX },
	{ -1, 0, SlopeDirection::MinusX },
	{ 0, 1, SlopeDirection::PulsZ },
	{ 0, -1, SlopeDirection::MinusZ }
} };
}

MapTerrainPostProcessor::MapTerrainPostProcessor(Grid& cells) : cells_(cells) {
	depth_ = static_cast<int>(cells_.size());
	if (!cells_.empty()) {
		width_ = static_cast<int>(cells_.front().size());
	}
}

void MapTerrainPostProcessor::SmoothHeights(int passCount) {
	if (width_ == 0 || depth_ == 0) {
		return;
	}

	// 同じ高さのつながりを残して孤立した凹凸だけを有限回で補正
	for (int pass = 0; pass < passCount; ++pass) {
		if (!SmoothLocalHeights()) {
			break;
		}
	}
}

bool MapTerrainPostProcessor::SmoothLocalHeights() {
	Grid nextCells = cells_;
	bool isChanged = false;
	for (int z = 0; z < depth_; ++z) {
		for (int x = 0; x < width_; ++x) {
			const uint32_t currentHeight = cells_[z][x].height;
			int sameHeightNeighborCount = 0;
			uint32_t lowestNeighborHeight = UINT32_MAX;
			bool isSurroundedByHigherCells = true;

			// 対角の低地に影響されず四方の閉塞を判定し、同じ高さの接続数も収集
			for (const SlopeCandidate& neighbor : kSlopeCandidates) {
				const int neighborX = x + neighbor.offsetX;
				const int neighborZ = z + neighbor.offsetZ;
				if (!IsInside(neighborX, neighborZ)) {
					isSurroundedByHigherCells = false;
					continue;
				}
				const uint32_t neighborHeight = cells_[neighborZ][neighborX].height;
				lowestNeighborHeight = std::min(lowestNeighborHeight, neighborHeight);
				if (neighborHeight <= currentHeight) {
					isSurroundedByHigherCells = false;
				}
				if (neighborHeight == currentHeight) {
					++sameHeightNeighborCount;
				}
			}

			// 四方が高い通常地形だけを最低の隣接高さまで埋め、脱出口のない一マスの窪みを除去
			if (cells_[z][x].type == MapBlockType::Ground && isSurroundedByHigherCells) {
				nextCells[z][x].height = lowestNeighborHeight;
				isChanged = true;
				continue;
			}

			// 平地の縁や尾根を毎回削らないよう、同じ高さへ二方向以上つながる地形を維持
			if (sameHeightNeighborCount >= 2) {
				continue;
			}
			std::array<uint32_t, 9> neighborHeights{};
			int neighborCount = 0;

			// 孤立したセルと細い先端だけを近傍の中央値へ寄せ、小さな突起や穴を抑制
			for (int offsetZ = -1; offsetZ <= 1; ++offsetZ) {
				for (int offsetX = -1; offsetX <= 1; ++offsetX) {
					if (IsInside(x + offsetX, z + offsetZ)) {
						neighborHeights[neighborCount] = cells_[z + offsetZ][x + offsetX].height;
						++neighborCount;
					}
				}
			}
			std::sort(neighborHeights.begin(), neighborHeights.begin() + neighborCount);

			// 外周で近傍数が偶数の場合は中央二値の範囲を残し、高低どちらかへの偏りを防止
			const uint32_t lowerMedian = neighborHeights[(neighborCount - 1) / 2];
			const uint32_t upperMedian = neighborHeights[neighborCount / 2];
			const uint32_t nextHeight = std::clamp(currentHeight, lowerMedian, upperMedian);
			nextCells[z][x].height = nextHeight;
			if (nextHeight != currentHeight) {
				isChanged = true;
			}
		}
	}

	// 全セルを同じ補正前の状態から判定し、走査順による平滑化の偏りを防止
	cells_.swap(nextCells);
	return isChanged;
}

void MapTerrainPostProcessor::GenerateSlopes(Random& random, float spawnRate, bool ensureFlatLandings) {
	std::vector<std::vector<bool>> landingCells(static_cast<std::size_t>(depth_),
		std::vector<bool>(static_cast<std::size_t>(width_), false));

	// 再適用時も以前の坂や向きに依存せず、確定済みの高さから地形種別を再構築
	for (std::vector<Cell>& row : cells_) {
		for (Cell& cell : row) {
			cell.type = MapBlockType::Ground;
			cell.slopeDirection = SlopeDirection::PulsX;
		}
	}

	for (int z = 0; z < depth_; ++z) {
		for (int x = 0; x < width_; ++x) {
			if (ensureFlatLandings && landingCells[z][x]) {
				continue;
			}

			Cell& cell = cells_[z][x];
			for (const SlopeCandidate& candidate : kSlopeCandidates) {
				const int highX = x + candidate.offsetX;
				const int highZ = z + candidate.offsetZ;
				const int lowX = x - candidate.offsetX;
				const int lowZ = z - candidate.offsetZ;

				// 坂の低端が外周を向く候補と一段上へ届かない候補を除外
				if (!IsInside(highX, highZ) || !IsInside(lowX, lowZ) ||
					cells_[highZ][highX].height != cell.height + 1) {
					continue;
				}
				if (ensureFlatLandings) {

					// 両端の地表高さと平地を確保し、崖や別方向の坂へ接続する候補を除外
					if (cells_[lowZ][lowX].height != cell.height ||
						cells_[lowZ][lowX].type != MapBlockType::Ground ||
						cells_[highZ][highX].type != MapBlockType::Ground) {
						continue;
					}
				}

				// 選択方向の優先順位を維持し、採用率の抽選は一セルにつき一回に限定
				if (random.Float(0.0f, 1.0f) < spawnRate) {
					cell.type = MapBlockType::Slope;
					cell.slopeDirection = candidate.direction;
					if (ensureFlatLandings) {

						// 後続セルを坂へ変えて入口や出口が浮く状態を防ぐため平地を予約
						landingCells[lowZ][lowX] = true;
						landingCells[highZ][highX] = true;
					}
				}
				break;
			}
		}
	}
}

void MapTerrainPostProcessor::FinalizeTerrain(Random& random, float spawnRate) {
	const Random slopeRandomStart = random;

	// 高さの補正後も同じ乱数系列の先頭から坂を再構築し、再試行回数による配置の変動を防止
	// 窪みの補正は既存地表の範囲で高さを上げる方向だけに限定し、有限回で収束
	do {
		random = slopeRandomStart;
		GenerateSlopes(random, spawnRate, true);
	} while (FillEnclosedGroundRegions());
}

bool MapTerrainPostProcessor::FillEnclosedGroundRegions() {
	Grid nextCells = cells_;
	std::vector<std::vector<bool>> visited(static_cast<std::size_t>(depth_),
		std::vector<bool>(static_cast<std::size_t>(width_), false));
	bool isChanged = false;
	for (int z = 0; z < depth_; ++z) {
		for (int x = 0; x < width_; ++x) {
			if (visited[z][x] || cells_[z][x].type != MapBlockType::Ground) {
				continue;
			}
			const uint32_t regionHeight = cells_[z][x].height;
			uint32_t lowestBoundaryHeightTwice = UINT32_MAX;
			bool hasOutlet = false;
			std::vector<std::array<int, 2>> region = { { x, z } };
			visited[z][x] = true;

			// 同じ高さの通常地形を一領域として探索し、複数マスの底を持つ窪みも検出
			for (std::size_t index = 0; index < region.size(); ++index) {
				const int regionX = region[index][0];
				const int regionZ = region[index][1];
				for (const SlopeCandidate& neighbor : kSlopeCandidates) {
					const int neighborX = regionX + neighbor.offsetX;
					const int neighborZ = regionZ + neighbor.offsetZ;
					if (!IsInside(neighborX, neighborZ)) {

						// 外周へ続く低地は一律に埋めず、Map全体の起伏と通常の崖を維持
						hasOutlet = true;
						continue;
					}
					const Cell& neighborCell = cells_[neighborZ][neighborX];
					if (neighborCell.type == MapBlockType::Air) {
						hasOutlet = true;
						continue;
					}
					if (neighborCell.type == MapBlockType::Ground && neighborCell.height == regionHeight) {
						if (!visited[neighborZ][neighborX]) {
							visited[neighborZ][neighborX] = true;
							region.push_back({ neighborX, neighborZ });
						}
						continue;
					}

					// 隣接セルの共有辺で判定し、坂の低端を出口として保ちつつ高端と側面の壁を検出
					const uint32_t boundaryHeightTwice = GetEdgeHeightTwice(neighborCell,
						-neighbor.offsetX, -neighbor.offsetZ);
					lowestBoundaryHeightTwice = std::min(lowestBoundaryHeightTwice, boundaryHeightTwice);
					if (boundaryHeightTwice <= regionHeight * 2) {
						hasOutlet = true;
					}
				}
			}
			if (hasOutlet || lowestBoundaryHeightTwice == UINT32_MAX) {
				continue;
			}

			// 坂の側面の半段差も整数段へ切り上げ、最も低い境界まで領域全体を埋める補正
			const uint32_t filledHeight = (lowestBoundaryHeightTwice + 1) / 2;
			for (const std::array<int, 2>& position : region) {
				nextCells[position[1]][position[0]].height = filledHeight;
			}
			isChanged = true;
		}
	}

	// 全領域を同じ補正前の状態で判定し、隣接する窪みの走査順による判定差を防止
	cells_.swap(nextCells);
	return isChanged;
}

uint32_t MapTerrainPostProcessor::GetEdgeHeightTwice(const Cell& cell, int offsetX, int offsetZ) const {
	const uint32_t baseHeightTwice = cell.height * 2;
	if (cell.type != MapBlockType::Slope) {
		return baseHeightTwice;
	}

	// 坂の方向と判定辺の向きから低端・高端・側面中央の半段差を整数で表現
	for (const SlopeCandidate& candidate : kSlopeCandidates) {
		if (candidate.direction != cell.slopeDirection) {
			continue;
		}
		const int facing = candidate.offsetX * offsetX + candidate.offsetZ * offsetZ;
		if (facing > 0) {
			return baseHeightTwice + 2;
		}
		if (facing < 0) {
			return baseHeightTwice;
		}
		return baseHeightTwice + 1;
	}
	return baseHeightTwice;
}

bool MapTerrainPostProcessor::IsInside(int x, int z) const {
	return x >= 0 && x < width_ && z >= 0 && z < depth_;
}
