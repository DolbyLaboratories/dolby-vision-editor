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

#include <jni.h>
#include <string>
#include "mp4remuxer.h"
#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "mp4remuxer-jni", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "mp4remuxer-jni", __VA_ARGS__))

extern "C" {
    JNIEXPORT jint JNICALL
    Java_com_dolby_capture_filtersimulation_TrimEncoder_remuxFile(JNIEnv *env, jobject /*thiz*/,
                                                                    jstring input_path,
                                                                    jstring output_path,
                                                                    jint dv_profile, jint dv_level,
                                                                    jint dv_back_compatibility) {

        const char *nativeInputPath = env->GetStringUTFChars(input_path, 0);
        const char *nativeOutputPath = env->GetStringUTFChars(output_path, 0);
        int nativeDVProfile = (int) dv_profile;
        int nativeDVLevel = (int) dv_level;
        int nativeDVBackCompatibility = (int) dv_back_compatibility;

        mp4_remuxer_t *remuxer = mp4remux_remuxer_create();
        if (remuxer == NULL) {
            LOGE("JNI unable to create remuxer\n");
            return 1;
        }
        LOGI("JNI created mp4 remuxer successfully for (profile: %d), (level: %d),(compatiblitty: %d)\n",
                                                        nativeDVProfile, nativeDVLevel,
                                                        nativeDVBackCompatibility);

        int ret = mp4remux_search_insert_track(remuxer, nativeInputPath);
        if (ret) {
            LOGE("JNI Invalid input mp4 to insert Dovi box: %s \n", nativeInputPath);
            mp4remux_remuxer_destroy(remuxer);
            return 1;
        }
        LOGI("JNI mp4remux_search_insert_track() completed\n");

        remuxer->dv_profile = nativeDVProfile;
        remuxer->dv_level = nativeDVLevel;
        remuxer->dv_bl_comp_id = nativeDVBackCompatibility;

        ret = mp4remux_parse_boxes(remuxer, nativeInputPath, nativeOutputPath);
        if (ret != 0) {
            LOGI("JNI Invalid unable to parse file from: %s  ==>> to: %s" ,nativeInputPath,
                                                          nativeOutputPath );
            mp4remux_remuxer_destroy(remuxer);
            return 1;
        }
        LOGI("JNI mp4remux_parse_boxes() completed\n");

        mp4remux_remuxer_destroy(remuxer);

        LOGI("JNI remuxing completed\n");

        return 0;
    }
}
