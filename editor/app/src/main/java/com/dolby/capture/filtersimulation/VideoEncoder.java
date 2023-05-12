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
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.util.Log;
import android.util.Size;

import androidx.annotation.NonNull;


import java.nio.ByteBuffer;

public class VideoEncoder extends Codec implements BroadcastClient{

    private final Muxer m;

    private int muxID;

    private BroadcastAction encoderComms;

    private AudioTransferCallback audioHandler;

    private OnFrameEncoded callback;

    private final String TAG = "VideoEncoder";

    public VideoEncoder(Context appContext, BroadcastAction cb, AudioTransferCallback audioHandler, Size resolution, String encoderFormat, int bitrate, int fps, int rotation, int iFrameInterval)  {
        super(false, appContext);

        Log.e("VideoEncoder", "resolution="+resolution + "  encoderFormat="+encoderFormat + "  bitrate="+bitrate + "  fps="+fps);

        this.audioHandler = audioHandler;
        encoderComms = cb;

        MediaCodecList list = new MediaCodecList(MediaCodecList.ALL_CODECS);

        MediaFormat format = new MediaFormat();

        switch (encoderFormat) {
            case Constants.HEVC:
                format.setString(MediaFormat.KEY_MIME, MediaFormat.MIMETYPE_VIDEO_HEVC);
                format.setInteger(MediaFormat.KEY_PROFILE, MediaCodecInfo.CodecProfileLevel.HEVCProfileMain);
                format.setInteger(MediaFormat.KEY_LEVEL, MediaCodecInfo.CodecProfileLevel.HEVCMainTierLevel51);
                format.setInteger(MediaFormat.KEY_COLOR_TRANSFER, MediaFormat.COLOR_TRANSFER_SDR_VIDEO);
                format.setInteger(MediaFormat.KEY_COLOR_STANDARD, MediaFormat.COLOR_STANDARD_BT709);
                format.setInteger(MediaFormat.KEY_COLOR_RANGE, MediaFormat.COLOR_RANGE_LIMITED);
                break;
            case Constants.AVC:
                format.setString(MediaFormat.KEY_MIME, MediaFormat.MIMETYPE_VIDEO_AVC);
                format.setInteger(MediaFormat.KEY_PROFILE, MediaCodecInfo.CodecProfileLevel.AVCProfileMain);
                format.setInteger(MediaFormat.KEY_LEVEL, MediaCodecInfo.CodecProfileLevel.AVCLevel51);
                format.setInteger(MediaFormat.KEY_COLOR_TRANSFER, MediaFormat.COLOR_TRANSFER_SDR_VIDEO);
                format.setInteger(MediaFormat.KEY_COLOR_STANDARD, MediaFormat.COLOR_STANDARD_BT709);
                format.setInteger(MediaFormat.KEY_COLOR_RANGE, MediaFormat.COLOR_RANGE_LIMITED);
                break;
            default:        // Dolby Vision 8.4
                format.setString(MediaFormat.KEY_MIME, MediaFormat.MIMETYPE_VIDEO_DOLBY_VISION);
                format.setInteger(MediaFormat.KEY_PROFILE, MediaCodecInfo.CodecProfileLevel.DolbyVisionProfileDvheSt);
                format.setInteger(MediaFormat.KEY_COLOR_TRANSFER, MediaFormat.COLOR_TRANSFER_HLG);
                format.setInteger(MediaFormat.KEY_COLOR_STANDARD, MediaFormat.COLOR_STANDARD_BT2020);
                format.setInteger(MediaFormat.KEY_COLOR_RANGE, MediaFormat.COLOR_RANGE_LIMITED);
                int level = getDolbyVisionLevel(fps, resolution);
                format.setInteger(MediaFormat.KEY_LEVEL, level);
                Log.e("VideoEncoder", "Dolby Vision P8.4 level = " + level);
                break;
        }


        format.setInteger(MediaFormat.KEY_WIDTH, resolution.getWidth());
        format.setInteger(MediaFormat.KEY_HEIGHT, resolution.getHeight());

        format.setInteger(MediaFormat.KEY_BIT_RATE, bitrate);

        format.setInteger(MediaFormat.KEY_FRAME_RATE, fps);

        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);


        format.setFloat(MediaFormat.KEY_I_FRAME_INTERVAL, iFrameInterval);

        format.setInteger(MediaFormat.KEY_PRIORITY, 0);

        String codecName = list.findEncoderForFormat(format);

        Log.e("Encoder", "VideoEncoder: " + codecName );

        this.createByCodecName(codecName);

        this.getCodec().setCallback(this, null);

        this.getCodec().configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);

        this.setInputSurface(this.getCodec().createInputSurface());

        Log.e("VideoEncoder", "VideoEncoder CodecSel: " + codecName);

        m = new Muxer(rotation);


    }

    private int getDolbyVisionLevel(int fps, Size videoSize) {
        int level = 0;
        if (videoSize.getHeight() == 3840 || videoSize.getWidth() == 3840) {
            if (fps <= 24) {
                level = MediaCodecInfo.CodecProfileLevel.DolbyVisionLevelUhd24;
            } else if (fps > 24 && fps <= 30) {
                level = MediaCodecInfo.CodecProfileLevel.DolbyVisionLevelUhd30;
            } else if (fps > 30 && fps <= 48) {
                level = MediaCodecInfo.CodecProfileLevel.DolbyVisionLevelUhd48;
            } else if (fps > 48 && fps <= 60) {
                level = MediaCodecInfo.CodecProfileLevel.DolbyVisionLevelUhd60;
            } else {
                level = MediaCodecInfo.CodecProfileLevel.DolbyVisionLevelUhd60;
            }
        } else if (videoSize.getHeight() == 1920 || videoSize.getWidth() == 1920) {
            if (fps <= 24) {
                level = MediaCodecInfo.CodecProfileLevel.DolbyVisionLevelFhd24;
            } else if (fps > 24 && fps <= 30) {
                level = MediaCodecInfo.CodecProfileLevel.DolbyVisionLevelFhd30;
            } else if (fps > 30 && fps <= 60) {
                level = MediaCodecInfo.CodecProfileLevel.DolbyVisionLevelFhd60;
            } else {
                level = MediaCodecInfo.CodecProfileLevel.DolbyVisionLevelFhd60;
            }
        }

        Log.d(TAG, "getDolbyVisionLevel videoSize = " + videoSize.getHeight() + " * " + videoSize.getWidth() + " level = " + level);
        return level;
    }


    public void setOnFrameCallback(OnFrameEncoded callback)
    {
        this.callback = callback;
    }

    @Override
    public void onInputBufferAvailable(@NonNull MediaCodec codec, int index) {

    }

    @Override
    public void onOutputBufferAvailable(@NonNull MediaCodec codec, int index, @NonNull MediaCodec.BufferInfo info) {

        Log.e("Encoder", "onOutputBufferAvailable: " + info.presentationTimeUs + " " + info.flags );


        ByteBuffer x = codec.getOutputBuffer(index);

        m.writeSampleData(muxID, x, info);
        codec.releaseOutputBuffer(index, false);

        if(!((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) == MediaCodec.BUFFER_FLAG_CODEC_CONFIG ))
        {
            callback.onFrameEncoded(info);
        }


    }



    @Override
    public void onError(@NonNull MediaCodec codec, @NonNull MediaCodec.CodecException e) {
        Log.e(TAG, "onError - " + e.getErrorCode());
        encoderComms.broadcast(new Message<MediaCodec.CodecException>("Codec error", e) {
        });
    }

    @Override
    public void onOutputFormatChanged(@NonNull MediaCodec codec, @NonNull MediaFormat format) {

        Log.e("VideoEncoder", "onOutputFormatChanged: TRANSFER " + format.getInteger(MediaFormat.KEY_COLOR_TRANSFER) );
        Log.e("VideoEncoder", "onOutputFormatChanged: STANDARD " + format.getInteger(MediaFormat.KEY_COLOR_STANDARD) );
        Log.e("VideoEncoder", "onOutputFormatChanged: RANGE " + format.getInteger(MediaFormat.KEY_COLOR_RANGE) );

        int audioTrack = audioHandler.addTrack(m);
        muxID = m.addTrack(format);
        m.setMuxerStarted();
        if(audioTrack >= 0) {
            audioHandler.copyAudio(m, audioTrack);
        }
    }

    @Override
    void stop() {
        super.stop();
    }

    @Override
    public void acceptMessage(Message<?> message) {

        if(message.getPayload() instanceof VideoDecoder && message.getTitle().equals("Done")) {

            Log.e("TAG", "acceptMessage: " + message.getTitle());

            MediaCodec decoder =  ((VideoDecoder) message.getPayload()).getCodec();
            decoder.flush();
            decoder.stop();
            decoder.release();


            Log.e("VideoEncoder", "onOutputBufferAvailable: Flushing Encoder" );
            this.getCodec().flush();
            Log.e("VideoEncoder", "onOutputBufferAvailable: Stopping Encoder" );
            this.getCodec().stop();
            Log.e("VideoEncoder", "onOutputBufferAvailable: Releasing Encoder" );
            this.getCodec().release();

            Log.e("VideoEncoder", "onOutputBufferAvailable: Stopping Muxer" );
            this.m.stopMuxer();


        }
    }
}
