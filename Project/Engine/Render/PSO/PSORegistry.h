#pragma once
#include "Render/PSO/PSODesc.h"
#include "Render/PSO/PSODescHash.h"
#include "Render/PSO/PSOFactory.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <unordered_map>
#include <vector>
#include <future>
#include <mutex>
#include <string>

namespace MadoEngine::Core {
	class DxDevice;
}

namespace MadoEngine::Render {

	/// @brief PSOのキャッシュ管理クラス
	class PSORegistry {
	public:
		/// @brief 初期化
		/// @param device DxDeviceポインタ
		/// @param factory PSOFactoryポインタ
		void Initialize(Core::DxDevice* device, PSOFactory* factory);

		/// @brief 終了処理（プリウォーム完了待機・ライブラリ保存）
		void Finalize();

		/// @brief PSOを取得する（キャッシュミス時は生成）
		/// @param desc PSO記述子
		/// @return ID3D12PipelineState ポインタ
		ID3D12PipelineState* Get(const PSODesc& desc);

		/// @brief 指定した複数のPSOを同期的にプリウォームする
		/// @param descs プリウォームするPSODescのリスト
		void Prewarm(const std::vector<PSODesc>& descs);

		/// @brief 指定した複数のPSOを非同期でプリウォームする
		/// @param descs プリウォームするPSODescのリスト
		void PrewarmAsync(const std::vector<PSODesc>& descs);

		/// @brief 全プリウォームタスクが完了しているか確認する
		/// @return 完了していれば true
		bool IsPrewarmComplete() const;

		/// @brief PipelineLibraryをファイルからロードする
		/// @param cachePath キャッシュファイルパス
		void LoadPipelineLibrary(const std::wstring& cachePath);

		/// @brief PipelineLibraryをファイルに保存する
		/// @param cachePath キャッシュファイルパス
		void SavePipelineLibrary(const std::wstring& cachePath);

	private:
		/// @brief PSOを生成してキャッシュに追加する
		/// @param desc PSO記述子
		/// @return ID3D12PipelineState ポインタ
		ID3D12PipelineState* CreateAndCache(const PSODesc& desc);

		/// @brief PSODescをPipelineLibrary用キー文字列に変換する
		/// @param desc PSO記述子
		/// @return ワイド文字列キー
		static std::wstring MakeLibraryKey(const PSODesc& desc);

		Core::DxDevice* device_  = nullptr;
		PSOFactory*     factory_ = nullptr;

		std::unordered_map<PSODesc, Microsoft::WRL::ComPtr<ID3D12PipelineState>,
						   PSODescHash, PSODescEqual> psoCache_;
		mutable std::mutex cacheMutex_;

		std::vector<std::future<void>> prewarmTasks_;

		Microsoft::WRL::ComPtr<ID3D12PipelineLibrary1> pipelineLibrary_;
		std::wstring cachePath_;
		bool isDirty_ = false;
	};

} // namespace MadoEngine::Render
