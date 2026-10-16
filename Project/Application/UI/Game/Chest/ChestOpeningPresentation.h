#pragma once
#include "GameObject/Rarity.h"
#include <RenderHeaders.h>
#include <cstddef>

namespace UI::Game {

	/// @brief レリック獲得前の宝箱Model表示と開封時の画面演出
	class ChestOpeningPresentation {
	public:
		/// @brief 専用Modelと開封フラッシュ用Spriteを初期化
		void Initialize();

		/// @brief 演出を終了して描画オブジェクトの参照を解放
		void Finalize();

		/// @brief 画面中央の宝箱表示を開始
		/// @param rarity 抽選されたレリックのレアリティ
		void Show(Rarity rarity);

		/// @brief 宝箱と開封フラッシュを非表示化
		void Hide();

		/// @brief 開封入力と宝箱の膨張演出、フラッシュを更新
		/// @param deltaTime 前フレームからの経過時間（秒）
		/// @param acceptOpenInput 開封入力を受け付ける場合はtrue
		/// @return 宝箱の開封演出が完了した場合はtrue
		bool Update(float deltaTime, bool acceptOpenInput);

		/// @brief 描画Cameraの前方へ宝箱Modelを追従
		/// @param camera 追従対象の描画Camera
		void FollowCamera(const Camera& camera);

		/// @brief 開封待ちまたは膨張演出の表示状態を取得
		/// @return 開封待ちまたは膨張演出を表示中の場合はtrue
		bool IsActive() const { return state_ == State::WaitingForOpen || state_ == State::Opening; }

		/// @brief 開封フラッシュの再生状態を取得
		/// @return 開封フラッシュを再生中の場合はtrue
		bool IsFlashActive() const { return flashRemainingTime_ > 0.0f; }

	private:
		enum class State {
			Hidden,
			WaitingForOpen,
			Opening,
			Flashing,
		};

		/// @brief 各段階の待ち時間とレアリティに応じた宝箱の膨張倍率を更新
		/// @param deltaTime 前フレームからの経過時間（秒）
		/// @return 膨張演出が完了してフラッシュを開始した場合はtrue
		bool UpdateOpening(float deltaTime);

		/// @brief 宝箱を非表示にして開封フラッシュを開始
		void StartFlash();

		/// @brief フラッシュを白からレアリティ色へ補間しながら透明化
		/// @param deltaTime 前フレームからの経過時間（秒）
		void UpdateFlash(float deltaTime);

		MadoEngine::ModelHandle model_{};
		MadoEngine::SpriteHandle flashSprite_{};
		Vector3 modelCenter_ = {};
		Vector4 flashTargetColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		State state_ = State::Hidden;
		Rarity openingRarity_ = Rarity::Uncommon;
		std::size_t openingPulseCount_ = 1;
		std::size_t appliedOpeningPulseCount_ = 0;
		float openingElapsedTime_ = 0.0f;
		float openingScaleMultiplier_ = 1.0f;
		float flashRemainingTime_ = 0.0f;

	};

}
