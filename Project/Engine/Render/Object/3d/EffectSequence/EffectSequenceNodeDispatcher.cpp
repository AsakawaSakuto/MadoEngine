#include "EffectSequenceNodeDispatcher.h"
#include "Math/Function/MatrixFunction.h"
#include "Render/Object/3d/BeamEffect/BeamEffectSystem3d.h"
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

	/// @brief BeamのLocal始終点をWorld座標へ変換
	/// @param settings Beam固有設定
	/// @param transform Node World Transform
	/// @return World座標の始点と終点
	std::pair<Vector3, Vector3> BuildBeamEndpoints(
		const BeamNodeSettings& settings,
		const Transform3D& transform) {
		const Matrix4x4 world = MakeMatrix(transform);
		return {
			Matrix::Transform(settings.startPosition, world),
			Matrix::Transform(settings.endPosition, world),
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
		case EffectSequenceNodeType::Beam: {
			const auto* settings = std::get_if<BeamNodeSettings>(&node.settings);
			if (!settings) {
				return std::nullopt;
			}
			const auto [startPosition, endPosition] = BuildBeamEndpoints(*settings, worldTransform);
			MadoEngine::Beam::BeamEffectPlayDesc desc;
			desc.startPosition = startPosition;
			desc.endPosition = endPosition;
			desc.sceneType = sceneType;
			desc.renderLayer = renderLayer;
			desc.loopOverride = false;
			const MadoEngine::Beam::BeamEffectHandle handle =
				MadoEngine::Beam::BeamEffectSystem3d::GetInstance().Play(node.effectAssetName, desc);
			if (!handle.HasValue()) {
				return std::nullopt;
			}
			MadoEngine::Beam::BeamEffectSystem3d::GetInstance().SetPlaybackSpeed(handle, playbackSpeed);
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
					mode == EffectSequenceStopMode::Immediate
						? MadoEngine::Particle::StopMode::Immediate
						: MadoEngine::Particle::StopMode::Finish
				);
			},
			[mode](MadoEngine::Effect::PrimitiveEffectHandle child) {
				MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().Stop(
					child,
					mode == EffectSequenceStopMode::Immediate
						? MadoEngine::Effect::PrimitiveEffectStopMode::Immediate
						: MadoEngine::Effect::PrimitiveEffectStopMode::Finish
				);
			},
			[mode](MadoEngine::Ribbon::RibbonEffectHandle child) {
				MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().Stop(
					child,
					mode == EffectSequenceStopMode::Immediate
						? MadoEngine::Ribbon::RibbonStopMode::Immediate
						: MadoEngine::Ribbon::RibbonStopMode::Finish
				);
			},
			[mode](MadoEngine::Beam::BeamEffectHandle child) {
				MadoEngine::Beam::BeamEffectSystem3d::GetInstance().Stop(
					child,
					mode == EffectSequenceStopMode::Immediate
						? MadoEngine::Beam::BeamStopMode::Immediate
						: MadoEngine::Beam::BeamStopMode::Finish
				);
			},
		}, handle);
	}

	void EffectSequenceNodeDispatcher::Pause(const EffectSequenceChildHandle& handle) const {
		std::visit(Overloaded{
			[](MadoEngine::Particle::EffectHandle child) { MadoEngine::Particle::ParticleSystem3d::GetInstance().Pause(child); },
			[](MadoEngine::Effect::PrimitiveEffectHandle child) { MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().Pause(child); },
			[](MadoEngine::Ribbon::RibbonEffectHandle child) { MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().Pause(child); },
			[](MadoEngine::Beam::BeamEffectHandle child) { MadoEngine::Beam::BeamEffectSystem3d::GetInstance().Pause(child); },
		}, handle);
	}

	void EffectSequenceNodeDispatcher::Resume(const EffectSequenceChildHandle& handle) const {
		std::visit(Overloaded{
			[](MadoEngine::Particle::EffectHandle child) { MadoEngine::Particle::ParticleSystem3d::GetInstance().Resume(child); },
			[](MadoEngine::Effect::PrimitiveEffectHandle child) { MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().Resume(child); },
			[](MadoEngine::Ribbon::RibbonEffectHandle child) { MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().Resume(child); },
			[](MadoEngine::Beam::BeamEffectHandle child) { MadoEngine::Beam::BeamEffectSystem3d::GetInstance().Resume(child); },
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
					if (settings->overrideManualControlPoints) {
						MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().SetLocalControlPoints(
							child,
							settings->controlPoints
						);
					}
				}
			},
			[&node, &worldTransform](MadoEngine::Beam::BeamEffectHandle child) {
				if (const auto* settings = std::get_if<BeamNodeSettings>(&node.settings)) {
					const auto [startPosition, endPosition] = BuildBeamEndpoints(*settings, worldTransform);
					MadoEngine::Beam::BeamEffectSystem3d::GetInstance().SetEndpoints(
						child,
						startPosition,
						endPosition
					);
				}
			},
		}, handle);
	}

	void EffectSequenceNodeDispatcher::SetPlaybackSpeed(
		const EffectSequenceChildHandle& handle,
		float playbackSpeed) const {
		std::visit(Overloaded{
			[playbackSpeed](MadoEngine::Particle::EffectHandle child) { MadoEngine::Particle::ParticleSystem3d::GetInstance().SetPlaybackSpeed(child, playbackSpeed); },
			[playbackSpeed](MadoEngine::Effect::PrimitiveEffectHandle child) { MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().SetPlaybackSpeed(child, playbackSpeed); },
			[playbackSpeed](MadoEngine::Ribbon::RibbonEffectHandle child) { MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().SetPlaybackSpeed(child, playbackSpeed); },
			[playbackSpeed](MadoEngine::Beam::BeamEffectHandle child) { MadoEngine::Beam::BeamEffectSystem3d::GetInstance().SetPlaybackSpeed(child, playbackSpeed); },
		}, handle);
	}

	bool EffectSequenceNodeDispatcher::IsAlive(const EffectSequenceChildHandle& handle) const {
		return std::visit(Overloaded{
			[](MadoEngine::Particle::EffectHandle child) { return MadoEngine::Particle::ParticleSystem3d::GetInstance().IsAlive(child); },
			[](MadoEngine::Effect::PrimitiveEffectHandle child) { return MadoEngine::Effect::PrimitiveEffectSystem3d::GetInstance().IsAlive(child); },
			[](MadoEngine::Ribbon::RibbonEffectHandle child) { return MadoEngine::Ribbon::RibbonEffectSystem3d::GetInstance().IsAlive(child); },
			[](MadoEngine::Beam::BeamEffectHandle child) { return MadoEngine::Beam::BeamEffectSystem3d::GetInstance().IsAlive(child); },
		}, handle);
	}

} // namespace MadoEngine::EffectSequence
