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

#ifndef FILTERSIMULATION_HARDWAREBUFFER_H
#define FILTERSIMULATION_HARDWAREBUFFER_H

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <android/hardware_buffer.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#include <GLES2/gl2ext.h>

#include "Tools.h"

namespace Simulation
{

class HardwareBuffer
{
public:
    enum class ColorSpace : uint16_t
    {
        UNSUPPORTED = 0,
        RGB         = 1,
        YUV         = 2,
    };

    enum class DataSpace : uint16_t
    {
        UNSUPPORTED = 0,
        HLG         = 1,
    };

public:
    HardwareBuffer(AHardwareBuffer_Format format, int width, int height);

    HardwareBuffer(AHardwareBuffer* inputBuffer);

    virtual ~HardwareBuffer();

    AHardwareBuffer* getHardwareBuffer();

    char readData(int index);

    ColorSpace getColorSpace() const;

    // TODO: replace with my own getters/setters.
    AHardwareBuffer_Desc* getHWBufferParameters();

    // Returns the mWidth in pixels from target
    uint32_t getHardwareBufferWidth() const;

    // Returns the mHeight in pixels from target
    uint32_t getHardwareBufferHeight() const;

    // Returns the number of images in the image array from target
    uint32_t getHardwareBufferLayers() const;

    // Returns the buffer pixel format from target
    uint32_t getHardwareBufferFormat() const;

    // Returns the buffer usage flags from target
    uint64_t getHardwareBufferUsage() const;

    // Returns the row stride in pixels from target
    uint32_t getHardwareBufferStride() const;

    uint32_t getHardwareBufferRFU0() const;

    uint64_t getHardwareBufferRFU1() const;

    bool isHardwareBufferFormatSupported() const;

    bool hasHardwareBufferUsageFlag(AHardwareBuffer_UsageFlags flag) const;

private:
    AHardwareBuffer*     target;
    AHardwareBuffer_Desc targetDescription;
    bool                 targetOwnedLocally;

};
}

#endif //FILTERSIMULATION_HARDWAREBUFFER_H
