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

#include "EGLMap.h"

#include <android/log.h>
#include <string>

#define LOG_TAG "EGLMap"

Simulation::EGLMap::EGLMap(Simulation::HardwareBuffer &hwBuffer, EGLDisplay display, GLenum texSource)
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG,  "CTOR");

    if (texSource == GL_TEXTURE_2D || texSource == GL_TEXTURE_EXTERNAL_OES)
    {
        this->textureType = texSource;
    }
    else
    {
        this->textureType = GL_TEXTURE_2D;
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "invalid texture type. Only GL_TEXTURE_2D and GL_TEXTURE_EXTERNAL_OES supported. Default to GL_TEXTURE_2D.");
    }

    this->buffer  = &hwBuffer;
    this->display = display;

    createEGLImage();

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "opengl bindings: (buffer texture: %d, lut texture: %d, input texture: %d, fbo: %d)", bufferTexture, lutTexture, inputTexture, fbo);
}

std::vector<int> Simulation::EGLMap::getTexValue()
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG,  "getTexValue");

    glReadBuffer(GL_COLOR_ATTACHMENT0);

    auto bw = static_cast<GLsizei>(buffer->getHardwareBufferWidth());
    auto bh = static_cast<GLsizei>(buffer->getHardwareBufferHeight());

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "hardware buffer parameters: (width: %d, height: %d)", bw, bh);

    // Pre-allocate vector on the heap so we can free before MDGen
    std::vector<int> bufferOut(bw * bh, 0);

    glReadPixels(0, 0, bw, bh, GL_RGBA, GL_UNSIGNED_BYTE, bufferOut.data());

    return bufferOut;
}

Simulation::EGLMap::~EGLMap()
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG,  "DTOR");

    eglDestroyImageKHR(display, image);
    glDeleteTextures(1, &inputTexture);
    glDeleteTextures(1, &bufferTexture);
    glDeleteTextures(1, &lutTexture);
    glDeleteFramebuffers(1, &fbo);
}

unsigned int Simulation::EGLMap::createEGLImage()
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG,  "createEGLImage");

    clientBuffer = eglGetNativeClientBufferANDROID(buffer->getHardwareBuffer());

    if (clientBuffer == nullptr)
    {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "native client buffer acquisition: FAIL");

        if (valid)
        {
            error = Simulation::EGLMapError::NATIVE_CLIENT_BUFFER_ACQUISITION_FAILED;
            valid = false;
        }
    }
    else
    {
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "native client buffer acquisition: PASS");
    }

    glGenTextures(1, &inputTexture);

    // Output lut for conversions on input.
    glGenTextures(1, &lutTexture);

    glGenTextures(1, &bufferTexture);
    glBindTexture(textureType, bufferTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);

    //glTexParameteri(textureType, GL_TEXTURE_PROTECTED_EXT, GL_TRUE);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

    int imageAttribs[] = {
            //EGL_PROTECTED_CONTENT_EXT, EGL_TRUE,
            EGL_NONE
    };

    image = eglCreateImageKHR(
            display,
            EGL_NO_CONTEXT,
            EGL_NATIVE_BUFFER_ANDROID,
            clientBuffer,
            imageAttribs);

    if (image == EGL_NO_IMAGE_KHR)
    {
        EGLint eglError = eglGetError();
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "image creation: FAIL (egl error: %d)", eglError);

        if (valid)
        {
            error = Simulation::EGLMapError::IMAGE_CREATION_FAILED;
            valid = false;
        }
    }
    else
    {
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "image creation: PASS");
    }

    glEGLImageTargetTexture2DOES(textureType, static_cast<GLeglImageOES>(image));
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureType, bufferTexture, 0);

    GLenum framebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    switch (framebufferStatus)
    {
        case GL_FRAMEBUFFER_COMPLETE:
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "framebuffer status: PASS");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "framebuffer status: WARN (status: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT). OK for input buffer. CHECK if occurs for output buffer");
            break;
        default:
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "framebuffer status: FAIL (status: %d)", framebufferStatus);

            if (valid)
            {
                error = EGLMapError::ILLEGAL_FRAMEBUFFER_STATUS;
                valid = false;
            }
            break;
    }

    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    glViewport(0, 0, buffer->getHardwareBufferWidth(), buffer->getHardwareBufferHeight());

    return bufferTexture;
}

void Simulation::EGLMap::loadInputTexture(const unsigned char* textureData, int width, int height)
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG,  "loadInputTexture");

    if (textureData == nullptr)
    {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "load input texture: %s", "FAIL");

        if (valid)
        {
            error = EGLMapError::LOAD_INPUT_TEXTURE_FAILED;
            valid = false;
        }
    }
    else
    {
        __android_log_print(ANDROID_LOG_INFO, "EGLMap", "load input texture: PASS");
    }

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, inputTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_PROTECTED_EXT, GL_TRUE);

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "load texture input parameters: (width: %d, height: %d)", width, height);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);

    glActiveTexture(GL_TEXTURE0);
}

void Simulation::EGLMap::loadInputRaw(const unsigned char* textureData, int width, int height, int length)
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG,  "loadInputRaw");

    if (textureData == nullptr)
    {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "load input RAW: %s", "FAIL");

        if (valid)
        {
            error = EGLMapError::LOAD_INPUT_RAW_FAILED;
            valid = false;
        }
    }
    else
    {
        __android_log_print(ANDROID_LOG_INFO, "EGLMap", "load input RAW: PASS");
    }

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, inputTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_PROTECTED_EXT, GL_TRUE);

    std::vector<uint32_t> data;
    data.reserve(width * height);

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "load RAW input parameters: (width: %d, height: %d, length: %d)", width, height, length);

    for (int i = 0; i < length; i+=6)
    {
        uint16_t yReadLower = textureData[i];
        uint16_t yReadUpper = textureData[i + 1];
        uint32_t y          = ((yReadUpper << 8) | yReadLower);

        uint16_t uReadLower = textureData[i + 2];
        uint16_t uReadUpper = textureData[i + 3];
        uint32_t u          = ((uReadUpper << 8) | uReadLower);

        uint16_t vReadLower = textureData[i + 4];
        uint16_t vReadUpper = textureData[i + 5];
        uint32_t v          = ((vReadUpper << 8) | vReadLower);

        uint32_t rMask = y;
        uint32_t gMask = u << 10;
        uint32_t bMask = v << 20;

        uint32_t pixel = rMask | gMask | bMask;

        //__android_log_print(ANDROID_LOG_INFO, "FRAME BUFFER DRAW BUFFERS:", "%x %x %x %x", r, g, b, pixel);

        data.push_back(pixel);
    }

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "RAW parameters: (width: %d, height: %d, length: %d, colors: %lu)", width, height, length, data.size());

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, data.data());

    glActiveTexture(GL_TEXTURE0);
}

GLuint Simulation::EGLMap::getBufferTexture() const
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG,  "getBufferTexture");

    return bufferTexture;
}

GLuint Simulation::EGLMap::getLutTexture() const
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG,  "getLutTexture");

    return lutTexture;
}

GLuint Simulation::EGLMap::getInputTexture() const
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG,  "getInputTexture");

    return inputTexture;
}

void Simulation::EGLMap::bindFBO() const
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG,  "bindFBO");

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}


bool Simulation::EGLMap::isMappingValid(Simulation::EGLMapError* rete) const
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG,  "isMappingValid");

    if (rete)
    {
        *rete = error;
    }

    return valid;
}
