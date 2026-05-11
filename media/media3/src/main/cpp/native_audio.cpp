#include <jni.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include <android/log.h>
#include "SoundTouch.h"
#include "airwindows_reverb.h"

#define LOG_TAG "SimpMusicAudio"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

constexpr int kSettingUseAaFilter = 0;
constexpr int kSettingAaFilterLength = 1;
constexpr int kSettingUseQuickSeek = 2;
constexpr int kSettingSequenceMs = 3;
constexpr int kSettingSeekWindowMs = 4;
constexpr int kSettingOverlapMs = 5;

class NativeTimeStretch {
public:
    NativeTimeStretch(int sampleRate, int channelCount)
        : sampleRate_(sampleRate), channelCount_(channelCount) {
        soundTouch_.setSampleRate(static_cast<uint>(sampleRate));
        soundTouch_.setChannels(static_cast<uint>(channelCount));
        soundTouch_.setTempo(1.0);
        soundTouch_.setPitch(1.0);
        soundTouch_.setRate(1.0);
        soundTouch_.setSetting(kSettingUseAaFilter, 1);
        soundTouch_.setSetting(kSettingAaFilterLength, 64);
        soundTouch_.setSetting(kSettingUseQuickSeek, 0);
        soundTouch_.setSetting(kSettingSequenceMs, 82);
        soundTouch_.setSetting(kSettingSeekWindowMs, 28);
        soundTouch_.setSetting(kSettingOverlapMs, 12);
    }

    void setTempoPitch(float tempo, float pitch) {
        soundTouch_.setTempo(std::max(0.05f, tempo));
        soundTouch_.setPitch(std::max(0.05f, pitch));
    }

    jint put(const jshort* input, int frameCount) {
        return putPcm16(reinterpret_cast<const int16_t*>(input), frameCount);
    }

    jint available() const {
        return static_cast<jint>(soundTouch_.numSamples());
    }

    jint putPcm16(const int16_t* input, int frameCount) {
        if (frameCount <= 0 || input == nullptr) return 0;
        const int sampleCount = frameCount * channelCount_;
        inputFloat_.resize(static_cast<size_t>(sampleCount));
        constexpr float scale = 1.0f / 32768.0f;
        for (int i = 0; i < sampleCount; ++i) {
            inputFloat_[static_cast<size_t>(i)] = static_cast<float>(input[i]) * scale;
        }
        soundTouch_.putSamples(inputFloat_.data(), static_cast<uint>(frameCount));
        return static_cast<jint>(soundTouch_.numSamples());
    }

    jint receive(jshort* output, int maxFrames) {
        if (maxFrames <= 0) return 0;
        const int maxSamples = maxFrames * channelCount_;
        outputFloat_.resize(static_cast<size_t>(maxSamples));
        const uint received = soundTouch_.receiveSamples(outputFloat_.data(), static_cast<uint>(maxFrames));
        const int receivedSamples = static_cast<int>(received) * channelCount_;
        for (int i = 0; i < receivedSamples; ++i) {
            const float clipped = std::max(-1.0f, std::min(1.0f, outputFloat_[static_cast<size_t>(i)]));
            output[i] = static_cast<jshort>(std::lrintf(clipped * 32767.0f));
        }
        return static_cast<jint>(received);
    }

    jint receivePcm16(int16_t* output, int maxFrames) {
        return receive(reinterpret_cast<jshort*>(output), maxFrames);
    }

    void flush() { soundTouch_.flush(); }

    void clear() { soundTouch_.clear(); }

private:
    int sampleRate_;
    int channelCount_;
    soundtouch::SoundTouch soundTouch_;
    std::vector<float> inputFloat_;
    std::vector<float> outputFloat_;
};

class NativeReverb {
public:
    explicit NativeReverb(int sampleRate) : reverb_(static_cast<double>(sampleRate)) {}

    void set(float wet) {
        const double mix = std::max(0.0f, std::min(1.0f, wet));
        reverb_.set(0.62, mix);
    }

    void process(const jshort* input, jshort* output, int frameCount, int channelCount) {
        constexpr double scale = 1.0 / 32768.0;
        for (int frame = 0; frame < frameCount; ++frame) {
            const int index = frame * channelCount;
            const double inL = static_cast<double>(input[index]) * scale;
            const double inR = channelCount == 2 ? static_cast<double>(input[index + 1]) * scale : inL;
            double outL = 0.0;
            double outR = 0.0;
            reverb_.process(inL, inR, outL, outR);
            output[index] = toPcm16(outL);
            if (channelCount == 2) output[index + 1] = toPcm16(outR);
        }
    }

    void clear() { reverb_.clear(); }

private:
    static jshort toPcm16(double value) {
        const double clipped = std::max(-1.0, std::min(1.0, value));
        return static_cast<jshort>(std::lrint(clipped * 32767.0));
    }

    AirwindowsReverb reverb_;
};

NativeTimeStretch* fromHandle(jlong handle) {
    return reinterpret_cast<NativeTimeStretch*>(handle);
}

NativeReverb* reverbFromHandle(jlong handle) {
    return reinterpret_cast<NativeReverb*>(handle);
}

}  // namespace

extern "C" JNIEXPORT jlong JNICALL
Java_com_maxrave_media3_audio_NativeSoundTouchBridge_nativeCreate(
        JNIEnv*, jobject, jint sampleRate, jint channelCount) {
    if (sampleRate <= 0 || channelCount <= 0 || channelCount > 2) return 0;
    return reinterpret_cast<jlong>(new NativeTimeStretch(sampleRate, channelCount));
}

extern "C" JNIEXPORT void JNICALL
Java_com_maxrave_media3_audio_NativeSoundTouchBridge_nativeRelease(
        JNIEnv*, jobject, jlong handle) {
    delete fromHandle(handle);
}

extern "C" JNIEXPORT void JNICALL
Java_com_maxrave_media3_audio_NativeSoundTouchBridge_nativeSetTempoPitch(
        JNIEnv*, jobject, jlong handle, jfloat tempo, jfloat pitch) {
    auto* processor = fromHandle(handle);
    if (processor == nullptr) return;
    processor->setTempoPitch(tempo, pitch);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_maxrave_media3_audio_NativeSoundTouchBridge_nativePutSamples(
        JNIEnv* env, jobject, jlong handle, jshortArray input, jint offsetFrames, jint frameCount) {
    auto* processor = fromHandle(handle);
    if (processor == nullptr || input == nullptr || frameCount <= 0) return 0;
    jboolean isCopy = JNI_FALSE;
    jshort* data = env->GetShortArrayElements(input, &isCopy);
    if (data == nullptr) return 0;
    const jint ready = processor->put(data + offsetFrames, frameCount);
    env->ReleaseShortArrayElements(input, data, JNI_ABORT);
    return ready;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_maxrave_media3_audio_NativeSoundTouchBridge_nativePutSamplesDirect(
        JNIEnv* env, jobject, jlong handle, jobject input, jint offsetBytes, jint frameCount) {
    auto* processor = fromHandle(handle);
    if (processor == nullptr || input == nullptr || frameCount <= 0 || offsetBytes < 0) return 0;
    auto* bytes = static_cast<uint8_t*>(env->GetDirectBufferAddress(input));
    if (bytes == nullptr) return 0;
    return processor->putPcm16(reinterpret_cast<const int16_t*>(bytes + offsetBytes), frameCount);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_maxrave_media3_audio_NativeSoundTouchBridge_nativeReceiveSamples(
        JNIEnv* env, jobject, jlong handle, jshortArray output, jint maxFrames) {
    auto* processor = fromHandle(handle);
    if (processor == nullptr || output == nullptr || maxFrames <= 0) return 0;
    jboolean isCopy = JNI_FALSE;
    jshort* data = env->GetShortArrayElements(output, &isCopy);
    if (data == nullptr) return 0;
    const jint received = processor->receive(data, maxFrames);
    env->ReleaseShortArrayElements(output, data, 0);
    return received;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_maxrave_media3_audio_NativeSoundTouchBridge_nativeReceiveSamplesDirect(
        JNIEnv* env, jobject, jlong handle, jobject output, jint offsetBytes, jint maxFrames) {
    auto* processor = fromHandle(handle);
    if (processor == nullptr || output == nullptr || maxFrames <= 0 || offsetBytes < 0) return 0;
    auto* bytes = static_cast<uint8_t*>(env->GetDirectBufferAddress(output));
    if (bytes == nullptr) return 0;
    return processor->receivePcm16(reinterpret_cast<int16_t*>(bytes + offsetBytes), maxFrames);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_maxrave_media3_audio_NativeSoundTouchBridge_nativeAvailableSamples(
        JNIEnv*, jobject, jlong handle) {
    auto* processor = fromHandle(handle);
    return processor == nullptr ? 0 : processor->available();
}

extern "C" JNIEXPORT void JNICALL
Java_com_maxrave_media3_audio_NativeSoundTouchBridge_nativeFlush(
        JNIEnv*, jobject, jlong handle) {
    auto* processor = fromHandle(handle);
    if (processor != nullptr) processor->flush();
}

extern "C" JNIEXPORT void JNICALL
Java_com_maxrave_media3_audio_NativeSoundTouchBridge_nativeClear(
        JNIEnv*, jobject, jlong handle) {
    auto* processor = fromHandle(handle);
    if (processor != nullptr) processor->clear();
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_maxrave_media3_audio_NativeReverbBridge_nativeCreate(
        JNIEnv*, jobject, jint sampleRate) {
    if (sampleRate <= 0) return 0;
    return reinterpret_cast<jlong>(new NativeReverb(sampleRate));
}

extern "C" JNIEXPORT void JNICALL
Java_com_maxrave_media3_audio_NativeReverbBridge_nativeRelease(
        JNIEnv*, jobject, jlong handle) {
    delete reverbFromHandle(handle);
}

extern "C" JNIEXPORT void JNICALL
Java_com_maxrave_media3_audio_NativeReverbBridge_nativeSetWet(
        JNIEnv*, jobject, jlong handle, jfloat wet) {
    auto* reverb = reverbFromHandle(handle);
    if (reverb != nullptr) reverb->set(wet);
}

extern "C" JNIEXPORT void JNICALL
Java_com_maxrave_media3_audio_NativeReverbBridge_nativeProcess(
        JNIEnv* env, jobject, jlong handle, jshortArray input, jshortArray output, jint frameCount, jint channelCount) {
    auto* reverb = reverbFromHandle(handle);
    if (reverb == nullptr || input == nullptr || output == nullptr || frameCount <= 0 || channelCount <= 0) return;
    jboolean inCopy = JNI_FALSE;
    jboolean outCopy = JNI_FALSE;
    jshort* inData = env->GetShortArrayElements(input, &inCopy);
    jshort* outData = env->GetShortArrayElements(output, &outCopy);
    if (inData != nullptr && outData != nullptr) {
        reverb->process(inData, outData, frameCount, channelCount);
    }
    if (inData != nullptr) env->ReleaseShortArrayElements(input, inData, JNI_ABORT);
    if (outData != nullptr) env->ReleaseShortArrayElements(output, outData, 0);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_maxrave_media3_audio_NativeReverbBridge_nativeProcessDirect(
        JNIEnv* env, jobject, jlong handle, jobject input, jint inputOffsetBytes, jobject output, jint outputOffsetBytes, jint frameCount, jint channelCount) {
    auto* reverb = reverbFromHandle(handle);
    if (reverb == nullptr || input == nullptr || output == nullptr || frameCount <= 0 || channelCount <= 0 || channelCount > 2 || inputOffsetBytes < 0 || outputOffsetBytes < 0) return JNI_FALSE;
    auto* inBytes = static_cast<uint8_t*>(env->GetDirectBufferAddress(input));
    auto* outBytes = static_cast<uint8_t*>(env->GetDirectBufferAddress(output));
    if (inBytes == nullptr || outBytes == nullptr) return JNI_FALSE;
    reverb->process(
            reinterpret_cast<const jshort*>(inBytes + inputOffsetBytes),
            reinterpret_cast<jshort*>(outBytes + outputOffsetBytes),
            frameCount,
            channelCount);
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_maxrave_media3_audio_NativeReverbBridge_nativeClear(
        JNIEnv*, jobject, jlong handle) {
    auto* reverb = reverbFromHandle(handle);
    if (reverb != nullptr) reverb->clear();
}
