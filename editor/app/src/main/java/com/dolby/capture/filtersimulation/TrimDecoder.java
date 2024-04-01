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

import android.annotation.SuppressLint;
import android.content.Context;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.MediaMetadataRetriever;
import android.net.Uri;
import android.util.Log;
import android.util.Size;

import androidx.annotation.NonNull;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;

/**
 * Dummy decoder for use when trimming only
 */
public class TrimDecoder extends DecoderOutput {
    private final static String TAG = "TrimDecoder";

    private MediaExtractor extractor;

    private MediaFormat format;

    private TrimEncoder encoder;

    private final Size dimensions;

    private BroadcastAction UIComms;

    private final boolean trim;

    private BroadcastServer server;

    /**
     * Sets up extractor to begin "decoding," as well as
     * creating the corresponding "encoder"
     *
     * @param inputUri file path for the video to trim
     */
    public TrimDecoder(MainActivity activity, Uri inputUri, BroadcastAction UIComms, Context appContext) throws MediaFormatNotFoundInFileException, IOException {
        super(inputUri, true, appContext);

        // get all necessary video metadata
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        retriever.setDataSource(appContext, inputUri);
        int width = Integer.parseInt(retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_WIDTH));
        int height = Integer.parseInt(retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_HEIGHT));
        this.dimensions = new Size(width, height);
        int rotation = Integer.parseInt(retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_ROTATION));
        retriever.release();

        this.UIComms = UIComms;

        // determine if the video is long enough to trim
        this.trim = this.getVideoLength() >= TEN_SECONDS_US * 3;
        try {
            if (!trim) {
                Log.w(TAG, "Video is less than 30 seconds long, not trimming");
            }
        } catch (IllegalStateException e) {
            Log.e(TAG, "TrimDecoder: STATE EXCEPTION");
            e.printStackTrace();
        }

        // setup extractor for retrieving the encoded video data
        extractor = new MediaExtractor();
        try {
            extractor.setDataSource(this.getAppContext(), this.getInputUri(), null);
        } catch (IOException e) {
            e.printStackTrace();
        }
        if (!findTrack(extractor)) {
            // should never get here, because the decoder will get called for preview first
            throw new MediaFormatNotFoundInFileException("Could not find video track");
        }

        server = new BroadcastServer(activity);
        // initialize encoder to setup the muxer and audio
        this.encoder = new TrimEncoder(inputUri, server, this, this.format, rotation, this.trim, this.getAppContext());
    }

    // find and select the video track
    private boolean findTrack(MediaExtractor extractor) {
        MediaCodecList codecList = new MediaCodecList(MediaCodecList.ALL_CODECS);
        MediaCodecInfo[] codecInfos = codecList.getCodecInfos();
        for (int i = 0; i < extractor.getTrackCount(); i++) {
            MediaFormat format = extractor.getTrackFormat(i);
            String mime = format.getString(MediaFormat.KEY_MIME);

            if (Arrays.stream(codecInfos).
                    filter(MediaCodecInfo::isEncoder).
                    filter(codec ->
                            Arrays.stream(codec.getSupportedTypes()).
                                    anyMatch(type -> type.equals(mime))).
                    filter(codec -> codec.getCapabilitiesForType(mime).colorFormats.length > 0).
                    anyMatch(codec ->
                            Arrays.stream(codec.getCapabilitiesForType(mime).colorFormats).
                                    filter(x -> x == MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface).count() > 0)) {
                extractor.selectTrack(i);
                Log.d(TAG, "Track \"" + mime + "\" selected");
                this.format = format;
                return true;
            }
        }
        return false;
    }

    /**
     * @return video dimensions
     */
    public Size getDimensions() {
        return dimensions;
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
        if (encoder != null) {
            encoder.stop();
            encoder = null;
        }
    }

    /**
     * Continuously send encoded video data straight to the muxer.
     * Skip frames that are in the first or last 10 seconds of the video,
     * unless the video is too short.
     */
    @SuppressLint("WrongConstant")
    @Override
    public void run() {
        super.run();

        long start = System.currentTimeMillis();

        if (trim) {
            this.extractor.seekTo(TEN_SECONDS_US, MediaExtractor.SEEK_TO_NEXT_SYNC);
            long videoStartPosi = extractor.getSampleTime();
            encoder.setVideoStartPosi(videoStartPosi);
            encoder.muxAudio();

            // Wait for the audio to finish processing
            // This only needs to be done in trim cases, because when not trimming,
            // the audio gets copied over, which is done on the same thread
            try {
                this.getAudioDoneSemaphore().acquire();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            Log.d(TAG, "Audio ready");
        } else {
            // no need to trim, simply mux audio synchronously
            encoder.muxAudio();
        }

        boolean extract = getCodecState() == STATE_STARTED;

        while (extract) {
            ByteBuffer buffer = ByteBuffer.allocate((int) extractor.getSampleSize());

            // get the next encoded frame
            int size = extractor.readSampleData(buffer, 0);

            if (!trim || this.getVideoLength() - extractor.getSampleTime() > TEN_SECONDS_US) {
                MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
                info.presentationTimeUs = extractor.getSampleTime();
                info.size = size;
                info.flags = extractor.getSampleFlags();
                // send the buffer to the dummy encoder
                if (encoder != null) {
                    encoder.sendBuffer(buffer, info);
                }
                if(extractor != null) {
                    extract = extractor.advance();
                }
                extract = extract && getCodecState() == STATE_STARTED;
            } else {
                Log.w(TAG, "Reached last 10 seconds of data, exiting");
                break;
            }
        }
        encoder.stop();
        encoder = null;
        extractor.release();
        extractor = null;

        long end = System.currentTimeMillis();
        Log.d(TAG, "Edit pass done in " + (end - start)/1000.0 + " seconds.");

        if (getCodecState() == STATE_STOPING || getCodecState() == STATE_STOPED) {
            // stop by user, no need to broadcast the message.
            return;
        }

        DecoderDoneMessage<TrimDecoder> m = new DecoderDoneMessage<>("Done",this);
        if(UIComms != null) {
            this.UIComms.broadcast(m);
        }
    }

    @Override
    public void onInputBufferAvailable(@NonNull MediaCodec mediaCodec, int i) {

    }

    @Override
    public void onOutputBufferAvailable(@NonNull MediaCodec mediaCodec, int i, @NonNull MediaCodec.BufferInfo bufferInfo) {

    }

    @Override
    public void onError(@NonNull MediaCodec mediaCodec, @NonNull MediaCodec.CodecException e) {

    }

    @Override
    public void onOutputFormatChanged(@NonNull MediaCodec mediaCodec, @NonNull MediaFormat mediaFormat) {

    }
}