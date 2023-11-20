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

public class Constants {

    public enum TextPosition {
        TOP,
        MIDDLE,
        BOTTOM,
    }

    public enum ColorStandard {         // Must match enum eColorStandard in EditShaders.h!
        eColorStandard10BitRec709,
        eColorStandard10BitRec2020,
        eColorStandardCount
    }

    public static final String HEVC = "HEVC";
    public static final String AVC = "AVC";
    public static final String DV_ME = "DV-ME";

    public static final String FILTER_NONE = "No Filter";
    public static final String FILTER_BW = "Black and White";
    public static final String FILTER_SEPIA = "Sepia";
    public static final String BLACK = "Black";
    public static final String WHITE = "White";
    public static final String GRAY = "Gray";
    public static final String PURPLE = "Purple";
    public static final String RESOLUTION_DEFAULT = "Default";
    public static final String RESOLUTION_2K = "1920x1080";
    public static final String RESOLUTION_4K = "3840x2160";
    public static final String ENCODER_HEVC = "HEVC";
    public static final String ENCODER_AVC = "AVC";
    public static final String ENCODER_P84 = "Profile 8.4";
    public static final String IFRAME_LOSSLESS = "Lossless";
    public static final String IFRAME_1_SEC = "1 second";
    public static final String DOLBY_HEVC_ENCODER = "c2.dolby.encoder.hevc";
    public static final String DOLBY_HEVC_DECODER = "c2.dolby.decoder.hevc";

    // Arrays to populate all the spinners in MainActivity
    public static final String[] FILTER_OPTIONS = {FILTER_NONE, FILTER_BW, FILTER_SEPIA};
    public static final String[] TEXT_COLORS = {BLACK, WHITE, GRAY, PURPLE};
    public static final String[] RESOLUTION_OPTIONS = {RESOLUTION_DEFAULT, RESOLUTION_2K, RESOLUTION_4K};
    public static final String[] ENCODER_FORMAT = {ENCODER_HEVC, ENCODER_AVC, ENCODER_P84};
    public static final String[] IFRAME_INTERVALS = {IFRAME_LOSSLESS, IFRAME_1_SEC};

    public static final int RESOLUTION_2K_WIDTH = 1920;
    public static final int RESOLUTION_2K_HEIGHT = 1080;
    public static final int RESOLUTION_4K_WIDTH = 3840;
    public static final int RESOLUTION_4K_HEIGHT = 2160;

}
