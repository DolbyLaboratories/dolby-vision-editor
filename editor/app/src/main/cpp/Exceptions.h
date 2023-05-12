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

#ifndef FILTERSIMULATION_EGLEXCEPTIONS_H
#define FILTERSIMULATION_EGLEXCEPTIONS_H

#include <string>
#include <memory>
#include <android/log.h>

namespace Simulation
{

enum class EGLContextError
{
    NO_ERROR                      = 0,
    DISPLAY_ACQUISITION_FAILED    = 1,
    DISPLAY_INITIALIZATION_FAILED = 2,
    CONFIG_ACQUISITION_FAILED     = 3,
    CONTEXT_ACQUISITION_FAILED    = 4,
};

enum class EGLMapError
{
    NO_ERROR                                = 0,
    NATIVE_CLIENT_BUFFER_ACQUISITION_FAILED = 1,
    IMAGE_CREATION_FAILED                   = 2,
    ILLEGAL_FRAMEBUFFER_STATUS              = 3,
    LOAD_INPUT_TEXTURE_FAILED               = 4,
    LOAD_INPUT_RAW_FAILED                   = 5,
};
}


#endif //FILTERSIMULATION_EGLEXCEPTIONS_H
