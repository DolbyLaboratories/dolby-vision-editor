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

#ifndef FILTERSIMULATION_EGLMAP_H
#define FILTERSIMULATION_EGLMAP_H

#include <memory>
#include <vector>

#include "HardwareBuffer.h"
#include "Exceptions.h"

namespace Simulation
{

class EGLMap
{
public:
    EGLMap(bool isInput, HardwareBuffer &hwBuffer, EGLDisplay display, GLenum texSource = GL_TEXTURE_2D);

    virtual ~EGLMap();

    std::vector<int> getTexValue();

    GLuint getBufferTexture() const;

    GLuint getLutTexture() const;

    GLuint getInputTexture() const;

    void bindFBO() const;

    void loadInputTexture(const unsigned char* textureData, int width, int height);

    void loadInputRaw(const unsigned char* textureData, int width, int height, int length);

    bool isMappingValid(EGLMapError* rete = nullptr) const;

    int getHardwareBufferWidth() {return (int)static_cast<GLsizei>(buffer->getHardwareBufferWidth());}
    int getHardwareBufferHeight() {return (int)static_cast<GLsizei>(buffer->getHardwareBufferHeight());}

private:
    GLuint createEGLImage(bool isInput);

private:
    // GL_TEXTURE_EXTERNAL_OES GL_TEXTURE_2D
    GLenum textureType;
    GLuint bufferTexture;
    GLuint lutTexture;
    GLuint inputTexture;
    GLuint fbo;

    EGLClientBuffer clientBuffer;
    EGLImageKHR     image;
    EGLDisplay      display;

    HardwareBuffer* buffer;

    bool        valid = true;
    EGLMapError error = EGLMapError::NO_ERROR;
};
} // namespace Simulation

#endif //FILTERSIMULATION_EGLMAP_H
