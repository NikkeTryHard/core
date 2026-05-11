package com.maxrave.media3.audio

import java.nio.ByteBuffer
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

    external fun nativeProcessDirect(
        handle: Long,
        input: ByteBuffer,
        inputOffsetBytes: Int,
        output: ByteBuffer,
        outputOffsetBytes: Int,
        frameCount: Int,
        channelCount: Int,
    ): Boolean

    external fun nativeClear(handle: Long)

    companion object {
        init {
            System.loadLibrary("simpmusic_audio")
        }
    }
}
