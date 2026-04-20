#pragma once
// AudioData.h
// WAVとMP3を読み込んでPCM16に統一する。Media Foundationは実装側(AudioData.cpp)で使用。
// バッファはstd::vectorで管理し、手動deleteを廃止。

#include <cstdint>
#include <string>
#include <vector>
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

// PCMサウンドデータ
struct SoundData {
    WAVEFORMATEX wfex{};        // PCM16 を想定
    std::vector<BYTE> pcm;      // 波形データ(先頭から末尾まで格納)
    std::string name;           // デバッグ用の名称
};

// 読み込み系
SoundData SoundLoadWave(const std::string& filePath);   // WAV→PCM16（元がPCM/非圧縮想定）
SoundData SoundLoadAudio(const std::string& filePath);  // 拡張子でWAV/MP3を自動判別

// 互換維持：従来の「すぐ再生して捨てる」関数（非推奨）
// 注意：DestroyVoiceしない設計のため大量連打はリークの温床。移行が済むまでの暫定用途。
void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData);
