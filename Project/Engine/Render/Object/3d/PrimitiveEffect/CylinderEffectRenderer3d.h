#pragma once
#include "PrimitiveEffectTypes.h"
#include "Render/PSO/PSORegistry.h"
#include "Utility/Camera/Camera.h"
#include <cstddef>
#include <cstdint>
#include <d3d12.h>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>

namespace MadoEngine::Effect {

	/// @brief Cylinderエフェクトをバッチ描画するRenderer
	class CylinderEffectRenderer3d final {
	public:
		/// @brief Rendererを構築する
		CylinderEffectRenderer3d() = default;

		/// @brief 所有するGPUリソースを解放する
		~CylinderEffectRenderer3d();

		CylinderEffectRenderer3d(const CylinderEffectRenderer3d&) = delete;
		CylinderEffectRenderer3d& operator=(const CylinderEffectRenderer3d&) = delete;

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

		/// @brief Cylinder描画データを登録する
		/// @param data 登録する描画データ
		void Submit(const CylinderRenderData& data);

		/// @brief 登録済みCylinderから対象Layerを描画する
		/// @param layerMask 描画対象LayerMask
		void Draw(MadoEngine::Render::RenderLayerMask layerMask);

		/// @brief 登録済みInstance数を取得する
		/// @return 登録済みInstance数
		std::size_t GetPendingInstanceCount() const {
			return instances_.size();
		}

	private:
		struct GeometryKey {
			uint32_t radialSegments = 0;
			uint32_t heightSegments = 0;

			/// @brief 2つのGeometryKeyを比較する
			/// @param other 比較対象Key
			/// @return 分割数が一致する場合はtrue
			bool operator==(const GeometryKey& other) const = default;
		};

		struct GeometryKeyHash {
			/// @brief GeometryKeyのHash値を計算する
			/// @param key 計算対象Key
			/// @return Hash値
			std::size_t operator()(const GeometryKey& key) const;
		};

		struct SharedGeometry {
			Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
			D3D12_INDEX_BUFFER_VIEW indexBufferView{};
			uint32_t indexCount = 0;
		};

		struct alignas(16) CylinderInstanceForGPU {
			Matrix4x4 world{};
			Vector4 radii{};
			Vector4 geometry{};
			Vector4 uvTransform{};
			Vector4 effectParameters{};
			Vector4 gradientColors[kMaximumCylinderGradientStops]{};
			Vector4 gradientPositions[2]{};
			uint32_t metadata[4]{};
		};
		static_assert(sizeof(CylinderInstanceForGPU) == 304, "Cylinder InstanceのCPU/GPUレイアウトが一致していません。");

		struct alignas(16) PerViewForGPU {
			Matrix4x4 viewProjection{};
		};

		struct DrawBatch {
			GeometryKey geometryKey;
			uint32_t firstInstance = 0;
			uint32_t instanceCount = 0;
			uint32_t textureIndex = 0;
			MadoEngine::Render::BlendMode blendMode = MadoEngine::Render::BlendMode::Add;
			MadoEngine::Render::CullMode cullMode = MadoEngine::Render::CullMode::None;
			MadoEngine::Render::RenderLayer renderLayer = MadoEngine::Render::RenderLayer::Effect;
		};

		/// @brief InstanceBuffer容量を必要数以上へ拡張する
		/// @param requiredCount 必要なInstance数
		void EnsureInstanceCapacity(std::size_t requiredCount);

		/// @brief 分割数に対応する共有インデックスを取得する
		/// @param key 分割数キー
		/// @return 共有インデックス
		SharedGeometry& ResolveGeometry(const GeometryKey& key);

		/// @brief テクスチャ名からTextureIndexを取得する
		/// @param textureName TextureManagerへ登録済みの名前
		/// @return TextureIndex
		uint32_t ResolveTextureIndex(const std::string& textureName);

		/// @brief Cylinder用PSO設定を作成する
		/// @param blendMode BlendMode
		/// @param cullMode CullMode
		/// @return Cylinder用PSO設定
		MadoEngine::Render::PSODesc CreatePSODesc(
			MadoEngine::Render::BlendMode blendMode,
			MadoEngine::Render::CullMode cullMode
		) const;

		ID3D12Device* device_ = nullptr;
		ID3D12GraphicsCommandList* commandList_ = nullptr;
		MadoEngine::Render::PSORegistry* psoRegistry_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> instanceResource_;
		Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource_;
		CylinderInstanceForGPU* mappedInstances_ = nullptr;
		PerViewForGPU* mappedPerView_ = nullptr;
		uint32_t instanceSrvIndex_ = (std::numeric_limits<uint32_t>::max)();
		std::size_t instanceCapacity_ = 0;
		std::vector<CylinderInstanceForGPU> instances_;
		std::vector<DrawBatch> batches_;
		std::unordered_map<GeometryKey, SharedGeometry, GeometryKeyHash> geometryCache_;
		std::unordered_map<std::string, uint32_t> textureIndexCache_;
		bool isInstanceDataDirty_ = false;
		bool isInitialized_ = false;
	};

} // namespace MadoEngine::Effect
