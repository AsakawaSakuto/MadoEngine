#include "RibbonEffectRenderer3d.h"
#include "Core/TextureManager/TextureManager.h"
#include "Math/Function/MatrixFunction.h"
#include "Shader/RootSignatureManager.h"
#include "Utility/Logger/Logger.h"
#include "Utility/ResourceHelper/ResourceHelper.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>

namespace {

	constexpr std::size_t kInitialRibbonVertexCapacity = 2048;
	constexpr std::size_t kInitialRibbonIndexCapacity = 6144;
	constexpr std::size_t kMaximumRibbonVertexCount = 1000000;
	constexpr std::size_t kMaximumRibbonIndexCount = 3000000;
	constexpr float kRibbonGeometryEpsilon = 0.000001f;
	constexpr UINT kRibbonRootPerView = 0;
	constexpr UINT kRibbonRootTexture = 1;

	/// @brief Vector3の全要素が有限値か確認
	/// @param value 確認対象
	/// @return 全要素が有限値の場合はtrue
	bool IsFiniteVector3(const Vector3& value) {
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
	}

	/// @brief 2つのVector3の内積を算出
	/// @param left 左辺
	/// @param right 右辺
	/// @return 内積
	float Dot(const Vector3& left, const Vector3& right) {
		return left.x * right.x + left.y * right.y + left.z * right.z;
	}

	/// @brief 2つのVector3の外積を算出
	/// @param left 左辺
	/// @param right 右辺
	/// @return 外積
	Vector3 Cross(const Vector3& left, const Vector3& right) {
		return {
			left.y * right.z - left.z * right.y,
			left.z * right.x - left.x * right.z,
			left.x * right.y - left.y * right.x,
		};
	}

	/// @brief Vector3を安全に正規化
	/// @param value 正規化対象
	/// @param fallback 長さが不足する場合の値
	/// @return 正規化済みVector3
	Vector3 NormalizeSafe(const Vector3& value, const Vector3& fallback) {
		const float lengthSquared = value.LengthSq();
		if (!std::isfinite(lengthSquared) || lengthSquared <= kRibbonGeometryEpsilon) {
			return fallback;
		}
		return value / std::sqrt(lengthSquared);
	}

	/// @brief Catmull-Rom補間位置を算出
	/// @param p0 1つ前の制御点
	/// @param p1 Segment始点
	/// @param p2 Segment終点
	/// @param p3 1つ後の制御点
	/// @param t 補間率
	/// @return 補間位置
	Vector3 CatmullRom(
		const Vector3& p0,
		const Vector3& p1,
		const Vector3& p2,
		const Vector3& p3,
		float t) {
		const float t2 = t * t;
		const float t3 = t2 * t;
		return (
			p1 * 2.0f +
			(p2 - p0) * t +
			(p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
			(-p0 + p1 * 3.0f - p2 * 3.0f + p3) * t3
		) * 0.5f;
	}

	/// @brief Ribbon頂点色を安全な範囲へ補正
	/// @param color 補正対象色
	/// @param globalAlpha 全体Alpha
	/// @return 補正後色
	Vector4 NormalizeColor(const Vector4& color, float globalAlpha) {
		return {
			(std::max)(0.0f, std::isfinite(color.x) ? color.x : 1.0f),
			(std::max)(0.0f, std::isfinite(color.y) ? color.y : 1.0f),
			(std::max)(0.0f, std::isfinite(color.z) ? color.z : 1.0f),
			std::clamp(std::isfinite(color.w) ? color.w * globalAlpha : globalAlpha, 0.0f, 1.0f),
		};
	}

} // namespace

namespace MadoEngine::Ribbon {

	RibbonEffectRenderer3d::~RibbonEffectRenderer3d() {
		Finalize();
	}

	void RibbonEffectRenderer3d::Initialize(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		MadoEngine::Render::PSORegistry* psoRegistry) {
		assert(device);
		assert(commandList);
		assert(psoRegistry);

		Finalize();
		device_ = device;
		commandList_ = commandList;
		psoRegistry_ = psoRegistry;
		for (FrameResource& frameResource : frameResources_) {
			frameResource.mappedPerView = CreateMappedBuffer<PerViewForGPU>(
				device_,
				frameResource.perViewResource,
				1,
				false
			);
		}
		isInitialized_ = true;
		Logger::Output("RibbonEffectRenderer3dを初期化しました。", Logger::Level::Engine);
	}

	void RibbonEffectRenderer3d::Finalize() {
		for (FrameResource& frameResource : frameResources_) {
			if (frameResource.mappedVertices && frameResource.vertexResource) {
				frameResource.vertexResource->Unmap(0, nullptr);
			}
			if (frameResource.mappedIndices && frameResource.indexResource) {
				frameResource.indexResource->Unmap(0, nullptr);
			}
			if (frameResource.mappedPerView && frameResource.perViewResource) {
				frameResource.perViewResource->Unmap(0, nullptr);
			}
			frameResource = {};
		}
		vertices_.clear();
		indices_.clear();
		batches_.clear();
		textureIndexCache_.clear();
		currentFrameResourceIndex_ = (std::numeric_limits<uint32_t>::max)();
		nextFrameResourceIndex_ = 0;
		completedFenceValue_ = 0;
		device_ = nullptr;
		commandList_ = nullptr;
		psoRegistry_ = nullptr;
		isInitialized_ = false;
	}

	void RibbonEffectRenderer3d::Begin(const Camera& camera, uint64_t submissionFenceValue) {
		vertices_.clear();
		indices_.clear();
		batches_.clear();
		currentFrameResourceIndex_ = (std::numeric_limits<uint32_t>::max)();
		if (!isInitialized_) {
			return;
		}

		for (uint32_t offset = 0; offset < kFrameResourceCount; ++offset) {
			const uint32_t candidate = (nextFrameResourceIndex_ + offset) % kFrameResourceCount;
			if (frameResources_[candidate].fenceValue > completedFenceValue_) {
				continue;
			}
			currentFrameResourceIndex_ = candidate;
			nextFrameResourceIndex_ = (candidate + 1) % kFrameResourceCount;
			break;
		}
		if (currentFrameResourceIndex_ == (std::numeric_limits<uint32_t>::max)()) {
			return;
		}

		FrameResource& frameResource = frameResources_[currentFrameResourceIndex_];
		frameResource.fenceValue = submissionFenceValue;
		frameResource.mappedPerView->viewProjection = camera.GetViewProjectionMatrix();
		cameraPosition_ = camera.GetPosition();
		const Matrix4x4 cameraRotation = Matrix::MakeRotateXYZ(camera.GetRotation());
		cameraRight_ = NormalizeSafe(
			Matrix::Transform(Vector3{ 1.0f, 0.0f, 0.0f }, cameraRotation),
			Vector3{ 1.0f, 0.0f, 0.0f }
		);
	}

	void RibbonEffectRenderer3d::Submit(const RibbonRenderData& data) {
		if (
			currentFrameResourceIndex_ == (std::numeric_limits<uint32_t>::max)() ||
			data.points.size() < kMinimumRibbonPointCount) {
			return;
		}

		const std::vector<SmoothedPoint> smoothedPoints = BuildSmoothedPoints(data);
		const std::vector<SmoothedPoint> points = BuildVisiblePoints(smoothedPoints, data);
		if (points.size() < kMinimumRibbonPointCount) {
			return;
		}
		if (
			points.size() > kMaximumRibbonVertexCount / 2 ||
			points.size() - 1 > kMaximumRibbonIndexCount / 6 ||
			vertices_.size() > kMaximumRibbonVertexCount - points.size() * 2 ||
			indices_.size() > kMaximumRibbonIndexCount - (points.size() - 1) * 6) {
			return;
		}

		std::vector<float> cumulativeDistances(points.size(), 0.0f);
		for (std::size_t index = 1; index < points.size(); ++index) {
			const float segmentLength = (points[index].position - points[index - 1].position).Length();
			cumulativeDistances[index] = cumulativeDistances[index - 1] +
				(std::isfinite(segmentLength) ? segmentLength : 0.0f);
		}
		const float totalLength = cumulativeDistances.back();
		if (!std::isfinite(totalLength) || totalLength <= kRibbonGeometryEpsilon) {
			return;
		}

		const uint32_t baseVertex = static_cast<uint32_t>(vertices_.size());
		const uint32_t firstIndex = static_cast<uint32_t>(indices_.size());
		Vector3 previousTangent = { 0.0f, 0.0f, 1.0f };
		Vector3 previousSide = cameraRight_;
		for (std::size_t index = 0; index < points.size(); ++index) {
			Vector3 tangent;
			if (index == 0) {
				tangent = points[1].position - points[0].position;
			} else if (index + 1 == points.size()) {
				tangent = points[index].position - points[index - 1].position;
			} else {
				tangent = points[index + 1].position - points[index - 1].position;
			}
			tangent = NormalizeSafe(tangent, previousTangent);
			previousTangent = tangent;
			Vector3 side = ResolveSideDirection(
				points[index].position,
				tangent,
				previousSide,
				data.cameraFacing
			);
			if (Dot(side, previousSide) < 0.0f) {
				side = -side;
			}
			previousSide = side;

			const float normalizedLifetime = std::clamp(points[index].normalizedLifetime, 0.0f, 1.0f);
			float width = data.widthOverLifetime.Evaluate(normalizedLifetime);
			width = std::isfinite(width) ? (std::max)(0.0f, width) : 0.0f;
			const Vector3 halfWidth = side * (width * 0.5f);
			const float startAlphaFade = std::clamp(
				std::isfinite(data.startAlphaFade) ? data.startAlphaFade : 0.0f,
				0.0f,
				1.0f
			);
			const float endAlphaFade = std::clamp(
				std::isfinite(data.endAlphaFade) ? data.endAlphaFade : 0.0f,
				0.0f,
				1.0f
			);
			float lengthAlpha = 1.0f;
			if (startAlphaFade > 0.0f) {
				lengthAlpha = (std::min)(lengthAlpha, normalizedLifetime / startAlphaFade);
			}
			if (endAlphaFade > 0.0f) {
				lengthAlpha = (std::min)(
					lengthAlpha,
					(1.0f - normalizedLifetime) / endAlphaFade
				);
			}
			Vector4 sourceColor = data.colorOverLifetime.Evaluate(normalizedLifetime);
			sourceColor.w *= std::clamp(lengthAlpha, 0.0f, 1.0f);
			const Vector4 color = NormalizeColor(sourceColor, data.globalAlpha);

			float ribbonU = data.uvMode == RibbonUvMode::Tile
				? cumulativeDistances[index] / (std::max)(data.tileLength, 0.001f)
				: cumulativeDistances[index] / totalLength;
			ribbonU = std::isfinite(ribbonU) ? ribbonU : 0.0f;
			const float leftU = ribbonU * data.uvScale.x + data.uvOffset.x;
			const float leftV = data.uvOffset.y;
			const float rightV = data.uvScale.y + data.uvOffset.y;
			vertices_.push_back({ points[index].position - halfWidth, { leftU, leftV }, color });
			vertices_.push_back({ points[index].position + halfWidth, { leftU, rightV }, color });
		}

		for (uint32_t index = 0; index + 1 < static_cast<uint32_t>(points.size()); ++index) {
			const uint32_t left = baseVertex + index * 2;
			const uint32_t right = left + 1;
			const uint32_t nextLeft = left + 2;
			const uint32_t nextRight = left + 3;
			indices_.push_back(left);
			indices_.push_back(nextLeft);
			indices_.push_back(right);
			indices_.push_back(nextLeft);
			indices_.push_back(nextRight);
			indices_.push_back(right);
		}

		const uint32_t indexCount = static_cast<uint32_t>(indices_.size()) - firstIndex;
		const uint32_t textureIndex = ResolveTextureIndex(data.textureName);
		if (!batches_.empty()) {
			DrawBatch& lastBatch = batches_.back();
			if (
				lastBatch.firstIndex + lastBatch.indexCount == firstIndex &&
				lastBatch.textureIndex == textureIndex &&
				lastBatch.blendMode == data.blendMode &&
				lastBatch.cullMode == data.cullMode &&
				lastBatch.renderLayer == data.renderLayer) {
				lastBatch.indexCount += indexCount;
				return;
			}
		}

		DrawBatch batch;
		batch.firstIndex = firstIndex;
		batch.indexCount = indexCount;
		batch.textureIndex = textureIndex;
		batch.blendMode = data.blendMode;
		batch.cullMode = data.cullMode;
		batch.renderLayer = data.renderLayer;
		batches_.push_back(batch);
	}

	void RibbonEffectRenderer3d::Draw(MadoEngine::Render::RenderLayerMask layerMask) {
		if (
			currentFrameResourceIndex_ == (std::numeric_limits<uint32_t>::max)() ||
			vertices_.empty() ||
			indices_.empty() ||
			layerMask == 0) {
			return;
		}
		if (!EnsureBufferCapacity(vertices_.size(), indices_.size())) {
			return;
		}

		FrameResource& frameResource = frameResources_[currentFrameResourceIndex_];
		std::memcpy(
			frameResource.mappedVertices,
			vertices_.data(),
			vertices_.size() * sizeof(RibbonVertex)
		);
		std::memcpy(
			frameResource.mappedIndices,
			indices_.data(),
			indices_.size() * sizeof(uint32_t)
		);

		ID3D12RootSignature* rootSignature =
			MadoEngine::RootSignatureManager::GetInstance().Get("RibbonEffect3d.RootSig");
		assert(rootSignature);
		commandList_->SetGraphicsRootSignature(rootSignature);
		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList_->IASetVertexBuffers(0, 1, &frameResource.vertexBufferView);
		commandList_->IASetIndexBuffer(&frameResource.indexBufferView);
		commandList_->SetGraphicsRootConstantBufferView(
			kRibbonRootPerView,
			frameResource.perViewResource->GetGPUVirtualAddress()
		);

		for (const DrawBatch& batch : batches_) {
			if (!MadoEngine::Render::ContainsRenderLayer(layerMask, batch.renderLayer)) {
				continue;
			}
			commandList_->SetPipelineState(psoRegistry_->Get(CreatePSODesc(batch.blendMode, batch.cullMode)));
			commandList_->SetGraphicsRootDescriptorTable(
				kRibbonRootTexture,
				MadoEngine::TextureManager::GetInstance().GetSrvHandleGPU(batch.textureIndex)
			);
			commandList_->DrawIndexedInstanced(batch.indexCount, 1, batch.firstIndex, 0, 0);
		}
	}

	void RibbonEffectRenderer3d::OnGpuFrameCompleted(uint64_t completedFenceValue) {
		completedFenceValue_ = (std::max)(completedFenceValue_, completedFenceValue);
	}

	std::vector<RibbonEffectRenderer3d::SmoothedPoint>
	RibbonEffectRenderer3d::BuildSmoothedPoints(const RibbonRenderData& data) const {
		std::vector<SmoothedPoint> source;
		source.reserve(data.points.size());
		for (const RibbonPoint& point : data.points) {
			if (!IsFiniteVector3(point.position)) {
				continue;
			}
			const float lifetime = (std::max)(point.lifetime, 0.001f);
			const float normalizedLifetime = std::clamp(point.age / lifetime, 0.0f, 1.0f);
			if (!source.empty()) {
				const Vector3 difference = point.position - source.back().position;
				if (difference.LengthSq() <= kRibbonGeometryEpsilon) {
					source.back().normalizedLifetime = normalizedLifetime;
					continue;
				}
			}
			source.push_back({ point.position, normalizedLifetime });
		}
		uint32_t smoothingSubdivision = (std::min)(
			data.smoothingSubdivision,
			kMaximumRibbonSmoothingSubdivision
		);
		if (
			data.interpolation == RibbonInterpolationMode::CatmullRom &&
			smoothingSubdivision == 0) {
			smoothingSubdivision = kDefaultRibbonCurveSubdivision;
		}
		if (source.size() < kMinimumRibbonPointCount || smoothingSubdivision == 0) {
			return source;
		}

		const uint32_t steps = smoothingSubdivision + 1;
		std::vector<SmoothedPoint> result;
		result.reserve((source.size() - 1) * steps + 1);
		result.push_back(source.front());
		for (std::size_t segment = 0; segment + 1 < source.size(); ++segment) {
			const SmoothedPoint& left = source[segment];
			const SmoothedPoint& right = source[segment + 1];
			for (uint32_t step = 1; step <= steps; ++step) {
				const float rate = static_cast<float>(step) / static_cast<float>(steps);
				Vector3 position = left.position * (1.0f - rate) + right.position * rate;
				if (data.interpolation == RibbonInterpolationMode::CatmullRom) {
					const Vector3& p0 = segment > 0 ? source[segment - 1].position : left.position;
					const Vector3& p3 = segment + 2 < source.size()
						? source[segment + 2].position
						: right.position;
					const Vector3 candidate = CatmullRom(p0, left.position, right.position, p3, rate);
					if (IsFiniteVector3(candidate)) {
						position = candidate;
					}
				}
				result.push_back({
					position,
					left.normalizedLifetime * (1.0f - rate) + right.normalizedLifetime * rate,
				});
			}
		}
		return result;
	}

	std::vector<RibbonEffectRenderer3d::SmoothedPoint>
	RibbonEffectRenderer3d::BuildVisiblePoints(
		const std::vector<SmoothedPoint>& points,
		const RibbonRenderData& data) const {
		if (points.size() < kMinimumRibbonPointCount || data.playbackMode == RibbonPlaybackMode::Full) {
			return points;
		}

		const float progress = std::clamp(
			std::isfinite(data.playbackProgress) ? data.playbackProgress : 0.0f,
			0.0f,
			1.0f
		);
		if (progress <= 0.0f) {
			return {};
		}

		std::vector<float> cumulativeDistances(points.size(), 0.0f);
		for (std::size_t index = 1; index < points.size(); ++index) {
			const float segmentLength = (points[index].position - points[index - 1].position).Length();
			if (!std::isfinite(segmentLength)) {
				return {};
			}
			cumulativeDistances[index] = cumulativeDistances[index - 1] + segmentLength;
		}

		const float totalLength = cumulativeDistances.back();
		if (!std::isfinite(totalLength) || totalLength <= kRibbonGeometryEpsilon) {
			return {};
		}

		const float endDistance = totalLength * progress;
		float startDistance = 0.0f;
		if (data.playbackMode == RibbonPlaybackMode::Sweep) {
			const float sweepLength = std::isfinite(data.sweepLength)
				? (std::max)(0.0f, data.sweepLength)
				: 0.0f;
			startDistance = (std::max)(0.0f, endDistance - sweepLength);
		}
		if (endDistance - startDistance <= kRibbonGeometryEpsilon) {
			return {};
		}

		const auto sampleAtDistance = [&](float distance) {
			if (distance <= 0.0f) {
				return points.front();
			}
			if (distance >= totalLength) {
				return points.back();
			}

			const auto rightDistance = std::upper_bound(
				cumulativeDistances.begin(),
				cumulativeDistances.end(),
				distance
			);
			const std::size_t rightIndex = static_cast<std::size_t>(
				std::distance(cumulativeDistances.begin(), rightDistance)
			);
			const std::size_t leftIndex = rightIndex - 1;
			const float segmentLength = cumulativeDistances[rightIndex] - cumulativeDistances[leftIndex];
			const float rate = segmentLength > kRibbonGeometryEpsilon
				? std::clamp((distance - cumulativeDistances[leftIndex]) / segmentLength, 0.0f, 1.0f)
				: 0.0f;
			return SmoothedPoint{
				points[leftIndex].position * (1.0f - rate) + points[rightIndex].position * rate,
				points[leftIndex].normalizedLifetime * (1.0f - rate) +
					points[rightIndex].normalizedLifetime * rate,
			};
		};

		std::vector<SmoothedPoint> result;
		result.reserve(points.size() + 2);
		result.push_back(sampleAtDistance(startDistance));
		for (std::size_t index = 1; index + 1 < points.size(); ++index) {
			if (
				cumulativeDistances[index] > startDistance + kRibbonGeometryEpsilon &&
				cumulativeDistances[index] < endDistance - kRibbonGeometryEpsilon) {
				result.push_back(points[index]);
			}
		}

		const SmoothedPoint endPoint = sampleAtDistance(endDistance);
		if ((endPoint.position - result.back().position).LengthSq() > kRibbonGeometryEpsilon) {
			result.push_back(endPoint);
		}
		return result;
	}

	Vector3 RibbonEffectRenderer3d::ResolveSideDirection(
		const Vector3& point,
		const Vector3& tangent,
		const Vector3& previousSide,
		bool cameraFacing) const {
		Vector3 side;
		if (cameraFacing) {
			const Vector3 toCamera = NormalizeSafe(cameraPosition_ - point, Vector3{});
			side = Cross(tangent, toCamera);
		} else {
			side = Cross(tangent, Vector3{ 0.0f, 1.0f, 0.0f });
		}
		if (side.LengthSq() <= kRibbonGeometryEpsilon) {
			side = previousSide - tangent * Dot(previousSide, tangent);
		}
		if (side.LengthSq() <= kRibbonGeometryEpsilon) {
			side = cameraRight_ - tangent * Dot(cameraRight_, tangent);
		}
		if (side.LengthSq() <= kRibbonGeometryEpsilon) {
			const Vector3 fallbackAxis = std::abs(tangent.y) < 0.9f
				? Vector3{ 0.0f, 1.0f, 0.0f }
				: Vector3{ 1.0f, 0.0f, 0.0f };
			side = Cross(tangent, fallbackAxis);
		}
		return NormalizeSafe(side, Vector3{ 1.0f, 0.0f, 0.0f });
	}

	bool RibbonEffectRenderer3d::EnsureBufferCapacity(
		std::size_t requiredVertexCount,
		std::size_t requiredIndexCount) {
		if (
			requiredVertexCount == 0 ||
			requiredIndexCount == 0 ||
			requiredVertexCount > kMaximumRibbonVertexCount ||
			requiredIndexCount > kMaximumRibbonIndexCount) {
			return false;
		}

		FrameResource& frameResource = frameResources_[currentFrameResourceIndex_];
		if (frameResource.vertexCapacity < requiredVertexCount) {
			std::size_t capacity = (std::max)(kInitialRibbonVertexCapacity, frameResource.vertexCapacity);
			while (capacity < requiredVertexCount && capacity <= kMaximumRibbonVertexCount / 2) {
				capacity *= 2;
			}
			capacity = (std::min)(capacity, kMaximumRibbonVertexCount);
			if (capacity < requiredVertexCount) {
				return false;
			}
			if (frameResource.mappedVertices && frameResource.vertexResource) {
				frameResource.vertexResource->Unmap(0, nullptr);
			}
			frameResource.mappedVertices = CreateMappedBuffer<RibbonVertex>(
				device_,
				frameResource.vertexResource,
				capacity,
				false
			);
			frameResource.vertexCapacity = capacity;
			frameResource.vertexBufferView.BufferLocation = frameResource.vertexResource->GetGPUVirtualAddress();
			frameResource.vertexBufferView.SizeInBytes = static_cast<UINT>(capacity * sizeof(RibbonVertex));
			frameResource.vertexBufferView.StrideInBytes = sizeof(RibbonVertex);
		}

		if (frameResource.indexCapacity < requiredIndexCount) {
			std::size_t capacity = (std::max)(kInitialRibbonIndexCapacity, frameResource.indexCapacity);
			while (capacity < requiredIndexCount && capacity <= kMaximumRibbonIndexCount / 2) {
				capacity *= 2;
			}
			capacity = (std::min)(capacity, kMaximumRibbonIndexCount);
			if (capacity < requiredIndexCount) {
				return false;
			}
			if (frameResource.mappedIndices && frameResource.indexResource) {
				frameResource.indexResource->Unmap(0, nullptr);
			}
			frameResource.mappedIndices = CreateMappedBuffer<uint32_t>(
				device_,
				frameResource.indexResource,
				capacity,
				false
			);
			frameResource.indexCapacity = capacity;
			frameResource.indexBufferView.BufferLocation = frameResource.indexResource->GetGPUVirtualAddress();
			frameResource.indexBufferView.SizeInBytes = static_cast<UINT>(capacity * sizeof(uint32_t));
			frameResource.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		}
		return true;
	}

	uint32_t RibbonEffectRenderer3d::ResolveTextureIndex(const std::string& textureName) {
		if (const auto found = textureIndexCache_.find(textureName); found != textureIndexCache_.end()) {
			return found->second;
		}
		uint32_t textureIndex = MadoEngine::TextureManager::GetInstance().GetTextureIndex(textureName);
		if (textureIndex == (std::numeric_limits<uint32_t>::max)()) {
			textureIndex = MadoEngine::TextureManager::GetInstance().GetTextureIndex("white2x2");
		}
		textureIndexCache_[textureName] = textureIndex;
		return textureIndex;
	}

	MadoEngine::Render::PSODesc RibbonEffectRenderer3d::CreatePSODesc(
		MadoEngine::Render::BlendMode blendMode,
		MadoEngine::Render::CullMode cullMode) const {
		MadoEngine::Render::PSODesc desc;
		desc.blendMode = blendMode;
		desc.depthMode = blendMode == MadoEngine::Render::BlendMode::None
			? MadoEngine::Render::DepthMode::ReadWrite
			: MadoEngine::Render::DepthMode::ReadOnly;
		desc.cullMode = cullMode;
		desc.fillMode = MadoEngine::Render::FillMode::Solid;
		desc.topology = MadoEngine::Render::TopologyType::Triangle;
		desc.inputLayout = MadoEngine::Render::InputLayoutType::Ribbon;
		desc.preserveRenderTargetAlpha = true;
		desc.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.dsvFormat = DXGI_FORMAT_D32_FLOAT;
		desc.vsKey = "Object3d/RibbonEffect/Ribbon.VS";
		desc.psKey = "Object3d/RibbonEffect/Ribbon.PS";
		desc.rootSigKey = "RibbonEffect3d.RootSig";
		return desc;
	}

} // namespace MadoEngine::Ribbon
