#include "GpuParticleEmitterRuntime.h"
#include "Math/Function/MatrixFunction.h"
#include "ParticleRenderer3d.h"
#include "Shader/RootSignatureManager.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <limits>
#include <type_traits>

namespace {

	constexpr uint32_t kComputeThreadCount = 64;
	constexpr uint32_t kMaximumGpuParticleCount =
		kComputeThreadCount * D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
	constexpr uint32_t kShapeFlagSurfaceOrEdge = 1u << 0;

	/// @brief 256byte境界へ切り上げ
	/// @param size 切り上げるSize
	/// @return 切り上げ後のSize
	uint64_t AlignConstantBufferSize(uint64_t size) {
		constexpr uint64_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		return (size + alignment - 1) & ~(alignment - 1);
	}

	/// @brief 指定境界へBuffer Offsetを切り上げ
	/// @param offset 切り上げるOffset
	/// @param alignment 境界Size
	/// @return 切り上げ後のOffset
	uint64_t AlignBufferOffset(uint64_t offset, uint64_t alignment) {
		return (offset + alignment - 1) & ~(alignment - 1);
	}

	/// @brief Dispatch Group数を計算
	/// @param itemCount 処理要素数
	/// @return Dispatch Group数
	uint32_t CalculateGroupCount(uint32_t itemCount) {
		return (itemCount + kComputeThreadCount - 1) / kComputeThreadCount;
	}

} // 無名名前空間

namespace MadoEngine::Particle {

	GpuParticleEmitterRuntime::GpuParticleEmitterRuntime(
		ID3D12Device* device,
		MadoEngine::Render::ComputePSORegistry* computePsoRegistry)
		: device_(device)
		, computePsoRegistry_(computePsoRegistry) {
	}

	GpuParticleEmitterRuntime::~GpuParticleEmitterRuntime() {
		for (uint32_t index = 0; index < kFrameResourceCount; ++index) {
			if (mappedPerFrameParameters_[index] && perFrameBuffers_[index]) {
				perFrameBuffers_[index]->Unmap(0, nullptr);
			}
			mappedPerFrameParameters_[index] = nullptr;
		}
	}

	bool GpuParticleEmitterRuntime::Initialize(const EmitterConfig& config, uint32_t randomSeed) {
		if (!device_ || !computePsoRegistry_) {
			return false;
		}
		if (
			config.emission.maxParticles > kMaximumGpuParticleCount ||
			config.emission.maxParticles > kMaximumParticleCountPerEmitter) {
			return false;
		}

		config_ = config;
		config_.emission.maxParticles = (std::max)(1u, config_.emission.maxParticles);
		const uint64_t particleCount = config_.emission.maxParticles;

		// Particle状態、Alive List、Free List、Indirect引数をGPU上へ固定容量で確保
		const bool resourcesCreated =
			CreateDefaultBuffer(
				sizeof(GpuParticleState) * particleCount,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				stateBuffer_
			) &&
			CreateDefaultBuffer(
				sizeof(GpuParticleDrawInstance) * particleCount,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				drawInstanceBuffer_
			) &&
			CreateDefaultBuffer(
				sizeof(uint32_t) * particleCount,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				aliveIndexBuffers_[0]
			) &&
			CreateDefaultBuffer(
				sizeof(uint32_t) * particleCount,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				aliveIndexBuffers_[1]
			) &&
			CreateDefaultBuffer(
				sizeof(uint32_t),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				aliveCounterBuffers_[0]
			) &&
			CreateDefaultBuffer(
				sizeof(uint32_t),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				aliveCounterBuffers_[1]
			) &&
			CreateDefaultBuffer(
				sizeof(uint32_t) * particleCount,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				freeIndexBuffer_
			) &&
			CreateDefaultBuffer(
				sizeof(uint32_t),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				freeCounterBuffer_
			) &&
			CreateDefaultBuffer(
				sizeof(GpuParticleDrawArguments),
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				indirectArgumentBuffer_
			) &&
			CreateDefaultBuffer(
				sizeof(GpuParticleTrailSample) * particleCount,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				trailSampleBuffer_
			);
		if (!resourcesCreated) {
			return false;
		}

		void* emitterParameterData = nullptr;
		if (!CreateUploadBuffer(
			sizeof(GpuParticleEmitterParameters),
			emitterParameterBuffer_,
			&emitterParameterData
		)) {
			return false;
		}
		const GpuParticleEmitterParameters emitterParameters = MakeEmitterParameters(config_, randomSeed);
		std::memcpy(emitterParameterData, &emitterParameters, sizeof(emitterParameters));
		emitterParameterBuffer_->Unmap(0, nullptr);

		aliveIndexReadbackOffset_ = AlignBufferOffset(
			sizeof(uint32_t),
			alignof(uint32_t)
		);
		trailSampleReadbackOffset_ = AlignBufferOffset(
			aliveIndexReadbackOffset_ + sizeof(uint32_t) * particleCount,
			alignof(GpuParticleTrailSample)
		);
		readbackBufferSize_ = config_.trail.isEnabled
			? trailSampleReadbackOffset_ + sizeof(GpuParticleTrailSample) * particleCount
			: sizeof(uint32_t);

		// GPU実行中のFrame DataをCPUが上書きしないようUploadとReadbackを多重化
		for (uint32_t index = 0; index < kFrameResourceCount; ++index) {
			void* perFrameData = nullptr;
			if (!CreateUploadBuffer(
				sizeof(GpuParticlePerFrameParameters),
				perFrameBuffers_[index],
				&perFrameData
				)) {
				return false;
			}
			mappedPerFrameParameters_[index] =
				static_cast<GpuParticlePerFrameParameters*>(perFrameData);
			if (!CreateReadbackBuffer(
				readbackBufferSize_,
				readbackSlots_[index].resource
				)) {
				return false;
			}
		}

		trailHistory_.Initialize(config_.trail, config_.simulationSpace);
		trailSamples_.clear();
		trailSamples_.reserve(config_.emission.maxParticles);
		CalculateGpuBufferCapacity();
		isInitialized_ = true;
		Reset();
		return true;
	}

	void GpuParticleEmitterRuntime::Update(
		float deltaTime,
		const Transform3D& emitterTransform) {
		pendingEmitterTransform_ = emitterTransform;
		trailHistory_.Advance(deltaTime);

		// GPU上のAlive数が0でも初回完了まではSimulationを継続して確定値を取得
		const bool needsSimulation =
			needsGpuInitialize_ ||
			pendingEmitCount_ > 0 ||
			cachedAliveCount_ > 0 ||
			hasGpuWorkInFlight_ ||
			!hasCompletedSimulation_;
		if (!needsSimulation) {
			return;
		}

		pendingDeltaTime_ += (std::max)(0.0f, deltaTime);
		hasPendingUpdate_ = true;
	}

	void GpuParticleEmitterRuntime::Emit(
		uint32_t count,
		const Transform3D& emitterTransform) {
		if (count == 0) {
			return;
		}

		pendingEmitterTransform_ = emitterTransform;
		const uint64_t combinedCount =
			static_cast<uint64_t>(pendingEmitCount_) +
			static_cast<uint64_t>(count);

		// 複数回のEmit要求を蓄積しつつEmitter最大数で飽和
		pendingEmitCount_ = static_cast<uint32_t>(
			(std::min)(
				combinedCount,
				static_cast<uint64_t>(config_.emission.maxParticles)
			)
		);
		suppressDraw_ = false;
	}

	void GpuParticleEmitterRuntime::SetTransform(const Transform3D& emitterTransform) {
		pendingEmitterTransform_ = emitterTransform;
		if (
			config_.simulationSpace == SimulationSpace::Local &&
			(cachedAliveCount_ > 0 || hasGpuWorkInFlight_)) {
			hasPendingUpdate_ = true;
		}
	}

	void GpuParticleEmitterRuntime::Stop(StopMode mode) {
		if (mode != StopMode::Immediate) {
			return;
		}

		pendingEmitCount_ = 0;
		pendingDeltaTime_ = 0.0f;
		hasPendingUpdate_ = false;
		needsGpuInitialize_ = true;
		hasCompletedSimulation_ = false;
		suppressDraw_ = true;
		cachedAliveCount_ = 0;
		trailHistory_.Clear();
		lastAppliedReadbackSequence_ = nextReadbackSequence_ - 1;
		lastAppliedTrailReadbackSequence_ = nextReadbackSequence_ - 1;
	}

	void GpuParticleEmitterRuntime::Reset() {
		pendingDeltaTime_ = 0.0f;
		pendingEmitCount_ = 0;
		nextEmitSequence_ = 0;
		currentAliveIndex_ = 0;
		cachedAliveCount_ = 0;
		needsGpuInitialize_ = true;
		hasPendingUpdate_ = false;
		hasCompletedSimulation_ = false;
		suppressDraw_ = false;
		trailHistory_.Clear();
		lastAppliedReadbackSequence_ = nextReadbackSequence_ - 1;
		lastAppliedTrailReadbackSequence_ = nextReadbackSequence_ - 1;
	}

	void GpuParticleEmitterRuntime::RecordGpuSimulation(
		ID3D12GraphicsCommandList* commandList,
		uint64_t submissionFenceValue) {
		if (!isInitialized_ || !commandList) {
			return;
		}
		if (!needsGpuInitialize_ && !hasPendingUpdate_ && pendingEmitCount_ == 0) {
			return;
		}

		// Fence未完了のReadback Slotを避けてCPU/GPU間のResource競合を防止
		uint32_t frameResourceIndex = nextFrameResourceIndex_;
		bool foundFrameResource = false;
		for (uint32_t offset = 0; offset < kFrameResourceCount; ++offset) {
			const uint32_t candidate = (nextFrameResourceIndex_ + offset) % kFrameResourceCount;
			if (!readbackSlots_[candidate].isPending) {
				frameResourceIndex = candidate;
				foundFrameResource = true;
				break;
			}
		}
		if (!foundFrameResource) {
			return;
		}
		nextFrameResourceIndex_ = (frameResourceIndex + 1) % kFrameResourceCount;

		GpuParticlePerFrameParameters& perFrame =
			*mappedPerFrameParameters_[frameResourceIndex];
		perFrame = {};
		perFrame.emitterMatrix = Matrix::MakeAffine(
			pendingEmitterTransform_.scale,
			pendingEmitterTransform_.rotate,
			pendingEmitterTransform_.translate
		);
		perFrame.emitterRotationMatrix = Matrix::MakeAffine(
			{ 1.0f, 1.0f, 1.0f },
			pendingEmitterTransform_.rotate,
			{}
		);
		perFrame.emitterScaleRotation = {
			pendingEmitterTransform_.scale.x,
			pendingEmitterTransform_.scale.y,
			pendingEmitterTransform_.rotate.z,
			0.0f,
		};
		perFrame.deltaTime = pendingDeltaTime_;
		perFrame.emitCount = pendingEmitCount_;
		perFrame.emitSequenceBase = nextEmitSequence_;
		const D3D12_GPU_VIRTUAL_ADDRESS perFrameAddress =
			perFrameBuffers_[frameResourceIndex]->GetGPUVirtualAddress();

		ID3D12RootSignature* rootSignature =
			MadoEngine::RootSignatureManager::GetInstance().Get("ParticleCompute.RootSig");
		if (!rootSignature) {
			return;
		}
		commandList->SetComputeRootSignature(rootSignature);

		if (needsGpuInitialize_) {

			// Free ListとCounterを初期化して全Particleを再利用可能な状態へ復元
			Transition(commandList, stateBuffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			Transition(commandList, drawInstanceBuffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			Transition(
				commandList,
				aliveIndexBuffers_[0],
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			);
			Transition(
				commandList,
				aliveIndexBuffers_[1],
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);
			for (BufferResource& aliveCounterBuffer : aliveCounterBuffers_) {
				Transition(commandList, aliveCounterBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			}
			Transition(commandList, freeIndexBuffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			Transition(commandList, freeCounterBuffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			Transition(commandList, indirectArgumentBuffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			Transition(commandList, trailSampleBuffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			ID3D12PipelineState* initializePso = computePsoRegistry_->Get({
				"Object3d/Particle/GpuParticleInitialize.CS",
				"ParticleCompute.RootSig",
			});
			if (!initializePso) {
				return;
			}
			commandList->SetPipelineState(initializePso);
			BindComputeResources(commandList, 0, 1, perFrameAddress);
			commandList->Dispatch(
				CalculateGroupCount(config_.emission.maxParticles),
				1,
				1
			);
			AddUavBarrier(commandList);
			currentAliveIndex_ = 0;
			cachedAliveCount_ = 0;
			needsGpuInitialize_ = false;
		}

		if (hasPendingUpdate_) {
			const uint32_t nextAliveIndex = 1u - currentAliveIndex_;

			// Alive IndexをPing-Pongして読み取り中Listと書き込み先Listを分離
			Transition(
				commandList,
				aliveIndexBuffers_[currentAliveIndex_],
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			);
			Transition(
				commandList,
				aliveIndexBuffers_[nextAliveIndex],
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);
			Transition(commandList, drawInstanceBuffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			Transition(commandList, indirectArgumentBuffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			ID3D12PipelineState* clearCounterPso = computePsoRegistry_->Get({
				"Object3d/Particle/GpuParticleClearCounter.CS",
				"ParticleCompute.RootSig",
			});
			ID3D12PipelineState* updatePso = computePsoRegistry_->Get({
				"Object3d/Particle/GpuParticleUpdate.CS",
				"ParticleCompute.RootSig",
			});
			if (!clearCounterPso || !updatePso) {
				return;
			}

			BindComputeResources(
				commandList,
				currentAliveIndex_,
				nextAliveIndex,
				perFrameAddress
			);
			commandList->SetPipelineState(clearCounterPso);
			commandList->Dispatch(1, 1, 1);
			AddUavBarrier(commandList, aliveCounterBuffers_[nextAliveIndex].resource.Get());

			commandList->SetPipelineState(updatePso);
			commandList->Dispatch(
				CalculateGroupCount(config_.emission.maxParticles),
				1,
				1
			);
			AddUavBarrier(commandList);
			currentAliveIndex_ = nextAliveIndex;
		}

		if (pendingEmitCount_ > 0) {
			const uint32_t unusedInputIndex = 1u - currentAliveIndex_;

			// Update後のAlive ListへFree Listから新規Particle Indexを追記
			Transition(
				commandList,
				aliveIndexBuffers_[unusedInputIndex],
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
			);
			Transition(
				commandList,
				aliveIndexBuffers_[currentAliveIndex_],
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			);
			Transition(commandList, drawInstanceBuffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			Transition(commandList, indirectArgumentBuffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			ID3D12PipelineState* prepareEmitPso = computePsoRegistry_->Get({
				"Object3d/Particle/GpuParticlePrepareEmit.CS",
				"ParticleCompute.RootSig",
			});
			ID3D12PipelineState* emitPso = computePsoRegistry_->Get({
				"Object3d/Particle/GpuParticleEmit.CS",
				"ParticleCompute.RootSig",
			});
			if (!prepareEmitPso || !emitPso) {
				return;
			}
			BindComputeResources(
				commandList,
				unusedInputIndex,
				currentAliveIndex_,
				perFrameAddress
			);
			commandList->SetPipelineState(prepareEmitPso);
			commandList->Dispatch(1, 1, 1);
			AddUavBarrier(commandList);

			commandList->SetPipelineState(emitPso);
			commandList->Dispatch(
				CalculateGroupCount(pendingEmitCount_),
				1,
				1
			);
			AddUavBarrier(commandList);
		}

		ID3D12PipelineState* buildArgsPso = computePsoRegistry_->Get({
			"Object3d/Particle/GpuParticleBuildArgs.CS",
			"ParticleCompute.RootSig",
		});
		if (!buildArgsPso) {
			return;
		}
		const uint32_t unusedInputIndex = 1u - currentAliveIndex_;

		// 最新Alive数からDrawIndirect引数をGPU上で生成してCPU同期を回避
		Transition(
			commandList,
			aliveIndexBuffers_[unusedInputIndex],
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);
		Transition(commandList, indirectArgumentBuffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetPipelineState(buildArgsPso);
		BindComputeResources(
			commandList,
			unusedInputIndex,
			currentAliveIndex_,
			perFrameAddress
		);
		commandList->Dispatch(1, 1, 1);
		AddUavBarrier(commandList, indirectArgumentBuffer_.resource.Get());

		BufferResource& currentCounter = aliveCounterBuffers_[currentAliveIndex_];

		// Alive数だけをReadback Slotへ複製しFence完了後の状態判定に利用
		Transition(commandList, currentCounter, D3D12_RESOURCE_STATE_COPY_SOURCE);
		commandList->CopyBufferRegion(
			readbackSlots_[frameResourceIndex].resource.Get(),
			0,
			currentCounter.resource.Get(),
			0,
			sizeof(uint32_t)
		);
		Transition(commandList, currentCounter, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Trail有効時のみAlive Indexと軽量SampleをReadbackしてParticle本体のGPU常駐を維持
		if (config_.trail.isEnabled) {
			BufferResource& currentAliveIndices = aliveIndexBuffers_[currentAliveIndex_];
			Transition(commandList, currentAliveIndices, D3D12_RESOURCE_STATE_COPY_SOURCE);
			Transition(commandList, trailSampleBuffer_, D3D12_RESOURCE_STATE_COPY_SOURCE);
			commandList->CopyBufferRegion(
				readbackSlots_[frameResourceIndex].resource.Get(),
				aliveIndexReadbackOffset_,
				currentAliveIndices.resource.Get(),
				0,
				sizeof(uint32_t) * config_.emission.maxParticles
			);
			commandList->CopyBufferRegion(
				readbackSlots_[frameResourceIndex].resource.Get(),
				trailSampleReadbackOffset_,
				trailSampleBuffer_.resource.Get(),
				0,
				sizeof(GpuParticleTrailSample) * config_.emission.maxParticles
			);
			Transition(commandList, trailSampleBuffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		}
		TransitionForDraw(commandList);

		nextEmitSequence_ += pendingEmitCount_;
		pendingEmitCount_ = 0;
		pendingDeltaTime_ = 0.0f;
		hasPendingUpdate_ = false;
		hasGpuWorkInFlight_ = true;
		readbackSlots_[frameResourceIndex].isPending = true;
		readbackSlots_[frameResourceIndex].sequence = nextReadbackSequence_++;
		readbackSlots_[frameResourceIndex].fenceValue = submissionFenceValue;
	}

	void GpuParticleEmitterRuntime::OnGpuFrameCompleted(uint64_t completedFenceValue) {
		std::array<ReadbackSlot*, kFrameResourceCount> completedSlots{};
		uint32_t completedSlotCount = 0;
		for (ReadbackSlot& slot : readbackSlots_) {
			if (slot.isPending && slot.fenceValue <= completedFenceValue) {
				completedSlots[completedSlotCount++] = &slot;
			}
		}
		std::sort(
			completedSlots.begin(),
			completedSlots.begin() + completedSlotCount,
			[](const ReadbackSlot* lhs, const ReadbackSlot* rhs) {
				return lhs->sequence < rhs->sequence;
			}
		);

		uint64_t latestSequence = lastAppliedReadbackSequence_;
		uint32_t latestAliveCount = cachedAliveCount_;
		bool appliedAny = false;

		// 完了済みSlotのうちSequenceが最も新しいAlive数だけをCPU Cacheへ反映
		for (uint32_t slotIndex = 0; slotIndex < completedSlotCount; ++slotIndex) {
			ReadbackSlot& slot = *completedSlots[slotIndex];
			void* mappedData = nullptr;
			const D3D12_RANGE readRange{
				0,
				static_cast<SIZE_T>(readbackBufferSize_),
			};
			const HRESULT result = slot.resource->Map(0, &readRange, &mappedData);
			if (SUCCEEDED(result) && mappedData) {
				const auto* readbackData = static_cast<const uint8_t*>(mappedData);
				const uint32_t aliveCount = (std::min)(
					*reinterpret_cast<const uint32_t*>(readbackData),
					config_.emission.maxParticles
				);

				// Stable IDと位置だけをCPU履歴へ渡し、既存Ribbon描画経路をGPU Particleでも共有
				if (
					config_.trail.isEnabled &&
					slot.sequence > lastAppliedTrailReadbackSequence_) {
					const auto* aliveIndices = reinterpret_cast<const uint32_t*>(
						readbackData + aliveIndexReadbackOffset_
					);
					const auto* gpuTrailSamples = reinterpret_cast<const GpuParticleTrailSample*>(
						readbackData + trailSampleReadbackOffset_
					);
					trailSamples_.clear();
					for (uint32_t aliveIndex = 0; aliveIndex < aliveCount; ++aliveIndex) {
						const uint32_t particleIndex = aliveIndices[aliveIndex];
						if (particleIndex >= config_.emission.maxParticles) {
							continue;
						}

						const GpuParticleTrailSample& gpuSample = gpuTrailSamples[particleIndex];
						trailSamples_.push_back({ gpuSample.identifier, gpuSample.position });
					}
					trailHistory_.UpdateParticles(trailSamples_);
					lastAppliedTrailReadbackSequence_ = slot.sequence;
				}

				slot.resource->Unmap(0, nullptr);
				if (slot.sequence > latestSequence) {
					latestSequence = slot.sequence;
					latestAliveCount = aliveCount;
					appliedAny = true;
				}
				slot.isPending = false;
			}
		}

		if (appliedAny) {
			cachedAliveCount_ = latestAliveCount;
			lastAppliedReadbackSequence_ = latestSequence;
			hasCompletedSimulation_ = true;
		}
		hasGpuWorkInFlight_ = std::any_of(
			readbackSlots_.begin(),
			readbackSlots_.end(),
			[](const ReadbackSlot& slot) {
				return slot.isPending;
			}
		);
	}

	void GpuParticleEmitterRuntime::SubmitRenderData(
		ParticleRenderer3d& renderer,
		const Transform3D& emitterTransform,
		MadoEngine::Render::RenderLayer renderLayer) const {
		(void)emitterTransform;
		if (!isInitialized_ || needsGpuInitialize_ || suppressDraw_) {
			return;
		}

		GpuParticleRenderData renderData;
		renderData.drawInstanceBufferAddress =
			drawInstanceBuffer_.resource->GetGPUVirtualAddress();
		renderData.aliveIndexBufferAddress =
			aliveIndexBuffers_[currentAliveIndex_].resource->GetGPUVirtualAddress();
		renderData.indirectArgumentBuffer = indirectArgumentBuffer_.resource.Get();
		renderData.maxParticleCount = config_.emission.maxParticles;
		renderData.bufferCapacityBytes = gpuBufferCapacityBytes_;
		renderer.SubmitGpu(renderData, config_, renderLayer);
	}

	void GpuParticleEmitterRuntime::SubmitTrailRenderData(
		MadoEngine::Ribbon::RibbonEffectRenderer3d& renderer,
		const Transform3D& emitterTransform,
		MadoEngine::Render::RenderLayer renderLayer) const {
		trailHistory_.SubmitRenderData(renderer, emitterTransform, renderLayer);
	}

	bool GpuParticleEmitterRuntime::IsIdle() const {
		return
			isInitialized_ &&
			hasCompletedSimulation_ &&
			!needsGpuInitialize_ &&
			!hasPendingUpdate_ &&
			pendingEmitCount_ == 0 &&
			!hasGpuWorkInFlight_ &&
			cachedAliveCount_ == 0 &&
			trailHistory_.IsEmpty();
	}

	GpuParticleEmitterParameters GpuParticleEmitterRuntime::MakeEmitterParameters(
		const EmitterConfig& config,
		uint32_t randomSeed) {
		GpuParticleEmitterParameters parameters;
		parameters.maxParticles = (std::max)(1u, config.emission.maxParticles);
		parameters.simulationSpace = static_cast<uint32_t>(config.simulationSpace);
		parameters.directionMode = static_cast<uint32_t>(config.initial.directionMode);
		parameters.randomSeed = randomSeed;

		std::visit([&parameters](const auto& shape) {
			using ShapeType = std::decay_t<decltype(shape)>;
			if constexpr (std::is_same_v<ShapeType, PointShape>) {
				parameters.shapeType = static_cast<uint32_t>(GpuParticleShapeType::Point);
				parameters.shapeData0 = {
					shape.offset.x,
					shape.offset.y,
					shape.offset.z,
					0.0f,
				};
			} else if constexpr (std::is_same_v<ShapeType, LineShape>) {
				parameters.shapeType = static_cast<uint32_t>(GpuParticleShapeType::Line);
				parameters.shapeData0 = {
					shape.start.x,
					shape.start.y,
					shape.start.z,
					0.0f,
				};
				parameters.shapeData1 = {
					shape.end.x,
					shape.end.y,
					shape.end.z,
					0.0f,
				};
			} else if constexpr (std::is_same_v<ShapeType, SphereShape>) {
				parameters.shapeType = static_cast<uint32_t>(GpuParticleShapeType::Sphere);
				parameters.shapeData0.x = shape.radius;
				if (shape.emitFromSurface) {
					parameters.shapeFlags |= kShapeFlagSurfaceOrEdge;
				}
			} else if constexpr (std::is_same_v<ShapeType, BoxShape>) {
				parameters.shapeType = static_cast<uint32_t>(GpuParticleShapeType::Box);
				parameters.shapeData0 = {
					shape.halfExtents.x,
					shape.halfExtents.y,
					shape.halfExtents.z,
					0.0f,
				};
				if (shape.emitFromSurface) {
					parameters.shapeFlags |= kShapeFlagSurfaceOrEdge;
				}
			} else if constexpr (std::is_same_v<ShapeType, PlaneShape>) {
				parameters.shapeType = static_cast<uint32_t>(GpuParticleShapeType::Plane);
				parameters.shapeData0 = {
					shape.halfExtents.x,
					shape.halfExtents.y,
					0.0f,
					0.0f,
				};
				parameters.shapeData1 = {
					shape.normal.x,
					shape.normal.y,
					shape.normal.z,
					0.0f,
				};
			} else if constexpr (std::is_same_v<ShapeType, RingShape>) {
				parameters.shapeType = static_cast<uint32_t>(GpuParticleShapeType::Ring);
				parameters.shapeData0 = {
					shape.innerRadius,
					shape.outerRadius,
					0.0f,
					0.0f,
				};
				parameters.shapeData1 = {
					shape.normal.x,
					shape.normal.y,
					shape.normal.z,
					0.0f,
				};
				if (shape.emitFromEdge) {
					parameters.shapeFlags |= kShapeFlagSurfaceOrEdge;
				}
			}
		}, config.shape);

		parameters.directionMin = {
			config.initial.direction.min.x,
			config.initial.direction.min.y,
			config.initial.direction.min.z,
			0.0f,
		};
		parameters.directionMax = {
			config.initial.direction.max.x,
			config.initial.direction.max.y,
			config.initial.direction.max.z,
			0.0f,
		};
		parameters.lifeTimeSpeedRange = {
			config.initial.lifeTime.min,
			config.initial.lifeTime.max,
			config.initial.speed.min,
			config.initial.speed.max,
		};
		parameters.rotationRange = {
			config.initial.rotation.min,
			config.initial.rotation.max,
			config.initial.angularVelocity.min,
			config.initial.angularVelocity.max,
		};
		parameters.gravityDrag = {
			config.motion.gravity.x,
			config.motion.gravity.y,
			config.motion.gravity.z,
			config.motion.drag,
		};
		parameters.acceleration = {
			config.motion.acceleration.x,
			config.motion.acceleration.y,
			config.motion.acceleration.z,
			0.0f,
		};
		parameters.startScaleMinMax = {
			config.sizeOverLifetime.start.min.x,
			config.sizeOverLifetime.start.min.y,
			config.sizeOverLifetime.start.max.x,
			config.sizeOverLifetime.start.max.y,
		};
		parameters.endScaleMinMax = {
			config.sizeOverLifetime.end.min.x,
			config.sizeOverLifetime.end.min.y,
			config.sizeOverLifetime.end.max.x,
			config.sizeOverLifetime.end.max.y,
		};
		parameters.startColorMin = config.colorOverLifetime.start.min;
		parameters.startColorMax = config.colorOverLifetime.start.max;
		parameters.endColorMin = config.colorOverLifetime.end.min;
		parameters.endColorMax = config.colorOverLifetime.end.max;
		return parameters;
	}

	bool GpuParticleEmitterRuntime::CreateDefaultBuffer(
		uint64_t sizeInBytes,
		D3D12_RESOURCE_STATES initialState,
		BufferResource& outBuffer) {
		const D3D12_RESOURCE_STATES resourceInitialState =
			initialState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS
			? D3D12_RESOURCE_STATE_COMMON
			: initialState;
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = (std::max)(1ull, sizeInBytes);
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		const HRESULT result = device_->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			resourceInitialState,
			nullptr,
			IID_PPV_ARGS(&outBuffer.resource)
		);
		if (FAILED(result)) {
			return false;
		}

		outBuffer.state = resourceInitialState;
		return true;
	}

	bool GpuParticleEmitterRuntime::CreateUploadBuffer(
		uint64_t sizeInBytes,
		Microsoft::WRL::ComPtr<ID3D12Resource>& outResource,
		void** outMappedData) {
		assert(outMappedData);
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = AlignConstantBufferSize(sizeInBytes);
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		HRESULT result = device_->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&outResource)
		);
		if (FAILED(result)) {
			return false;
		}

		result = outResource->Map(0, nullptr, outMappedData);
		return SUCCEEDED(result);
	}

	bool GpuParticleEmitterRuntime::CreateReadbackBuffer(
		uint64_t sizeInBytes,
		Microsoft::WRL::ComPtr<ID3D12Resource>& outResource) {
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_READBACK;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = (std::max)(1ull, sizeInBytes);
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		const HRESULT result = device_->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&outResource)
		);
		return SUCCEEDED(result);
	}

	void GpuParticleEmitterRuntime::Transition(
		ID3D12GraphicsCommandList* commandList,
		BufferResource& buffer,
		D3D12_RESOURCE_STATES nextState) {
		if (!buffer.resource || buffer.state == nextState) {
			return;
		}

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = buffer.resource.Get();
		barrier.Transition.StateBefore = buffer.state;
		barrier.Transition.StateAfter = nextState;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &barrier);
		buffer.state = nextState;
	}

	void GpuParticleEmitterRuntime::AddUavBarrier(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* resource) const {
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = resource;
		commandList->ResourceBarrier(1, &barrier);
	}

	void GpuParticleEmitterRuntime::BindComputeResources(
		ID3D12GraphicsCommandList* commandList,
		uint32_t inputAliveIndex,
		uint32_t outputAliveIndex,
		D3D12_GPU_VIRTUAL_ADDRESS perFrameAddress) {
		commandList->SetComputeRootUnorderedAccessView(
			0,
			stateBuffer_.resource->GetGPUVirtualAddress()
		);
		commandList->SetComputeRootUnorderedAccessView(
			1,
			drawInstanceBuffer_.resource->GetGPUVirtualAddress()
		);
		commandList->SetComputeRootShaderResourceView(
			2,
			aliveIndexBuffers_[inputAliveIndex].resource->GetGPUVirtualAddress()
		);
		commandList->SetComputeRootUnorderedAccessView(
			3,
			aliveIndexBuffers_[outputAliveIndex].resource->GetGPUVirtualAddress()
		);
		commandList->SetComputeRootUnorderedAccessView(
			4,
			aliveCounterBuffers_[inputAliveIndex].resource->GetGPUVirtualAddress()
		);
		commandList->SetComputeRootUnorderedAccessView(
			5,
			aliveCounterBuffers_[outputAliveIndex].resource->GetGPUVirtualAddress()
		);
		commandList->SetComputeRootUnorderedAccessView(
			6,
			freeIndexBuffer_.resource->GetGPUVirtualAddress()
		);
		commandList->SetComputeRootUnorderedAccessView(
			7,
			freeCounterBuffer_.resource->GetGPUVirtualAddress()
		);
		commandList->SetComputeRootUnorderedAccessView(
			8,
			indirectArgumentBuffer_.resource->GetGPUVirtualAddress()
		);
		commandList->SetComputeRootConstantBufferView(
			9,
			emitterParameterBuffer_->GetGPUVirtualAddress()
		);
		commandList->SetComputeRootConstantBufferView(10, perFrameAddress);
		commandList->SetComputeRootUnorderedAccessView(
			11,
			trailSampleBuffer_.resource->GetGPUVirtualAddress()
		);
	}

	void GpuParticleEmitterRuntime::TransitionForDraw(
		ID3D12GraphicsCommandList* commandList) {

		// Compute書き込み後のBufferをVertex ShaderとDrawIndirectから参照可能な状態へ遷移
		Transition(
			commandList,
			drawInstanceBuffer_,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);
		Transition(
			commandList,
			aliveIndexBuffers_[currentAliveIndex_],
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		);
		Transition(
			commandList,
			indirectArgumentBuffer_,
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
		);
	}

	void GpuParticleEmitterRuntime::CalculateGpuBufferCapacity() {
		gpuBufferCapacityBytes_ = 0;
		auto addResourceSize = [this](ID3D12Resource* resource) {
			if (resource) {
				gpuBufferCapacityBytes_ += resource->GetDesc().Width;
			}
		};
		addResourceSize(stateBuffer_.resource.Get());
		addResourceSize(drawInstanceBuffer_.resource.Get());
		for (const BufferResource& buffer : aliveIndexBuffers_) {
			addResourceSize(buffer.resource.Get());
		}
		for (const BufferResource& buffer : aliveCounterBuffers_) {
			addResourceSize(buffer.resource.Get());
		}
		addResourceSize(freeIndexBuffer_.resource.Get());
		addResourceSize(freeCounterBuffer_.resource.Get());
		addResourceSize(indirectArgumentBuffer_.resource.Get());
		addResourceSize(trailSampleBuffer_.resource.Get());
		addResourceSize(emitterParameterBuffer_.Get());
		for (const auto& buffer : perFrameBuffers_) {
			addResourceSize(buffer.Get());
		}
		for (const ReadbackSlot& slot : readbackSlots_) {
			addResourceSize(slot.resource.Get());
		}
	}

} // MadoEngine::Particle名前空間
