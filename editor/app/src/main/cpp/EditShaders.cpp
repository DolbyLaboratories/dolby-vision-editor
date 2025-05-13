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
#define LOG_TAG "EditShaders"
//#define LOG_NDEBUG 0

#include <iostream>
#include <stdio.h>
#include <math.h>

#include "EditShaders.h"
#include "hlg_lut_1000_bt2020_pq_33.h"

using namespace std;

namespace Simulation
{
///////////////////////////////////////////////////////////////////////////////
// EditShadersUniformBlock

// Pay attention here!
// We're building data fields that have to exactly match the data in an OpenGL
// Uniform Block. Only floats and 32 bit ints are allowed because everything
// must align on 4-byte boundaries. Parameters must *exactly match* those
// in the uniform declaration in the shader.
// No other non-static member values allowed!!!

EditShadersUniformBlock::EditShadersUniformBlock()
{
    if((sizeof(*this) / 4) != PARAMETER_COUNT)
        LOGE("EditShadersUniformBlock: enum vs. uniform block mismatch!!");
    assert((sizeof(*this) / 4) == PARAMETER_COUNT);

    SetDefaults();
}

void EditShadersUniformBlock::SetDefaults()
{
    // Enable: == 0.0 means Off, != 0.0 means On
    MemClr(*this);

    // For HLG transform of compositing buffer
    gamma2020 = 1.2f;
    gamma709 = 2.2f;
    hlgRgbUpScale = 1.0f;
    hlgPeakLevel = 1.0f;
    hlgBlackLevel = 0.0001f;
    hlgGamma = 1.2f;
    RecomputeRgb2HlgCoefficients();

    SetEffectDefaults();
}

// Set HLG coefficients based on peak level
void EditShadersUniformBlock::RecomputeRgb2HlgCoefficients()
{
    if (hlgGamma == 0.0f)
    {
        if (hlgPeakLevel > 400.0f && hlgPeakLevel < 2000.0f)
            hlgGamma = (float)(1.2f + 0.42f * log((hlgPeakLevel / 1000.0f) / log(10.0f)));
        else
            hlgGamma = (float)pow(1.2f * 1.1110f, log((hlgPeakLevel / 1000.0f) / log(2.0f)));
    }

    hlgInvGamma       = 1.0f / hlgGamma;
    hlgPowNegInvGamma = (float)pow(hlgPeakLevel, -hlgInvGamma);
    hlgBeta           = (float)sqrt(3.0f * pow(hlgBlackLevel / hlgPeakLevel, hlgInvGamma));
    hlgBetaFraction   = 1.0f / (1.0f - hlgBeta);
    hlgInvGammaMinus1 = hlgInvGamma - 1.0f;
}

void EditShadersUniformBlock::SetEffectDefaults()
{
    // Enable: != 0.0 means On, == 0.0 means Off

    gain       = 1.0f; // RGB Digital gain
    offset     = 0.0f; // RGB Digital offset
    contrast   = 1.0f; // Luma contrast
    saturation = 1.0f; // Chroma saturation

    shiftY = 0.0f; // Shift luma Y
    shiftU = 0.0f; // Shift chroma U
    shiftV = 0.0f; // Shift chroma V

    overrideY = -1.0f; // Override luma Y
    overrideU = -1.0f; // Override chroma U
    overrideV = -1.0f; // Override chroma V
    overrideA = -1.0f; // Override alpha or key

    wiperLeft   = 0.0f;
    wiperRight  = 1.0f;
    wiperTop    = 0.0f;
    wiperBottom = 1.0f;

    lutEnable   = 0.0f;
    zebraEnable = 0.0f;
    gamutEnable = 0.0f;
    compositorEnable = 0.0f;

    gamutLMaxLuma   = 0.9216f;
    gamutLMidLuma   = 0.5f;
    gamutLMinLuma   = 0.0627f;
    gamutLMaxChroma = 0.9412f;
    gamutLMidChroma = 0.5f;
    gamutLMinChroma = 0.0627f;
    gamutLMaxRGB    = 1.0f;
    gamutLMidRGB    = 0.5f;
    gamutLMinRGB    = 0.0f;

    inputVideoScaleX = 1.0f;
    inputVideoScaleY = 1.0f;
}

void EditShadersUniformBlock::setTestEffectParams()
{
//    contrast = 3.0f; // Luma contrast
    saturation = 4.0f; // Chroma saturation
//    offset = 0.01f;

    // Only show processed center
    wiperLeft   = 0.1f;
    wiperRight  = 0.9f;
    wiperTop    = 0.1f;
    wiperBottom = 0.9f;

    compositorEnable = 1.0f;
    zebraEnable = 1.0f;
    gamutEnable = 1.0f;
}

float EditShadersUniformBlock::GetParameter(EffectParameters parameter)
{
    if (parameter < 0 || parameter > PARAMETER_COUNT)
    {
        LOGE("EditShadersUniformBlock::GetParameter: Illegal parameter: %d", parameter );
        return 0.0f;
    }
    if (parameter == PARAMETER_COUNT) return (float)PARAMETER_COUNT;
    return GetFloatPointer()[parameter];
}

void EditShadersUniformBlock::SetParameter(EffectParameters parameter, float val)
{
    if (parameter < 0 || parameter > PARAMETER_COUNT)
    {
        LOGE("EditShadersUniformBlock::SetParameter: Illegal parameter: %d", parameter);
        return;
    }

    // Weed out read-only parameters
    switch (parameter)
    {
        case GAMMA_2020:
        case GAMMA_709:
        case GAMMA_709_INV:
        case HLG_INV_GAMMA:
        case HLG_INV_GAMMA_M1:
        case HLG_POW_NEG_INV_GAMMA:
        case HLG_BETA:
        case HLG_BETA_FRACTION:
        case PARAMETER_COUNT:
        {
            LOGI("EditShaders::SetParameter: Read-only parameter: %d", parameter);
            return;
        }
        default: break;
    }

    // Set the selected value
    GetFloatPointer()[parameter] = val;

    // These parameters trigger RecomputeRgb2HlgCoefficients()
    switch (parameter)
    {
        case HLG_PEAK_LEVEL:
        case HLG_BLACK_LEVEL:
        case HLG_GAMMA:
        {
            RecomputeRgb2HlgCoefficients();
            break;
        }
        default: break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// EditShaders

static const float identityMatrix[4][4] =
{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

static const float halfSizeMatrix[4][4] =
{
    2.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 2.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 2.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

static const float twoThirdsSizeMatrix[4][4] =
{
    1.3f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.3f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.3f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

EditShaders::EditShaders()
{
    LOGI("EditShaders:: CTOR");
    Init();
}

EditShaders::EditShaders(
    int output_width,
    int output_height,
    eColorStandard color_standard,
    eColorSpace input_colorspace,
    eColorSpace output_colorspace)
{
    LOGI("EditShaders:: CTOR");
    Init(output_width, output_height, color_standard, input_colorspace, output_colorspace);
}

EditShaders::~EditShaders()
{
    LOGI("EditShaders:: DTOR");
    ReleaseAll();
}

void EditShaders::Init(
    int output_width,
    int output_height,
    eColorStandard color_standard,
    eColorSpace input_colorspace,
    eColorSpace output_colorspace)
{
    LOGI("EditShaders::Init output_width: %d, output_height: %d, color_standard: %d",
        output_width, output_height, color_standard);
    ReleaseAll();

    // Arbitrate UI control with uniform block and compositing texture access
    std::unique_lock<std::mutex> lock(mPipelineMutex);

    mOutputWidth  = output_width;
    mOutputHeight = output_height;

    mColorStandard = color_standard;
    mInputColorSpace = input_colorspace;
    mOutputColorSpace = output_colorspace;

    // Since pipeline is off, don't need to use mutex
    mEditShadersUniformBlock.SetDefaults();
    if (mTestStrobe)
        mEditShadersUniformBlock.setTestEffectParams(); // For test only

    // Set up the compositing image
    mTextTextureData.resize(mOutputWidth * mOutputHeight * 4);
    memset(mTextTextureData.data(), 0, mTextTextureData.size());
    if (mTestStrobe)
        CompositingImageTestPattern();
    mReloadCompositingTexture = true;

    m3dLutSize = LUT_33_SIZE;
    float *m3dLutData = hlg_lut_1000_bt2020_pq_33;
    CHECK_GL_ERROR;

#if 1
    MemCopy(mInputTransformMatrix, identityMatrix);
    MemCopy(mCompositorTransformMatrix, identityMatrix);
#else
    MemCopy(mInputTransformMatrix, twoThirdsSizeMatrix);
    MemCopy(mCompositorTransformMatrix, halfSizeMatrix);
#endif
    LOGI("EditShaders::Init done");
}

void EditShaders::ReleaseAll()
{
    // Arbitrate UI control with uniform block and compositing texture access
    std::unique_lock<std::mutex> lock(mPipelineMutex);

    glFinish();

    // Nuke the textures
    DeleteTexture(mCompositingTextureID);
    DeleteTexture(mTransferTextureID);
    DeleteTexture(m3dLutTextureID);

    // Nuke the programs
    DeleteProgram(mEditComputeProgram);

   // Nuke the compositing data
// The clear processing delay is too long while loading 8k content.
//    mTextTextureData.clear();
    mReloadCompositingTexture = true;
    mOutputWidth  = 0;
    mOutputHeight = 0;

    mFrameCounter = 0;

    // OpenGL Uniform handles
    // Handles are only indicies and control no actual resources, and so do not need to be released
    mCompositingTextureHandle = GL_INVALID_VALUE;
    mInputTextureHandle = GL_INVALID_VALUE;
    m3dLutHandle = GL_INVALID_VALUE;
    mInputTransformHandle = GL_INVALID_VALUE;
    mCompositorTransformHandle = GL_INVALID_VALUE;
    mEditShadersUniformBlockIndex = GL_INVALID_VALUE;
    mTransferTextureHandle = GL_INVALID_VALUE;
}

float EditShaders::GetParameter(EditShadersUniformBlock::EffectParameters parameter)
{
    // Arbitrate UI control with uniform block and compositing texture access
    std::unique_lock<std::mutex> lock(mPipelineMutex);
    return mEditShadersUniformBlock.GetParameter(parameter);
}

void EditShaders::SetParameter(EditShadersUniformBlock::EffectParameters parameter, float val)
{
    // Arbitrate UI control with uniform block and compositing texture access
    std::unique_lock<std::mutex> lock(mPipelineMutex);
    mEditShadersUniformBlock.SetParameter(parameter, val);
}

void EditShaders::EnableLut(bool enable)
{
    SetParameter(EditShadersUniformBlock::LUT_ENABLE, enable ? 1.0f : 0.0f);
}

void EditShaders::SetEffectDefaults()
{
    // Arbitrate UI control with uniform block and compositing texture access
    std::unique_lock<std::mutex> lock(mPipelineMutex);
    mEditShadersUniformBlock.SetEffectDefaults();
}

void EditShaders::SetInputTransformMatrix(float transformMatrix[4][4])
{
    // Arbitrate UI control with uniform block and compositing texture access
    std::unique_lock<std::mutex> lock(mPipelineMutex);
    memcpy(mInputTransformMatrix, transformMatrix, sizeof(mInputTransformMatrix));
};

void EditShaders::SetCompositorTransformMatrix(float transformMatrix[4][4])
{
    // Arbitrate UI control with uniform block and compositing texture access
    std::unique_lock<std::mutex> lock(mPipelineMutex);
    memcpy(mCompositorTransformMatrix, transformMatrix, sizeof(mCompositorTransformMatrix));
};

bool EditShaders::SetCompositingImage(const char *data)
{
    if (!IsValidSetup()) return true;
    // Arbitrate UI control with uniform block and compositing texture access
    std::unique_lock<std::mutex> lock(mPipelineMutex);
    memcpy(mTextTextureData.data(), data, mTextTextureData.size());
    mReloadCompositingTexture = true;
    return false;
}

bool EditShaders::CompositingImageTestPattern()
{
    LOGI("EditShaders::CompositingImageTestPattern: mOutputWidth = %d, mOutputHeight = %d", mOutputWidth, mOutputHeight);
    if (mOutputWidth == 0 || mOutputHeight == 0) return false; // No compositing texture

    int i = 0;
    for (int row = 0; row < mOutputHeight; row++)
    {
        float row_fade = (float)row / (float)(mOutputHeight - 1);
        for (int col = 0; col < mOutputWidth; col++)
        {
            float col_fade = (float)col / (float)(mOutputWidth - 1);
            mTextTextureData[i++] = (unsigned char) (255.0f * (1.0f - col_fade));
            mTextTextureData[i++] = (unsigned char) (255.0f * (1.0f - row_fade));
            mTextTextureData[i++] = (unsigned char) (255.0f * col_fade);
            mTextTextureData[i++] = (unsigned char) (255.0f * row_fade);
        }
    }
    mReloadCompositingTexture = true;
    return false;
}

bool EditShaders::ReloadCompositingTextureIfNeeded()
{
    LOGI("EditShaders::ReloadCompositingTextureIfNeeded: mOutputWidth = %d, mOutputHeight = %d", mOutputWidth, mOutputHeight);
    if (mOutputWidth == 0 || mOutputHeight == 0) return false; // No compositing texture

    // Compositing texture
    if (mReloadCompositingTexture)
    {
        LOGI("EditShaders::ReloadCompositingTextureIfNeeded: Set up Texture");
        if ((mCompositingTextureID == GL_INVALID_VALUE) && (mTextTextureData.size() > 0))
        {
            GLuint item[1];
            glGenTextures(1, item);
            if (CHECK_GL_ERROR) return true;
            mCompositingTextureID = item[0];
            glActiveTexture(TEX_NUM_COMPOSITING);
            if (CHECK_GL_ERROR) return true;
            glBindTexture(GL_TEXTURE_2D, mCompositingTextureID);
            if (CHECK_GL_ERROR) return true;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            if (CHECK_GL_ERROR) return true;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            if (CHECK_GL_ERROR) return true;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            if (CHECK_GL_ERROR) return true;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            if (CHECK_GL_ERROR) return true;
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mOutputWidth, mOutputHeight);
            if (CHECK_GL_ERROR) return true;
            glBindImageTexture(TEX_BINDING_COMPOSITING, mCompositingTextureID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
            if (CHECK_GL_ERROR) return true;
        }

        if (mTextTextureData.size() > 0)
        {
            unsigned char *ptr = mTextTextureData.data();

            glActiveTexture(TEX_NUM_COMPOSITING);
            if (CHECK_GL_ERROR) return true;
            glBindTexture(GL_TEXTURE_2D, mCompositingTextureID);
            if (CHECK_GL_ERROR) return true;
            LOGI("EditShaders::ReloadCompositingTextureIfNeeded: call glTexSubImage2D");
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mOutputWidth, mOutputHeight, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
            if (CHECK_GL_ERROR) return true;
            LOGI("EditShaders::ReloadCompositingTextureIfNeeded: mCompositingTextureID = %d", mCompositingTextureID);
            mReloadCompositingTexture = false;
        }
        else
        {
            LOGI("EditShaders::ReloadCompositingTextureIfNeeded: No Compositing text buffer");
        }
    }

    return false;
}

bool EditShaders::CreateTransferTexture()
{
    GLuint item[1];
    glGenTextures(1, item);
    if (CHECK_GL_ERROR) return true;
    mTransferTextureID = item[0];

    glActiveTexture(TEX_NUM_TRANSFER);
    if (CHECK_GL_ERROR) return true;
    glBindTexture(GL_TEXTURE_2D, mTransferTextureID);
    if (CHECK_GL_ERROR) return true;

    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, mOutputWidth, mOutputHeight);
    if (CHECK_GL_ERROR) return true;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    if (CHECK_GL_ERROR) return true;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (CHECK_GL_ERROR) return true;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    if (CHECK_GL_ERROR) return true;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (CHECK_GL_ERROR) return true;

    return false;
}

std::string EditShaders::EditShaderPreamble(bool include_version)
{
    char buffer[2048];

    std::string preamble;

    if (include_version) preamble += mVersionAndExtensions;

    if (mColorStandard == eColorStandard10BitRec2020)
        preamble += "#define INPUT_REC_2020_10_BIT\n#define EDIT_REC_2020\n#define OUTPUT_REC_2020_10_BIT\n";
    else if (mColorStandard == eColorStandard10BitRec709)
        preamble += "#define INPUT_REC_709\n#define EDIT_REC_709\n#define OUTPUT_REC_709\n";

    if (mInputColorSpace == eColorSpaceYUV)
        preamble += "#define INPUT_CSC_YUV\n";
    else if (mInputColorSpace == eColorSpaceRGB)
        preamble += "#define INPUT_CSC_RGB\n";

    if (mOutputColorSpace == eColorSpaceYUV)
        preamble += "#define OUTPUT_CSC_YUV\n";
    else if (mOutputColorSpace == eColorSpaceRGB)
        preamble += "#define OUTPUT_CSC_RGB\n";

    snprintf(
        buffer,
        SafeSize(buffer),
        "#define LUT_SIZE %d.0f\n",
        m3dLutSize);
    preamble += buffer;

    snprintf(
        buffer,
        SafeSize(buffer),
        "#define OUTPUT_WIDTH %d.0f\n#define OUTPUT_HEIGHT %d.0f\n",
        mOutputWidth,
        mOutputHeight);
    preamble += buffer;

    snprintf(
        buffer,
        SafeSize(buffer),
        "#define WORK_GROUP_SIZE_X %d\n#define WORK_GROUP_SIZE_Y %d\n\n",
        mWorkGroupSizeX,
        mWorkGroupSizeY);
    preamble += buffer;
    snprintf(
        buffer,
        SafeSize(buffer),
        "layout(local_size_x = %d, local_size_y = %d) in;\n\n",
        mWorkGroupSizeX,
        mWorkGroupSizeY);
    preamble += buffer;

    return preamble;
}

bool EditShaders::BuildEditComputeShader()
{
    DeleteProgram(mEditComputeProgram);

    if ((mOutputWidth == 0) || (mOutputHeight == 0))
    {
        LOGE("EditShaders::BuildEditComputeShader: Image size undefined");
        return true;
    }

    std::string shader_code = EditShaderPreamble();
    shader_code += mEditComputeShaderDeclarations;
    shader_code += mEditShadersUniformBlock.mUniformBlock;
    shader_code += mColorMatrixLibrary;

    if (mComputeShaderQuadFactor == 4)
    {
        if (mShaderEditMode == eShaderEditModeFourPixels)
            shader_code += mEditLibraryQuad; // Four pixels at a time
        else if (mShaderEditMode == eShaderEditModeSinglePixel)
            shader_code += mEditLibrary; // Single pixel at a time
        shader_code += mEditComputeShaderQuadBody;
    }
    else
    {
        mComputeShaderQuadFactor = 1; // Force legal value
        if (mShaderEditMode != eShaderEditModePassThrough)
            shader_code += mEditLibrary; // Single pixel at a time
        shader_code += mEditComputeShaderBody;
    }

    if (CompileAndLinkShader(mEditComputeProgram, shader_code.c_str())) return true;
    if (AssureLutLoaded()) return true;
    FindUniforms(mEditComputeProgram);

    // Transfer texture
    CreateTransferTexture();
    mTransferTextureHandle = glGetUniformLocation(mEditComputeProgram, "transferTexture");
    if (CHECK_GL_ERROR) return true;
    LOGI("EditShaders::BuildEditComputeShader: mTransferTextureHandle = %d", mTransferTextureHandle);

    return ReloadCompositingTextureIfNeeded();
}

bool EditShaders::FindUniforms(GLuint program)
{
    LOGI("EditShaders::FindUniforms: program = %d", program);

    glUseProgram(program);
    if (CHECK_GL_ERROR) return true;

    // Get input texture handle
    mInputTextureHandle = glGetUniformLocation(program, "inputTexture");
    if (CHECK_GL_ERROR) return true;

    // Get text compositing texture handle
    mCompositingTextureHandle = glGetUniformLocation(program, "compositingTexture");
    if (CHECK_GL_ERROR) return true;

    // Get handles for input  and compositor transform matrices
    mInputTransformHandle = glGetUniformLocation(program, "inputTextureTransform");
    if (CHECK_GL_ERROR) return true;
    mCompositorTransformHandle = glGetUniformLocation(program, "compositorTextureTransform");
    if (CHECK_GL_ERROR) return true;

    //  Get handle for 3D LUT
    m3dLutHandle = glGetUniformLocation(program, "lut3dtex");
    if (CHECK_GL_ERROR) return true;

    //  Get Uniform for Block index
    mEditShadersUniformBlockIndex = glGetUniformBlockIndex(program, "EditShadersUniformBlock");
    if (CHECK_GL_ERROR) return true;

    LOGI("mInputTextureHandle: 0x%X", mInputTextureHandle);
    LOGI("mCompositingTextureHandle: 0x%X", mCompositingTextureHandle);
    LOGI("mInputTransformHandle: 0x%X", mInputTransformHandle);
    LOGI("mCompositorTransformHandle: 0x%X", mCompositorTransformHandle);
    LOGI("m3dLutHandle: 0x%X", m3dLutHandle);
    LOGI("mEditShadersUniformBlockIndex: 0x%X", mEditShadersUniformBlockIndex);

    return false;
}

bool EditShaders::LoadUniforms(GLuint program, GLuint input_texture_id)
{
    LOGI("EditShaders::LoadUniforms: program = %d", program);

    glUseProgram(program);
    if (CHECK_GL_ERROR) return true;

    // Input texture
    glActiveTexture(TEX_NUM_INPUT);
    if (CHECK_GL_ERROR) return true;
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, input_texture_id);
    if (CHECK_GL_ERROR) return true;
    glUniform1i(mInputTextureHandle, TEX_BINDING_INPUT);
    if (CHECK_GL_ERROR) return true;

    // 3D LUT
    glActiveTexture(TEX_NUM_LUT);
    if (CHECK_GL_ERROR) return true;
    glBindTexture(GL_TEXTURE_3D, m3dLutTextureID);
    if (CHECK_GL_ERROR) return true;
    glUniform1i(m3dLutHandle, TEX_BINDING_LUT);
    if (CHECK_GL_ERROR) return true;

    // Transfer texture
    glActiveTexture(TEX_NUM_TRANSFER);
    if (CHECK_GL_ERROR) return true;
    glBindImageTexture(TEX_BINDING_TRANSFER, mTransferTextureID, 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);
    if (CHECK_GL_ERROR) return true;

    // Transform matrices
    glUniformMatrix4fv(mInputTransformHandle, 1, false, mInputTransformMatrix[0]);
    if (CHECK_GL_ERROR) return true;
    glUniformMatrix4fv(mCompositorTransformHandle, 1, false, mCompositorTransformMatrix[0]);
    if (CHECK_GL_ERROR) return true;

    return false;
}

bool EditShaders::AssureLutLoaded()
{
    LOGI("EditShaders::AssureLutLoaded");

    if (m3dLutTextureID != GL_INVALID_VALUE)
        return false; // Already loaded

    GLuint item[1];
    glGenTextures(1, item);
    if (CHECK_GL_ERROR) return true;
    m3dLutTextureID = item[0];

    glActiveTexture(TEX_NUM_LUT);
    if (CHECK_GL_ERROR) return true;

    glBindTexture(GL_TEXTURE_3D, m3dLutTextureID);
    if (CHECK_GL_ERROR) return true;

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    if (CHECK_GL_ERROR) return true;
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (CHECK_GL_ERROR) return true;
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    if (CHECK_GL_ERROR) return true;
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (CHECK_GL_ERROR) return true;
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    if (CHECK_GL_ERROR) return true;

    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGB32F, m3dLutSize, m3dLutSize, m3dLutSize);
    if (CHECK_GL_ERROR) return true;

    glActiveTexture(TEX_NUM_LUT);
    if (CHECK_GL_ERROR) return true;
    glBindTexture(GL_TEXTURE_3D, m3dLutTextureID);
    if (CHECK_GL_ERROR) return true;

    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, m3dLutSize, m3dLutSize, m3dLutSize, GL_RGB, GL_FLOAT, m3dLutData);
    if (CHECK_GL_ERROR) return true;

    LOGI("EditShaders::AssureLutLoaded: m3dLutTextureID = %d", m3dLutTextureID);

    return false;
}

bool EditShaders::RunEditComputeShader(
        GLuint input_texture_id,
        int input_texture_width,
        int input_texture_height)
{
    if (!IsValidSetup())
    {
        LOGE("EditShaders::RunEditComputeShader: Invalid setup, output_width: %d, output_height: %d", mOutputWidth, mOutputHeight);
        return true;
    }
    if (mEditComputeProgram == GL_INVALID_VALUE)
    {
        LOGE("EditShaders::RunEditComputeShader: Invalid program ID, output_width: %d, output_height: %d", mOutputWidth, mOutputHeight);
        return true;
    }
    LOGI("EditShaders::RunEditComputeShader: input_texture_id = %d, output_width: %d, output_height: %d,input_texture_width: %d, input_texture_height: %d, lutEnable: %g",
         input_texture_id, mOutputWidth, mOutputHeight, input_texture_width, input_texture_height, mEditShadersUniformBlock.lutEnable);

    // Arbitrate UI control with uniform block and compositing texture access
    std::unique_lock<std::mutex> lock(mPipelineMutex);

    // Lambda cleanup function to sweep up the floor after the party is over no matter how we leave
    GLuint binding_point = 0, buffer = GL_INVALID_VALUE;
    ScopeExitFunction cleanup([&]()
    {
        DeleteBuffer(buffer);
        glUseProgram(0);
    });

    // Is the input texture larger than the active image size?
    // If so, adjust for the shader's sampler coordinate system
    if (input_texture_width == 0) input_texture_width = mOutputWidth;
    if (input_texture_height == 0) input_texture_height = mOutputHeight;
    mEditShadersUniformBlock.inputVideoScaleX = std::min(1.0f, (float)mOutputWidth / (float)input_texture_width);
    mEditShadersUniformBlock.inputVideoScaleY = std::min(1.0f, (float)mOutputHeight / (float)input_texture_height);

    if (mTestStrobe)
    {
        mEditShadersUniformBlock.gamutEnable = (float)(mFrameCounter % 60 >= 30);
        mEditShadersUniformBlock.zebraEnable = (float)(mFrameCounter % 120 >= 60);
        mEditShadersUniformBlock.compositorEnable = (float)(mFrameCounter % 240 >= 120);
    }

    glUseProgram(mEditComputeProgram);
    if (CHECK_GL_ERROR) return true;

    if (ReloadCompositingTextureIfNeeded()) return true;
    if (LoadUniforms(mEditComputeProgram, input_texture_id)) return true;

    glActiveTexture(TEX_NUM_TRANSFER);
    if (CHECK_GL_ERROR) return true;
    glBindImageTexture(TEX_BINDING_TRANSFER, mTransferTextureID, 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);
    if (CHECK_GL_ERROR) return true;

    // Uniform Block
    glUniformBlockBinding(mEditComputeProgram, mEditShadersUniformBlockIndex, binding_point);
    if (CHECK_GL_ERROR) return true;
    glGenBuffers(1, &buffer);
    if (CHECK_GL_ERROR) return true;
    glBindBuffer(GL_UNIFORM_BUFFER, buffer);
    if (CHECK_GL_ERROR) return true;
    glBufferData(GL_UNIFORM_BUFFER, sizeof(mEditShadersUniformBlock), &mEditShadersUniformBlock, GL_DYNAMIC_DRAW);
    if (CHECK_GL_ERROR) return true;
    glBindBufferBase(GL_UNIFORM_BUFFER, mEditShadersUniformBlockIndex, buffer);
    if (CHECK_GL_ERROR) return true;

    ScopeTimerGPU scope_timer("RunEditComputeShader ", mTimingLoops, &mAccumulator);
    { // Note scope limit for timer
        for (int i = 0; i < scope_timer.LoopCount(); i++)
        {
            // Note when we're processing four pixels per GPU core
            glDispatchCompute(DivUp(Width(), mWorkGroupSizeX * mComputeShaderQuadFactor), DivUp(Height(), mWorkGroupSizeY), 1);
            if (CHECK_GL_ERROR) return true;
        }
    }
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    mFrameCounter++;
    return false;
}

} // namespace Simulation
