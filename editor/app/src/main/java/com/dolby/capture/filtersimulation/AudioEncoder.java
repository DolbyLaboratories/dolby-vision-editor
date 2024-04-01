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
import android.media.AudioFormat;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.NonNull;

import java.beans.PropertyChangeEvent;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.Semaphore;

public class AudioEncoder extends EncoderOutput implements Runnable{

    private static final String mime = MediaFormat.MIMETYPE_AUDIO_AAC;

    public static final int highQualityAACBitRate = 320000;

    private boolean firstSample = true;

    private long videoLen;

    private long PrevpresentationTimeUs = 0;

    private int NUM_SAMPLES_FADEOUT = 3;

    private boolean EOS = false;

    private WeakReference<ArrayBlockingQueue> bufferQueue = null;
    private WeakReference<Semaphore> audioUp = null;
    private WeakReference<Semaphore> audioDown = null;

    private final Object lock = new Object();

    private static final String TAG = "AudioEncoder";

    public AudioEncoder(MediaFormat format, long videoLen, boolean trim, Context appContext) {
        super(trim, appContext);

        this.createByType(mime);

        Log.d("AudioEncoder", "FORMAT: " + format.toString());

        MediaFormat mediaFormat = MediaFormat.createAudioFormat(mime, format.getInteger(MediaFormat.KEY_SAMPLE_RATE), AudioFormat.CHANNEL_OUT_STEREO);

        mediaFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);

        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, highQualityAACBitRate);

        mediaFormat.setInteger(MediaFormat.KEY_CHANNEL_COUNT, format.getInteger(MediaFormat.KEY_CHANNEL_COUNT));

        this.getCodec().configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);

        this.videoLen = videoLen;

        Log.d(TAG, "AudioEncoder: Configured");
    }



    @Override
    public void onInputBufferAvailable(@NonNull MediaCodec mediaCodec, int i) {
        if(getCodecState() == STATE_STOPING || getCodecState() == STATE_STOPED) {
            return;
        }

        synchronized (lock) {

            if (this.bufferQueue == null) {
                try {
                    lock.wait();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

            if (!EOS) {

                ByteBuffer b = mediaCodec.getInputBuffer(i);

                AudioDecoder.AudioData data = null;

                if(this.getMuxer().isRunning()) {

                    if (audioUp.get().tryAcquire()) {
                        synchronized (this.bufferQueue.get()) {

                            if (!this.bufferQueue.get().isEmpty()) {
                                data = (AudioDecoder.AudioData) this.bufferQueue.get().poll();
                            }

                        }
                        audioDown.get().release();

                    }
                }

                // need to check codec state again
                if(getCodecState() == STATE_STOPING || getCodecState() == STATE_STOPED) {
                    return;
                }

                if (data != null) {
                    b.put(data.getData());
                    b.flip();

                    if(firstSample) {
                        this.applyFadeCurve(b);
                        this.firstSample = false;
                    }
                    // VideoLen - is the total length of the file before trimming
                    // presentationTimeUs - Number of samples processed
                    // Check the number of samples left is less than -> (10s (trim period at the end of video) + each sample takes (PTS - PrevPTS) * number of samples to fade
                    else if(this.videoLen -  data.getInfo().presentationTimeUs < (TEN_SECONDS_US + ((data.getInfo().presentationTimeUs - PrevpresentationTimeUs) * NUM_SAMPLES_FADEOUT)))
                    {
                        Log.d(TAG, "applyFadeCurveEnd: ");
                        this.applyFadeCurveEnd(b);
                    }

                    mediaCodec.queueInputBuffer(i, 0, data.getInfo().size, data.getInfo().presentationTimeUs, data.getInfo().flags);
                    PrevpresentationTimeUs = data.getInfo().presentationTimeUs;
                } else {
                    mediaCodec.queueInputBuffer(i, 0, 0, 0, 0);
                }
            }

        }
    }

    public void applyFadeCurve(ByteBuffer b) {
        int len = b.remaining() / 4;

        Log.d(TAG, "applyFadeCurve: " + len);

        b.order(ByteOrder.LITTLE_ENDIAN);

        for (int sampleCount = 0; sampleCount < len; sampleCount++) {

            short left = b.getShort(4*sampleCount);

            short right = b.getShort(4*sampleCount+2);

            int fade = (sampleCount*((Short.MAX_VALUE+1)/len));

            left = (short) ( (left*fade) >> 15);
            right = (short) ( (right*fade) >> 15);

            b.putShort(4*sampleCount, left);

            b.putShort(4*sampleCount+2, right);


        }
    }

    public void applyFadeCurveEnd(ByteBuffer b) {
        int len = b.remaining() / 4;

        b.order(ByteOrder.LITTLE_ENDIAN);

        for (int sampleCount = 0; sampleCount < len; sampleCount++) {

            short left = b.getShort(4*sampleCount);

            short right = b.getShort(4*sampleCount+2);

            int fade = ((len-sampleCount)*((Short.MAX_VALUE+1)/len));
            left = (short) ( (left*fade) >> 15);
            right = (short) ( (right*fade) >> 15);

            b.putShort(4*sampleCount, left);

            b.putShort(4*sampleCount+2, right);


        }
    }

    @Override
    public void onOutputBufferAvailable(@NonNull MediaCodec mediaCodec, int i, @NonNull MediaCodec.BufferInfo bufferInfo) {
        if(getCodecState() == STATE_STOPING || getCodecState() == STATE_STOPED) {
            return;
        }

        synchronized (lock) {

            if (!EOS) {

                ByteBuffer encodedData = mediaCodec.getOutputBuffer(i);

                if(this.shouldTrim())
                {
                    this.clipAdjustTS(bufferInfo);
                }


                if(bufferInfo.presentationTimeUs > 0) {

                    Log.d(TAG, "onOutputBufferAvailable, pts: " + bufferInfo.presentationTimeUs );
                    this.getMuxer().writeSampleData(this.getTrackID(), encodedData, bufferInfo);
                }

                if(getCodecState() == STATE_STOPING || getCodecState() == STATE_STOPED) {
                    return;
                }
                mediaCodec.releaseOutputBuffer(i, false);
            }

        }
    }

    @Override
    public void onError(@NonNull MediaCodec mediaCodec, @NonNull MediaCodec.CodecException e) {

    }

    @Override
    public void onOutputFormatChanged(@NonNull MediaCodec mediaCodec, @NonNull MediaFormat mediaFormat) {
        Log.d(TAG, "onOutputFormatChanged");

        this.setTrackId(this.getMuxer().addTrack(mediaFormat));
        this.getMuxer().setMuxerStarted();

    }

    @Override
    void stop() {
        if(getCodecState() == STATE_STOPING || getCodecState() == STATE_STOPED) {
            return;
        }

        super.stop();
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    public void run() {
        Looper.prepare();
        new EncoderHandler(Looper.myLooper(), this);
        Looper.loop();
    }

    @Override
    public void propertyChange(PropertyChangeEvent propertyChangeEvent) {
        Log.d(TAG, "propertyChange");
        synchronized (lock) {

            if (propertyChangeEvent.getPropertyName().equals("AudioUp")) {

                if(this.audioUp == null) {
                    this.audioUp = new WeakReference<Semaphore>((Semaphore) propertyChangeEvent.getNewValue());
                }
            }

            if (propertyChangeEvent.getPropertyName().equals("AudioDown")) {

                if(this.audioDown == null) {
                    this.audioDown = new WeakReference<Semaphore>((Semaphore) propertyChangeEvent.getNewValue());
                }
            }

            if (propertyChangeEvent.getPropertyName().equals("AudioData")) {
                if (this.bufferQueue == null) {


                    this.bufferQueue = new WeakReference<ArrayBlockingQueue>((ArrayBlockingQueue) propertyChangeEvent.getNewValue());

                    lock.notify();
                }
            }
        }

        if (propertyChangeEvent.getPropertyName().equals("EOS")) {

            Log.d(TAG, "propertyChange: EOS FIRED" );

            while (bufferQueue.get().size() != 0) {
                try {
                    Log.d("Encoder", "propertyChange: " + bufferQueue.get().size());
                    Thread.sleep(1);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            EOS = true;


            synchronized (lock) {
                stop();
            }

            this.getAudioDoneSemaphore().release();

            Log.d(TAG, "propertyChange: Audio finished, Video signaled.");

        }
    }

    private static class EncoderHandler extends Handler {

        private WeakReference<AudioEncoder> encoder;

        public EncoderHandler(Looper looper, AudioEncoder audioEncoder) {
            super(looper);
            this.encoder = new WeakReference<AudioEncoder>(audioEncoder);
            this.encoder.get().getCodec().setCallback(this.encoder.get(), this);
            this.encoder.get().start();
        }
    }
}
