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
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.media.MediaMetadataRetriever;
import android.net.Uri;
import android.util.Log;
import android.util.Size;
import android.view.Surface;

import java.io.IOException;

public class ContentLoader {

    private static final String TAG = ContentLoader.class.getSimpleName();
    public static final int DEFAULT_FPS = 30;
    public static final int VARIABLE_FPS_THRESHOLD = 2;

    private VideoEncoder encoder;

    private VideoDecoder decoder;

    private FrameHandler frameHandler;

    private ImagePipeline pipeline;

    private MainActivity activity;

    private Uri inputUri;

    private BroadcastServer server;


    static {
        System.loadLibrary("filtersimulation");
    }



    public native int EditShadersSetParameter(int parameter, float val);
    public native int EditShadersSetCompositingImage(byte[] data);

    public ContentLoader(MainActivity activity) {
        Log.v(TAG, "CTOR");
        this.activity = activity;
    }


    public synchronized void load_trim(Uri inputUri, Surface screen, Context appContext, Size screenDim) throws MediaFormatNotFoundInFileException, IOException {

        this.inputUri = inputUri;

        decoder.stop();
        server = new BroadcastServer(this.activity);

        try {
            TrimDecoder d = new TrimDecoder(this.activity, this.inputUri, server, appContext);
            Thread t = new Thread(d);
            t.start();
        } catch (MediaFormatNotFoundInFileException e) {
            return;
        }
    }


    public synchronized void load_preview(Uri inputUri, Surface screen, Context appContext, Size screenDim, String encoderFormat, int transfer, boolean outputFormatChange) throws MediaFormatNotFoundInFileException, IOException {

        stop();

        this.inputUri = inputUri;


        int profile = getProfile(appContext, inputUri);
        Log.e(TAG, "profile = " + profile);

        Size inputSize = getResolution(appContext, inputUri);

        CodecSynchro sync = new CodecSynchro(screen);
        this.pipeline = new ImagePipeline(inputSize, screenDim, sync.getInputSurface(), true, profile, encoderFormat);

        this.frameHandler = new FrameHandler(true, sync, pipeline);

        try {
            decoder = new VideoDecoder(inputUri, appContext, this.frameHandler, server, pipeline, true, transfer, encoderFormat, outputFormatChange);
        } catch (NullPointerException e) {
            activity.acceptMessage(new Message<NullPointerException>("Codec error", e) {
            });
            return;
        }
        new Thread(decoder).start();
    }


    public synchronized void load_transcode(MainActivity activity, Uri inputUri, Context appContext, Size screenDim, String resolution, String encoderFormat, int transfer, int iFrameInterval) throws MediaFormatNotFoundInFileException, IOException {

        stop();

        this.inputUri = inputUri;
        this.activity = activity;

        AudioExtractor aux = null;

        try {
            aux = new AudioExtractor(this.inputUri, appContext, false);
        } catch (MediaFormatNotFoundInFileException e) {
            activity.acceptMessage(new Message<MediaFormatNotFoundInFileException>("Unsupported Format", e) {
            });
            return;
        }
        TranscodeAudioTransfer audioHandler = new TranscodeAudioTransfer(aux);

        Size inputSize = getResolution(appContext, inputUri);

        Size outputSize;
        switch (resolution) {
            case Constants.RESOLUTION_2K:
                outputSize = new Size(Constants.RESOLUTION_2K_WIDTH, Constants.RESOLUTION_2K_HEIGHT);
                break;
            case Constants.RESOLUTION_4K:
                outputSize = new Size(Constants.RESOLUTION_4K_WIDTH, Constants.RESOLUTION_4K_HEIGHT);
                break;
            default:
                outputSize = getResolution(appContext, inputUri);
                break;
        }
        Log.e(TAG, "outputSize = " + outputSize.toString());

        int bitrate = getBitrate(appContext, inputUri);
        Log.e(TAG, "bitrate = " + bitrate);

        int inputFPS = getFPS(appContext, inputUri);
        Log.e(TAG, "inputFPS = " + inputFPS);

        int profile = getProfile(appContext, inputUri);
        Log.e(TAG, "profile = " + profile);

        int rotation = getRotation(appContext, inputUri);
        Log.e(TAG, "rotation = " + rotation);

        BroadcastServer callback = new BroadcastServer(this.activity);

        try {
            encoder = new VideoEncoder(appContext, callback, audioHandler, outputSize, encoderFormat, bitrate, inputFPS, rotation, iFrameInterval);
        } catch (NullPointerException e) {
            activity.acceptMessage(new Message<NullPointerException>("Codec error", e) {
            });
            return;
        }

        server = new BroadcastServer(encoder, this.activity);

        this.pipeline = new ImagePipeline(inputSize, outputSize, encoder.getInputSurface(), false, profile, encoderFormat);

        this.frameHandler = new FrameHandler(false, null, pipeline);    // sync is only used during preview

        encoder.setOnFrameCallback(frameHandler);

        if (encoder != null) {
            new Thread(encoder).start();
        }


        try {
            decoder = new VideoDecoder(inputUri, appContext, this.frameHandler, server, pipeline, false, transfer, encoderFormat, false);
        } catch (NullPointerException e) {
            activity.acceptMessage(new Message<NullPointerException>("Codec error", e) {
            });
            return;
        }

       new Thread(decoder).start();
    }


    public void pause()
    {
        this.decoder.setPaused();
    }

    public void play()
    {
        this.decoder.setPlay();
    }

    public void stop()
    {
        if(decoder != null) {
            decoder.setPaused();
            decoder.stop();
            decoder = null;
        }

        if(encoder != null) {
            encoder.stop();
            encoder = null;
        }

        // stop decoder first, then release frameHandler
        if(frameHandler != null) {
            frameHandler.release();
            frameHandler = null;
        }

        if (pipeline != null) {
            Log.d(TAG, "close pipline");
            pipeline.closePipline();
            pipeline = null;
        }
    }


    // Static methods used to query information about a given video file

    /*
     Helper method to create MediaMetadataRetriever.
     Used by getResolution(), getRotation(), and getBitrate().
     */
    private static MediaMetadataRetriever getRetriever(Context appContext, Uri inputUri) {
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        retriever.setDataSource(appContext, inputUri);
        return retriever;
    }

    public static Size getResolution(Context appContext, Uri inputUri) {
        MediaMetadataRetriever retriever = getRetriever(appContext, inputUri);
        int width = Integer.parseInt(retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_WIDTH));
        int height = Integer.parseInt(retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_HEIGHT));
        return new Size(width, height);
    }

    public static int getRotation(Context appContext, Uri inputUri) {
        MediaMetadataRetriever retriever = getRetriever(appContext, inputUri);
        int rotation = Integer.parseInt(retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_ROTATION));
        return rotation;
    }

    public static int getBitrate(Context appContext, Uri inputUri) {
        MediaMetadataRetriever retriever = getRetriever(appContext, inputUri);
        int bitrate = Integer.parseInt(retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_BITRATE));
        return bitrate;
    }

    /*
     Helper method to create MediaExtractor.
     Used by getFPS() and getProfile().
     */
    private static MediaExtractor getExtractor(Context appContext, Uri inputUri) {
        MediaExtractor extractor = new MediaExtractor();
        try {
            extractor.setDataSource(appContext, inputUri, null);
        } catch (IOException e) {
            e.printStackTrace();
        }
        return extractor;
    }

    public static int getFPS(Context appContext, Uri inputUri) {
        MediaExtractor extractor = getExtractor(appContext, inputUri);
        int fps = DEFAULT_FPS;
        for (int i = 0; i < extractor.getTrackCount(); i++) {
            MediaFormat format = extractor.getTrackFormat(i);
            if (format.containsKey(MediaFormat.KEY_FRAME_RATE)) {
                fps = format.getInteger(MediaFormat.KEY_FRAME_RATE);
                // Account for variable frame rate (float has been cast to integer)
                if (Math.abs(fps - 30) <= VARIABLE_FPS_THRESHOLD) {
                    fps = 30;
                }
                if (Math.abs(fps - 60) <= VARIABLE_FPS_THRESHOLD) {
                    fps = 60;
                }
                break;
            }
        }
        return fps;
    }

    public static int getProfile(Context appContext, Uri inputUri) {
        MediaExtractor extractor = getExtractor(appContext, inputUri);
        int profile = -1;
        for (int i = 0; i < extractor.getTrackCount(); i++) {
            MediaFormat format = extractor.getTrackFormat(i);
            if (format.containsKey(MediaFormat.KEY_PROFILE)) {
                profile = format.getInteger(MediaFormat.KEY_PROFILE);
                break;
            }
        }
        return profile;
    }
}
