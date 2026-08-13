#pragma once
#include "RibbonEffectTypes.h"
#include "Render/PSO/PSORegistry.h"
#include "Utility/Camera/Camera.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <d3d12.h>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>

namespace MadoEngine::Ribbon {

	/// @brief Ribbon描画データからCPU帯メッシュを生成して描画するRenderer
	class RibbonEffectRenderer3d final {
	public:
		/// @brief 未初期化のRendererを構築
		RibbonEffectRenderer3d() = default;

		/// @brief Rendererが所有するResourceを解放して破棄
		~RibbonEffectRenderer3d();

		/// @brief RendererのCopy構築を禁止
		/// @param other Copy元Renderer
		RibbonEffectRenderer3d(const RibbonEffectRenderer3d&) = delete;

		/// @brief RendererのCopy代入を禁止
		/// @param other Copy元Renderer
		/// @return 代入結果
		RibbonEffectRenderer3d& operator=(const RibbonEffectRenderer3d&) = delete;

		/// @brief Rendererを初期化
		/// @param device D3D12 Device
		/// @param commandList 描画Command List
		/// @param psoRegistry PSO Registry
		void Initialize(
			ID3D12Device* device,
			ID3D12GraphicsCommandList* commandList,
			MadoEngine::Render::PSORegistry* psoRegistry
		);

		/// @brief Rendererが所有するGPU Resourceを解放
		void Finalize();

		/// @brief 1 Frame分の描画データ登録を開始
		/// @param camera 描画Camera
		/// @param submissionFenceValue 今回のCommand提出に対応するFence値
		void Begin(const Camera& camera, uint64_t submissionFenceValue);

		/// @brief Ribbon描画データを登録して帯メッシュへ変換
		/// @param data Instanceが生成した描画データ
		void Submit(const RibbonRenderData& data);

		/// @brief 対象Layerの登録済みRibbonを描画
		/// @param layerMask 描画対象Layer Mask
		void Draw(MadoEngine::Render::RenderLayerMask layerMask);

		/// @brief GPU完了済みFence値を通知
		/// @param completedFenceValue GPU完了済みFence値
		void OnGpuFrameCompleted(uint64_t completedFenceValue);

	private:
		struct RibbonVertex {
			Vector3 position{};
			Vector2 uv{};
			Vector4 color{};
		};
		static_assert(sizeof(RibbonVertex) == 36, "Ribbon VertexのCPU/HLSL Layoutが一致していません。");

		struct alignas(16) PerViewForGPU {
			Matrix4x4 viewProjection{};
		};
		static_assert(sizeof(PerViewForGPU) == 64, "Ribbon PerViewのCPU/HLSL Layoutが一致していません。");

		struct SmoothedPoint {
			Vector3 position{};
			float normalizedLifetime = 0.0f;
			Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		};

		struct DrawBatch {
			uint32_t firstIndex = 0;
			uint32_t indexCount = 0;
			uint32_t textureIndex = 0;
			MadoEngine::Render::BlendMode blendMode = MadoEngine::Render::BlendMode::Add;
			MadoEngine::Render::CullMode cullMode = MadoEngine::Render::CullMode::None;
			MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::Effect;
		};

		struct FrameResource {
			Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
			Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
			Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource;
			RibbonVertex* mappedVertices = nullptr;
			uint32_t* mappedIndices = nullptr;
			PerViewForGPU* mappedPerView = nullptr;
			D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
			D3D12_INDEX_BUFFER_VIEW indexBufferView{};
			std::size_t vertexCapacity = 0;
			std::size_t indexCapacity = 0;
			uint64_t fenceValue = 0;
		};

		/// @brief Point列を補間して描画用Point列を構築
		/// @param data Ribbon描画データ
		/// @return 重複除去と補間を適用したPoint列
		std::vector<SmoothedPoint> BuildSmoothedPoints(const RibbonRenderData& data) const;

		/// @brief 再生モードに応じて距離基準の表示区間を切り出し
		/// @param points 補間済みPoint列
		/// @param data Ribbon描画データ
		/// @return 表示区間の境界点を補間したPoint列
		std::vector<SmoothedPoint> BuildVisiblePoints(
			const std::vector<SmoothedPoint>& points,
			const RibbonRenderData& data
		) const;

		/// @brief Cameraと接線から安全なRibbon横方向を算出
		/// @param point Ribbon中心位置
		/// @param tangent Ribbon接線
		/// @param previousSide 直前の有効な横方向
		/// @param cameraFacing Camera Facingを使用する場合はtrue
		/// @return 正規化済み横方向
		Vector3 ResolveSideDirection(
			const Vector3& point,
			const Vector3& tangent,
			const Vector3& previousSide,
			bool cameraFacing
		) const;

		/// @brief 選択中Frame ResourceのBuffer容量を確保
		/// @param requiredVertexCount 必要Vertex数
		/// @param requiredIndexCount 必要Index数
		/// @return 容量確保に成功した場合はtrue
		bool EnsureBufferCapacity(
			std::size_t requiredVertexCount,
			std::size_t requiredIndexCount
		);

		/// @brief Texture名からTexture Indexを解決
		/// @param textureName Texture Manager登録名
		/// @return Texture Index
		uint32_t ResolveTextureIndex(const std::string& textureName);

		/// @brief Ribbon用PSO設定を生成
		/// @param blendMode Blend Mode
		/// @param cullMode Cull Mode
		/// @return Ribbon用PSO設定
		MadoEngine::Render::PSODesc CreatePSODesc(
			MadoEngine::Render::BlendMode blendMode,
			MadoEngine::Render::CullMode cullMode
		) const;

		static constexpr uint32_t kFrameResourceCount = 3;
		ID3D12Device* device_ = nullptr;
		ID3D12GraphicsCommandList* commandList_ = nullptr;
		MadoEngine::Render::PSORegistry* psoRegistry_ = nullptr;
		std::array<FrameResource, kFrameResourceCount> frameResources_;
		std::vector<RibbonVertex> vertices_;
		std::vector<uint32_t> indices_;
		std::vector<DrawBatch> batches_;
		std::unordered_map<std::string, uint32_t> textureIndexCache_;
		Vector3 cameraPosition_{};
		Vector3 cameraRight_ = { 1.0f, 0.0f, 0.0f };
		uint32_t currentFrameResourceIndex_ = (std::numeric_limits<uint32_t>::max)();
		uint32_t nextFrameResourceIndex_ = 0;
		uint64_t completedFenceValue_ = 0;
		bool isInitialized_ = false;
	};

} // namespace MadoEngine::Ribbon
