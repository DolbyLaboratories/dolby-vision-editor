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

import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.view.Surface;

import androidx.annotation.NonNull;

public class OpenGLContext {
    public static final int EGLINIT = 0;
    public static final int SDR = 0;
    public static final int TRANSFER_HLG = 2;
    public static final int TRANSFER_SDR = 0;
    private static HandlerThread imageCallback = new HandlerThread("imageCallback");
    private static Handler imageProcessHandler;
    private static boolean started = false;
    public native int eglInitWithSurface(Surface outputSurface, boolean isDPUSoution, boolean isPreviewMode, int transfer);

    public OpenGLContext() {
        if(!started) {
            imageCallback = new HandlerThread("imageCallback");
            imageCallback.start();
            imageProcessHandler = new Handler(imageCallback.getLooper()) {
                @Override
                public void handleMessage(@NonNull Message msg) {
                    switch (msg.what) {
                        case OpenGLContext.EGLINIT:
                            Surface outputSurface = (Surface) msg.obj;
                            Bundle eglInitData = msg.getData();
                            int transfer = eglInitData.getInt("transfer");
                            boolean isPreviewMode = eglInitData.getBoolean("isPreviewMode");
                            boolean isDPUSolution = eglInitData.getBoolean("isDPUSolution");
                            eglInitWithSurface(outputSurface, isDPUSolution, isPreviewMode, transfer);
                            break;
                    }
                }
            };
            started = true;
        }
    }

    public Handler getImageProcessHandler() {
        return imageProcessHandler;
    }
}
