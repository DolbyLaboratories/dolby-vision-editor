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
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Size;

import androidx.annotation.NonNull;


import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.Semaphore;

public class VideoEncoder extends Codec implements BroadcastClient{

    private final Muxer m;

    private int muxID;

    private BroadcastAction encoderComms;

    private AudioTransferCallback audioHandler;

    private OnFrameEncoded callback;

    private long endFramePts = -1;
    private Semaphore encodingDone;

    private final String TAG = "VideoEncoder";

    public VideoEncoder(Context appContext, BroadcastAction cb, AudioTransferCallback audioHandler, Size resolution, String encoderFormat, int bitrate, int fps, int rotation, int iFrameInterval)  {
        super(false, appContext);

        Log.d(TAG, "resolution="+resolution + "  encoderFormat="+encoderFormat + "  bitrate="+bitrate + "  fps="+fps);

        this.audioHandler = audioHandler;
        encoderComms = cb;

        MediaCodecList list = new MediaCodecList(MediaCodecList.ALL_CODECS);
        HandlerThread decoderCallbacks = new HandlerThread("decoderCallbacks");
        decoderCallbacks.start();
        Handler main = new Handler(decoderCallbacks.getLooper());

        MediaFormat format = new MediaFormat();

        switch (encoderFormat) {
            case Constants.HEVC:
                format.setString(MediaFormat.KEY_MIME, MediaFormat.MIMETYPE_VIDEO_HEVC);
                format.setInteger(MediaFormat.KEY_PROFILE, MediaCodecInfo.CodecProfileLevel.HEVCProfileMain10);
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
                Log.d("VideoEncoder", "Dolby Vision P8.4 level = " + level);
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
        if (codecName != null && codecName != "") {
            createByCodecName(codecName);
        } else {
            Log.w(TAG, "Could not find the codec for the specified output video file format.");
            switch (encoderFormat) {
                case Constants.DV_ME:
                    createByCodecName(Constants.DOLBY_HEVC_ENCODER);
                    break;
                case Constants.HEVC:
                    createByType(MediaFormat.MIMETYPE_VIDEO_HEVC);
                    break;
                case Constants.AVC:
                    createByType(MediaFormat.MIMETYPE_VIDEO_AVC);
                    break;
                default:
                    Log.e(TAG, "Unknown encoder format");
            }
        }

        this.getCodec().setCallback(this, main);

        this.getCodec().configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);

        this.setInputSurface(this.getCodec().createInputSurface());

        Log.d(TAG, "VideoEncoder CodecSel: " + getCodec().getName() );

        m = new Muxer(rotation);
    }

    @Override
    public void createByType(String mime) {
        try {
            this.createCodec(MediaCodec.createEncoderByType(mime));
        } catch (IOException e) {
            e.printStackTrace();
        }
        super.createByType(mime);
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

        Log.d(TAG, "onOutputBufferAvailable: " + info.presentationTimeUs + " " + info.flags );


        ByteBuffer x = codec.getOutputBuffer(index);

        m.writeSampleData(muxID, x, info);
        codec.releaseOutputBuffer(index, false);

        if(!((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) == MediaCodec.BUFFER_FLAG_CODEC_CONFIG ))
        {
            callback.onFrameEncoded(info);
        }

        if (endFramePts != -1 && endFramePts <= info.presentationTimeUs ) {
            Log.d(TAG, "get EOS");
            if (encodingDone != null && encodingDone.availablePermits() == 0) {
                encodingDone.release();
            }
            sendEncodeDone();
        }
    }


    private void sendEncodeDone() {
        DecoderDoneMessage<VideoEncoder> encodeDoneMsg = new DecoderDoneMessage<>("Done", this);
        encoderComms.broadcast(encodeDoneMsg);
    }

    @Override
    public void onError(@NonNull MediaCodec codec, @NonNull MediaCodec.CodecException e) {
        Log.d(TAG, "onError - " + e.getErrorCode());
        if (encodingDone != null && encodingDone.availablePermits() == 0) {
            encodingDone.release();
        }
        encoderComms.broadcast(new Message<MediaCodec.CodecException>("Codec error", e) {
        });
    }

    @Override
    public void onOutputFormatChanged(@NonNull MediaCodec codec, @NonNull MediaFormat format) {

        Log.d(TAG, "onOutputFormatChanged: TRANSFER " + format.getInteger(MediaFormat.KEY_COLOR_TRANSFER) );
        Log.d(TAG, "onOutputFormatChanged: STANDARD " + format.getInteger(MediaFormat.KEY_COLOR_STANDARD) );
        Log.d(TAG, "onOutputFormatChanged: RANGE " + format.getInteger(MediaFormat.KEY_COLOR_RANGE) );

        int audioTrack = audioHandler.addTrack(m);
        muxID = m.addTrack(format);
        m.setMuxerStarted();
        if(audioTrack >= 0) {
            audioHandler.copyAudio(m, audioTrack);
        }
    }

    @Override
    public void onStart() {
        Log.d(TAG, "codec started");
//        if (encodingDone == null) {
//            encodingDone = new Semaphore(0);
//        }
    }

    @Override
    public void onStop() {
        Log.d(TAG, "codec stopped");
        this.m.stopMuxer();
    }

    @Override
    void stop() {
        Log.d(TAG, "stop");

        if (encodingDone != null) {
            try {
                // wait for encoding to finish
                Log.d(TAG, "waiting for encoding to finish");
                encodingDone.acquire();
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }finally {
                encodingDone = null;
            }
        }

        super.stop();
    }

    @Override
    public void acceptMessage(Message<?> message) {

        if(message.getPayload() instanceof VideoDecoder && message.getTitle().equals("Done")) {

//            Log.d(TAG, "acceptMessage: " + message.getTitle());
//
//            MediaCodec decoder =  ((VideoDecoder) message.getPayload()).getCodec();
//            decoder.flush();
//            decoder.stop();
//            decoder.release();
//
//            Log.d(TAG, "onOutputBufferAvailable: Flushing Encoder" );
//            this.getCodec().flush();
//            Log.d(TAG, "onOutputBufferAvailable: Stopping Encoder" );
//            this.getCodec().stop();
//            Log.d(TAG, "onOutputBufferAvailable: Releasing Encoder" );
//            this.getCodec().release();
//
//            Log.d("VideoEncoder", "onOutputBufferAvailable: Stopping Muxer" );
//            this.m.stopMuxer();
        } else if (message.getPayload() instanceof Long && message.getTitle().equals("endFramePts")) {
            Log.d(TAG, "acceptMessage: " + message.getTitle());

            endFramePts = ((Long) message.getPayload()).longValue();
        }
    }
}
