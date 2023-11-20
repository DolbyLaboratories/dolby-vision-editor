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
import android.graphics.PixelFormat;
import android.hardware.HardwareBuffer;
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

    private MediaExtractor mediaExtractor;

    private CodecBuilderImpl builder;

    private final VideoCallback callback;

    private boolean preview;

    private long fr;

    private BroadcastAction encoderComms;

    private boolean paused = false;

    private boolean shaderinit = false;

    private final ConcurrentLinkedQueue<DecoderInputInfo> decoderOutputBuffers = new ConcurrentLinkedQueue<>();

    private final ConcurrentLinkedQueue<DecoderInputInfo> decoderIntputBuffers = new ConcurrentLinkedQueue<>();

    private BufferProcessor bufferProcessor = null;

    private InputFeedThread inputFeedThread = null;

    private OpenGLContext context = new OpenGLContext();

    private final Semaphore regulator = new Semaphore(0);

    private final Semaphore down = new Semaphore(1);

    private final Semaphore up = new Semaphore(0);

    private Constants.ColorStandard standard;

    private final String codecName;

    private boolean outputFormatChange;

    private final String TAG = "VideoDecoder";

    public native int EditShadersInit(int output_width, int output_height, int color_standard, int input_colorspace, int output_colorspace);

    public native int EditShadersRelease();
    public native int EditShadersEnableLut(int enable);


    public VideoDecoder(Uri inputUri, Context appContext, VideoCallback callback, BroadcastAction encoderComms, ImagePipeline pipeline, boolean preview, int transfer, int profile, String encoderFormat, boolean outputFormatChange) {
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
            Log.d(TAG, "VideoDecoder codecSel: " + codecName);

            this.createByCodecName(codecName);

            this.getCodec().setCallback(this, main);

            this.getCodec().setOnFrameRenderedListener(this, main);

            this.setInputSurface(reader.getSurface());

//            this.getInputSurface().setFrameRate(33, Surface.FRAME_RATE_COMPATIBILITY_FIXED_SOURCE, Surface.CHANGE_FRAME_RATE_ALWAYS);

            this.fr = (long) ((1.0f / (float) builder.getFrameRate()) * 1000);

            Log.d(TAG, "VideoDecoder: FRAME RATE " + fr);


            try {
                mediaExtractor = builder.configure(this.getCodec(), this.getInputSurface(), this, transfer);
            } catch (CodecBuilderImpl.NoCodecException e) {
                e.printStackTrace();
            }
        } catch (IOException | CodecBuilderImpl.NoCodecException e) {
            e.printStackTrace();
        }

        bufferProcessor = new BufferProcessor(decoderOutputBuffers, fr, this.preview, regulator, down, up, mediaExtractor);
        bufferProcessor.setStarted();
        bufferProcessor.setEOSCallback(encoderComms);

        if (profile == MediaCodecInfo.CodecProfileLevel.DolbyVisionProfileDvheSt && encoderFormat.equals(Constants.DV_ME)) {
            Log.d("VideoDecoder", "edit in BT2020 color space");
            this.standard = Constants.ColorStandard.eColorStandard10BitRec2020;
        } else {
            Log.d("VideoDecoder", "edit in BT709 color space");
            this.standard = Constants.ColorStandard.eColorStandard10BitRec709;
        }
        Log.d("VideoDecoder", "standard.ordinal()=" + this.standard.ordinal());

        this.codecName = this.getCodec().getName();
    }

    public void editShaderRelease() {
        Log.d(TAG, "Release editor shader");
        if (shaderinit) {
            EditShadersRelease();
            shaderinit = false;
        }
    }

    @Override
    void stop() {
        Log.d(TAG, "stop");
        if (inputFeedThread != null) {
            inputFeedThread.setStop();
            inputFeedThread = null;
        }

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

        shaderinit = false;
        editShaderRelease();
    }

    @Override
    public void onInputBufferAvailable(@NonNull MediaCodec codec, int index) {

        DecoderInputInfo inputInfo = new DecoderInputInfo(index, -1, -1, -1, codec);
        decoderIntputBuffers.add(inputInfo);
    }

    public void setPaused() {
        Log.d(TAG, "setPaused");
        if (!paused) {
            paused = true;
            if(inputFeedThread != null && inputFeedThread.isStart()) {
                inputFeedThread.setStop();
                inputFeedThread = null;
            }
        }
    }

    public void setPlay() {
        Log.d(TAG, "setPlay");
        if(paused) {
            paused = false;
            if(inputFeedThread == null) {
                Log.d(TAG, "create new InputFeedThread");
                inputFeedThread = new InputFeedThread(mediaExtractor, decoderIntputBuffers);
                inputFeedThread.setStart();
            }
        }
    }

    @Override
    public void onStart() {
        if (inputFeedThread == null) {
            inputFeedThread = new InputFeedThread(mediaExtractor, decoderIntputBuffers);
            inputFeedThread.setStart();
        }

        super.onStart();
    }

    @Override
    public void onOutputBufferAvailable(@NonNull MediaCodec codec, int index, @NonNull MediaCodec.BufferInfo info) {

        decoderOutputBuffers.add(new DecoderInputInfo(index, info.size, info.presentationTimeUs, info.flags, codec));
    }


    @Override
    public void onError(@NonNull MediaCodec codec, @NonNull MediaCodec.CodecException e) {
        Log.e(TAG, "Codec error: " + e.toString());
        if (encoderComms != null) {
            encoderComms.broadcast(new Message<MediaCodec.CodecException>("Codec error", e) {
            });
        }
    }

    @Override
    public void onOutputFormatChanged(@NonNull MediaCodec codec, @NonNull MediaFormat format) {

    }

    @Override
    public void onImageAvailable(ImageReader reader) {

        Log.d(TAG, "onImageAvailable: ");

        Image in = reader.acquireLatestImage();

        if (in != null) {

            try {
                down.acquire();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            if (!shaderinit) {
                /**
                 * 0: YUV color space
                 * 1: RGB color space
                 */
                int inputColorSpace = isRGB(in) ? 1 : 0;
                Log.d(TAG, "onImageAvailable, input color space: " + inputColorSpace);
                EditShadersInit(in.getWidth(), in.getHeight(), this.standard.ordinal(), inputColorSpace, 0); // Set color configuration here

                if (CodecBuilderImpl.isDolbyGPUDecoder(codecName) && preview && standard == Constants.ColorStandard.eColorStandard10BitRec2020) {
                    Log.d(TAG, "Enable LUT");
                    EditShadersEnableLut(1);
                } else {
                    Log.d(TAG, "Do not enable LUT");
                    EditShadersEnableLut(0);
                }

                shaderinit = true;
                sendEditShaderInitDone();
            }

            if (shaderinit) {
                callback.onFrameAvailable(in, this.codecName, this.standard);
            }

            in.close();

            up.release();

        }
    }

    private boolean isRGB(Image image) {
        HardwareBuffer buffer = image.getHardwareBuffer();
        if(buffer.isClosed()) {
            return false;
        }

        int hardwareBufferFormat = buffer.getFormat();
        if(hardwareBufferFormat == PixelFormat.RGBA_1010102 || hardwareBufferFormat == PixelFormat.RGBA_8888) {
            return true;
        }

        return false;
    }

    private void sendEditShaderInitDone() {
        if (encoderComms != null) {
            encoderComms.broadcast(new Message<VideoDecoder>("EditShaderInitDone", this) {
            });
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

        private MediaExtractor mediaExtractor;

        private final String TAG = "VideoDecoder-BufferProcessor";

        public BufferProcessor(ConcurrentLinkedQueue<DecoderInputInfo> bufferList, long frDelay, boolean preview, Semaphore regulator, Semaphore down, Semaphore up, MediaExtractor ex)
        {
            this.bufferList = new WeakReference<>(bufferList);
            this.frDelay = frDelay;
            this.preview = preview;

            this.regulator = regulator;
            this.down = down;
            this.up = up;

            mediaExtractor = ex;

        }

        public void setEOSCallback(BroadcastAction cb)
        {
            callback = cb;
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
            Log.d(TAG, "renderBuffer: " + x.sampleTime );

            try {
                x.getCodec().releaseOutputBuffer(x.getIndex(), true);
            }
            catch (IllegalStateException ignore)
            {}
        }

        private void frameRateDelay(long renderTime)
        {

            if(preview) {

                long frameDelay  = frDelay - renderTime;

                Log.d(TAG, "frameRateDelay: " + frameDelay );

                try {
                    Thread.sleep(Math.max(0, frDelay - renderTime));
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

            }

        }

        private boolean EOS(DecoderInputInfo x)
        {
            if ((x.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) == MediaCodec.BUFFER_FLAG_END_OF_STREAM) {
                Log.d(TAG, "run: EOS Buffer Processor queue size " + bufferList.get().size() );

                if(preview)
                {
                    synchronized (mediaExtractor) {
                        mediaExtractor.seekTo(0, MediaExtractor.SEEK_TO_CLOSEST_SYNC);
                    }
                    x.getCodec().flush();
                    bufferList.get().clear();
                    x.getCodec().start();
                }
                else
                {
                    Message<Long> endFramePtsMsg = new Message<Long>("endFramePts", lastFramePts) {
                    };
                    this.setStopped();
                    /**
                     * EOS event should be sent from encoder when all frames encoded
                     */
                    this.callback.broadcast(endFramePtsMsg);
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
                    
                } else {
                    try {
                        // no output buffer available, wait for a while
                        Thread.sleep(1);
                    } catch (InterruptedException e) {
                        throw new RuntimeException(e);
                    }
                }
            }
        }
    }

    private class InputFeedThread extends Thread {

        private final WeakReference<ConcurrentLinkedQueue<DecoderInputInfo>> bufferList;

        private final String TAG = "VideoDecoder-InputFeedThread";
        private MediaExtractor mediaExtractor;

        private boolean isStart = false;

        public boolean isStart() {
            return isStart;
        }

        public void setStart() {
            if (!isStart) {
                isStart = true;
                start();
            }
        }

        public void setStop() {
            if (isStart) {
                isStart = false;
            }
        }

        InputFeedThread(MediaExtractor ex, ConcurrentLinkedQueue<DecoderInputInfo> bl) {
            mediaExtractor = ex;
            bufferList = new WeakReference<>(bl);
        }

        @Override
        public void run() {

            while (isStart) {

                if (!bufferList.get().isEmpty() && getCodecState() == STATE_STARTED) {

                    DecoderInputInfo inputInfo = bufferList.get().poll();
                    MediaCodec codec = inputInfo.codec;
                    int index = inputInfo.index;

                    ByteBuffer inputBuffer = null;
                    try {
                        inputBuffer = codec.getInputBuffer(index);
                    } catch (IllegalStateException ignore) {
                    }

                    if (inputBuffer == null) {
                        continue;
                    }

                    synchronized (mediaExtractor) {
                        int size = mediaExtractor.readSampleData(inputBuffer, 0);

                        try {
                            if (size <= 0 ||
                                    (mediaExtractor.getSampleFlags() & MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                                            == MediaCodec.BUFFER_FLAG_END_OF_STREAM) {
                                codec.queueInputBuffer(index, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                            } else {
                                codec.queueInputBuffer(index, 0, size, mediaExtractor.getSampleTime(), mediaExtractor.getSampleFlags());
                            }
                        } catch (IllegalStateException ignored) {
                        }

                        mediaExtractor.advance();
                    }
                } else {
                    try {
                        // no input buffer available, try to wait for a while
                        Thread.sleep(5);
                    } catch (InterruptedException e) {
                        throw new RuntimeException(e);
                    }
                }
            }
        }
    }

    public class DecoderInputInfo implements Comparator<DecoderInputInfo> {

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

