/******************************************************************************
 * The Clear BSD License
 * Copyright (c) 2023 Dolby Laboratories
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *   - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *   - Neither the name of Dolby Laboratories nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

package com.dolby.capture.filtersimulation;

import android.content.Context;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.net.Uri;
import android.util.Log;
import android.util.Size;

import androidx.annotation.NonNull;

import java.beans.PropertyChangeEvent;
import java.io.File;
import java.nio.ByteBuffer;
import java.util.concurrent.Semaphore;

/**
 * Dummy encoder for use when trimming only.
 */
public class TrimEncoder extends EncoderOutput {
    private final static String TAG = "TrimEncoder";
    private Uri inputUri;
    private TrimDecoder decoder;
    private AudioDecoder audioDecoder;
    private AudioExtractor extractor;
    private boolean trim;
    private BroadcastAction UIComms;
    private long mVideoStartPosi = -1;
    private Semaphore audioDone;

    /**
     * Sets up the muxer, and muxes the audio. This should not need to be
     * called manually, it is set up by the TrimEncoder.
     *
     * @param inputUri filepath of video
     * @param decoder reference to the corresponding TrimDecoder
     * @param format MediaFormat for the video
     * @param rotation video rotation
     * @param trim whether the video is long enough to trim or not
     */
    public TrimEncoder(Uri inputUri, BroadcastAction UIComms, TrimDecoder decoder, MediaFormat format, int rotation, boolean trim, Context appContext) throws MediaFormatNotFoundInFileException {
        super(trim, appContext);
        this.inputUri = inputUri;
        this.decoder = decoder;
        this.trim = trim;
        this.UIComms = UIComms;

        //Dolby Vision on input
        //Since the source bitstream parameters (resolution, bitrate, frame rate) aren't changed,
        //the Dolby Vision (profile and level) signalization for the output mp4 container is taken from the source.

        // Try to create audio objects before starting the muxer
        // If there is an exception, don't let the muxer create an empty file
        try {
            if (trim) {
                this.audioDecoder = new AudioDecoder(this.inputUri, trim, this.getAppContext());
            } else {
                this.extractor = new AudioExtractor(this.inputUri, this.getAppContext(), true);
            }
        } catch (MediaFormatNotFoundInFileException e) {
            Log.e(TAG, e.toString());
            DecoderDoneMessage<MediaFormatNotFoundInFileException> m = new DecoderDoneMessage<>("Audio mux error", e);

            if(UIComms != null) {
                this.UIComms.broadcast(m);
            }
            throw e;
        }

        EncoderOutput.refreshMuxer(rotation);
        this.setTrackId(this.getMuxer().addTrack(format));
    }

    public void setVideoStartPosi(long posi) {
        mVideoStartPosi = posi;
    }

    public void setAudioDoneSemaphore(Semaphore sem) {
        audioDone = sem;
        if(audioDecoder != null) {
            audioDecoder.setAudioDoneSemaphore(audioDone);
        }
    }

    /**
     * Send a buffer of video data to the muxer
     * @param buffer encoded video data
     * @param info buffer information
     */
    public void sendBuffer(ByteBuffer buffer, MediaCodec.BufferInfo info) {
        if (trim) {
            this.clipAdjustTS(info);
        }
        this.getMuxer().writeSampleData(this.getTrackID(), buffer, info);
    }

    /**
     * Stop the muxer, and remux if the original file is Dolby Vision Profile 8.4
     */
    @Override
    public void stop() {
        if(getCodecState() == STATE_STOPING || getCodecState() == STATE_STOPED) {
            return;
        }

        super.stop();
        if(trim) {
            audioDecoder.stop();
        } else {
            extractor.stop();
        }
        this.getMuxer().stopMuxer();
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    public void muxAudio() {
        if (trim) {
            Thread audioThread = new Thread(audioDecoder);
            if (mVideoStartPosi != -1) {
                audioDecoder.setVideoStartPosi(mVideoStartPosi);
            }
            audioThread.start();
            Log.d(TAG, "Audio thread started");
        } else {
            int trackID = this.getMuxer().addTrack(extractor.getAudioFormat());
            this.getMuxer().setMuxerStarted();
            extractor.copyAudio(this.getMuxer(), trackID);
            Log.d(TAG, "Finished copying audio");
        }
    }

    @Override
    public void onInputBufferAvailable(@NonNull MediaCodec mediaCodec, int i) {

    }

    @Override
    public void onOutputBufferAvailable(@NonNull MediaCodec mediaCodec, int i, @NonNull MediaCodec.BufferInfo bufferInfo) {

    }

    @Override
    public void onError(@NonNull MediaCodec mediaCodec, @NonNull MediaCodec.CodecException e) {

    }

    @Override
    public void onOutputFormatChanged(@NonNull MediaCodec mediaCodec, @NonNull MediaFormat mediaFormat) {

    }

    @Override
    public void propertyChange(PropertyChangeEvent evt) {

    }
}
