#pragma once
#include <cstdint>
#include <random>

/// @brief 乱数生成クラス
/// 
/// メルセンヌ・ツイスタ法(std::mt19937)を使用した乱数生成器。
/// 整数および浮動小数点数の一様分布乱数を生成できる。
class Random {
public:
    /// @brief デフォルトコンストラクタ
    /// 
    /// std::random_deviceを使用してランダムなシード値で初期化する。
    Random() : Random(CreateRandomSeed()) {}

    /// @brief シード値を指定したコンストラクタ
    /// 
    /// 指定されたシード値で乱数エンジンを初期化する。
    /// デバッグやテスト時に再現可能な乱数列を生成したい場合に使用する。
    /// 
    /// @param seed シード値
    explicit Random(uint32_t seed) : seed_(seed), engine(seed) {}

    /// @brief シード値を設定
    /// 
    /// 乱数エンジンのシード値を変更する。
    /// これにより乱数列がリセットされる。
    /// 
    /// @param seed 新しいシード値
    void SetSeed(uint32_t seed) {
        seed_ = seed;
        engine.seed(seed);
    }

    /// @brief 現在設定されているシード値を取得します。
    /// @return uint32_t 現在設定されているシード値です。
    uint32_t GetSeed() const {
        return seed_;
    }

    /// @brief 整数乱数を生成
    /// 
    /// 指定された範囲内で一様分布に従う整数乱数を生成する。
    /// 
    /// @param min 乱数の最小値（この値を含む）
    /// @param max 乱数の最大値（この値を含む）
    /// @return int 生成された乱数値 [min, max]
    int Int(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(engine);
    }

    /// @brief 浮動小数点乱数を生成
    /// 
    /// 指定された範囲内で一様分布に従う浮動小数点乱数を生成する。
    /// 
    /// @param min 乱数の最小値（この値を含む）
    /// @param max 乱数の最大値（この値を含まない）
    /// @return float 生成された乱数値 [min, max)
    float Float(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(engine);
    }

private:
    /// @brief ランダムなシード値を生成します。
    /// @return uint32_t 生成されたシード値です。
    static uint32_t CreateRandomSeed() {
        return std::random_device{}();
    }

    uint32_t seed_ = 0; ///< 現在設定されているシード値
    std::mt19937 engine; ///< メルセンヌ・ツイスタ乱数エンジン
};

/// @brief グローバル乱数生成機能を提供する名前空間
/// 
/// アプリケーション全体で共有されるスレッドローカルなRandomインスタンスを使用した
/// 簡易的な乱数生成機能を提供する。各スレッドごとに独立した乱数列を生成する。
namespace MyRand {

    /// @brief ランダムなシード値を生成します。
    /// @return uint32_t 生成されたシード値です。
    inline uint32_t CreateSeed() {
        return std::random_device{}();
    }

    /// @brief 基準シードとストリームIDから派生シードを生成します。
    /// @param baseSeed 基準となるシード値です。
    /// @param streamId 用途ごとのストリームIDです。
    /// @return uint32_t 派生されたシード値です。
    inline uint32_t MakeDerivedSeed(uint32_t baseSeed, uint32_t streamId) {
        uint32_t seed = baseSeed;
        seed ^= streamId + 0x9e3779b9u + (seed << 6) + (seed >> 2);
        return seed;
    }

    /// @brief スレッドローカルなRandomインスタンスを取得
    /// 
    /// 各スレッドごとに独立したRandomインスタンスを返す。
    /// スレッドセーフな乱数生成を保証する。
    /// 
    /// @return Random& スレッドローカルなRandomインスタンスへの参照
    inline Random& GetInstance() {
        thread_local Random instance;
        return instance;
    }

    /// @brief グローバルインスタンスのシード値を設定
    /// 
    /// スレッドローカルなRandomインスタンスのシード値を変更する。
    /// デバッグやテスト時に再現可能な乱数列を生成したい場合に使用する。
    /// 
    /// @param seed 新しいシード値
    inline void SetSeed(uint32_t seed) {
        GetInstance().SetSeed(seed);
    }

    /// @brief グローバルなRandomインスタンスの現在のシード値を取得します。
    /// @return uint32_t 現在設定されているシード値です。
    inline uint32_t GetSeed() {
        return GetInstance().GetSeed();
    }

    /// @brief 整数乱数を生成
    /// 
    /// グローバルなRandomインスタンスを使用して整数乱数を生成する。
    /// 
    /// @param min 乱数の最小値（この値を含む）
    /// @param max 乱数の最大値（この値を含む）
    /// @return int 生成された乱数値 [min, max]
    inline int GetInt(int min, int max) {
        return GetInstance().Int(min, max);
    }

    /// @brief 浮動小数点乱数を生成
    /// 
    /// グローバルなRandomインスタンスを使用して浮動小数点乱数を生成する。
    /// 
    /// @param min 乱数の最小値（この値を含む）
    /// @param max 乱数の最大値（この値を含まない）
    /// @return float 生成された乱数値 [min, max)
    inline float GetFloat(float min, float max) {
        return GetInstance().Float(min, max);
    }
}
