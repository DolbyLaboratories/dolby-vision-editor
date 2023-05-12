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

import android.hardware.HardwareBuffer;
import android.media.Image;
import android.media.ImageWriter;
import android.media.MediaCodec;
import android.util.Log;

import com.dolby.vision.codecselection.CodecBuilderImpl;

import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Semaphore;

public class FrameHandler implements VideoCallback, OnFrameEncoded{

    private ImagePipeline pipeline;

    private ImageWriter writer;

    private boolean preview;

    private CodecSynchro sync;

    private ConcurrentLinkedQueue<Long> buffers;

    private Semaphore bufferLock = new Semaphore(1);

    private Semaphore dowait = new Semaphore(10);

    private static int totalEncodeTime = 0;

    private static final String TAG = "FrameHandler";

    private enum FrameHandlerState {
        INITIALIZED,
        RELEASED
    }

    private FrameHandlerState handlerState;

    public FrameHandler(boolean preview, CodecSynchro sync, ImagePipeline pipeline)
    {
        this.preview = preview;
        this.sync = sync;
        this.pipeline = pipeline;

        this.writer = this.pipeline.getImageWriter();

        buffers = new ConcurrentLinkedQueue<Long>();

        handlerState = FrameHandlerState.INITIALIZED;
    }

    public void release()
    {
        handlerState = FrameHandlerState.RELEASED;
        if(this.sync != null) {
            this.sync.release();
        }

        if (bufferLock != null && bufferLock.getQueueLength() > 0) {
            int queueLen = bufferLock.getQueueLength();
            bufferLock.release(queueLen);
            bufferLock = null;
        }

        if (dowait != null && dowait.getQueueLength() > 0) {
            int queueLen = dowait.getQueueLength();
            dowait.release(queueLen);
            dowait = null;
        }
    }

    public FrameHandlerState getHandlerState() {
        return handlerState;
    }

    public native int processFrame(HardwareBuffer inbuf, HardwareBuffer opbuf);
    public native int EditShadersEnableLut(int enable);


    @Override
    public void onFrameAvailable(Image inputImage, String codecName, Constants.ColorStandard standard) {

        if(handlerState != FrameHandlerState.INITIALIZED) {
            // not in initialized state, do nothing
            return;
        }

        HardwareBuffer input = inputImage.getHardwareBuffer();

        Image outputImage = writer.dequeueInputImage();

        HardwareBuffer output = outputImage.getHardwareBuffer();

        Log.e(TAG, "codecName=" + codecName);
        if (CodecBuilderImpl.isDolbyGPUDecoder(codecName) && preview && standard == Constants.ColorStandard.eColorStandard10BitRec2020) {
            Log.e(TAG, "Enable LUT");
            EditShadersEnableLut(1);
        } else {
            Log.e(TAG, "Do not enable LUT");
            EditShadersEnableLut(0);
        }

        processFrame(input, output);

        outputImage.setDataSpace(pipeline.getOutputDataSpace());

        outputImage.setTimestamp(inputImage.getTimestamp());

        if(!preview) {
            try {
                bufferLock.acquire();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            Log.e(TAG, "onFrameAvailable: " + buffers );
            Log.e(TAG, "onFrameAvailable: QUEUED FRAME TS: " + outputImage.getTimestamp() );
            buffers.add(outputImage.getTimestamp());
            Log.e(TAG, "onFrameAvailable: QUEUED FRAME Size: " + buffers.size());

            bufferLock.release();

        }


        long start = System.currentTimeMillis();

        writer.queueInputImage(outputImage);

        if (!preview) {

            try {
                Log.e(TAG, "onFrameAvailable: WAITING ON ENCODER...");

                dowait.acquire();

            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            long end = System.currentTimeMillis();

            long encodeTime = (end -  start);

            totalEncodeTime += encodeTime;

            Log.e(TAG, "onFrameAvailable: Encode time " +  encodeTime + " " + totalEncodeTime);
        }

        inputImage.close();
        outputImage.close();
        output.close();
        input.close();


        Log.e(TAG, "onFrameAvailable: FINISHED" );
    }


    @Override
    public void onFrameEncoded(MediaCodec.BufferInfo info) {

        Log.e(TAG, "onFrameEncoded: " );

        try {
            bufferLock.acquire();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }


        Long ts = buffers.poll();
        if(ts != null) {

            if (ts == (info.presentationTimeUs * 1000)) {
                Log.e(TAG, "onFrameEncoded: Frame check okay " + info.presentationTimeUs);
                dowait.release();
            }
            else
            {
                throw new IllegalStateException("TS miss match!");
            }
        }
        else
        {
            Log.e(TAG, "onFrameEncoded: Queue empty, likely CSD buffer. Ignoring." );
        }

        bufferLock.release();

    }

}
