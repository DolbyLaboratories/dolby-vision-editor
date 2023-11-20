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

import android.graphics.SurfaceTexture;
import android.media.SyncParams;

import android.media.MediaSync;
import android.media.PlaybackParams;
import android.util.Log;
import android.view.Surface;

import androidx.annotation.NonNull;

import java.nio.ByteBuffer;

public class CodecSynchro extends MediaSync.Callback implements MediaSync.OnErrorListener{

    private final Surface in;

    private final Surface out;

    private final MediaSync synchro = new MediaSync();;

    public CodecSynchro(Surface output)
    {

        this.out = output;

        synchro.setSurface(this.out);

        synchro.setCallback(this,null);

        this.in = synchro.createInputSurface();

        SyncParams sync = new SyncParams().allowDefaults();

        Log.d("Synchro", "CodecSynchro: " + sync.getSyncSource() );

        synchro.setSyncParams(sync);

        PlaybackParams playbackParams = new PlaybackParams().allowDefaults();

        playbackParams.setSpeed(1.0f);

        synchro.setPlaybackParams(playbackParams);

    }

    public void release()
    {
        try {
            //This looks weird but the Codec Synchro cannot be without a surface at any time, so we give it a dud one whilst we carryout the change over.
            synchro.flush();
            SurfaceTexture temp = new SurfaceTexture(false);
            synchro.setSurface(new Surface(temp));
            this.synchro.release();
            temp.release();


        }catch (IllegalStateException ignored)
        {}

    }

    public Surface getInputSurface()
    {
        return this.in;
    }

    @Override
    public void onAudioBufferConsumed(@NonNull MediaSync sync, @NonNull ByteBuffer audioBuffer, int bufferId) {

    }

    @Override
    public void onError(@NonNull MediaSync sync, int what, int extra) {
        Log.e("SYNCHRO", "onError: " );
    }
}
