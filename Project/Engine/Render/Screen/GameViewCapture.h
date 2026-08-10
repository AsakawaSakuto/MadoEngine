#pragma once
#include <d3d12.h>
#include <filesystem>

namespace MadoEngine::Render {

	class RenderTexture;

	/// @brief GameViewの最終描画テクスチャをPNGファイルへ保存するクラス
	class GameViewCapture final {
	public:
		/// @brief キャプチャ機能を初期化
		/// @param commandQueue GPUからの読み戻しに使用するコマンドキュー
		/// @param outputDirectory PNGファイルの出力先ディレクトリ
		void Initialize(
			ID3D12CommandQueue* commandQueue,
			const std::filesystem::path& outputDirectory
		);

		/// @brief GameViewの描画結果をPNGファイルへ保存
		/// @param sourceTexture GameViewに表示している最終描画テクスチャ
		/// @return 保存に成功した場合はtrue
		[[nodiscard]] bool Capture(const RenderTexture& sourceTexture) const;

	private:
		/// @brief 現在日時を含む重複しない出力パスを生成
		/// @return PNGファイルの出力パス
		[[nodiscard]] std::filesystem::path CreateOutputPath() const;

		ID3D12CommandQueue* commandQueue_ = nullptr;
		std::filesystem::path outputDirectory_;
	};

} // namespace MadoEngine::Render
