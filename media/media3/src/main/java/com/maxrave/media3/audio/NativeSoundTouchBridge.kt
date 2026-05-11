package com.maxrave.media3.audio

import java.nio.ByteBuffer
internal class NativeSoundTouchBridge {
    external fun nativeCreate(
        sampleRate: Int,
        channelCount: Int,
    ): Long

    external fun nativeRelease(handle: Long)

    external fun nativeSetTempoPitch(
        handle: Long,
        tempo: Float,
        pitch: Float,
    )

    external fun nativePutSamples(
        handle: Long,
        input: ShortArray,
        offsetFrames: Int,
        frameCount: Int,
    ): Int

    external fun nativePutSamplesDirect(
        handle: Long,
        input: ByteBuffer,
        offsetBytes: Int,
        frameCount: Int,
    ): Int

    external fun nativeReceiveSamples(
        handle: Long,
        output: ShortArray,
        maxFrames: Int,
    ): Int

    external fun nativeReceiveSamplesDirect(
        handle: Long,
        output: ByteBuffer,
        offsetBytes: Int,
        maxFrames: Int,
    ): Int

    external fun nativeAvailableSamples(handle: Long): Int

    external fun nativeFlush(handle: Long)

    external fun nativeClear(handle: Long)

    companion object {
        init {
            System.loadLibrary("simpmusic_audio")
        }
    }
}
