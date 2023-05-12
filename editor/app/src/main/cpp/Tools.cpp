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

#include <iostream>
#include <stdio.h>
#include <math.h>

#ifndef LOG_TAG
#define LOG_TAG "tools"
#endif // LOG_TAG

#include "Tools.h"

using namespace std;

namespace Simulation
{
///////////////////////////////////////////////////////////////////////////////
// ScopeTimer & ScopeTimerGPU

atomic_flag ScopeTimer::mBusy = ATOMIC_FLAG_INIT;

///////////////////////////////////////////////////////////////////////////////
// Some OpenGL tools

#ifdef _DEBUG
bool mDumpShaderErrors = true;
#else  // ! _DEBUG
bool mDumpShaderErrors = true; // Normally false for release
#endif // ! _DEBUG

bool CompileAndLinkShader(GLuint &program, char const *fragment_or_compute_text, char const *vertex_text)
{
    DeleteProgram(program); // Nuke any existing program

    // Used by the cleanup Lambda
    bool guilty = true; // Guilty until proven innocent
    GLuint main_shader = GL_INVALID_VALUE, vertex_shader = GL_INVALID_VALUE;

    // Lambda cleanup function to sweep up the floor after the party is over no matter how we leave
    ScopeExitFunction cleanup([&]()
    {
        DeleteShader(main_shader);
        DeleteShader(vertex_shader);
        if (guilty) DeleteProgram(program);
    });

    bool vertex_fragment_shader = (vertex_text != nullptr); // Compute shader if no vertex source provided
    if (vertex_fragment_shader)
    {
        vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        if (vertex_shader == 0)
        {
            CHECK_GL_ERROR;
            return true;
        }
    }
    main_shader = glCreateShader(vertex_fragment_shader ? GL_FRAGMENT_SHADER: GL_COMPUTE_SHADER);
    if (main_shader == 0)
    {
        CHECK_GL_ERROR;
        return true;
    }

    glShaderSource(main_shader, 1, &fragment_or_compute_text, nullptr);
    if (CHECK_GL_ERROR) return true;
    glCompileShader(main_shader);
    CHECK_GL_ERROR;
    GLint compiled = 0;
    glGetShaderiv(main_shader, GL_COMPILE_STATUS, &compiled);
    CHECK_GL_ERROR;
    if (compiled == GL_FALSE)
    {
        LOGE("Error compiling shader");
        if (mDumpShaderErrors)
        {
            DumpEmbeddedLines(fragment_or_compute_text, "Compute or Fragment Shader:");
        }
        GLint infoLen = 500;
        glGetShaderiv(main_shader, GL_INFO_LOG_LENGTH, &infoLen);
        CHECK_GL_ERROR;
        if (infoLen > 0)
        {
            char *buf = new char[infoLen];
            if (buf)
            {
                glGetShaderInfoLog(main_shader, infoLen, nullptr, buf);
                LOGE("Compile log : %s", buf);
                delete[] buf;
            }
        }
        return true;
    }
    if (vertex_fragment_shader)
    {
        glShaderSource(vertex_shader, 1, &vertex_text, nullptr);
        if (CHECK_GL_ERROR) return true;
        glCompileShader(vertex_shader);
        CHECK_GL_ERROR;
        GLint compiled = 0;
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
        CHECK_GL_ERROR;
        if (compiled == GL_FALSE)
        {
            LOGE("Error compiling shader");
            if (mDumpShaderErrors)
            {
                DumpEmbeddedLines(vertex_text, "Vertex Shader:");
            }
            GLint infoLen = 500;
            glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &infoLen);
            CHECK_GL_ERROR;
            if (infoLen > 0)
            {
                char *buf = new char[infoLen];
                if (buf)
                {
                    glGetShaderInfoLog(main_shader, infoLen, nullptr, buf);
                    LOGE("Compile log : %s", buf);
                    delete[] buf;
                }
            }
            return true;
        }
    }

    // Link the shader
    program = glCreateProgram();
    if (CHECK_GL_ERROR) return true;
    if (vertex_fragment_shader)
    {
        glAttachShader(program, vertex_shader);
        if (CHECK_GL_ERROR) return true;
    }
    glAttachShader(program, main_shader);
    if (CHECK_GL_ERROR) return true;
    glLinkProgram(program);
    if (CHECK_GL_ERROR) return true;

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    CHECK_GL_ERROR;
    if (!linked)
    {
        LOGE("Error linking shader");
        if (mDumpShaderErrors)
        {
            DumpEmbeddedLines(vertex_text, "Vertex Shader:");
            DumpEmbeddedLines(fragment_or_compute_text, "Compute or Fragment Shader:");
        }
        GLint infoLen = 500;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        CHECK_GL_ERROR;
        if (infoLen > 0)
        {
            char *buf = new char[infoLen];
            if (buf)
            {
                glGetProgramInfoLog(program, infoLen, nullptr, buf);
                CHECK_GL_ERROR;
                LOGE("Link log : %s", buf);
                delete[] buf;
            }
        }
        return true;
    }

    glUseProgram(program);
    if (CHECK_GL_ERROR) return true;
    LOGI("Program created: %d", program);

#if 0
    DumpEmbeddedLines(vertex_text, "Vertex Shader:");
    DumpEmbeddedLines(fragment_or_compute_text, "Compute or Fragment Shader:");
#endif

    guilty = false; // Tell the cleanup Lambda that we're successful
    return guilty;
}

// Note that the argument is passed by reference to these functions
// These functions use the convention that unallocated handles are initlialized
// to GL_INVALID_VALUE, and when release are reset to GL_INVALID_VALUE
void DeleteTexture(GLuint &handle)
{
    GLuint item[1];
    if (handle != GL_INVALID_VALUE)
    {
        item[0] = handle;
        glDeleteTextures(1, item);
        handle = GL_INVALID_VALUE;
    }
}

// Note that the argument is passed by reference
void DeleteProgram(GLuint &handle)
{
    if (handle != GL_INVALID_VALUE)
    {
        glDeleteProgram(handle);
        handle = GL_INVALID_VALUE;
    }
}

// Note that the argument is passed by reference
void DeleteShader(GLuint &handle)
{
    if ((handle != GL_INVALID_VALUE) && (handle != 0))
    {
        glDeleteShader(handle);
        handle = GL_INVALID_VALUE;
    }
}

// Note that the argument is passed by reference
void DeleteBuffer(GLuint &handle)
{
    if (handle != GL_INVALID_VALUE)
    {
        glDeleteBuffers(1, &handle);
        handle = GL_INVALID_VALUE;
    }
}

///////////////////////////////////////////////////////////////////////////////
// CheckGlError

bool CheckGlError(const char *function, const char *file, int line)
{
    GLenum last_code = GL_NO_ERROR;
    bool guilty = false; // Innocent until proven guilty
    for (GLenum code = glGetError(); code != GL_NO_ERROR && code != last_code; code = glGetError())
    {
        last_code = code;
        LOGE("GL error in function: %s, file: %s, line: %d, 0x%X", function, file, line, code);
        guilty = true;
    }
//    if (guilty) assert(false);
    return guilty;
}

///////////////////////////////////////////////////////////////////////////////
// Other tools

void DumpEmbeddedLines(const char *text, const char *error_label)
{
    char *next = nullptr, *ptr = (char *)text;
    char line[2048];
    int index = 1;

    if (text == nullptr) return;
    if (error_label) LOGE("%s", error_label);
    next = strstr(ptr, "\n");
    while (next)
    {
        int count = std::min((size_t)(next - ptr), GetLength(line) - 1);
        strncpy(line, ptr, count);
        line[count] = 0;
        LOGI("%d: %s", index++, line);
        ptr = next + 1;
        next = strstr(ptr, "\n");
    }

    // Last line doesn't end in newline?
    if (*ptr)
    {
        int count = std::min((size_t)(next - ptr), strlen(ptr));
        strncpy(line, ptr, count);
        line[count] = 0;
        LOGI("%d: %s", index++, line);
    }
}

} // namespace Simulation
