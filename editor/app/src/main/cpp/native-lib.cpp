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
#define LOG_TAG "native-lib"
//#define LOG_NDEBUG 0

#include <jni.h>
#include <string>
#include <android/log.h>
#include <iostream>
#include <memory>

#include <android/hardware_buffer_jni.h>

#include "Renderer.h"
#include "EGLContext.h"
#include "EGLMap.h"

#include "EditShaders.h"

namespace JNI_GLOBAL
{
    std::shared_ptr<Simulation::Context>              context;
    std::shared_ptr<Simulation::Renderer>             renderer;

    std::shared_ptr<Simulation::CopyRenderer> rendererCopyRGB;
    std::shared_ptr<Simulation::CopyRenderer> rendererCopyYUV;

    // Buffer dimensions
    int32_t bufferWidth  = 0;
    int32_t bufferHeight = 0;

    // Texture dimensions
    int32_t textureWidth  = 0;
    int32_t textureHeight = 0;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_dolby_capture_filtersimulation_OpenGLContext_eglInit(JNIEnv *env, jobject /* this */)
{
    LOGI("native-lib", "eglInit");

    JNI_GLOBAL::context = std::make_shared<Simulation::Context>(nullptr);
    JNI_GLOBAL::context->makeCurrent();

    JNI_GLOBAL::renderer = std::make_shared<Simulation::Renderer>();
    JNI_GLOBAL::renderer->init();

    JNI_GLOBAL::rendererCopyRGB = std::make_shared<Simulation::CopyRenderer>();
    JNI_GLOBAL::rendererCopyRGB->init(false);

    JNI_GLOBAL::rendererCopyYUV = std::make_shared<Simulation::CopyRenderer>();
    JNI_GLOBAL::rendererCopyYUV->init(true);

    return 0;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_dolby_capture_filtersimulation_FrameHandler_processFrame(JNIEnv *env, jobject thiz, jobject inbuf, jobject opbuf)
{

    AHardwareBuffer* inAHB = AHardwareBuffer_fromHardwareBuffer(env,inbuf);

    AHardwareBuffer* outAHB = AHardwareBuffer_fromHardwareBuffer(env,opbuf);

    JNI_GLOBAL::context->makeCurrent();

    Simulation::HardwareBuffer inputBuffer(inAHB);

    Simulation::HardwareBuffer outputBuffer(outAHB);

    Simulation::EGLMap inputImage(inputBuffer, JNI_GLOBAL::context->getDisplay(), GL_TEXTURE_EXTERNAL_OES);

    Simulation::EGLMap outputImage(outputBuffer, JNI_GLOBAL::context->getDisplay(), GL_TEXTURE_EXTERNAL_OES);

    outputImage.bindFBO();

    // loop_count = 0 disables timing, set > 1 for GPU timing, 100 recommended.
    Simulation::ScopeTimerGPU scope_timer("ContentLoader_processFrame: ", 0);
    { // Note scope limit for timer
        for (int i = 0; i < scope_timer.LoopCount(); i++) {
            if (true)
            {
                JNI_GLOBAL::renderer->RunEditComputeShader(inputImage.getBufferTexture(),
                                               inputImage.getHardwareBufferWidth(),
                                               inputImage.getHardwareBufferHeight());
                JNI_GLOBAL::renderer->render(JNI_GLOBAL::renderer->GetTransferTextureID(),
                                             outputImage.getLutTexture(),
                                             false,
                                             false);
             } else { // For debugging
                JNI_GLOBAL::renderer->render(inputImage.getBufferTexture(),
                                             outputImage.getLutTexture(),
                                             false,
                                             false);
            }
        }
    }

    return 0;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_dolby_capture_filtersimulation_VideoDecoder_EditShadersInit(JNIEnv *env, jobject thiz,
    jint output_width,
    jint output_height,
    jint color_standard,
    jint input_colorspace,
    jint output_colorspace) {
    JNI_GLOBAL::renderer->Init(
        output_width,
        output_height,
        (Simulation::EditShaders::eColorStandard) color_standard,
        (Simulation::EditShaders::eColorSpace) input_colorspace,
        (Simulation::EditShaders::eColorSpace) output_colorspace);
    return JNI_GLOBAL::renderer->BuildEditComputeShader();
}


extern "C" JNIEXPORT jint JNICALL
Java_com_dolby_capture_filtersimulation_VideoDecoder_EditShadersRelease(JNIEnv *env, jobject thiz) {
    JNI_GLOBAL::renderer->ReleaseAll();
    return 0;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_dolby_capture_filtersimulation_VideoDecoder_EditShadersEnableLut(JNIEnv *env, jobject thiz, jint enable) {
    JNI_GLOBAL::renderer->EnableLut(enable);
    return 0;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_dolby_capture_filtersimulation_ContentLoader_EditShadersSetParameter(JNIEnv *env, jobject thiz, jint parameter, jfloat val) {
    JNI_GLOBAL::renderer->SetParameter((Simulation::EditShadersUniformBlock::EffectParameters) parameter, val);
    return 0; //
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_dolby_capture_filtersimulation_ContentLoader_EditShadersGetParameter(JNIEnv *env, jobject thiz, jint parameter) {
    return JNI_GLOBAL::renderer->GetParameter((Simulation::EditShadersUniformBlock::EffectParameters) parameter);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_dolby_capture_filtersimulation_ContentLoader_EditShadersSetCompositingImage(JNIEnv *env, jobject thiz, jbyteArray data) {

    jboolean isCopy;
    jbyte* b = env->GetByteArrayElements(data, &isCopy);

    int result = JNI_GLOBAL::renderer->SetCompositingImage((char *) b);

    env->ReleaseByteArrayElements( data, b, 0);

    return result;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_dolby_capture_filtersimulation_ContentLoader_EditShadersSetInputTransformMatrix(JNIEnv *env, jobject thiz, _jfloatArray data) {
//    return JNI_GLOBAL::renderer->SetInputTransformMatrix(data);
    return 0;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_dolby_capture_filtersimulation_ContentLoader_EditShadersSetCompositorTransformMatrix(JNIEnv *env, jobject thiz, _jfloatArray data) {
//    return JNI_GLOBAL::renderer->SetCompositorTransformMatrix(data);
    return 0;
}
