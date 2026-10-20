#include "EffectSequenceNodeDispatcher.h"
#include "Math/Function/MatrixFunction.h"
#include "Render/Object/3d/Particle/ParticleSystem3d.h"
#include "Render/Object/3d/PrimitiveEffect/PrimitiveEffectSystem3d.h"
#include "Render/Object/3d/RibbonEffect/RibbonEffectSystem3d.h"
#include <type_traits>
#include <vector>

namespace {

	using namespace MadoEngine::EffectSequence;

	template<class... TCallbacks>
	struct Overloaded : TCallbacks... {
		using TCallbacks::operator()...;
	};

	/// @brief TransformからAffine Matrixを生成
	/// @param transform 変換元Transform
	/// @return 生成したMatrix
	Matrix4x4 MakeMatrix(const Transform3D& transform) {
		return Matrix::MakeAffine(transform.scale, transform.rotate, transform.translate);
	}

	/// @brief RibbonのLocal始終点をWorld座標へ変換
	/// @param settings Ribbon固有設定
	/// @param transform Node World Transform
	/// @return World座標の始点と終点
	std::pair<Vector3, Vector3> BuildRibbonEndpoints(
		const RibbonNodeSettings& settings,
		const Transform3D& transform) {
		const Matrix4x4 world = MakeMatrix(transform);
		return {
			Matrix::Transform(settings.endpoints.startPosition, world),
			Matrix::Transform(settings.endpoints.endPosition, world),
		};
	}

} // namespace

namespace MadoEngine::EffectSequence {

	std::optional<EffectSequenceChildHandle> EffectSequenceNodeDispatcher::Play(
		const EffectSequenceNode& node,
		const Transform3D& worldTransform,
		SceneType sceneType,
		MadoEngine::Render::RenderLayer defaultRenderLayer,
		float playbackSpeed) const {
		const MadoEngine::Render::RenderLayer renderLayer = node.renderLayer.value_or(defaultRenderLayer);

		// Node種別ごとのSystemへ再生条件を統一して委譲し、HandleをVariantへ集約
		switch (node.nodeType) {
		case EffectSequenceNodeType::Particle: {
			MadoEngine::Particle::PlayDesc desc;
			desc.transform = worldTransform;
			desc.sceneType = sceneType;
			desc.renderLayer = renderLayer;
			desc.loopOverride = false;
			const MadoEngine::Particle::EffectHandle handle =
				MadoEngine::Particle::ParticleSystem3d::GetInstance().Play(node.effectAssetName, desc);
			if (!handle.HasValue()) {
				return std::nullopt;
			}
			MadoEngine::Particle::ParticleSystem3d::GetInstance().SetPlaybackSpeed(handle, playbackSpeed);
			return EffectSequenceChildHandle{ handle };
		}
		case EffectSequenceNodeType::PrimitiveEffect: {
			const auto* settings = std::get_if<PrimitiveEffectNodeSettings>(&node.settings);
			if (!settings || settings->kind != PrimitiveEffectNodeKind::Cylinder) {
				return std::nullopt;
			}
			MadoEngine::Effect::PrimitiveEffectPlayDesc desc;
			desc.transform = worldTransform;
			desc.sceneType = sceneType;
			desc.renderLayer = renderLayer;
			desc.loopOverride = false;
			const MadoEngine::Effect::PrimitiveEffectHandle handle =
				MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().Play(node.effectAssetName, desc);
			if (!handle.HasValue()) {
				return std::nullopt;
			}
			MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().SetPlaybackSpeed(handle, playbackSpeed);
			return EffectSequenceChildHandle{ handle };
		}
		case EffectSequenceNodeType::Ribbon: {
			MadoEngine::Ribbon::RibbonEffectPlayDesc desc;
			desc.transform = worldTransform;
			if (const auto* settings = std::get_if<RibbonNodeSettings>(&node.settings)) {
				if (settings->overrideEndpoints) {
					const auto [start, end] = BuildRibbonEndpoints(*settings, worldTransform);
					desc.endpoints = MadoEngine::Ribbon::RibbonEndpoints{ start, end };
				}
			}
			desc.sceneType = sceneType;
			desc.renderLayer = renderLayer;
			desc.loopOverride = false;
			const MadoEngine::Ribbon::RibbonEffectHandle handle =
				MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().Play(node.effectAssetName, desc);
			if (!handle.HasValue()) {
				return std::nullopt;
			}
			MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().SetPlaybackSpeed(handle, playbackSpeed);
			if (const auto* settings = std::get_if<RibbonNodeSettings>(&node.settings)) {
				if (settings->overrideManualControlPoints) {

					// Asset側の制御点よりSequence Node固有の軌跡を優先
					MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().SetLocalControlPoints(
						handle,
						settings->controlPoints
					);
				}
			}
			return EffectSequenceChildHandle{ handle };
		}

		case EffectSequenceNodeType::Count:
		default:
			return std::nullopt;
		}
	}

	void EffectSequenceNodeDispatcher::Stop(
		const EffectSequenceChildHandle& handle,
		EffectSequenceStopMode mode) const {

		// Variantが保持する実体のSystemへ共通停止Modeを変換して委譲
		std::visit(Overloaded{
			[mode](MadoEngine::Particle::EffectHandle child) {
				MadoEngine::Particle::ParticleSystem3d::GetInstance().Stop(
					child,
					[&]() {
						if (mode == EffectSequenceStopMode::Immediate) {
							return (MadoEngine::Particle::StopMode::Immediate);
						} else {
							return (MadoEngine::Particle::StopMode::Finish);
						}
					}()
				);
			},
			[mode](MadoEngine::Effect::PrimitiveEffectHandle child) {
				MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().Stop(
					child,
					[&]() {
						if (mode == EffectSequenceStopMode::Immediate) {
							return (MadoEngine::Effect::PrimitiveEffectStopMode::Immediate);
						} else {
							return (MadoEngine::Effect::PrimitiveEffectStopMode::Finish);
						}
					}()
				);
			},
			[mode](MadoEngine::Ribbon::RibbonEffectHandle child) {
				MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().Stop(
					child,
					[&]() {
						if (mode == EffectSequenceStopMode::Immediate) {
							return (MadoEngine::Ribbon::RibbonStopMode::Immediate);
						} else {
							return (MadoEngine::Ribbon::RibbonStopMode::Finish);
						}
					}()
				);
			},
		}, handle);
	}

	void EffectSequenceNodeDispatcher::Pause(const EffectSequenceChildHandle& handle) const {
		std::visit(Overloaded{
			[](MadoEngine::Particle::EffectHandle child) { MadoEngine::Particle::ParticleSystem3d::GetInstance().Pause(child); },
			[](MadoEngine::Effect::PrimitiveEffectHandle child) { MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().Pause(child); },
			[](MadoEngine::Ribbon::RibbonEffectHandle child) { MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().Pause(child); },
		}, handle);
	}

	void EffectSequenceNodeDispatcher::Resume(const EffectSequenceChildHandle& handle) const {
		std::visit(Overloaded{
			[](MadoEngine::Particle::EffectHandle child) { MadoEngine::Particle::ParticleSystem3d::GetInstance().Resume(child); },
			[](MadoEngine::Effect::PrimitiveEffectHandle child) { MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().Resume(child); },
			[](MadoEngine::Ribbon::RibbonEffectHandle child) { MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().Resume(child); },
		}, handle);
	}

	void EffectSequenceNodeDispatcher::SetTransform(
		const EffectSequenceNode& node,
		const EffectSequenceChildHandle& handle,
		const Transform3D& worldTransform) const {

		// Effect固有の座標表現へ変換しながら親NodeのWorld Transformを反映
		std::visit(Overloaded{
			[&worldTransform](MadoEngine::Particle::EffectHandle child) {
				MadoEngine::Particle::ParticleSystem3d::GetInstance().SetTransform(child, worldTransform);
			},
			[&worldTransform](MadoEngine::Effect::PrimitiveEffectHandle child) {
				MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().SetTransform(child, worldTransform);
			},
			[&node, &worldTransform](MadoEngine::Ribbon::RibbonEffectHandle child) {
				MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().SetTransform(child, worldTransform);
				if (const auto* settings = std::get_if<RibbonNodeSettings>(&node.settings)) {
					if (settings->overrideEndpoints) {
						const auto [start, end] = BuildRibbonEndpoints(*settings, worldTransform);
						MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().SetEndpoints(child, { start, end });
					}
					if (settings->overrideManualControlPoints) {
						MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().SetLocalControlPoints(
							child,
							settings->controlPoints
						);
					}
				}
			},
		}, handle);
	}

	void EffectSequenceNodeDispatcher::SetVisible(
		const EffectSequenceChildHandle& handle,
		bool isVisible) const {
		std::visit(Overloaded{
			[isVisible](MadoEngine::Particle::EffectHandle child) { MadoEngine::Particle::ParticleSystem3d::GetInstance().SetVisible(child, isVisible); },
			[isVisible](MadoEngine::Effect::PrimitiveEffectHandle child) { MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().SetVisible(child, isVisible); },
			[isVisible](MadoEngine::Ribbon::RibbonEffectHandle child) { MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().SetVisible(child, isVisible); },
		}, handle);
	}

	void EffectSequenceNodeDispatcher::SetColorMultiplier(
		const EffectSequenceChildHandle& handle,
		const Vector4& colorMultiplier) const {

		// 現在色倍率に対応するPrimitive Effectだけへ反映し未対応Effect種別は維持
		std::visit(Overloaded{
			[](MadoEngine::Particle::EffectHandle) {},
			[&colorMultiplier](MadoEngine::Effect::PrimitiveEffectHandle child) {
				MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().SetColorMultiplier(
					child,
					colorMultiplier
				);
			},
			[](MadoEngine::Ribbon::RibbonEffectHandle) {},
		}, handle);
	}

	void EffectSequenceNodeDispatcher::SetPlaybackSpeed(
		const EffectSequenceChildHandle& handle,
		float playbackSpeed) const {
		std::visit(Overloaded{
			[playbackSpeed](MadoEngine::Particle::EffectHandle child) { MadoEngine::Particle::ParticleSystem3d::GetInstance().SetPlaybackSpeed(child, playbackSpeed); },
			[playbackSpeed](MadoEngine::Effect::PrimitiveEffectHandle child) { MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().SetPlaybackSpeed(child, playbackSpeed); },
			[playbackSpeed](MadoEngine::Ribbon::RibbonEffectHandle child) { MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().SetPlaybackSpeed(child, playbackSpeed); },
		}, handle);
	}

	bool EffectSequenceNodeDispatcher::IsAlive(const EffectSequenceChildHandle& handle) const {
		return std::visit(Overloaded{
			[](MadoEngine::Particle::EffectHandle child) { return MadoEngine::Particle::ParticleSystem3d::GetInstance().IsAlive(child); },
			[](MadoEngine::Effect::PrimitiveEffectHandle child) { return MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().IsAlive(child); },
			[](MadoEngine::Ribbon::RibbonEffectHandle child) { return MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().IsAlive(child); },
		}, handle);
	}

} // namespace MadoEngine::EffectSequence
