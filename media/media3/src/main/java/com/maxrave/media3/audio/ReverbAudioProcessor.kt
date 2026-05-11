package com.maxrave.media3.audio

import androidx.media3.common.C
import androidx.media3.common.audio.AudioProcessor
import androidx.media3.common.audio.BaseAudioProcessor
import androidx.media3.common.util.UnstableApi
import java.nio.ByteBuffer
import java.nio.ByteOrder

/**
 * Airwindows Reverb for Media3 PCM streams.
 *
 * Uses a native C++ port of Airwindows "Reverb" (MIT): a simplified/tuned
 * MatrixVerb design with modulated delay taps and Householder feedback matrices.
 */
@UnstableApi
class ReverbAudioProcessor : BaseAudioProcessor() {
    @Volatile
    var wetMix: Float = 0f
        set(value) {
            val coerced = value.coerceIn(0f, 1f)
            if (field != coerced) {
                field = coerced
                wetDirty = true
            }
        }

    private var bridge: NativeReverbBridge? = null
    private var handle = 0L
    private var channelCount = 0
    private var wetDirty = true
    private var inputShorts = ShortArray(0)
    private var outputShorts = ShortArray(0)

    override fun onConfigure(inputAudioFormat: AudioProcessor.AudioFormat): AudioProcessor.AudioFormat {
        if (inputAudioFormat.encoding != C.ENCODING_PCM_16BIT || inputAudioFormat.channelCount !in 1..2) {
            return AudioProcessor.AudioFormat.NOT_SET
        }
        recreate(inputAudioFormat.sampleRate, inputAudioFormat.channelCount)
        return inputAudioFormat
    }

    override fun isActive(): Boolean = true

    override fun queueInput(inputBuffer: ByteBuffer) {
        val remaining = inputBuffer.remaining()
        if (remaining == 0) return

        val output = replaceOutputBuffer(remaining)
        inputBuffer.order(ByteOrder.nativeOrder())
        output.order(ByteOrder.nativeOrder())

        if (wetMix <= 0f) {
            copyBuffer(inputBuffer, output, remaining)
            output.flip()
            return
        }

        applyWetIfNeeded()
        val frameCount = remaining / (channelCount * BYTES_PER_SAMPLE)
        val sampleCount = frameCount * channelCount
        ensureCapacity(sampleCount)
        for (i in 0 until sampleCount) {
            inputShorts[i] = inputBuffer.short
        }
        bridge?.nativeProcess(handle, inputShorts, outputShorts, frameCount, channelCount)
        for (i in 0 until sampleCount) {
            output.putShort(outputShorts[i])
        }
        output.flip()
    }

    private fun recreate(
        sampleRate: Int,
        channels: Int,
    ) {
        releaseNative()
        channelCount = channels
        handle = 0L
        wetDirty = true
        applyWetIfNeeded(force = true)
    }

    private fun applyWetIfNeeded(force: Boolean = false) {
        if ((wetDirty || force) && wetMix > 0f) {
            ensureNative()
            if (handle != 0L) {
                bridge?.nativeSetWet(handle, wetMix)
                wetDirty = false
            }
        }
    }

    private fun ensureNative() {
        if (handle == 0L) {
            val nativeBridge = bridge ?: NativeReverbBridge().also { bridge = it }
            handle = nativeBridge.nativeCreate(inputAudioFormat.sampleRate)
        }
    }

    private fun releaseNative() {
        if (handle != 0L) {
            bridge?.nativeRelease(handle)
            handle = 0L
        }
    }

    private fun ensureCapacity(sampleCount: Int) {
        if (inputShorts.size < sampleCount) inputShorts = ShortArray(sampleCount)
        if (outputShorts.size < sampleCount) outputShorts = ShortArray(sampleCount)
    }

    private fun copyBuffer(src: ByteBuffer, dst: ByteBuffer, size: Int) {
        val pos = src.position()
        for (i in 0 until size) {
            dst.put(src.get(pos + i))
        }
        src.position(pos + size)
    }

    override fun onFlush() {
        super.onFlush()
        if (handle != 0L) {
            bridge?.nativeClear(handle)
            if (wetMix > 0f) applyWetIfNeeded(force = true)
        }
    }

    override fun onReset() {
        super.onReset()
        releaseNative()
        channelCount = 0
        wetMix = 0f
        inputShorts = ShortArray(0)
        outputShorts = ShortArray(0)
    }

    private companion object {
        private const val BYTES_PER_SAMPLE = 2
    }
}
