package com.maxrave.media3.audio

import androidx.media3.common.C
import androidx.media3.common.audio.AudioProcessor
import org.junit.Assume.assumeTrue
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.math.abs

class ReverbAudioProcessorTest {
    @Test
    fun zeroWetMixPassesPcm16ThroughUnchanged() {
        val processor = ReverbAudioProcessor()
        processor.configure(FORMAT)
        processor.flush()
        processor.wetMix = 0f

        val input = shortArrayOf(0, 1000, -1000, Short.MAX_VALUE, (Short.MIN_VALUE + 1).toShort(), 42)
        processor.queueInput(input.toByteBuffer())
        processor.queueEndOfStream()

        assertArrayEquals(input, processor.output().toShortArray())
    }

    @Test
    fun wetMixIsClampedToSupportedRange() {
        val processor = ReverbAudioProcessor()
        processor.wetMix = -1f
        assertEquals(0f, processor.wetMix, 0f)

        processor.wetMix = 2f
        assertEquals(1f, processor.wetMix, 0f)
    }

    @Test
    fun fullWetMixKeepsPcm16OutputBounded() {
        assumeTrue(System.getProperty("java.vm.vendor")?.contains("Android", ignoreCase = true) == true)
        val processor = ReverbAudioProcessor()
        processor.configure(FORMAT)
        processor.flush()
        processor.wetMix = 1f

        val input = ShortArray(8192) { index -> if (index == 0 || index == 1) Short.MAX_VALUE else 0 }
        processor.queueInput(input.toByteBuffer())
        processor.queueEndOfStream()

        val output = processor.output().toShortArray()
        assertEquals(input.size, output.size)
        assertTrue(output.any { it.toInt() != 0 })
        output.forEach { sample ->
            assertTrue(abs(sample.toInt()) <= Short.MAX_VALUE)
        }
    }

    private fun ReverbAudioProcessor.output(): ByteBuffer {
        var output = getOutput()
        if (!output.hasRemaining()) {
            output = getOutput()
        }
        return output
    }

    private fun ShortArray.toByteBuffer(): ByteBuffer {
        val buffer = ByteBuffer.allocate(size * Short.SIZE_BYTES).order(ByteOrder.nativeOrder())
        forEach(buffer::putShort)
        buffer.flip()
        return buffer
    }

    private fun ByteBuffer.toShortArray(): ShortArray {
        order(ByteOrder.nativeOrder())
        val copy = slice().order(ByteOrder.nativeOrder())
        return ShortArray(copy.remaining() / Short.SIZE_BYTES) { copy.short }
    }

    private companion object {
        val FORMAT = AudioProcessor.AudioFormat(44_100, 2, C.ENCODING_PCM_16BIT)
    }
}
