// Audio.cpp
// AudioX実装。XAudio2エンジンとマスターボイスを作成し、PCM共有→インスタンス再生。

#include "Audio.h"
#include <cassert>

void AudioX::Initialize(const std::string& filePath) {
    filePath_ = "resources/sound/" + filePath;

    HRESULT hr = XAudio2Create(&xAudio2_);
    assert(SUCCEEDED(hr) && "XAudio2Create failed");

    IXAudio2MasteringVoice* mv = nullptr;
    hr = xAudio2_->CreateMasteringVoice(&mv);
    assert(SUCCEEDED(hr) && "CreateMasteringVoice failed");
    mastering_.reset(mv);

    SoundData sd = SoundLoadAudio(filePath); // WAVでもMP3でもOK
    wfex_ = sd.wfex;

    // PCM共有用に移し替え
    pcmShared_ = std::make_shared<std::vector<BYTE>>(std::move(sd.pcm));
}

void AudioX::PlayAudio(float volume, bool loop) {
    if (!xAudio2_ || !mastering_ || !pcmShared_ || pcmShared_->empty()) return;

    auto inst = std::make_unique<ClipInstance>();
    inst->pcm_ = pcmShared_;
    inst->wfex_ = wfex_;
    inst->loop_ = loop;
    inst->Play(xAudio2_.Get(), volume);
    actives_.push_back(std::move(inst));
}

void AudioX::Update() {
    for (size_t i = 0; i < actives_.size(); ) {
        if (!actives_[i] || actives_[i]->Finished()) {
            actives_.erase(actives_.begin() + i);
        }
        else {
            ++i;
        }
    }
}

void AudioX::Reset() {
    // まず全インスタンス停止→破棄
    for (auto& a : actives_) {
        if (a) a->Stop();
    }
    actives_.clear();

    // 共有PCM破棄（他所で共有されていれば自動延命される）
    pcmShared_.reset();

    mastering_.reset();
    xAudio2_.Reset();
}

void AudioX::SetVolume(float volume) {
    // クランプ（0.0f～1.0fの範囲）
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    for (auto& a : actives_) {
        if (a && a->voice_) {
            a->voice_->SetVolume(volume);
        }
    }
}

void AudioX::StopAll() {
    // 再生中のボイスを即停止→キュー破棄
    for (auto& a : actives_) {
        if (a) a->Stop();
    }
    // DestroyVoice は unique_ptr のデリータが実行
    actives_.clear();
    // PCM(pcmShared_)とXAudio2は残るので、すぐ PlayAudio() し直せる
}

void AudioX::StopLatest() {
    if (actives_.empty()) return;
    auto& a = actives_.back();
    if (a) a->Stop();
    actives_.pop_back();
}

void AudioX::StopAllLoops() {
    for (size_t i = 0; i < actives_.size();) {
        auto& a = actives_[i];
        if (a && a->loop_) {
            a->Stop();
            actives_.erase(actives_.begin() + i);
            continue;
        }
        ++i;
    }
}