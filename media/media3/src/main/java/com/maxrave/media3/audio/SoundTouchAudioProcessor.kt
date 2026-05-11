package com.maxrave.media3.audio

import androidx.media3.common.C
import androidx.media3.common.audio.AudioProcessor
import androidx.media3.common.audio.BaseAudioProcessor
import androidx.media3.common.util.UnstableApi
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.math.abs

@UnstableApi
class SoundTouchAudioProcessor : BaseAudioProcessor() {
    var tempo: Float = 1f
        set(value) {
            val coerced = value.coerceIn(MIN_FACTOR, MAX_FACTOR)
            if (field != coerced) {
                field = coerced
                paramsDirty = true
            }
        }

    var pitch: Float = 1f
        set(value) {
            val coerced = value.coerceIn(MIN_FACTOR, MAX_FACTOR)
            if (field != coerced) {
                field = coerced
                paramsDirty = true
            }
        }

    private val bridge = NativeSoundTouchBridge()
    private var handle = 0L
    private var channelCount = 0
    private var sampleRate = 0
    private var paramsDirty = true
    private var inputShorts = ShortArray(0)
    private var outputShorts = ShortArray(0)
    private var inputEnded = false
    private var nativeFlushed = false

    override fun onConfigure(inputAudioFormat: AudioProcessor.AudioFormat): AudioProcessor.AudioFormat {
        if (inputAudioFormat.encoding != C.ENCODING_PCM_16BIT || inputAudioFormat.channelCount !in 1..2) {
            return AudioProcessor.AudioFormat.NOT_SET
        }
        recreate(inputAudioFormat.sampleRate, inputAudioFormat.channelCount)
        return inputAudioFormat
    }

    override fun isActive(): Boolean = abs(tempo - 1f) > ACTIVE_EPSILON || abs(pitch - 1f) > ACTIVE_EPSILON

    override fun queueInput(inputBuffer: ByteBuffer) {
        val remaining = inputBuffer.remaining()
        if (remaining == 0) return
        if (!isActive()) {
            val output = replaceOutputBuffer(remaining)
            copyBuffer(inputBuffer, output, remaining)
            output.flip()
            return
        }

        ensureNative()
        if (handle == 0L) return
        applyParamsIfNeeded()
        inputBuffer.order(ByteOrder.nativeOrder())
        val frameCount = remaining / (channelCount * BYTES_PER_SAMPLE)
        val sampleCount = frameCount * channelCount
        val byteCount = sampleCount * BYTES_PER_SAMPLE
        if (inputBuffer.isDirect) {
            bridge.nativePutSamplesDirect(handle, inputBuffer, inputBuffer.position(), frameCount)
            inputBuffer.position(inputBuffer.position() + byteCount)
        } else {
            ensureInputCapacity(sampleCount)
            for (i in 0 until sampleCount) {
                inputShorts[i] = inputBuffer.short
            }
            bridge.nativePutSamples(handle, inputShorts, 0, frameCount)
        }
        drainNativeOutput(frameCount + EXTRA_OUTPUT_FRAMES)
    }

    override fun onQueueEndOfStream() {
        inputEnded = true
        if (isActive()) {
            ensureNative()
            if (handle != 0L && !nativeFlushed) {
                bridge.nativeFlush(handle)
                nativeFlushed = true
                drainNativeOutput(END_OF_STREAM_OUTPUT_FRAMES)
            }
        }
    }

    override fun onFlush() {
        inputEnded = false
        nativeFlushed = false
        if (handle != 0L) {
            bridge.nativeClear(handle)
            applyParamsIfNeeded(force = true)
        }
    }

    override fun onReset() {
        releaseNative()
        sampleRate = 0
        inputEnded = false
        nativeFlushed = false
        inputShorts = ShortArray(0)
        outputShorts = ShortArray(0)
    }

    override fun isEnded(): Boolean = inputEnded && !hasPendingOutput()

    private fun recreate(
        newSampleRate: Int,
        channels: Int,
    ) {
        releaseNative()
        sampleRate = newSampleRate
        channelCount = channels
        paramsDirty = true
    }

    private fun releaseNative() {
        if (handle != 0L) {
            bridge.nativeRelease(handle)
            handle = 0L
        }
    }

    private fun applyParamsIfNeeded(force: Boolean = false) {
        if ((paramsDirty || force) && handle != 0L) {
            bridge.nativeSetTempoPitch(handle, tempo, pitch)
            paramsDirty = false
        }
    }

    private fun ensureNative() {
        if (handle == 0L && sampleRate > 0 && channelCount > 0) {
            handle = bridge.nativeCreate(sampleRate, channelCount)
            paramsDirty = true
            applyParamsIfNeeded(force = true)
        }
    }

    private fun drainNativeOutput(maxFrames: Int) {
        val availableFrames = bridge.nativeAvailableSamples(handle)
        val framesToRead = when {
            availableFrames > 0 -> availableFrames
            inputEnded -> maxFrames
            else -> maxFrames
        }
        val byteCount = framesToRead * channelCount * BYTES_PER_SAMPLE
        val output = replaceOutputBuffer(byteCount)
        output.order(ByteOrder.nativeOrder())
        val receivedFrames = if (output.isDirect) {
            bridge.nativeReceiveSamplesDirect(handle, output, output.position(), framesToRead)
        } else {
            ensureOutputCapacity(framesToRead * channelCount)
            val received = bridge.nativeReceiveSamples(handle, outputShorts, framesToRead)
            val sampleCount = received * channelCount
            for (i in 0 until sampleCount) {
                output.putShort(outputShorts[i])
            }
            received
        }
        if (receivedFrames <= 0) {
            output.limit(0)
            return
        }
        if (output.isDirect) {
            output.position(output.position() + receivedFrames * channelCount * BYTES_PER_SAMPLE)
        }
        output.flip()
    }

    private fun ensureInputCapacity(sampleCount: Int) {
        if (inputShorts.size < sampleCount) {
            inputShorts = ShortArray(sampleCount)
        }
    }

    private fun ensureOutputCapacity(sampleCount: Int) {
        if (outputShorts.size < sampleCount) {
            outputShorts = ShortArray(sampleCount)
        }
    }

    private fun copyBuffer(src: ByteBuffer, dst: ByteBuffer, size: Int) {
        val pos = src.position()
        for (i in 0 until size) {
            dst.put(src.get(pos + i))
        }
        src.position(pos + size)
    }

    private companion object {
        private const val BYTES_PER_SAMPLE = 2
        private const val MIN_FACTOR = 0.05f
        private const val MAX_FACTOR = 4f
        private const val ACTIVE_EPSILON = 0.0001f
        private const val EXTRA_OUTPUT_FRAMES = 8192
        private const val END_OF_STREAM_OUTPUT_FRAMES = 32768
    }
}
