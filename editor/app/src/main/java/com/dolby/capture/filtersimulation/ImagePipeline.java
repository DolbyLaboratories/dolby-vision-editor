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

import android.graphics.ImageFormat;
import android.hardware.DataSpace;
import android.hardware.HardwareBuffer;
import android.media.ImageReader;
import android.media.ImageWriter;
import android.media.MediaCodecInfo;
import android.util.Log;
import android.util.Size;
import android.view.Surface;


/**
 * Configures image pipeline for Dolby Vision editing.
 * This class creates ImageReader and ImageWriter objects that support Dolby Vision, which app
 * developers can retrieve and access in their editing code. This class configures the
 * appropriate output DataSpace and hardware buffer format for both transcoding and video
 * previewing.
 */
public class ImagePipeline {

    public static final String TAG = ImagePipeline.class.getSimpleName();
    public static final int MAX_IMAGES = 10;
    private ImageReader reader;
    private Surface outputSurface;
    private int dataspace;

    /**
     * This constructor builds the ImageReader and ImageWriter objects. The reader and writer are
     * ready for use after the constructor returns.
     *
     * @param inputSize The dimensions of the input video.
     * @param outputSize The dimensions of the output surface.
     * @param outputSurface The destination Surface object.
     * @param previewMode True if the output surface is writing to a preview, False if transcoding.
     * @param inputProfile The profile of the input video, as specified by MediaFormat.
     * @param encoderFormat The intended format of the output ("HEVC", "AVC, or "DV-ME").
     */
    public ImagePipeline(Size inputSize, Size outputSize, Surface outputSurface, boolean previewMode, int inputProfile, String encoderFormat) {

        Log.d(TAG, "inputSize=" + inputSize + ", outputSize=" + outputSize + ", previewMode=" + previewMode + ", inputProfile=" + inputProfile + ", encoderFormat=" + encoderFormat);

        this.outputSurface = outputSurface;
        // Configure dataspace
        if (previewMode && inputProfile != MediaCodecInfo.CodecProfileLevel.DolbyVisionProfileDvheSt) {
            this.dataspace = DataSpace.pack(DataSpace.STANDARD_BT709, DataSpace.TRANSFER_SMPTE_170M, DataSpace.RANGE_LIMITED);
        } else {
            if (encoderFormat.equals(Constants.DV_ME)) {
                this.dataspace = DataSpace.pack(DataSpace.STANDARD_BT2020, DataSpace.TRANSFER_HLG, DataSpace.RANGE_LIMITED);
            } else {
                this.dataspace = DataSpace.pack(DataSpace.STANDARD_BT709, DataSpace.TRANSFER_SMPTE_170M, DataSpace.RANGE_LIMITED);
            }
        }

        // Build ImageReader
        long usage = HardwareBuffer.USAGE_CPU_READ_OFTEN | HardwareBuffer.USAGE_CPU_WRITE_OFTEN;
//        long usage = HardwareBuffer.USAGE_GPU_COLOR_OUTPUT;
        reader = new ImageReader.Builder(inputSize.getWidth(), inputSize.getHeight())
                .setMaxImages(MAX_IMAGES)
                .setUsage(usage)
                .setImageFormat(ImageFormat.PRIVATE)
                .build();
    }

    /**
     * Gets a reference to the ImageReader object configured for Dolby Vision editing.
     *
     * @return The ImageReader object.
     */
    public ImageReader getImageReader() {
        return reader;
    }


    public Surface getOutputSurface() {
        return outputSurface;
    }

    /**
     * Returns the DataSpace configuration for the output surface. This is based on the input
     * format, output format, and mode (preview vs. transcode).
     *
     * @return The DataSpace integer value.
     */
    public int getOutputDataSpace() {
        return dataspace;
    }

    public void closePipline() {
        if (reader != null) {
            reader.close();
            reader = null;
        }
    }

}
