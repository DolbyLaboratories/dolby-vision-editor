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

#ifndef EDIT_SHADERS_H
#define EDIT_SHADERS_H

#include <stdlib.h>

#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>

#include <android/log.h>

#include "Tools.h"

#ifndef LOG_TAG
#define LOG_TAG "EditShaders"
#endif // LOG_TAG

#include "hlg_lut_500_p3_33.h"

using namespace std;
using namespace std::chrono;

namespace Simulation
{
///////////////////////////////////////////////////////////////////////////////
// EditShadersUniformBlock

// Pay attention here!
// We're build data fields that have to *exactly* match the data in an OpenGL
// Uniform Block. Only floats are allowed because everything must align on 4-byte boundaries,
// and match both the enum herein and any Java enum used to generate matching indices.
// Parameters must *exactly match* those in the uniform declaration in the shader.
// No other non-static member values allowed!!!

#pragma pack(push)
#pragma pack(4)

class EditShadersUniformBlock
{
public:
    // Pay attention here!
    // Parameter Selector enum *must match* uniform block contents and any Java-declared enum.
    // On Java side, to get CONTRAST for example, use int param = EffectParameters.CONTRAST.ordinal(),
    // where EffectParameters is the name of the Java enum. Order must be identical with what's here.
    enum EffectParameters
    {
        GAIN = 0,
        OFFSET,
        CONTRAST,
        SATURATION,
        SHIFT_Y,
        SHIFT_U,
        SHIFT_V,
        OVERRIDE_Y,
        OVERRIDE_U,
        OVERRIDE_V,
        OVERRIDE_A,
        WIPER_LEFT,
        WIPER_RIGHT,
        WIPER_TOP,
        WIPER_BOTTOM,
        LUT_ENABLE,
        ZEBRA_ENABLE,
        GAMUT_ENABLE,
        COMPOSITOR_ENABLE,
        GAMUT_MAX_LUMA,
        GAMUT_MID_LUMA,
        GAMUT_MIN_LUMA,
        GAMUT_MAX_CHROMA,
        GAMUT_MID_CHROMA,
        GAMUT_MIN_CHROMA,
        GAMUT_MAX_RGB,
        GAMUT_MIN_RGB,
        GAMUT_MID_RGB,
        GAMMA_2020, // read only
        GAMMA_709, // read only
        GAMMA_709_INV, // read only
        HLG_RGB_UPSCALE, // read only
        HLG_PEAK_LEVEL,
        HLG_BLACK_LEVEL,
        HLG_GAMMA,
        HLG_INV_GAMMA, // read only
        HLG_INV_GAMMA_M1, // read only
        HLG_POW_NEG_INV_GAMMA, // read only
        HLG_BETA, // read only
        HLG_BETA_FRACTION, // read only
        INPUT_VIDEO_SCALE_X,
        INPUT_VIDEO_SCALE_Y,
        // Number of parameter selections-- *Must come last*
        // Can be used for used for UI matching check
        // Not contained in the uniform block data itself
        PARAMETER_COUNT
    };

    EditShadersUniformBlock();
    ~EditShadersUniformBlock(){};
    void SetDefaults();
    void SetEffectDefaults();
    void setTestEffectParams();
    float GetParameter(EffectParameters parameter);
    void SetParameter(EffectParameters parameter, float val);
    bool IsDifferent(const EditShadersUniformBlock &blk) const { return MemCompare(*this, blk) != 0; }
    void RecomputeRgb2HlgCoefficients();
    float *GetFloatPointer() { return (float *)this; };

    inline bool GlBufferData()
    {
        int size = sizeof(*this);
        glBufferData(GL_UNIFORM_BUFFER, size, this, GL_DYNAMIC_DRAW);
        return (CHECK_GL_ERROR);
    }

    // Pay attention here!
    // The OpenGL uniform block declaration must *exactly* match the uniform block member value layout below
    static const char mUniformBlock[]; // Note: must be static

    // Public effect controls
    // All parameters *must* be floats for indexed access
    // Note: IEEE floating point standard defines integer values within the range of the mantissa
    // as exact (such as 0.0f, 1.0f, etc.)
    // Float used instead of boolean for consistency of interface
    float gain; // RGB Digital gain
    float offset; // RGB Digital offset
    float contrast; // Luma contrast
    float saturation; // Chroma saturation

    float shiftY; // Shift luma Y
    float shiftU; // Shift chroma U
    float shiftV; // Shift chroma V

    float overrideY; // Override luma Y
    float overrideU; // Override chroma U
    float overrideV; // Override chroma V
    float overrideA; // Override alpha or key

    // Defines area to be processed as opposed to left original
    float wiperLeft;
    float wiperRight;
    float wiperTop;
    float wiperBottom;

    // Enable: == 0.0 means Off, != 0.0 means On
    float lutEnable;
    float zebraEnable;
    float gamutEnable;
    float compositorEnable;

    // Default SDR ranges
    float gamutLMaxLuma;
    float gamutLMidLuma;
    float gamutLMinLuma;
    float gamutLMaxChroma;
    float gamutLMidChroma;
    float gamutLMinChroma;
    float gamutLMaxRGB;
    float gamutLMidRGB;
    float gamutLMinRGB;

    // For HLG transform of text buffer
    float gamma2020;
    float gamma709;
    float gamma709inv;
    float hlgRgbUpScale;
    float hlgPeakLevel;
    float hlgBlackLevel;
    float hlgGamma;
    float hlgInvGamma;
    float hlgInvGammaMinus1;
    float hlgPowNegInvGamma;
    float hlgBeta;
    float hlgBetaFraction;

    // Decoder fudge factors
    float inputVideoScaleX;
    float inputVideoScaleY;
};

#pragma pack(pop)

///////////////////////////////////////////////////////////////////////////////
// EditShaders

#define TEX_NUM_INPUT       GL_TEXTURE1
#define TEX_NUM_COMPOSITING GL_TEXTURE5
#define TEX_NUM_LUT         GL_TEXTURE3
#define TEX_NUM_TRANSFER    GL_TEXTURE4

#define TEX_BINDING_INPUT       1
#define TEX_BINDING_COMPOSITING 5
#define TEX_BINDING_LUT         3
#define TEX_BINDING_TRANSFER    4

class EditShaders
{
public:
    // Must be set before shaders are compiled
    enum eColorStandard
    {
        eColorStandard10BitRec709 = 0,
        eColorStandard10BitRec2020,
        eColorStandardCount // Number of selections
    };
    // Must be set before shaders are compiled
    enum eColorSpace
    {
        eColorSpaceYUV = 0,
        eColorSpaceRGB,
        eColorSpaceCount // Number of selections
    };
    // Must be set before shaders are compiled
    enum eShaderEditMode
    {
        eShaderEditModePassThrough = 0, // No edit functions
        eShaderEditModeSinglePixel, // Edit function processes one pixel at a time
        eShaderEditModeFourPixels, // Edit function processes four pixels at a time
        eShaderEditModeCount // Number of selections
    };

    EditShaders();
    EditShaders(
        int output_width,
        int output_height,
        eColorStandard color_standard = eColorStandard10BitRec709,
        eColorSpace input_colorspace = eColorSpaceYUV,
        eColorSpace output_colorspace = eColorSpaceYUV);
    virtual ~EditShaders();
    void  Init(
        int output_width = 0,
        int output_height = 0,
        eColorStandard color_standard = eColorStandard10BitRec709,
        eColorSpace input_colorspace = eColorSpaceYUV,
        eColorSpace output_colorspace = eColorSpaceYUV);
    void ReleaseAll();
    bool BuildEditComputeShader();
    bool IsValidSetup() const { return ((mOutputWidth > 0) && (mOutputHeight > 0)); }
    int Width() const { return mOutputWidth; }
    int Height() const { return mOutputHeight; };
    void SetEffectDefaults();
    void EnableLut(bool enable = true);
    float GetParameter(EditShadersUniformBlock::EffectParameters parameter);
    void SetParameter(EditShadersUniformBlock::EffectParameters parameter, float val);
    void SetInputTransformMatrix(float transformMatrix[4][4]);
    void SetCompositorTransformMatrix(float transformMatrix[4][4]);
    bool SetCompositingImage(const char *data);
    bool CompositingImageTestPattern();
    bool RunEditComputeShader(
        GLuint input_texture_id,
        int input_texture_width = 0,
        int input_texture_height = 0);
    GLuint GetTransferTextureID() const { return mTransferTextureID ; }

protected:
    bool BuildCopyFragmentShader();
    bool FindUniforms(GLuint program);
    bool LoadUniforms(GLuint program, GLuint input_texture_id);
    std::string EditShaderPreamble(bool include_version = true);
    bool AssureLutLoaded();
    bool ReloadCompositingTextureIfNeeded();
    bool CreateTransferTexture(); // Used between compute shader and copy fragment shader

    // Pipeline mutex is used to arbitrate UI access to real-time GPU objects
    // such as uniform and texture data
    std::mutex mPipelineMutex;
    float mInputTransformMatrix[4][4];
    float mCompositorTransformMatrix[4][4]; // Not yet used
    vector<unsigned char> mTextTextureData;
    EditShadersUniformBlock mEditShadersUniformBlock;

    // Shader source texts
    static const char mEditLibrary[];
    static const char mEditLibraryQuad[];
    static const char mColorMatrixLibrary[];
    static const char mVersionAndExtensions[];
    static const char mEditComputeShaderDeclarations[];
    static const char mEditComputeShaderBody[];
    static const char mEditComputeShaderQuadBody[];
    static const char mVertexShaderBody[];

    int mOutputWidth = 0;
    int mOutputHeight = 0;
    int mWorkGroupSizeX = 8;
    int mWorkGroupSizeY = 8;

    eColorSpace mInputColorSpace = eColorSpaceYUV;
    eColorSpace mOutputColorSpace = eColorSpaceYUV;
    eColorStandard mColorStandard = eColorStandard10BitRec709;

    volatile bool mReloadCompositingTexture = true;
    int m3dLutSize = HLG_LUT_500_P3_33_SIZE;
    float *m3dLutData = hlg_lut_500_p3_33;

    // Compute shader configuration options
    int mComputeShaderQuadFactor = 4; // Compute shader body does: 1 = single pixel at a time, 4 = 4 pixels at a time
    eShaderEditMode mShaderEditMode = eShaderEditModeFourPixels; // Configures which edit function to use

    // OpenGL Programs
    GLuint mEditComputeProgram = GL_INVALID_VALUE;

    // OpenGL Textures
    GLuint mCompositingTextureID = GL_INVALID_VALUE;
    GLuint mTransferTextureID = GL_INVALID_VALUE;
    GLuint m3dLutTextureID = GL_INVALID_VALUE;

    // OpenGL Uniform handles
    // Handles are only indices and posess no resources and so do not need to be released
    GLuint mInputTextureHandle = GL_INVALID_VALUE;
    GLuint mCompositingTextureHandle = GL_INVALID_VALUE;
    GLuint m3dLutHandle = GL_INVALID_VALUE;
    GLuint mInputTransformHandle = GL_INVALID_VALUE;
    GLuint mCompositorTransformHandle = GL_INVALID_VALUE;
    GLuint mEditShadersUniformBlockIndex = GL_INVALID_VALUE;
    GLuint mTransferTextureHandle = GL_INVALID_VALUE;

    // For timing and testing
    double mAccumulator = 0.0;
    int mTimingLoops = 0; // Set > 1 for GPU timing, 100 recommended.
    int mFrameCounter = 0;
    bool mTestStrobe = false;
};

} // namespace dovi
#endif // EDIT_SHADERS_H
