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
import android.media.Image;
import android.media.ImageReader;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.Surface;

import androidx.annotation.NonNull;

import com.dolby.vision.codecselection.CodecBuilderImpl;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.Comparator;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Semaphore;

public class VideoDecoder extends DecoderOutput implements ImageReader.OnImageAvailableListener, MediaCodec.OnFrameRenderedListener {

    private ImagePipeline pipeline;

    private ImageReader reader;

    private MediaExtractor ex;

    private CodecBuilderImpl builder;

    private final VideoCallback callback;

    private boolean preview;

    private long fr;

    private BroadcastAction encoderComms;

    private boolean paused = false;

    private boolean shaderinit = false;

    private final ConcurrentLinkedQueue<DecoderInputInfo> buffers = new ConcurrentLinkedQueue<>();

    private BufferProcessor bufferProcessor = null;

    private OpenGLContext context = new OpenGLContext();

    private final Semaphore regulator = new Semaphore(0);

    private final Semaphore down = new Semaphore(1);

    private final Semaphore up = new Semaphore(0);

    private Constants.ColorStandard standard;

    private final String codecName;

    private final boolean outputFormatChange;

    private final String TAG = "VideoDecoder";

    public native int EditShadersInit(int output_width, int output_height, int color_standard, int input_colorspace, int output_colorspace);

    public native int EditShadersRelease();


    public VideoDecoder(Uri inputUri, Context appContext, VideoCallback callback, BroadcastAction encoderComms, ImagePipeline pipeline, boolean preview, int transfer, String encoderFormat, boolean outputFormatChange) {
        super(inputUri, false, appContext);
        this.callback = callback;

        this.encoderComms = encoderComms;

        this.preview = preview;

        this.outputFormatChange = outputFormatChange;

        this.pipeline = pipeline;

        this.reader = this.pipeline.getImageReader();

        HandlerThread decoderCallbacks = new HandlerThread("decoderCallbacks");
        decoderCallbacks.start();
        Handler main = new Handler(decoderCallbacks.getLooper());


        reader.setOnImageAvailableListener(this, context.getImage());

        try {

            builder = new CodecBuilderImpl(appContext, encoderFormat, inputUri);

            String codecName = builder.getCodecName();
            Log.e(TAG, "VideoDecoder codecSel: " + codecName);

            this.createByCodecName(codecName);

            this.getCodec().setCallback(this, main);

            this.getCodec().setOnFrameRenderedListener(this, main);

            this.setInputSurface(reader.getSurface());

            this.getInputSurface().setFrameRate(24, Surface.FRAME_RATE_COMPATIBILITY_FIXED_SOURCE, Surface.CHANGE_FRAME_RATE_ALWAYS);

            this.fr = (long) ((1.0f / (float) builder.getFrameRate()) * 1000);

            Log.d(TAG, "VideoDecoder: FRAME RATE " + fr);


            try {
                this.ex = builder.configure(this.getCodec(), this.getInputSurface(), this, transfer);
            } catch (CodecBuilderImpl.NoCodecException e) {
                e.printStackTrace();
            }
        } catch (IOException | CodecBuilderImpl.NoCodecException e) {
            e.printStackTrace();
        }

        this.bufferProcessor = new BufferProcessor(buffers, fr, this.preview, regulator, down, up, ex);
        bufferProcessor.setStarted();
        DecoderDoneMessage<VideoDecoder> m = new DecoderDoneMessage<>("Done", this);
        this.bufferProcessor.setEOS(encoderComms, m);

        if (this.getCodec().getInputFormat().getInteger(MediaFormat.KEY_PROFILE) == MediaCodecInfo.CodecProfileLevel.DolbyVisionProfileDvheSt && encoderFormat.equals(Constants.DV_ME)) {
            Log.e("VideoDecoder", "Detected 8.4");
            this.standard = Constants.ColorStandard.eColorStandard10BitRec2020;
        } else {
            Log.e("VideoDecoder", "Not 8.4");
            this.standard = Constants.ColorStandard.eColorStandard10BitRec709;
        }
        Log.e("VideoDecoder", "standard.ordinal()=" + this.standard.ordinal());

        this.codecName = this.getCodec().getName();
    }

    public void release() {
        this.EditShadersRelease();
    }

    @Override
    void stop() {
        Log.d(TAG, "stop");
        setPaused();
        super.stop();
    }

    @Override
    public void onStop() {
        Log.d(TAG, "codec stopped");

        if(bufferProcessor != null) {
            bufferProcessor.setStopped();
        }

        if(up != null && up.getQueueLength() != 0) {
            int queueLen = up.getQueueLength();
            up.release(queueLen);
        }
        if(down != null && down.getQueueLength() != 0) {
            int queueLen = down.getQueueLength();
            down.release(queueLen);
        }
    }

    @Override
    public void onInputBufferAvailable(@NonNull MediaCodec codec, int index) {
        if (getCodecState() > STATE_STARTED) {
            Log.d(TAG, "decoderState: " + getCodecState() + ", just return");
            return;
        }

        ByteBuffer inputBuffer = null;

        if (paused) {
            codec.queueInputBuffer(index, 0, 0, 0, 0);
            return;
        }

        try {
            inputBuffer = codec.getInputBuffer(index);
        } catch (IllegalStateException ignore) {
        }

        if(inputBuffer == null) {
            return;
        }

        int size = ex.readSampleData(inputBuffer, 0);

        try {
            if ((ex.getSampleFlags() & MediaCodec.BUFFER_FLAG_END_OF_STREAM) == MediaCodec.BUFFER_FLAG_END_OF_STREAM) {
                codec.queueInputBuffer(index, 0, 0, 0, ex.getSampleFlags());
            } else {
                codec.queueInputBuffer(index, 0, size, ex.getSampleTime(), ex.getSampleFlags());
            }
        }
        catch (IllegalStateException ignored) {}

        if (!paused) {
            ex.advance();
        }

    }

    public void setPaused() {
        this.paused = true;
    }

    public void setPlay() {
        this.paused = false;
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onOutputBufferAvailable(@NonNull MediaCodec codec, int index, @NonNull MediaCodec.BufferInfo info) {

        buffers.add(new DecoderInputInfo(index, info.size, info.presentationTimeUs, info.flags, codec));
    }


    @Override
    public void onError(@NonNull MediaCodec codec, @NonNull MediaCodec.CodecException e) {
        Log.e(TAG, "Codec error: " + e.toString());
        encoderComms.broadcast(new Message<MediaCodec.CodecException>("Codec error", e) {
        });
    }

    @Override
    public void onOutputFormatChanged(@NonNull MediaCodec codec, @NonNull MediaFormat format) {

    }

    @Override
    public void onImageAvailable(ImageReader reader) {

        Log.e(TAG, "onImageAvailable: ");

        Image in = reader.acquireLatestImage();

        if (in != null) {

            try {
                down.acquire();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            if (preview && !shaderinit && !outputFormatChange) {
                EditShadersInit(in.getWidth(), in.getHeight(), this.standard.ordinal(), 0, 0); // Set color configuration here
                shaderinit = true;
            }

            callback.onFrameAvailable(in, this.codecName, this.standard);

            in.close();

            up.release();

        }
    }

    @Override
    public void onFrameRendered(@NonNull MediaCodec codec, long presentationTimeUs, long nanoTime) {

    }

    public static class BufferProcessor extends Thread {
        private final WeakReference<ConcurrentLinkedQueue<DecoderInputInfo>> bufferList;

        private Semaphore regulator;
        private Semaphore down;
        private Semaphore up;

        private boolean isStarted = false;

        private long lastFramePts = -1;

        private final long frDelay;

        private final boolean preview;

        private BroadcastAction callback;

        private Message toSend;

        private MediaExtractor ex;

        private final String TAG = "VideoDecoder-BufferProcessor";

        public BufferProcessor(ConcurrentLinkedQueue<DecoderInputInfo> bufferList, long frDelay, boolean preview, Semaphore regulator, Semaphore down, Semaphore up, MediaExtractor ex)
        {
            this.bufferList = new WeakReference<>(bufferList);
            this.frDelay = frDelay;
            this.preview = preview;

            this.regulator = regulator;
            this.down = down;
            this.up = up;

            this.ex = ex;

        }

        public void setEOS(BroadcastAction callback, Message<?> m)
        {
            this.callback = callback;
            this.toSend = m;
        }

        public void setStarted()
        {
            Log.d(TAG, "setStarted");
            if(!isStarted)
            {
                this.isStarted = true;
                this.start();
            }
        }

        public void setStopped()
        {
            Log.d(TAG, "setStopped");
            this.isStarted = false;
        }

        private void renderBuffer(DecoderInputInfo x)
        {
            Log.e(TAG, "renderBuffer: " + x.sampleTime );

            try {
                x.getCodec().releaseOutputBuffer(x.getIndex(), true);
            }
            catch (IllegalStateException ignore)
            {}
        }

        private void frameRateDelay(long renderTime)
        {

            if(preview) {

                long frameDelay  = frDelay-renderTime;

                Log.e(TAG, "frameRateDelay: " + frameDelay );

                try {
                    Thread.sleep(Math.max(0, frDelay-renderTime));
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

            }

        }

        private boolean EOS(DecoderInputInfo x)
        {
            if (((x.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) == MediaCodec.BUFFER_FLAG_END_OF_STREAM)) {
                Log.d(TAG, "run: EOS Buffer Processor queue size " + bufferList.get().size() );

                if(preview)
                {
                    this.ex.seekTo(0, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                    x.codec.flush();
                    bufferList.get().clear();
                    x.getCodec().start();
                }
                else
                {
                    Message<Long> endFramePtsMsg = new Message<Long>("endFramePts", lastFramePts) {
                    };
                    this.setStopped();
                    this.callback.broadcast(endFramePtsMsg);
                    this.callback.broadcast(this.toSend);
                }

                return true;
            }

            return false;
        }

        private void confirmBuffer()
        {
            try {
                up.acquire();

            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            down.release();
        }

        @Override
        public void run()
        {
            while(this.isStarted)
            {

                DecoderInputInfo x = bufferList.get().poll();

                if(x!=null) {
                    
                    long start = System.currentTimeMillis();

                    lastFramePts = lastFramePts > x.sampleTime ? lastFramePts : x.sampleTime;
                    renderBuffer(x);

                    if(!EOS(x)) {
                        confirmBuffer();
                    }

                    long end = System.currentTimeMillis();

                    long renderTime = (end-start);

                    frameRateDelay(renderTime);
                    
                }
            }
        }
    }

    public static class DecoderInputInfo implements Comparator<DecoderInputInfo> {

        private int index;
        private int size;
        private long sampleTime;
        private int flags;
        private MediaCodec codec;

        public DecoderInputInfo(int index, int size, long sampleTime, int flags, MediaCodec codec) {
            this.index = index;
            this.size = size;
            this.sampleTime = sampleTime;
            this.flags = flags;
            this.codec = codec;
        }

        public DecoderInputInfo() {}

        public int getSize() {
            return size;
        }

        public int getIndex() {
            return index;
        }

        public int getFlags() {
            return flags;
        }

        public long getSampleTime() {
            return sampleTime;
        }

        public MediaCodec getCodec() {
            return codec;
        }


        @Override
        public String toString()
        {
            return Long.toString(this.getSampleTime());
        }

        @Override
        public int compare(DecoderInputInfo o1, DecoderInputInfo o2) {
            return Long.compare(o1.getSampleTime(), o2.getSampleTime());

        }
    }
}

