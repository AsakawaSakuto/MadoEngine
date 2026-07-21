#pragma once
#include "ParticleTypes.h"
#include "Render/PSO/PSORegistry.h"
#include "Utility/Camera/Camera.h"
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

	/// @brief CPU Particleをビルボードでインスタンス描画するRenderer
	class ParticleRenderer3d final {
	public:
		ParticleRenderer3d() = default;
		~ParticleRenderer3d();

		ParticleRenderer3d(const ParticleRenderer3d&) = delete;
		ParticleRenderer3d& operator=(const ParticleRenderer3d&) = delete;

		/// @brief Rendererを初期化する
		/// @param device D3D12Device
		/// @param commandList 描画に使用するCommandList
		/// @param psoRegistry PSO Registry
		void Initialize(
			ID3D12Device* device,
			ID3D12GraphicsCommandList* commandList,
			MadoEngine::Render::PSORegistry* psoRegistry
		);

		/// @brief Rendererが所有するGPUリソースを解放する
		void Finalize();

		/// @brief 1回分の描画データ登録を開始する
		/// @param camera 描画に使用するCamera
		void Begin(const Camera& camera);

		/// @brief Emitterの生存Particleを描画データへ登録する
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

		/// @brief 登録済みParticleから対象Layerを描画する
		/// @param layerMask 描画対象LayerMask
		void Draw(MadoEngine::Render::RenderLayerMask layerMask);

		/// @brief 登録済みParticle数を取得する
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
		};

		struct DrawBatch {
			uint32_t firstInstance = 0;
			uint32_t instanceCount = 0;
			uint32_t textureIndex = 0;
			MadoEngine::Render::BlendMode blendMode = MadoEngine::Render::BlendMode::Add;
			MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::Effect;
		};

		/// @brief InstanceBuffer容量を必要数以上へ拡張する
		/// @param requiredCount 必要なInstance数
		void EnsureInstanceCapacity(std::size_t requiredCount);

		/// @brief テクスチャ名からTextureIndexを取得する
		/// @param textureName TextureManagerへ登録されている名前
		/// @return TextureIndex
		uint32_t ResolveTextureIndex(const std::string& textureName);

		/// @brief Particle用PSO設定を作成する
		/// @param blendMode 使用するBlendMode
		/// @return Particle用PSO設定
		MadoEngine::Render::PSODesc CreatePSODesc(MadoEngine::Render::BlendMode blendMode) const;

		ID3D12Device* device_ = nullptr;
		ID3D12GraphicsCommandList* commandList_ = nullptr;
		MadoEngine::Render::PSORegistry* psoRegistry_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
		Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
		Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource_;
		Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource_;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
		ParticleInstanceForGPU* mappedInstances_ = nullptr;
		PerViewForGPU* mappedPerView_ = nullptr;
		uint32_t instanceSrvIndex_ = (std::numeric_limits<uint32_t>::max)();
		std::size_t instanceCapacity_ = 0;

		Vector3 cameraPosition_{};
		std::vector<ParticleInstanceForGPU> instances_;
		std::vector<DrawBatch> batches_;
		std::unordered_set<std::string> missingTextureNames_;
		bool isInstanceDataDirty_ = false;
		bool isInitialized_ = false;
	};

} // namespace MadoEngine::Particle
