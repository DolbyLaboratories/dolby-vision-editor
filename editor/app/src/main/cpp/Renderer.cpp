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
#define LOG_TAG "Renderer"
//#define LOG_NDEBUG 0

#include "Renderer.h"
#include "Tools.h"

#include <android/log.h>
#include <chrono>
#include <thread>


////////////////////////////////////////////////////////////////////////////////////////////////////
// SHADERS: vertex shader
////////////////////////////////////////////////////////////////////////////////////////////////////

static const GLchar *VERTEX_SHADER_SOURCE =
        R"glsl(#version 320 es

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aTextureCoord;

out vec2 vTextureCoord;

void main()
{
    gl_Position   = vec4(aPosition, 1.0);
    vTextureCoord = aTextureCoord.xy;
}
)glsl";

////////////////////////////////////////////////////////////////////////////////////////////////////
// FRAGMENT SHADER: output buffer YUV layout
////////////////////////////////////////////////////////////////////////////////////////////////////

static const GLchar *FRAGMENT_SHADER_SOURCE_YUV_OUT =
        R"glsl(#version 320 es

#extension GL_OES_EGL_image_external_essl3 : require
#extension GL_OES_EGL_image_external       : require
#extension GL_EXT_YUV_target               : require

precision highp float;
precision highp sampler3D;

layout (yuv) out vec4 FragColor;
             in  vec2 vTextureCoord;

uniform sampler2D uTexture;
uniform sampler3D uLutTexture;
uniform int       uLutSpec;
uniform int       uConvert;

float luminance(vec4 pixel)
{
    return pixel.r * 0.299 + pixel.g * 0.587 + pixel.b * 0.114;
}

vec4 saturate(float scalar, vec4 pixel)
{
    return scalar * pixel + (1.0 - scalar) * luminance(pixel);
}

vec4 grayscale(vec4 pixel)
{
    float average = luminance(pixel);
    return vec4(average, average, average, 1.0);
}

vec4 redTint(vec4 pixel)
{
    pixel   = grayscale(pixel);
    pixel.r = pixel.r * 2.0;

    return pixel;
}

vec4 greenTint(vec4 pixel)
{
    pixel   = grayscale(pixel);
    pixel.g = pixel.g * 2.0;

    return pixel;
}

vec4 blueTint(vec4 pixel)
{
    pixel   = grayscale(pixel);
    pixel.b = pixel.b * 2.0;

    return pixel;
}

vec4 fixedValue(vec4 pixel)
{
    pixel.r = 1.0;
    pixel.g = 0.0;
    pixel.b = 0.0;

    return pixel;
}

vec3 RGB2YCC_REC2020(vec3 pixel)
{
    vec3 ret = vec3(0.0, 0.0, 0.0);

    ret = (pixel*(1023.0/1024.0));

    ret.r = pixel.r *  0.22495132 + pixel.g *  0.58057478 + pixel.b *  0.050778886;
    ret.g = pixel.r * -0.122295734 + pixel.g * -0.315631929 + pixel.b *  0.437927664;
    ret.b = pixel.r *  0.437927664 + pixel.g * -0.402705759 + pixel.b * -0.035221905;

    ret.r = ret.r + 0.0625;
    ret.g = ret.g + 0.5;
    ret.b = ret.b + 0.5;

    ret = (ret*(1024.0/1023.0));


    return ret;
}

vec3 YCC2RGB_REC2020(vec3 pixel)
{
    vec3 ret = vec3(0.0, 0.0, 0.0);

   ret = (pixel*(1023.0/1024.0));

    pixel.r = pixel.r - 0.0625;
    pixel.g = pixel.g - 0.5;
    pixel.b = pixel.b - 0.5;

    ret.r = pixel.r *  1.16780821827423 + pixel.g *  0.000000000000000 + pixel.b *  1.68361138229091;
    ret.g = pixel.r *  1.16780821849363 + pixel.g * -0.187877064364962 + pixel.b * -0.652337331549052;
    ret.b = pixel.r *  1.1678082157657 + pixel.g *  2.14807164979131 + pixel.b *  0.000000000000000;

    ret = (ret*(1024.0/1023.0));

    return ret;
}

void main()
{
    vec4 pixel = texture(uTexture,vec2(vTextureCoord.x, 1.0 - vTextureCoord.y));

    //vec4 pixel = texelFetch(uTexture, ivec2(gl_FragCoord.xy), 0);

    // LUT
    if (uLutSpec == 1)
    {
        const float LUT_SIZE_Y = 17.0;
        const float LUT_SIZE_U = 17.0;
        const float LUT_SIZE_V = 17.0;

        vec3 scale  = vec3((LUT_SIZE_Y - 1.0) / LUT_SIZE_Y, (LUT_SIZE_U - 1.0) / LUT_SIZE_U, (LUT_SIZE_V - 1.0) / LUT_SIZE_V);
        vec3 offset = vec3(0.5 / LUT_SIZE_Y, 0.5 / LUT_SIZE_U, 0.5 / LUT_SIZE_V);

        pixel.xyz *= scale;
        pixel.xyz += offset;

        pixel.rgb = texture(uLutTexture, pixel.rgb).rgb;
    }

//pixel.xyz = YCC2RGB_REC2020(pixel.rgb).xyz;
//
//pixel = vec4(1.0, 0.0, 0.0, 0.0);
//
//    pixel = fixedValue(pixel);
//pixel.xyz = RGB2YCC_REC2020(pixel.rgb).xyz;

    // RGB TO YUV COLOR SPACE CONVERSION
    if (uConvert == 1)
    {
        pixel.xyz = YCC2RGB_REC2020(pixel.rgb).xyz;
    }

//    pixel = vec4(1.0f - vTextureCoord.x, vTextureCoord.y, vTextureCoord.x, 1.0f);

    FragColor = pixel;
}
)glsl";

////////////////////////////////////////////////////////////////////////////////////////////////////
// FRAGMENT SHADER: output buffer RGB layout
////////////////////////////////////////////////////////////////////////////////////////////////////

static const GLchar *FRAGMENT_SHADER_SOURCE_RGB_OUT =
        R"glsl(#version 320 es

#extension GL_OES_EGL_image_external_essl3 : require
#extension GL_OES_EGL_image_external       : require
#extension GL_EXT_YUV_target               : require

precision highp float;
precision highp sampler3D;

out vec4 FragColor;
in  vec2 vTextureCoord;

uniform sampler2D uTexture;
uniform sampler3D uLutTexture;
uniform int       uLutSpec;
uniform int       uConvert;

float luminance(vec4 pixel)
{
    return pixel.r * 0.299 + pixel.g * 0.587 + pixel.b * 0.114;
}

vec4 saturate(float scalar, vec4 pixel)
{
    return scalar * pixel + (1.0 - scalar) * luminance(pixel);
}

vec4 grayscale(vec4 pixel)
{
    float average = luminance(pixel);
    return vec4(average, average, average, 1.0);
}

vec4 redTint(vec4 pixel)
{
    pixel   = grayscale(pixel);
    pixel.r = pixel.r * 2.0;

    return pixel;
}

vec4 greenTint(vec4 pixel)
{
    pixel   = grayscale(pixel);
    pixel.g = pixel.g * 2.0;

    return pixel;
}

vec4 blueTint(vec4 pixel)
{
    pixel   = grayscale(pixel);
    pixel.b = pixel.b * 2.0;

    return pixel;
}

vec4 fixedValue(vec4 pixel)
{
    pixel.r = 1.0;
    pixel.g = 0.0;
    pixel.b = 0.0;

    return pixel;
}

vec3 RGB2YCC_REC2020(vec3 pixel)
{
    vec3 ret = vec3(0.0, 0.0, 0.0);

    ret = (pixel*(1023.0/1024.0));

    ret.r = pixel.r *  0.22495132 + pixel.g *  0.58057478 + pixel.b *  0.050778886;
    ret.g = pixel.r * -0.122295734 + pixel.g * -0.315631929 + pixel.b *  0.437927664;
    ret.b = pixel.r *  0.437927664 + pixel.g * -0.402705759 + pixel.b * -0.035221905;

    ret.r = ret.r + 0.0625;
    ret.g = ret.g + 0.5;
    ret.b = ret.b + 0.5;

    ret = (ret*(1024.0/1023.0));


    return ret;
}

vec3 YCC2RGB_REC2020(vec3 pixel)
{
    vec3 ret = vec3(0.0, 0.0, 0.0);

   ret = (pixel*(1023.0/1024.0));

    pixel.r = pixel.r - 0.0625;
    pixel.g = pixel.g - 0.5;
    pixel.b = pixel.b - 0.5;

    ret.r = pixel.r *  1.16780821827423 + pixel.g *  0.000000000000000 + pixel.b *  1.68361138229091;
    ret.g = pixel.r *  1.16780821849363 + pixel.g * -0.187877064364962 + pixel.b * -0.652337331549052;
    ret.b = pixel.r *  1.1678082157657 + pixel.g *  2.14807164979131 + pixel.b *  0.000000000000000;

    ret = (ret*(1024.0/1023.0));

    return ret;
}

void main()
{
    vec4 pixel = texture(uTexture,vec2(vTextureCoord.x, 1.0 - vTextureCoord.y));

    //vec4 pixel = texelFetch(uTexture, ivec2(gl_FragCoord.xy), 0);

    // LUT
    if (uLutSpec == 1)
    {
        const float LUT_SIZE_Y = 17.0;
        const float LUT_SIZE_U = 17.0;
        const float LUT_SIZE_V = 17.0;

        vec3 scale  = vec3((LUT_SIZE_Y - 1.0) / LUT_SIZE_Y, (LUT_SIZE_U - 1.0) / LUT_SIZE_U, (LUT_SIZE_V - 1.0) / LUT_SIZE_V);
        vec3 offset = vec3(0.5 / LUT_SIZE_Y, 0.5 / LUT_SIZE_U, 0.5 / LUT_SIZE_V);

        pixel.xyz *= scale;
        pixel.xyz += offset;

        pixel.rgb = texture(uLutTexture, pixel.rgb).rgb;
    }

//pixel.xyz = YCC2RGB_REC2020(pixel.rgb).xyz;
//
//pixel = vec4(1.0, 0.0, 0.0, 0.0);
//
//    pixel = fixedValue(pixel);
//pixel.xyz = RGB2YCC_REC2020(pixel.rgb).xyz;

    // RGB TO YUV COLOR SPACE CONVERSION
    if (uConvert == 1)
    {
        pixel.xyz = YCC2RGB_REC2020(pixel.rgb).xyz;
    }

//    pixel = vec4(1.0f - vTextureCoord.x, vTextureCoord.y, vTextureCoord.x, 1.0f);

    FragColor = pixel;
}
)glsl";

////////////////////////////////////////////////////////////////////////////////////////////////////
// COPY FRAGMENT SHADER: output buffer RGB layout, simulation only
////////////////////////////////////////////////////////////////////////////////////////////////////

static const char *COPY_SHADER_SOURCE_RGB =
        R"glsl(#version 320 es

#extension GL_OES_EGL_image_external_essl3  : require
#extension GL_OES_EGL_image_external        : require
#extension GL_EXT_YUV_target                : require

precision highp float;

layout(location = 0) out vec4 FragColor;
                     in  vec2 vTextureCoord;

uniform samplerExternalOES uTexture;

void main()
{
    FragColor = texture(uTexture, vTextureCoord);
}
)glsl";

////////////////////////////////////////////////////////////////////////////////////////////////////
// COPY FRAGMENT SHADER: output buffer YUV layout, simulation only
////////////////////////////////////////////////////////////////////////////////////////////////////

static const char *COPY_SHADER_SOURCE_YUV =
        R"glsl(#version 320 es

#extension GL_OES_EGL_image_external_essl3  : require
#extension GL_OES_EGL_image_external        : require
#extension GL_EXT_YUV_target                : require

precision highp float;

layout(yuv) out vec4 FragColor;
in  vec2 vTextureCoord;

uniform sampler2D uTexture;

void main()
{
    FragColor  = texture(uTexture, vTextureCoord);
}
)glsl";

////////////////////////////////////////////////////////////////////////////////////////////////////
// VERTEX DATA
////////////////////////////////////////////////////////////////////////////////////////////////////

static const GLfloat VERTEX_POSITIONS[] = {
        -1.0f, 1.0f, 0.0f,                             // top left
        -1.0f, -1.0f, 0.0f,                             // bottom left
        1.0f, -1.0f, 0.0f,                             // bottom right
        1.0f, 1.0f, 0.0f,                             // top right
};

static const GLfloat VERTEX_TEXTURE_COORDINATES[] = {
        0.0f, 1.0f, 0.0f, 1.0f,                         // top left
        0.0f, 0.0f, 0.0f, 1.0f,                         // bottom left
        1.0f, 0.0f, 0.0f, 1.0f,                         // bottom right
        1.0f, 1.0f, 0.0f, 1.0f,                         // top right
};

static const GLushort VERTEX_INDICES[] = {
        0, 1, 2,                                        // triangle 1
        2, 3, 0,                                        // triangle 2
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// RENDERER
////////////////////////////////////////////////////////////////////////////////////////////////////

Simulation::Renderer::Renderer()
        : program(0) {
    LOGI("CTOR");
    CHECK_GL_ERROR;
}

Simulation::Renderer::~Renderer() {
    LOGI("Renderer DTOR");

    if (program) {
        glDeleteProgram(program);
        program = 0;
    }
}

void Simulation::Renderer::init(bool preview) {
    CompileAndLinkShader(program, preview ? FRAGMENT_SHADER_SOURCE_RGB_OUT : FRAGMENT_SHADER_SOURCE_YUV_OUT,
                         VERTEX_SHADER_SOURCE);
}

void Simulation::Renderer::setScreenSize(int width, int height) {
    mScreenHeight = height;
    mScreenWidth = width;
}

void Simulation::Renderer::render(GLuint textureId, bool convert, bool useLUT) const {
    LOGI("render");
    glUseProgram(program);
    CHECK_GL_ERROR;
    glViewport(0, 0, mScreenWidth, mScreenHeight);
    // Position VBO specification
    GLuint vboPosition;
    glGenBuffers(1, &vboPosition);
    glBindBuffer(GL_ARRAY_BUFFER, vboPosition);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX_POSITIONS), VERTEX_POSITIONS, GL_STATIC_DRAW);
    CHECK_GL_ERROR;

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    CHECK_GL_ERROR;

    // Texture coordinate VBO specification
    GLuint vboTextureCoord;
    glGenBuffers(1, &vboTextureCoord);
    glBindBuffer(GL_ARRAY_BUFFER, vboTextureCoord);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX_TEXTURE_COORDINATES), VERTEX_TEXTURE_COORDINATES,
                 GL_STATIC_DRAW);
    CHECK_GL_ERROR;

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);
    CHECK_GL_ERROR;

    // EBO binding
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(VERTEX_INDICES), VERTEX_INDICES, GL_STATIC_DRAW);
    CHECK_GL_ERROR;

    // Texture uniform
    GLint uTextureLoc = glGetUniformLocation(program, "uTexture");
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(uTextureLoc, 2);
    CHECK_GL_ERROR;

    // LUT texture uniform
    // todo remove
//    GLint uLutTextureLoc = glGetUniformLocation(program, "uLutTexture");
//    glActiveTexture(GL_TEXTURE3);
//    glBindTexture(GL_TEXTURE_3D, lutTextureId);
//    glUniform1i(uLutTextureLoc, 3);
//    CHECK_GL_ERROR;

    // Use LUT spec uniform
    GLint uLutSpecLoc = glGetUniformLocation(program, "uLutSpec");
    glUniform1i(uLutSpecLoc, useLUT ? 1 : 0);
    CHECK_GL_ERROR;

    // Color space conversion uniform
    GLint uConvertLoc = glGetUniformLocation(program, "uConvert");
    glUniform1i(uConvertLoc, convert ? 1 : 0);
    CHECK_GL_ERROR;

    LOGI("opengl: (program: %d, position vbo: %d, texture coord vbo: %d, ebo: %d, uniforms: (texture: %d, lut spec: %d, convert: %d))",
         program,
         vboPosition,
         vboTextureCoord,
         ebo,
         uTextureLoc,
         uLutSpecLoc,
         uConvertLoc);

    //std::this_thread::sleep_for(std::chrono::milliseconds(33));

    // loop_count = 0 disables timing, set > 1 for GPU timing, 100 recommended.
    ScopeTimerGPU scope_timer("Renderer::render: ", 0);
    { // Note scope limit for timer
        for (int i = 0; i < scope_timer.LoopCount(); i++) {
            // Draw call
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
            CHECK_GL_ERROR;
        }
    }
    CHECK_GL_ERROR;

    // Clean up
    glDeleteBuffers(1, &vboPosition);
    glDeleteBuffers(1, &vboTextureCoord);
    glDeleteBuffers(1, &ebo);
    CHECK_GL_ERROR;

    glUseProgram(0);
    CHECK_GL_ERROR;

    // Wait for opengl calls to finish
    glFinish();
    CHECK_GL_ERROR;
}


void Simulation::Renderer::render(GLuint textureId, GLuint lutTextureId, bool convert,
                                  bool useLUT) const {
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "render");

    glUseProgram(program);
    CHECK_GL_ERROR;

    // Position VBO specification
    GLuint vboPosition;
    glGenBuffers(1, &vboPosition);
    glBindBuffer(GL_ARRAY_BUFFER, vboPosition);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX_POSITIONS), VERTEX_POSITIONS, GL_STATIC_DRAW);
    CHECK_GL_ERROR;

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    CHECK_GL_ERROR;

    // Texture coordinate VBO specification
    GLuint vboTextureCoord;
    glGenBuffers(1, &vboTextureCoord);
    glBindBuffer(GL_ARRAY_BUFFER, vboTextureCoord);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX_TEXTURE_COORDINATES), VERTEX_TEXTURE_COORDINATES,
                 GL_STATIC_DRAW);
    CHECK_GL_ERROR;

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);
    CHECK_GL_ERROR;

    // EBO binding
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(VERTEX_INDICES), VERTEX_INDICES, GL_STATIC_DRAW);
    CHECK_GL_ERROR;

    // Texture uniform
    GLint uTextureLoc = glGetUniformLocation(program, "uTexture");
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(uTextureLoc, 2);
    CHECK_GL_ERROR;

    // LUT texture uniform
    GLint uLutTextureLoc = glGetUniformLocation(program, "uLutTexture");
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_3D, lutTextureId);
    glUniform1i(uLutTextureLoc, 3);
    CHECK_GL_ERROR;

    // Use LUT spec uniform
    GLint uLutSpecLoc = glGetUniformLocation(program, "uLutSpec");
    glUniform1i(uLutSpecLoc, useLUT ? 1 : 0);
    CHECK_GL_ERROR;

    // Color space conversion uniform
    GLint uConvertLoc = glGetUniformLocation(program, "uConvert");
    glUniform1i(uConvertLoc, convert ? 1 : 0);
    CHECK_GL_ERROR;

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG,
                        "opengl: (program: %d, position vbo: %d, texture coord vbo: %d, ebo: %d, uniforms: (texture: %d, lut texture: %d, lut spec: %d, convert: %d))",
                        program,
                        vboPosition,
                        vboTextureCoord,
                        ebo,
                        uTextureLoc,
                        uLutTextureLoc,
                        uLutSpecLoc,
                        uConvertLoc);

    //std::this_thread::sleep_for(std::chrono::milliseconds(33));

    // loop_count = 0 disables timing, set > 1 for GPU timing, 100 recommended.
    ScopeTimerGPU scope_timer("Renderer::render: ", 0);
    { // Note scope limit for timer
        for (int i = 0; i < scope_timer.LoopCount(); i++) {
            // Draw call
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
        }
    }
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    // Clean up
    glDeleteBuffers(1, &vboPosition);
    glDeleteBuffers(1, &vboTextureCoord);
    glDeleteBuffers(1, &ebo);

    glUseProgram(0);

    // Wait for opengl calls to finish
    glFinish();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// COPY RENDERER
////////////////////////////////////////////////////////////////////////////////////////////////////

Simulation::CopyRenderer::CopyRenderer()
        : program(0) {
    LOGI("CopyRenderer", "CTOR");
}

Simulation::CopyRenderer::CopyRenderer(std::shared_ptr<Context> ctx)
        : program(0), eglContext(ctx) {
    LOGI("CTOR");
}

Simulation::CopyRenderer::~CopyRenderer() {
    LOGI("CopyRenderer", "DTOR");

    if (program) {
        glDeleteProgram(program);
        program = 0;
    }
}

void Simulation::CopyRenderer::init(bool destBufferIsYUV) {
    LOGI("CopyRenderer", "init");

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &VERTEX_SHADER_SOURCE, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1,
                   (destBufferIsYUV ? &COPY_SHADER_SOURCE_YUV : &COPY_SHADER_SOURCE_RGB), nullptr);
    glCompileShader(fragmentShader);

    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Simulation::CopyRenderer::render(GLuint textureId) const {
    LOGI("CopyRenderer", "render");

    glUseProgram(program);

    // Position VBO specification
    GLuint vboPosition;
    glGenBuffers(1, &vboPosition);
    glBindBuffer(GL_ARRAY_BUFFER, vboPosition);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX_POSITIONS), VERTEX_POSITIONS, GL_STATIC_DRAW);
    CHECK_GL_ERROR;
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    CHECK_GL_ERROR;
    // Texture coordinate VBO specification
    GLuint vboTextureCoord;
    glGenBuffers(1, &vboTextureCoord);
    glBindBuffer(GL_ARRAY_BUFFER, vboTextureCoord);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX_TEXTURE_COORDINATES), VERTEX_TEXTURE_COORDINATES,
                 GL_STATIC_DRAW);
    CHECK_GL_ERROR;
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);

    // EBO binding
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(VERTEX_INDICES), VERTEX_INDICES, GL_STATIC_DRAW);
    CHECK_GL_ERROR;
    // Texture uniform
    GLint uTextureLoc = glGetUniformLocation(program, "uTexture");
    CHECK_GL_ERROR;
    glActiveTexture(GL_TEXTURE2);
    CHECK_GL_ERROR;
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);
    CHECK_GL_ERROR;
    glUniform1i(uTextureLoc, 2);

    LOGI("opengl: (program: %d, position vbo: %d, texture coord vbo: %d, ebo: %d, uniforms: (texture: %d))",
         program,
         vboPosition,
         vboTextureCoord,
         ebo,
         uTextureLoc);

    // Draw call
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    CHECK_GL_ERROR;

    // Clean up
    glDeleteBuffers(1, &vboPosition);
    glDeleteBuffers(1, &vboTextureCoord);
    glDeleteBuffers(1, &ebo);
    CHECK_GL_ERROR;
    // Wait for opengl calls to finish
    glFinish();
    LOGI("After Draw Elements!");
}
