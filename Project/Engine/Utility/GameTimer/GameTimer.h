#pragma once

class GameTimer {
public:

    /// @brief デフォルトコンストラクタ
    GameTimer() = default;

    /// @brief コンストラクタ
    /// @param duration タイマーの継続時間
    /// @param loop ループする場合はtrue
    GameTimer(float duration, bool loop = false);

    /// @brief タイマーを更新
    /// @param deltaTime 経過時間
    void Update(float deltaTime = 1.0f / 60.0f);

    /// @brief タイマーを開始
    /// @param duration タイマーの継続時間
    /// @param loop ループする場合はtrue
    void Start(float duration, bool loop = false);

    /// @brief タイマーを停止
    void Stop();

    /// @brief タイマーをリセット
    void Reset();

    /// @brief タイマーを一時停止
    void Pause();

    /// @brief 一時停止中のタイマーを再開
    void Resume();

    /// @brief タイマーが動作中かどうかを取得
    /// @return 動作中ならtrue
    bool IsActive() const;

    /// @brief タイマーが完了したかどうかを取得
    /// @return 完了していればtrue
    bool IsFinished() const;

    /// @brief このフレームでループしたかどうかを取得
    /// @return このフレームでループしていればtrue
    bool WasLoopedThisFrame() const;

	/// @brief タイマーの進行率を0.0fから1.0fの範囲で取得
    /// @return 進行率
    float GetProgress() const;

	/// @brief タイマーの逆進行率を1.0fから0.0fの範囲で取得
    /// @return 逆進行率
    float GetReverseProgress() const;

    /// @brief 残り時間を取得
    /// @return 残り時間
    float GetRemainingTime() const;

    /// @brief 経過時間を取得
    /// @return 経過時間
    float GetElapsedTime() const;

    /// @brief タイマーの継続時間を取得
    /// @return 継続時間
    float GetDuration() const;

    /// @brief 継続時間を変更
    /// @param duration 新しい継続時間
    void SetDuration(float duration);

    /// @brief ループ設定を変更
    /// @param loop ループする場合はtrue
    void SetLoop(bool loop);

private:
    enum class State {
		Stopped,  // タイマーが停止している状態
		Running,  // タイマーが動作している状態
		Paused,   // タイマーが一時停止している状態
		Finished, // タイマーが完了した状態（ループしない場合）
    };

    float currentTime_ = 0.0f;
    float duration_ = 0.0f;
    bool loop_ = false;
    bool finished_ = false;
    bool loopedThisFrame_ = false;
    State state_ = State::Stopped;
};
