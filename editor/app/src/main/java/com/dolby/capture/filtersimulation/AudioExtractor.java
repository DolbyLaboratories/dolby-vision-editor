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
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.net.Uri;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;

public class AudioExtractor {

    private static final String requestedMime = MediaFormat.MIMETYPE_AUDIO_AAC;

    private MediaExtractor extractor = null;

    private MediaFormat format = null;

    private int NUM_AUD_CHANNELS = 2;

    public AudioExtractor(Uri inputUri, Context appContext, boolean trimOnly) throws MediaFormatNotFoundInFileException {
        extractor = new MediaExtractor();

        try {
            this.extractor.setDataSource(appContext, inputUri, null);
        } catch (IOException e) {
            e.printStackTrace();
        }

        String actualMime;

        //Get the decoder attached to the HEVC track in the file.
        for (int i = 0; i < extractor.getTrackCount(); i++) {
            MediaFormat format = extractor.getTrackFormat(i);
            actualMime = format.getString(MediaFormat.KEY_MIME);

            if (actualMime.startsWith(AudioExtractor.requestedMime)) {
                extractor.selectTrack(i);
                this.format = format;
                Log.e("DECODER", "Decoder: initialized on AAC track.");
                break;
            }
        }

        Log.e("AudioExtractor", "AudioExtractor: " + this.format);

        if (!trimOnly) {
            if(this.format == null) {
                // Allow transcode with no audio channel
                Log.e("AudioExtractor", "File did not contain AAC audio channel");
            }
        }

        /*
        The app is capable of handling 6-channel audio in a transcoding scenario. The phone cannot
        encode 6 channel audio but it can copy it. This check is mandatory for trimming.
         */

        if (trimOnly) {
            if(this.format == null) {
                throw new MediaFormatNotFoundInFileException("File did not contain specified audio codec AAC.");
            } else {
                int count = this.format.getInteger(MediaFormat.KEY_CHANNEL_COUNT);
                Log.e("AudioExtractor", "Trim only: number of audio channels = " + count);
                if (count > NUM_AUD_CHANNELS) {
                    throw new MediaFormatNotFoundInFileException("File contains more than 2 audio channels");
                }
            }
        }

    }

    public MediaFormat getAudioFormat()
    {
        return this.format;
    }

    public void rewind()
    {
        extractor.seekTo(0,MediaExtractor.SEEK_TO_CLOSEST_SYNC);
    }

    public MediaCodec.BufferInfo getChunk(ByteBuffer buffer)
    {
        int size = extractor.readSampleData(buffer, 0);

        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        info.presentationTimeUs = extractor.getSampleTime();
        info.size = size;
        info.flags = extractor.getSampleFlags();

        extractor.advance();

        return info;

    }


    public void copyAudio(Muxer copyTo, int trackID)
    {
        boolean result;

        do {

            ByteBuffer inputBuffer = ByteBuffer.allocate((int)extractor.getSampleSize());

            int size = extractor.readSampleData(inputBuffer, 0);

            if(size < 0)
                return;

            MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();

            info.presentationTimeUs = extractor.getSampleTime();
            info.size = size;
            info.flags = extractor.getSampleFlags();

            copyTo.writeSampleData(trackID, inputBuffer, info);

            result = extractor.advance();
        }
        while(result);
    }

}
