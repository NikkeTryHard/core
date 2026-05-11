package com.maxrave.media3.audio

import androidx.media3.common.PlaybackParameters
import androidx.media3.common.audio.AudioProcessor
import androidx.media3.common.util.UnstableApi
import androidx.media3.exoplayer.audio.DefaultAudioSink
import androidx.media3.exoplayer.audio.SilenceSkippingAudioProcessor

@UnstableApi
class HighQualityAudioProcessorChain(
    private val userProcessors: Array<AudioProcessor>,
) : DefaultAudioSink.AudioProcessorChain {
    private val silenceSkippingAudioProcessor =
        SilenceSkippingAudioProcessor(
            2_000_000,
            (20_000 / 2_000_000).toFloat(),
            2_000_000,
            0,
            256,
        )
    private val soundTouchAudioProcessor = SoundTouchAudioProcessor()
    private val audioProcessors = userProcessors + silenceSkippingAudioProcessor + soundTouchAudioProcessor

    override fun getAudioProcessors(): Array<AudioProcessor> = audioProcessors

    override fun applyPlaybackParameters(playbackParameters: PlaybackParameters): PlaybackParameters {
        soundTouchAudioProcessor.tempo = playbackParameters.speed
        soundTouchAudioProcessor.pitch = playbackParameters.pitch
        return PlaybackParameters.DEFAULT
    }

    override fun applySkipSilenceEnabled(skipSilenceEnabled: Boolean): Boolean {
        silenceSkippingAudioProcessor.setEnabled(skipSilenceEnabled)
        return skipSilenceEnabled
    }

    override fun getMediaDuration(durationUs: Long): Long = durationUs

    override fun getSkippedOutputFrameCount(): Long = silenceSkippingAudioProcessor.skippedFrames
}
