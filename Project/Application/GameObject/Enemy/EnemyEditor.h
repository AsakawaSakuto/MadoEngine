#pragma once
#include "EnemyManager.h"
#include "EnemySettings.h"
#include "EnemySpawner.h"
#include <cstddef>
#include <cstdint>

namespace Enemy {

	/// @brief EnemyStatusとEnemySpawner設定を統合編集するクラス
	class Editor final {
	public:
		/// @brief EnemyEditorを実行中Systemへ接続
		/// @param spawner 編集対象のEnemy Spawner
		/// @param manager 実行状態を表示するEnemy Manager
		void Initialize(Spawner* spawner, Manager* manager);

		/// @brief EnemyEditorのImGuiを描画
		void DrawImGui();

	private:
		/// @brief EnemyStatusタブを描画
		/// @return 設定が変更された場合はtrue
		bool DrawEnemyStatus();

		/// @brief EnemySpawnerタブを描画
		/// @return 設定が変更された場合はtrue
		bool DrawEnemySpawner();

		/// @brief 一つのWave設定を描画
		/// @param index Waveの位置
		/// @param wave 編集するWave設定
		/// @param outRemoveRequested 削除操作を受け付けた場合はtrue
		/// @return 設定が変更された場合はtrue
		bool DrawWave(std::size_t index, WaveSettings& wave, bool& outRemoveRequested);

		Spawner* spawner_ = nullptr;
		Manager* manager_ = nullptr;
		Data::Type selectedType_ = Data::Type::Normal;
		std::uint32_t immediateSpawnCount_ = 1;
	};

} // namespace Enemy
