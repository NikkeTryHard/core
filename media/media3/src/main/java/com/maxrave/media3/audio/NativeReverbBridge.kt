package com.maxrave.media3.audio

internal class NativeReverbBridge {
    external fun nativeCreate(sampleRate: Int): Long

    external fun nativeRelease(handle: Long)

    external fun nativeSetWet(
        handle: Long,
        wet: Float,
    )

    external fun nativeProcess(
        handle: Long,
        input: ShortArray,
        output: ShortArray,
        frameCount: Int,
        channelCount: Int,
    )

    external fun nativeClear(handle: Long)

    companion object {
        init {
            System.loadLibrary("simpmusic_audio")
        }
    }
}
