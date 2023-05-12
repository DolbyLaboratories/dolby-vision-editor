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
import android.util.Log;
import android.util.Size;

import androidx.annotation.NonNull;

import java.beans.PropertyChangeEvent;
import java.io.File;
import java.nio.ByteBuffer;

/**
 * Dummy encoder for use when trimming only.
 */
public class TrimEncoder extends EncoderOutput {
    private final static String TAG = "TrimEncoder";

    private Uri inputUri;

    private TrimDecoder decoder;

    private AudioDecoder audioDecoder;

    private AudioExtractor extractor;

    private boolean isDolby;

    private String dolbyType;

    private int fps;

    private boolean trim;

    private BroadcastAction UIComms;

    /**
     * Sets up the muxer, and muxes the audio. This should not need to be
     * called manually, it is set up by the TrimEncoder.
     *
     * @param inputUri filepath of video
     * @param decoder reference to the corresponding TrimDecoder
     * @param format MediaFormat for the video
     * @param rotation video rotation
     * @param trim whether the video is long enough to trim or not
     */
    public TrimEncoder(Uri inputUri, BroadcastAction UIComms, TrimDecoder decoder, MediaFormat format, int rotation, boolean trim, Context appContext) throws MediaFormatNotFoundInFileException {
        super(trim, appContext);
        this.inputUri = inputUri;
        this.decoder = decoder;
        this.fps = format.getInteger(MediaFormat.KEY_FRAME_RATE);
        this.trim = trim;
        this.UIComms = UIComms;

        String mime = format.getString(MediaFormat.KEY_MIME);
        int profile = format.getInteger(MediaFormat.KEY_PROFILE);
        this.isDolby = mime.equals(MediaFormat.MIMETYPE_VIDEO_DOLBY_VISION);
        if (this.isDolby) {
            format.setString(MediaFormat.KEY_MIME, MediaFormat.MIMETYPE_VIDEO_HEVC);
            this.dolbyType = Constants.DV_ME;
        }

        // Try to create audio objects before starting the muxer
        // If there is an exception, don't let the muxer create an empty file
        try {
            if (trim) {
                this.audioDecoder = new AudioDecoder(this.inputUri, trim, this.getAppContext());
            } else {
                this.extractor = new AudioExtractor(this.inputUri, this.getAppContext(), true);
            }
        } catch (MediaFormatNotFoundInFileException e) {
            Log.e(TAG, e.toString());
            DecoderDoneMessage<MediaFormatNotFoundInFileException> m = new DecoderDoneMessage<>("Audio mux error", e);

            if(UIComms != null) {
                this.UIComms.broadcast(m);
            }

            throw e;
        }

        EncoderOutput.refreshMuxer(rotation);
        this.setTrackId(this.getMuxer().addTrack(format));
        muxAudio();
    }

    /**
     * Send a buffer of video data to the muxer
     * @param buffer encoded video data
     * @param info buffer information
     */
    public void sendBuffer(ByteBuffer buffer, MediaCodec.BufferInfo info) {
        if (trim) {
            this.clipAdjustTS(info);
        }
        this.getMuxer().writeSampleData(this.getTrackID(), buffer, info);
        Log.e(TAG, "Sent buffer with timestamp " + info.presentationTimeUs);
    }

    /**
     * Stop the muxer, and remux if the original file is Dolby Vision Profile 8.4
     */
    public void stop() {
        this.getMuxer().stopMuxer();
        if (this.isDolby && this.dolbyType.equals(Constants.DV_ME)) {
            String inputPath = this.getMuxer().getOutputPath();
            String outputPath = "/sdcard" + "/DCIM/" + System.currentTimeMillis() + "_REMUXED.mp4";
            int dvProfile = 8;
            int dvLevel = fr2lv(this.fps, this.decoder.getDimensions());
            int dvBackCompatibility = 4;
            remuxFile(inputPath, outputPath, dvProfile, dvLevel, dvBackCompatibility);
            deleteMuxedFile(inputPath);
        }
        Log.e(TAG, "Muxer stopped");
    }

    private void muxAudio() {
        if (trim) {
            Thread audioThread = new Thread(audioDecoder);
            audioThread.start();
            Log.e(TAG, "Audio thread started");
        } else {
            int trackID = this.getMuxer().addTrack(extractor.getAudioFormat());
            this.getMuxer().setMuxerStarted();
            extractor.copyAudio(this.getMuxer(), trackID);
            Log.e(TAG, "Finished copying audio");
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

    @Override
    public void propertyChange(PropertyChangeEvent evt) {

    }


    /**
     * JNI call to mp4remuxer.
     * Remuxes the output file to insert the MP4 metadata box for Dolby Vision.
     *
     * @param inputPath path to input video
     * @param outputPath path to output video
     * @param dvProfile Dolby Vision profile (raw value, not as specified in MediaCodecInfo.CodecProfileLevel)
     * @param dvLevel Dolby Vision level (raw value before bit shift, not as specified in MediaCodecInfo.CodecProfileLevel)
     * @param dvBackCompatibility Dolby Vision back compatibility ID
     */
    private native int remuxFile(String inputPath, String outputPath, int dvProfile, int dvLevel, int dvBackCompatibility);


    /**
     * Calculates Dolby Vision level given video frame rate and dimensions.
     * Returns the raw level value before bit shift, not as specified in MediaCodecInfo.CodecProfileLevel.
     *
     * @param fr frame rate
     * @param dim dimensions
     */
    private int fr2lv(int fr, Size dim) {
        if (fr <= 30) {
            if (dim.getWidth() == Constants.RESOLUTION_2K_WIDTH && dim.getHeight() == Constants.RESOLUTION_2K_HEIGHT) {
                Log.e(TAG, "fr2lv: 4");
                return 4;
            } else if (dim.getWidth() == Constants.RESOLUTION_4K_WIDTH && dim.getHeight() == Constants.RESOLUTION_4K_HEIGHT) {
                Log.e(TAG, "fr2lv: 7");
                return 7;
            } else {
                Log.e(TAG, "fr2lv: Unrecognized dimension and frame rate defaulting to 7");
                return 7;
            }
        } else if (fr <= 60) {
            if (dim.getWidth() == Constants.RESOLUTION_2K_WIDTH && dim.getHeight() == Constants.RESOLUTION_2K_HEIGHT) {
                Log.e(TAG, "fr2lv: 5");
                return 5;
            } else if (dim.getWidth() == Constants.RESOLUTION_4K_WIDTH && dim.getHeight() == Constants.RESOLUTION_4K_HEIGHT) {
                Log.e(TAG, "fr2lv: 9");
                return 9;
            } else {
                Log.e(TAG, "fr2lv: Unrecognized dimension and frame rate defaulting to 9");
                return 9;
            }
        } else {
            Log.e(TAG, "fr2lv: Unrecognized dimension and frame rate defaulting to 9");
            return 9;
        }
    }


    /**
     * Deletes the muxed MP4 file.
     *
     * @param filePath path to muxed file
     */
    private void deleteMuxedFile(String filePath) {
        File inFile = new File(filePath);
        if (inFile.exists()) {
            if (inFile.delete()) {
                Log.d(TAG, "muxed file deleted: " + filePath);
            } else {
                Log.d(TAG, "muxed file not deleted: " + filePath);
            }
        }
    }

}
