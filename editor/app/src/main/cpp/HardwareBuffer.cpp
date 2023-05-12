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

#include "HardwareBuffer.h"

#include <android/log.h>
#include <string>
#include <memory>

//#define SHARE_CURRENT_CONTEXT

#define LOG_TAG "HardwareBuffer"

// Qualcomm AHardwareBuffer format value constants
static const uint32_t TP10 = 0x7fa30c09;
static const uint32_t P010 = 0x7fa30c0a;

Simulation::HardwareBuffer::HardwareBuffer(AHardwareBuffer* inputBuffer)
    : target(inputBuffer)
    , targetOwnedLocally(false)
{
    AHardwareBuffer_describe(target, &targetDescription);
}

Simulation::HardwareBuffer::HardwareBuffer(AHardwareBuffer_Format format, int width, int height)
    : targetOwnedLocally(true)
{
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "CTOR: (format: 0x%02X, width: %d, height: %d)", format, width, height);

    // TODO: change name
    targetDescription.width  = width;
    targetDescription.height = height;
    targetDescription.layers = 1; // for standard 2D textures.
    targetDescription.format = format;
    targetDescription.usage  = AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER | AHARDWAREBUFFER_USAGE_VIDEO_ENCODE | AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY; //AHARDWAREBUFFER_USAGE_PROTECTED_CONTENT;
    targetDescription.rfu0   = 0; // 0 per docs.
    targetDescription.rfu1   = 0; // 0 per docs.

    if (AHardwareBuffer_isSupported(&targetDescription))
    {
        int allocateResult = AHardwareBuffer_allocate(&targetDescription, &target);
        if (allocateResult != 0)
        {
            __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "native hardware buffer allocation: %s (code: %d)", "FAIL", allocateResult);
        }
        else
        {
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "native hardware buffer allocation: %s", "PASS");
        }

        void*   bufferAddress = nullptr;
        int32_t outBPP; // bytes per pixel
        int32_t outBPS; // bytes per stride

        if ((targetDescription.usage & AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY) == AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY)
        {
            AHardwareBuffer_lockAndGetInfo(target, AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY, 0, nullptr, &bufferAddress, &outBPP, &outBPS);
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "native hardware buffer locked information: (BPP: %d, BPS: %d)", outBPP, outBPS);

            auto* data  = static_cast<char*>(bufferAddress);
            char  value = 255;

            for (int i = 0; i < (width * height * outBPP); i++)
            {
                data[i] = value;
            }

            int writeResult = AHardwareBuffer_unlock(target, nullptr);
            if (writeResult != 0)
            {
                __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "native hardware buffer write: %s (code: %d)", "FAIL", writeResult);
            }
            else
            {
                __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "native hardware buffer write: %s", "PASS");
            }
        }
        else
        {
            __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "native hardware buffer missing flag: %s", "AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY");
        }
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "native hardware buffer configuration supported: %s", "FAIL");
    }
}

Simulation::HardwareBuffer::~HardwareBuffer()
{
    if (target != nullptr && targetOwnedLocally)
    {
        AHardwareBuffer_release(target);
    }
}

AHardwareBuffer* Simulation::HardwareBuffer::getHardwareBuffer()
{
    return this->target;
}

char Simulation::HardwareBuffer::readData(int index)
{
    void*   bufferAddress = nullptr;
    int32_t outBPP; // bytes per pixel
    int32_t outBPS; // bytes per stride

    AHardwareBuffer_lockAndGetInfo(target, AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY, 0, nullptr, &bufferAddress, &outBPP, &outBPS);
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "native hardware buffer locked information: (BPP: %d, BPS: %d)", outBPP, outBPS);

    auto* data = static_cast<char*>(bufferAddress);

    int readResult = AHardwareBuffer_unlock(target, nullptr);
    if (readResult != 0)
    {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "native hardware buffer read: %s (code: %d)", "FAIL", readResult);
    }
    else
    {
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "native hardware buffer read: %s", "PASS");
    }

    return data[index];
}

Simulation::HardwareBuffer::ColorSpace Simulation::HardwareBuffer::getColorSpace() const
{
    switch (targetDescription.format)
    {
        case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM:
        case AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM:
        case AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM:
        case AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM:
        case AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT:
        case AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM:
            return ColorSpace::RGB;
        case AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420:
            return ColorSpace::YUV;
        default:
            return ColorSpace::UNSUPPORTED;
    }
}

AHardwareBuffer_Desc* Simulation::HardwareBuffer::getHWBufferParameters()
{
    return &targetDescription;
}

uint32_t Simulation::HardwareBuffer::getHardwareBufferWidth() const
{
    return targetDescription.width;
}

uint32_t Simulation::HardwareBuffer::getHardwareBufferHeight() const
{
    return targetDescription.height;
}

uint32_t Simulation::HardwareBuffer::getHardwareBufferLayers() const
{
    return targetDescription.layers;
}

uint32_t Simulation::HardwareBuffer::getHardwareBufferFormat() const
{
    return targetDescription.format;
}

uint64_t Simulation::HardwareBuffer::getHardwareBufferUsage() const
{
    return targetDescription.usage;
}

uint32_t Simulation::HardwareBuffer::getHardwareBufferStride() const
{
    return targetDescription.stride;
}

uint32_t Simulation::HardwareBuffer::getHardwareBufferRFU0() const
{
    return targetDescription.rfu0;
}

uint64_t Simulation::HardwareBuffer::getHardwareBufferRFU1() const
{
    return targetDescription.rfu1;
}

bool Simulation::HardwareBuffer::isHardwareBufferFormatSupported() const
{
    return AHardwareBuffer_isSupported(&targetDescription);
}

bool Simulation::HardwareBuffer::hasHardwareBufferUsageFlag(AHardwareBuffer_UsageFlags flag) const
{
    return (targetDescription.usage & flag) == flag;
}


