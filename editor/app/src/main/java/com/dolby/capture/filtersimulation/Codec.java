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
import android.view.Surface;

import androidx.annotation.NonNull;

import com.dolby.vision.codecselection.BuilderCodecTemplate;

import java.io.IOException;
import java.util.concurrent.Semaphore;

public abstract class Codec extends BuilderCodecTemplate implements Runnable {

    private Surface inputSurface;

    private final Context appContext;

    private final boolean trim;

    public static final long TEN_SECONDS_US = 10000000;

    public static final long TWO_SECONDS_US = 2000000;

    public Codec(boolean trim, Context appContext) {
        this.trim = trim;
        this.appContext = appContext;
    }

    public void setInputSurface(Surface inputSurface) {
        if (this.inputSurface == null) {
            this.inputSurface = inputSurface;
        }
    }

    public void createByCodecName(String codecName) {
        try {
            this.createCodec(MediaCodec.createByCodecName(codecName));
        } catch (IOException e) {
            e.printStackTrace();
        }

    }

    public void run()
    {
        this.start();
    }

    private void start()
    {
        this.getCodec().start();
    }


    public Context getAppContext()
    {
        return this.appContext;
    }

    public void clipAdjustTS(MediaCodec.BufferInfo b)
    {
        b.presentationTimeUs -= TEN_SECONDS_US;

    }

    public boolean shouldTrim() {
        return this.trim;
    }

    void stop() {
        if(this.getCodec() != null) {
            try {
                this.getCodec().flush();
                this.getCodec().stop();
            }
            catch (IllegalStateException ignored)
            {
            }
            finally {
                this.getCodec().release();
            }
        }
    }

    private static final Semaphore audioDoneSemaphore = new Semaphore(0);

    public Semaphore getAudioDoneSemaphore()
    {
        return audioDoneSemaphore;
    }

    public Surface getInputSurface()
    {
        return this.inputSurface;
    }


    @Override
    public abstract void onInputBufferAvailable(@NonNull MediaCodec codec, int index);

    @Override
    public abstract void onOutputBufferAvailable(@NonNull MediaCodec codec, int index, @NonNull MediaCodec.BufferInfo info);

    @Override
    public abstract void onError(@NonNull MediaCodec codec, @NonNull MediaCodec.CodecException e);

    @Override
    public abstract void onOutputFormatChanged(@NonNull MediaCodec codec, @NonNull MediaFormat format);
}
