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

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;


/**
 * The muxer for the mp4 files, is aware of both the video and audio streams.
 */
public class Muxer {

    //The android muxer instance.
    private MediaMuxer mMuxer;

    private boolean running = false;

    private String outputPath;

    /**
     * Muxer constructor.
     */
    public Muxer(int rotation) {

        try {

            Log.e("MUXER", "Muxer: initializing" );
            this.outputPath = "/sdcard" + "/DCIM/" + System.currentTimeMillis() + ".mp4";
            this.mMuxer = new MediaMuxer(outputPath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
            this.mMuxer.setOrientationHint(rotation);
            Log.e("MUXER", "Muxer: initialized" );
        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    public synchronized int addTrack(MediaFormat f)
    {
       return this.mMuxer.addTrack(f);
    }



    public synchronized void setMuxerStarted()
    {
        if(!running) {

            this.mMuxer.start();

            Log.e("MUXER", "startMuxer: Lifting access gate...");

            running = true;

            Log.e("MUXER", "startMuxer: Lifted access gate...");
        }
    }

    public synchronized void writeSampleData(int track, ByteBuffer data, MediaCodec.BufferInfo info)
    {
        this.mMuxer.writeSampleData(track, data, info);
    }

    public boolean isRunning()
    {
        return running;
    }

    public synchronized void stopMuxer()
    {
        if(running) {
            Log.e("MUXER", "stopMuxer: Attempting to replace access gate...");

            running = false;

            Log.e("MUXER", "stopMuxer: Replaced access gate...");

            this.mMuxer.stop();

            this.mMuxer.release();

            this.mMuxer = null;
        }
    }

    public String getOutputPath() {
        return outputPath;
    }
}
