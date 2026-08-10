#pragma once
#include "GpuParticleTypes.h"
#include "ParticleTypes.h"
#include "Render/PSO/PSORegistry.h"
#include "Utility/Camera/Camera.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <d3d12.h>
#include <limits>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>
#include <wrl/client.h>

namespace MadoEngine::Particle {

	/// @brief Particleへ適用するFogパラメータ
	struct alignas(16) ParticleFogParameters {
		Vector4 color = { 0.58f, 0.68f, 0.74f, 1.0f };
		Vector4 distanceParams = { 850.0f, 1000.0f, 1.0f, 0.0f };
		Vector4 cameraParams = { 0.1f, 1000.0f, 0.0f, 0.0f };
	};
	static_assert(sizeof(ParticleFogParameters) == 48, "Particle Fog ParameterのCPU/GPU Layoutが一致していません。");

	/// @brief CPU Particleをビルボードでインスタンス描画するRenderer
	class ParticleRenderer3d final {
	public:
		ParticleRenderer3d() = default;
		~ParticleRenderer3d();

		ParticleRenderer3d(const ParticleRenderer3d&) = delete;
		ParticleRenderer3d& operator=(const ParticleRenderer3d&) = delete;

		/// @brief Rendererを初期化
		/// @param device D3D12Device
		/// @param commandList 描画に使用するCommandList
		/// @param psoRegistry PSO Registry
		void Initialize(
			ID3D12Device* device,
			ID3D12GraphicsCommandList* commandList,
			MadoEngine::Render::PSORegistry* psoRegistry
		);

		/// @brief Rendererが所有するGPUリソースを解放
		void Finalize();

		/// @brief 1回分の描画データ登録を開始
		/// @param camera 描画に使用するCamera
		/// @param submissionFenceValue Command提出へ紐付けるFence値
		void Begin(const Camera& camera, uint64_t submissionFenceValue);

		/// @brief 完了済みFence値を描画Frame Resourceへ通知
		/// @param completedFenceValue GPUが完了済みのFence値
		void OnGpuFrameCompleted(uint64_t completedFenceValue);

		/// @brief Emitterの生存Particleを描画データへ登録
		/// @param particles 生存Particle
		/// @param config Emitter設定
		/// @param emitterTransform EmitterのTransform
		/// @param renderLayer 描画先Layer
		void Submit(
			std::span<const ParticleState> particles,
			const EmitterConfig& config,
			const Transform3D& emitterTransform,
			MadoEngine::Render::RenderLayer renderLayer
		);

		/// @brief GPU Particle Bufferを描画データへ登録
		/// @param renderData GPU描画Resource
		/// @param config Emitter設定
		/// @param renderLayer 描画Layer
		void SubmitGpu(
			const GpuParticleRenderData& renderData,
			const EmitterConfig& config,
			MadoEngine::Render::RenderLayer renderLayer
		);

		/// @brief 登録済みParticleから対象Layerを描画
		/// @param layerMask 描画対象LayerMask
		void Draw(MadoEngine::Render::RenderLayerMask layerMask);

		/// @brief Particle描画へ適用するFogパラメータを設定
		/// @param parameters 適用するFogパラメータ
		void SetFogParameters(const ParticleFogParameters& parameters);

		/// @brief GPU Particle描画経路を利用できるか確認
		/// @return RootSignature、CommandSignature、PSOを利用できる場合はtrue
		bool IsGpuRenderingAvailable() const;

		/// @brief 登録済みParticle数を取得
		/// @return 登録済みParticle数
		std::size_t GetPendingInstanceCount() const { return instances_.size(); }

	private:
		struct alignas(16) ParticleInstanceForGPU {
			Vector3 position{};
			float rotation = 0.0f;
			Vector2 scale = { 1.0f, 1.0f };
			Vector2 padding{};
			Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		};
		static_assert(sizeof(ParticleInstanceForGPU) == 48, "Particle InstanceのCPU/GPU Layoutが一致していません。");

		struct alignas(16) PerViewForGPU {
			Matrix4x4 viewProjection{};
			Vector4 cameraRight{};
			Vector4 cameraUp{};
			ParticleFogParameters fog{};
		};
		static_assert(sizeof(PerViewForGPU) == 144, "Particle PerViewのCPU/GPU Layoutが一致していません。");

		struct DrawBatch {
			uint32_t firstInstance = 0;
			uint32_t instanceCount = 0;
			uint32_t textureIndex = 0;
			MadoEngine::Render::BlendMode blendMode = MadoEngine::Render::BlendMode::Add;
			MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::Effect;
		};

		struct GpuDrawBatch {
			GpuParticleRenderData renderData;
			uint32_t textureIndex = 0;
			MadoEngine::Render::BlendMode blendMode = MadoEngine::Render::BlendMode::Add;
			MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::Effect;
		};

		struct PerViewFrameResource {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			PerViewForGPU* mappedData = nullptr;
			Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource;
			ParticleInstanceForGPU* mappedInstances = nullptr;
			uint32_t instanceSrvIndex = (std::numeric_limits<uint32_t>::max)();
			std::size_t instanceCapacity = 0;
			uint64_t fenceValue = 0;
		};

		/// @brief InstanceBuffer容量を必要数以上へ拡張
		/// @param frameResourceIndex 更新するFrame Resource Index
		/// @param requiredCount 必要なInstance数
		void EnsureInstanceCapacity(
			uint32_t frameResourceIndex,
			std::size_t requiredCount
		);

		/// @brief テクスチャ名からTextureIndexを取得
		/// @param textureName TextureManagerへ登録されている名前
		/// @return TextureIndex
		uint32_t ResolveTextureIndex(const std::string& textureName);

		/// @brief Particle用PSO設定を作成
		/// @param blendMode 使用するBlendMode
		/// @return Particle用PSO設定
		MadoEngine::Render::PSODesc CreatePSODesc(MadoEngine::Render::BlendMode blendMode) const;

		/// @brief GPU Particle用PSO設定を作成
		/// @param blendMode 使用するBlendMode
		/// @return GPU Particle用PSO設定
		MadoEngine::Render::PSODesc CreateGpuPSODesc(
			MadoEngine::Render::BlendMode blendMode
		) const;

		ID3D12Device* device_ = nullptr;
		ID3D12GraphicsCommandList* commandList_ = nullptr;
		MadoEngine::Render::PSORegistry* psoRegistry_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
		Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> gpuDrawCommandSignature_;
		static constexpr uint32_t kFrameResourceCount = 3;
		std::array<PerViewFrameResource, kFrameResourceCount> perViewFrameResources_;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
		uint32_t currentPerViewFrameIndex_ = 0;
		uint32_t nextPerViewFrameIndex_ = 0;
		uint64_t completedFenceValue_ = 0;

		Vector3 cameraPosition_{};
		ParticleFogParameters fogParameters_{};
		std::vector<ParticleInstanceForGPU> instances_;
		std::vector<DrawBatch> batches_;
		std::vector<GpuDrawBatch> gpuBatches_;
		std::unordered_set<std::string> missingTextureNames_;
		bool isInstanceDataDirty_ = false;
		bool hasCurrentPerViewFrame_ = false;
		bool isInitialized_ = false;
	};

} // namespace MadoEngine::Particle
