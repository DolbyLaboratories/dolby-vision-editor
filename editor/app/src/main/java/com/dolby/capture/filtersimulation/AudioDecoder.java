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
import android.media.MediaFormat;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.NonNull;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.Optional;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.Semaphore;

public class AudioDecoder extends DecoderOutput implements Runnable {
    private final static String TAG = "AudioDecoder";

    private AudioExtractor ex;

    private Optional<MediaFormat> format;

    private AudioEncoder encoder;

    private static final int QUEUE_SIZE = 10;

    private ArrayBlockingQueue<AudioData> bufferQueue = new ArrayBlockingQueue<AudioData>(QUEUE_SIZE);

    private Semaphore audioUp = new Semaphore(0);
    private Semaphore audioDown = new Semaphore(QUEUE_SIZE);


    public AudioDecoder(Uri inputUri, boolean shouldTrim, Context appContext) throws MediaFormatNotFoundInFileException {
        super(inputUri, shouldTrim, appContext);

        format = Optional.empty();

        this.ex = new AudioExtractor(inputUri, appContext, shouldTrim);

        this.setCodec(ex.getAudioFormat().getString(MediaFormat.KEY_MIME));

        Log.e(TAG, "AudioDecoder: " + ex.getAudioFormat());

        Log.e(TAG, "AudioDecoder: CHANNELS " + ex.getAudioFormat().getInteger(MediaFormat.KEY_CHANNEL_COUNT));
        this.getCodec().configure(ex.getAudioFormat(), null, null, 0);

    }

    @Override
    public void onInputBufferAvailable(@NonNull MediaCodec mediaCodec, int i) {
        MediaCodec.BufferInfo info = this.ex.getChunk(this.getCodec().getInputBuffer(i));
        Log.e(TAG, "onInputBufferAvailable: started");

        if(info.size >= 0) {

            Log.e(TAG, "onInputBufferAvailable: Valid Buffer " + info.presentationTimeUs);
            this.getCodec().queueInputBuffer(i, 0, info.size, info.presentationTimeUs, info.flags);
        }
        else
        {
            Log.e(TAG, "onInputBufferAvailable: Empty Buffer");
            this.getCodec().queueInputBuffer(i, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
        }

        synchronized (format)
        {
            if(!format.isPresent() && info.presentationTimeUs > 0)
            {
                this.ex.rewind();
            }
        }

    }

    @Override
    public void onOutputBufferAvailable(MediaCodec decoder, int index, MediaCodec.BufferInfo info) {

        Log.e(TAG, "onOutputBufferAvailable: started");
        synchronized (format)
        {
            if(!format.isPresent())
            {

                decoder.releaseOutputBuffer(index, false);
                return;
            }
        }

        ByteBuffer data = decoder.getOutputBuffer(index);

        AudioData audioPacket = new AudioData(data,info);

        if(includePacket(audioPacket.info)) {

            try {
                audioDown.acquire();

            } catch (InterruptedException e) {
                e.printStackTrace();
            }


            synchronized (bufferQueue) {
                try {

                    bufferQueue.add(audioPacket);

                } catch (IllegalStateException e) {
                    Log.e(TAG, "onOutputBufferAvailable: STATE EXCEPTION");
                    e.printStackTrace();
                }
            }

            audioUp.release();

            while(bufferQueue.size() > 0)
            {
                try {
                    Thread.sleep(1);
                }
                catch (InterruptedException e)
                {
                    e.printStackTrace();
                }
            }
        }


        this.getNotifier().firePropertyChange("AudioUp", null, this.audioUp);
        this.getNotifier().firePropertyChange("AudioDown", null, this.audioDown);
        this.getNotifier().firePropertyChange("AudioData", null, this.bufferQueue);

        decoder.releaseOutputBuffer(index, false);

        if( (MediaCodec.BUFFER_FLAG_END_OF_STREAM & audioPacket.getInfo().flags) == MediaCodec.BUFFER_FLAG_END_OF_STREAM)
        {
            Log.e(TAG, "onOutputBufferAvailable: EOS");
            this.getNotifier().firePropertyChange("EOS", null, null);
            stop();
        }
    }

    @Override
    public void onError(@NonNull MediaCodec mediaCodec, @NonNull MediaCodec.CodecException e) {

    }

    @Override
    public void onOutputFormatChanged(@NonNull MediaCodec mediaCodec, @NonNull MediaFormat mediaFormat) {

        Log.e(TAG, "onOutputFormatChanged: " + mediaFormat);

        synchronized (format) {
            this.format = Optional.of(mediaFormat);
            encoder = new AudioEncoder(this.format.get(), this.getVideoLength(), this.shouldTrim(), this.getAppContext());
            this.getNotifier().addPropertyChangeListener(encoder);
            new Thread(encoder).start();
        }
    }

    public void run() {
        Looper.prepare();
        new DecoderHandler(Looper.myLooper(), this);
        Looper.loop();
    }

    private static class DecoderHandler extends Handler {

        private WeakReference<AudioDecoder> decoder;

        public DecoderHandler(Looper looper, AudioDecoder audioDecoder) {
            super(looper);
            this.decoder = new WeakReference<AudioDecoder>(audioDecoder);
            this.decoder.get().getCodec().setCallback(this.decoder.get(), this);
            this.decoder.get().getCodec().start();
        }
    }

    public static class AudioData
    {
        private ByteBuffer data;
        private MediaCodec.BufferInfo info;

        public AudioData(ByteBuffer data, MediaCodec.BufferInfo info)
        {
            this.data = data;
            this.info = info;
        }

        public ByteBuffer getData() {
            return data;
        }

        public MediaCodec.BufferInfo getInfo() {
            return info;
        }
    }
}




