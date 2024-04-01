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
#define LOG_TAG "EditShadersText"
#endif // LOG_TAG

#include "EditShaders.h"

using namespace std;

namespace Simulation
{
///////////////////////////////////////////////////////////////////////////////
// EditShadersUniformBlock

// Pay attention here!
// Must *exactly* match the member data declaration in the class
const char EditShadersUniformBlock::mUniformBlock[] =
    R"(
uniform EditShadersUniformBlock
{
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
} editKnobs;

#define U_GAIN       editKnobs.gain
#define U_OFFSET     editKnobs.offset
#define U_CONTRAST   editKnobs.contrast
#define U_SATURATION editKnobs.saturation

#define U_LUT_ENABLE        editKnobs.lutEnable
#define U_ZEBRA_ENABLE      editKnobs.zebraEnable
#define U_GAMUT_ENABLE      editKnobs.gamutEnable
#define U_COMPOSITOR_ENABLE editKnobs.compositorEnable

#define U_GAMUT_MAX_LUMA editKnobs.gamutLMaxLuma
#define U_GAMUT_MID_LUMA editKnobs.gamutLMidLuma
#define U_GAMUT_MIN_LUMA editKnobs.gamutLMinLuma

#define U_GAMUT_MAX_CHROMA editKnobs.gamutLMaxChroma
#define U_GAMUT_MID_CHROMA editKnobs.gamutLMidChroma
#define U_GAMUT_MIN_CHROMA editKnobs.gamutLMinChroma

#define U_GAMUT_MAX_RGB editKnobs.gamutLMaxRGB
#define U_GAMUT_MID_RGB editKnobs.gamutLMidRGB
#define U_GAMUT_MIN_RGB editKnobs.gamutLMinRGB

#define U_SHIFT_Y editKnobs.shiftY
#define U_SHIFT_U editKnobs.shiftU
#define U_SHIFT_V editKnobs.shiftV
#define U_SHIFT_YUV vec3(editKnobs.shiftY, editKnobs.shiftU, editKnobs.shiftV)

#define U_OVERRIDE_Y editKnobs.overrideY
#define U_OVERRIDE_U editKnobs.overrideU
#define U_OVERRIDE_V editKnobs.overrideV
#define U_OVERRIDE_A editKnobs.overrideA

#define U_WIPER_LEFT   editKnobs.wiperLeft
#define U_WIPER_RIGHT  editKnobs.wiperRight
#define U_WIPER_TOP    editKnobs.wiperTop
#define U_WIPER_BOTTOM editKnobs.wiperBottom

#define U_GAMMA_2020            editKnobs.gamma2020
#define U_GAMMA_709             editKnobs.gamma709
#define U_GAMMA_709_INV         editKnobs.gamma709inv
#define U_HLG_RGB_UP_SCALE      editKnobs.hlgRgbUpScale
#define U_HLG_PEAK_LEVEL        editKnobs.hlgPeakLevel
#define U_HLG_BLACK_LEVEL       editKnobs.hlgBlackLevel
#define U_HLG_GAMMA             editKnobs.hlgGamma
#define U_HLG_INV_GAMMA         editKnobs.hlgInvGamma
#define U_HLG_INV_GAMMA_M1      editKnobs.hlgInvGammaMinus1
#define U_HLG_POW_NEG_INV_GAMMA editKnobs.hlgPowNegInvGamma
#define U_HLG_BETA              editKnobs.hlgBeta
#define U_HLG_BETA_FRACTION     editKnobs.hlgBetaFraction

#define U_INPUT_VIDEO_SCALE_X  editKnobs.inputVideoScaleX
#define U_INPUT_VIDEO_SCALE_Y  editKnobs.inputVideoScaleY
#define U_INPUT_VIDEO_SCALE_XY vec2(editKnobs.inputVideoScaleX, editKnobs.inputVideoScaleY)
)";

///////////////////////////////////////////////////////////////////////////////
// EditShaders

const char EditShaders::mColorMatrixLibrary[] =
    R"(
//////////////////////////////////////////////
// Colorspace matrices

// Note that because OpenGL is column major the matrices are transposed.

const mat3 rec2020Ycc2Rgb10bit = mat3(
    1.167808219178082,  1.167808219178082,  1.167808219178082,
    0.0,               -0.187882693133655,  2.148067732257436,
    1.683610899804629, -0.652339525586461,  0.0);
const mat3 rec2020Rgb2Ycc10bit = mat3(
    0.224951501194399, -0.122296056156102,  0.437927663734115,
    0.580573128690646, -0.315631607578013, -0.402704729220717,
    0.050780355452199,  0.437927663734115, -0.035222934513398);

const mat3 rec2020Ycc2Rgb8bit = mat3(
    1.164383561643836,  1.164383561643835,  1.164383561643836,
    0.0,               -0.187331717493967,  2.141768413394511,
    1.678673624438633, -0.650426506449844,  0.0);
const mat3 rec2020Rgb2Ycc8bit = mat3(
    0.650426506449844, -0.122655750438915,  0.439215686274510,
    0.582280696716207, -0.316559935835595, -0.403889154894896,
    0.050929709438823,  0.439215686274510, -0.035326531379614);

const mat3 rec2020Ycc2Rgb = mat3(
    1.0,                1.0,                1.0,
    0.0,               -0.164558057720190,  1.881396567060276,
    1.474599575977466, -0.571355048803000,  0.0);
const mat3 rec2020Rgb2Ycc = mat3(
    0.262700212011267, -0.139630430187157,  0.5,
    0.677998071518871, -0.360369569812843, -0.459784529009814,
    0.059301716469862,  0.5,               -0.040215470990186);

const mat3 rec709Ycc2Rgb8bit = mat3(
    1.164383561643836,  1.164383561643835,  1.164383561643836,
    0.0,               -0.213237021568621,  2.112419281991186,
    1.792652263417544, -0.533004040142264,  0.0);
const mat3 rec709Rgb2Ycc8bit = mat3(
    0.182619381513179, -0.100661363813662,  0.439215686274510,
    0.614203688824073, -0.338554322460848, -0.398944454182289,
    0.062000459074513,  0.439215686274510, -0.040271232092221);

const mat3 rec709Ycc2Rgb = mat3(
    1.0,                1.0,                1.0,
    0.0,               -0.187314089534789,  1.855615369278533,
    1.574721988256979, -0.468207470556342,  0.0);
const mat3 rec709Rgb2Ycc = mat3(
    0.212639005871510, -0.114592177555732,  0.5,
    0.715168678767756, -0.385407822444268, -0.454155517037873,
    0.072192315360734,  0.5,               -0.045844482962127);

// Forward only required for compositor in rec2020 mode
const mat3 RgbRec709xyzRec2020 = mat3(
    0.627403895934699,  0.069097289358232,  0.016391438875150,
    0.329283038377884,  0.919540395075459,  0.088013307877226,
    0.043313065687417,  0.011362315566309,  0.895595253247624);

//////////////////////////////////////////////
// Matrix tools

vec4 ColorSpaceConversion(mat3 csc, vec4 pixel)
{
    pixel.rgb = csc * pixel.rgb;
    return pixel;
}

// Note that we're using a mat4 as a convenient container to
// hold four pixels at a time. Since OpenGL is column major
// that means the (color) components are adjacent forming nice
// vec4 vectors, e.g., mtx[0] is a vec4 of the red (or x)
// components for the four pixels. This makes it easy to toss
// four pixels around as inputs and output to/from functions.

mat4 ColorSpaceConversion(mat3 csc, mat4 pixels)
{
    mat4 result;
    result[0] = fma(vec4(csc[0][0]), pixels[0], fma(vec4(csc[1][0]), pixels[1], vec4(csc[2][0]) * pixels[2]));
    result[1] = fma(vec4(csc[0][1]), pixels[0], fma(vec4(csc[1][1]), pixels[1], vec4(csc[2][1]) * pixels[2]));
    result[2] = fma(vec4(csc[0][2]), pixels[0], fma(vec4(csc[1][2]), pixels[1], vec4(csc[2][2]) * pixels[2]));
    result[3] = pixels[3];
    return result;
}

)";

const char EditShaders::mEditLibrary[] =
    R"(
#define SINGLE_EDIT // Single pixel at at time version

//////////////////////////////////////////////
// Color space tools

vec3 sRGB2L(vec3 pixel)
{
    for (int i = 0; i < 3; i++)
    {
        if(pixel[i] <= 0.040448236277108f)
            pixel[i] /= 12.92f;
        else
            pixel[i] = pow(((pixel[i] + 0.055f) / 1.055f), U_GAMMA_709);
    }
    pixel *= vec3(U_HLG_RGB_UP_SCALE);
    return pixel;
}

#define HLG_A (0.17883277f)
#define HLG_B (1.0f - (4.0f * HLG_A))
#define HLG_C (0.5f - HLG_A * log(4.0f * HLG_A))
#define HLG_EPSILON (0.0001f) // Could be different between mediump and highp

vec3 HLGOETF(vec3 pixel)
{
    pixel = max(pixel, 0.0f);
    for (int i = 0; i < 3; i++)
    {
        if(pixel[i] <= (1.0f / 12.0f))
            pixel[i] = sqrt(pixel[i] * 3.0f);
        else
            pixel[i] = HLG_A * log(max(HLG_EPSILON, 12.0f * pixel[i] - HLG_B)) + HLG_C;
    }
    return pixel;
}

vec3 L2HLG(vec3 pixel)
{
    float Ys = 0.0f;
    float Yd = dot(pixel, vec3(0.2627f, 0.6780f, 0.0593f));
    if (Yd > 0.0f)
        Ys = pow(Yd, U_HLG_INV_GAMMA_M1);
    vec3 Ls = pixel * vec3(Ys) * vec3(U_HLG_POW_NEG_INV_GAMMA);
    return (HLGOETF(Ls) - U_HLG_BETA) * U_HLG_BETA_FRACTION;
}

const vec3 narrowRangeOffset = vec3(0.065f, 0.5f, 0.5f);
const vec3 narrowRangeScaleFactor = vec3(1.167808f, 1.141741f, 1.141741f); // Baked into the matrix transfomations

vec3 icsc(vec3 pixel)
{
#ifdef INPUT_CSC_YUV
#ifdef INPUT_REC_2020_10_BIT
    pixel = rec2020Ycc2Rgb10bit * (pixel - narrowRangeOffset);
#elif defined INPUT_REC_709
    pixel = rec709Ycc2Rgb8bit * (pixel - narrowRangeOffset);
#endif
#endif // INPUT_CSC_YUV
    return pixel;
}

#define LUT_SCALE vec3((LUT_SIZE-1.0f)/LUT_SIZE,(LUT_SIZE-1.0f)/LUT_SIZE,(LUT_SIZE-1.0f)/LUT_SIZE)
#define LUT_OFFSET vec3(0.5/LUT_SIZE,0.5/LUT_SIZE,0.5/LUT_SIZE)

vec3 ocsc(vec3 pixel)
{
    if (U_LUT_ENABLE >= 0.5f)
    {
        pixel = texture(lut3dtex, fma(pixel, LUT_SCALE, LUT_OFFSET)).rgb;
    }

#ifdef OUTPUT_CSC_YUV
#ifdef OUTPUT_REC_2020_10_BIT
    pixel = (rec2020Rgb2Ycc10bit * pixel) + narrowRangeOffset;
#elif defined OUTPUT_REC_709
    pixel = (rec709Rgb2Ycc8bit * pixel) + narrowRangeOffset;
#endif // !defined OUTPUT_REC_709
#endif // OUTPUT_CSC_YUV

    return pixel;
}

vec3 editYUV(vec3 pixel)
{
#ifdef EDIT_REC_2020
    pixel = (rec2020Rgb2Ycc * pixel) + narrowRangeOffset;
#else // !EDIT_REC_2020
    pixel = (rec709Rgb2Ycc * pixel) + narrowRangeOffset;
#endif // !EDIT_REC_2020
    return pixel;
}

vec3 editRGB(vec3 pixel)
{
#ifdef EDIT_REC_2020
    pixel = rec2020Ycc2Rgb * (pixel - narrowRangeOffset);
#else // !EDIT_REC_2020
    pixel = rec709Ycc2Rgb * (pixel - narrowRangeOffset);
#endif // !EDIT_REC_2020
    return pixel;
}

vec3 Gamut(vec3 yuvColor)
{
    // Gamut restriction
    // Find worst overshoot in RGB and calculate ratio to reduce saturation

    yuvColor.x = max(min(yuvColor.x, U_GAMUT_MAX_LUMA), U_GAMUT_MIN_LUMA);
    vec3 rgbTest = editRGB(yuvColor);
    float luma = (yuvColor.x - U_GAMUT_MID_LUMA) / (U_GAMUT_MAX_LUMA - U_GAMUT_MID_LUMA);
    float headroom = U_GAMUT_MAX_LUMA - luma;
    vec3 rgbRatios = vec3(min(U_GAMUT_MAX_LUMA, headroom)) / max(vec3(0.0001f), rgbTest - vec3(luma));
    float ratio = min(1.0, max(0.0, min(rgbRatios.r, min(rgbRatios.g, rgbRatios.b))));
    luma = (U_GAMUT_MID_LUMA - yuvColor.x) / (U_GAMUT_MID_LUMA - U_GAMUT_MIN_LUMA);
    headroom = luma - U_GAMUT_MIN_LUMA;
    rgbRatios = vec3(max(U_GAMUT_MIN_LUMA, headroom)) / max(vec3(0.0001f), vec3(luma) - rgbTest);
    ratio = min(1.0, max(0.0, min(rgbRatios.r, min(rgbRatios.g, min(rgbRatios.b, ratio)))));
    yuvColor.yz = ((yuvColor.yz - vec2(U_GAMUT_MID_CHROMA)) * vec2(ratio)) + vec2(U_GAMUT_MID_CHROMA);
    return yuvColor;
}

vec3 Zebra(vec3 pixel, vec2 fCoords)
{
    float modulo = mod((fCoords.x - fCoords.y) / 8.0, 2.0) - 1.0;
    bool stripe = modulo >= 0.0;
    if (stripe)
    {
        vec3 yuv = editYUV(pixel);
        if (yuv.x > U_GAMUT_MAX_LUMA)
            pixel.rgb = vec3(1.0, 0.0, 0.0);
        else if (yuv.x < U_GAMUT_MIN_LUMA)
            pixel.rgb = vec3(0.0, 0.0, 1.0);
        else if (any(greaterThan(pixel.rgb, vec3(U_GAMUT_MAX_RGB))) || any(lessThan(pixel.rgb, vec3(U_GAMUT_MIN_RGB))))
            pixel.rgb = vec3(0.0, 1.0, 0.0);
    }
    return pixel;
}

vec2 Transform(mat4 matrix, vec2 coords)
{
    vec4 result = matrix * vec4(coords.x, coords.y, 0.0f, 0.0f);
    return result.xy;
}

//////////////////////////////////////////////
// Edit Function

// Single pixel at at time version

vec4 editPixel(vec4 originalPixel, vec4 compositePixel, vec2 vidCoords, vec2 fCoords)
{
    // Input YUV? convert to RGB
    originalPixel.rgb = icsc(originalPixel.xyz);

#ifdef EDIT_REC_2020
    // Convert from Rec709 RGB to Rec 2020 RGB
    vec3 linear;
#if 1
    linear = sRGB2L(compositePixel.rgb);
    compositePixel.rgb = L2HLG(RgbRec709xyzRec2020 * linear);
#else // For testing
    linear = pow(compositePixel.rgb, vec3(U_GAMMA_709_INV));
    compositePixel.rgb = pow(RgbRec709xyzRec2020 * linear, vec3(U_GAMMA_2020));
#endif
#endif // EDIT_REC_2020

    // Perform RGB gain and offset
    vec4 texColor = originalPixel;
    texColor.rgb *= vec3(U_GAIN); // Digital gain
    texColor.rgb += vec3(U_OFFSET); // Digital offset

    // Convert to YUV and perform YUV processing
    vec3 yuvColor = editYUV(texColor.rgb);
    yuvColor -= vec3(0.5);
    yuvColor.x *= U_CONTRAST; // Contrast
    yuvColor.yz *= vec2(U_SATURATION); // Saturation
    yuvColor += vec3(0.5);
    yuvColor += U_SHIFT_YUV; // YUV shifts

    // Gamut restriction
    if (U_GAMUT_ENABLE > 0.5) yuvColor = Gamut(yuvColor);

    // Force Overrides
    if (U_OVERRIDE_Y >= 0.0) yuvColor.x = U_OVERRIDE_Y;
    if (U_OVERRIDE_U >= 0.0) yuvColor.y = U_OVERRIDE_U;
    if (U_OVERRIDE_V >= 0.0) yuvColor.z = U_OVERRIDE_V;
    if (U_OVERRIDE_A >= 0.0) texColor.a = U_OVERRIDE_A;

    // Convert to RGB, apply composite texture
    texColor.rgb = editRGB(yuvColor.xyz);
    if (U_COMPOSITOR_ENABLE > 0.5f)
        texColor.rgb = mix(texColor.xyz, compositePixel.rgb, vec3(compositePixel.a));

    // Wiper between original and processed
    if (vidCoords.x < U_WIPER_LEFT || vidCoords.x > U_WIPER_RIGHT || vidCoords.y < U_WIPER_TOP || vidCoords.y > U_WIPER_BOTTOM)
       texColor = originalPixel;

    // Zebra
    if (U_ZEBRA_ENABLE > 0.5) texColor.rgb = Zebra(texColor.rgb, fCoords);

    // Output YUV? convert from RGB
    texColor.xyz = ocsc(texColor.rgb);
    return texColor;
}
)";

const char EditShaders::mEditLibraryQuad[] =
    R"(
#define QUAD_EDIT // Four pixels at at time version

// Note that we're using a mat4 as a convenient container to
// hold four pixels at a time. Since OpenGL is column major
// that means the (color) components are adjacent forming nice
// vec4 vectors, e.g., mtx[0] is a vec4 of the red (or x)
// components for the four pixels. This makes it easy to toss
// four pixels around as inputs and output to/from functions.

// Defines for the vec4 component vectors in the mat4 holding four pixels.
// Also works for vec4 indicies.
#define IRED 0 // Red, Y, or I
#define PGRN 1 // Green, U, or P
#define TBLU 2 // Blue, V, or T
#define AKEY 3 // Alpha Key

//////////////////////////////////////////////
// Color space tools

mat4 sRGB2L(mat4 pixels)
{
    vec4 alpha = pixels[AKEY];
    mat4 result = pixels / 12.92f;

    for (int i = 0; i < 3; i++)
    {
        for (int p = 0; p < 4; p++)
        {
            if(pixels[p][i] > 0.040448236277108f)
                result[p][i] = pow(((pixels[p][i] + 0.055f) / 1.055f), U_GAMMA_709);
        }
    }

    result *= U_HLG_RGB_UP_SCALE;
    result[AKEY] = pixels[AKEY]; // Restores alpha
    return result;
}

mat4 sRGB2Lx(mat4 pixels) // Slower?!
{
    vec4 alpha = pixels[AKEY];
    for (int i = 0; i < 3; i++)
    {
        pixels[i] = mix(
            pixels[i] / vec4(12.92f),
            pow(((pixels[i] + vec4(0.055f)) / vec4(1.055f)), vec4(U_GAMMA_709)),
            lessThanEqual(pixels[i], vec4(0.040448236277108f)));
    }
    pixels *= U_HLG_RGB_UP_SCALE; // Munches alpha
    pixels[AKEY] = alpha;
    return pixels;
}

#define HLG_A (0.17883277f)
#define HLG_B (1.0f - (4.0f * HLG_A))
#define HLG_C (0.5f - HLG_A * log(4.0f * HLG_A))
#define HLG_EPSILON (0.0001f) // Could be different between mediump and highp

vec3 HLGOETF(vec3 pixel)
{
    pixel = max(pixel, 0.0f);
    for (int i = 0; i < 3; i++)
    {
        if(pixel[i] <= (1.0f / 12.0f))
            pixel[i] = sqrt(pixel[i] * 3.0f);
        else
            pixel[i] = HLG_A * log(max(HLG_EPSILON, 12.0f * pixel[i] - HLG_B)) + HLG_C;
    }
    return pixel;
}

vec3 L2HLG(vec3 pixel)
{
    float Ys = 0.0f;
    float Yd = dot(pixel, vec3(0.2627f, 0.6780f, 0.0593f));
    if (Yd > 0.0f)
        Ys = pow(Yd, U_HLG_INV_GAMMA_M1);
    vec3 Ls = pixel * vec3(Ys) * vec3(U_HLG_POW_NEG_INV_GAMMA);
    return (HLGOETF(Ls) - U_HLG_BETA) * U_HLG_BETA_FRACTION;
}

mat4 L2HLG(mat4 pixels)
{
    pixels = transpose(pixels);
    for (int p = 0; p < 4; p++)
    {
        pixels[p].xyz = L2HLG(pixels[p].xyz);
    }
    return transpose(pixels);
}

const vec3 narrowRangeOffset = vec3(0.065f, 0.5f, 0.5f);
const vec3 narrowRangeScaleFactor = vec3(1.167808f, 1.141741f, 1.141741f); // Baked into the matrix transformations

mat4 icsc(mat4 pixels)
{
#ifdef INPUT_CSC_YUV
    pixels[IRED] -= 0.065f;
    pixels[PGRN] -= 0.5f;
    pixels[TBLU] -= 0.5f;
#ifdef INPUT_REC_2020_10_BIT
    pixels = ColorSpaceConversion(rec2020Ycc2Rgb10bit, pixels);
#else // !INPUT_REC_2020_10_BIT
    pixels = ColorSpaceConversion(rec709Ycc2Rgb8bit, pixels);
#endif // !INPUT_REC_2020_10_BIT
#endif // INPUT_CSC_YUV
    return pixels;
}

#define LUT_SCALE vec3((LUT_SIZE-1.0f)/LUT_SIZE,(LUT_SIZE-1.0f)/LUT_SIZE,(LUT_SIZE-1.0f)/LUT_SIZE)
#define LUT_OFFSET vec3(0.5/LUT_SIZE,0.5/LUT_SIZE,0.5/LUT_SIZE)

mat4 ocsc(mat4 pixels)
{
    if (U_LUT_ENABLE >= 0.5f)
    {
        pixels[IRED] = fma(pixels[IRED], vec4(LUT_SCALE.x), vec4(LUT_OFFSET.x));
        pixels[PGRN] = fma(pixels[PGRN], vec4(LUT_SCALE.y), vec4(LUT_OFFSET.y));
        pixels[TBLU] = fma(pixels[TBLU], vec4(LUT_SCALE.z), vec4(LUT_OFFSET.z));
        pixels = transpose(pixels);
        for (int i = 0; i < 4; i++)
            pixels[i].rgb = texture(lut3dtex, pixels[i].rgb).rgb;
        pixels = transpose(pixels);
    }

#ifdef OUTPUT_CSC_YUV
#ifdef OUTPUT_REC_2020_10_BIT
    pixels = ColorSpaceConversion(rec2020Rgb2Ycc10bit, pixels);
#else // !OUTPUT_REC_2020_10_BIT
    pixels = ColorSpaceConversion(rec709Rgb2Ycc8bit, pixels);
#endif // !OUTPUT_REC_2020_10_BIT
    pixels[IRED] += 0.065f;
    pixels[PGRN] += 0.5f;
    pixels[TBLU] += 0.5f;
#endif // OUTPUT_CSC_YUV

    return pixels;
}

vec3 editYUV(vec3 pixel)
{
#ifdef EDIT_REC_2020
    pixel = (rec2020Rgb2Ycc * pixel) + narrowRangeOffset;
#else // !EDIT_REC_2020
    pixel = (rec709Rgb2Ycc * pixel) + narrowRangeOffset;
#endif // !EDIT_REC_2020
    return pixel;
}

vec3 editRGB(vec3 pixel)
{
#ifdef EDIT_REC_2020
    pixel = rec2020Ycc2Rgb * (pixel - narrowRangeOffset);
#else // !EDIT_REC_2020
    pixel = yuv_2_rgb(pixel, itu_709);
#endif // !EDIT_REC_2020
    return pixel;
}

mat4 editYUV(mat4 pixels)
{
#ifdef EDIT_REC_2020
    pixels = ColorSpaceConversion(rec2020Rgb2Ycc, pixels);
#else // !EDIT_REC_2020
    pixels = ColorSpaceConversion(rec709Rgb2Ycc, pixels);
#endif // !EDIT_REC_2020_10_BIT
    pixels[IRED] += 0.065f;
    pixels[PGRN] += 0.5f;
    pixels[TBLU] += 0.5f;
    return pixels;
}

mat4 editRGB(mat4 pixels)
{
    pixels[IRED] -= 0.065f;
    pixels[PGRN] -= 0.5f;
    pixels[TBLU] -= 0.5f;
#ifdef EDIT_REC_2020
    pixels = ColorSpaceConversion(rec2020Ycc2Rgb, pixels);
#else // !EDIT_REC_2020
    pixels = ColorSpaceConversion(rec709Ycc2Rgb, pixels);
#endif // !EDIT_REC_2020
    return pixels;
}

mat4 Gamut(mat4 yuvColor)
{
    // Gamut restriction
    // Find worst overshoot in RGB and calculate ratio to reduce saturation

    mat4 rgbRatios, rgbTest = editRGB(yuvColor);
    yuvColor[IRED] = max(min(yuvColor[IRED], vec4(U_GAMUT_MAX_LUMA)), vec4(U_GAMUT_MIN_LUMA));
    vec4 luma = (yuvColor[IRED] - vec4(U_GAMUT_MID_LUMA)) / vec4(U_GAMUT_MAX_LUMA - U_GAMUT_MID_LUMA);
    vec4 headroom = vec4(U_GAMUT_MAX_LUMA) - luma;
    vec4 headroomLimit = min(vec4(U_GAMUT_MAX_LUMA), headroom);
    rgbRatios[IRED] = headroomLimit / max(vec4(0.0001f), rgbTest[IRED] - luma);
    rgbRatios[PGRN] = headroomLimit / max(vec4(0.0001f), rgbTest[PGRN] - luma);
    rgbRatios[TBLU] = headroomLimit / max(vec4(0.0001f), rgbTest[TBLU] - luma);
    vec4 minRatios = min(rgbRatios[IRED], min(rgbRatios[PGRN], rgbRatios[TBLU]));
    vec4 ratio = min(vec4(1.0), max(vec4(0.0), minRatios));
    luma = (vec4(U_GAMUT_MID_LUMA) - yuvColor[IRED]) / vec4(U_GAMUT_MID_LUMA - U_GAMUT_MIN_LUMA);
    headroom = luma - vec4(U_GAMUT_MIN_LUMA);
    headroomLimit = max(vec4(U_GAMUT_MIN_LUMA), headroom);
    rgbRatios[IRED] = headroomLimit / max(vec4(0.0001f), luma - rgbTest[IRED]);
    rgbRatios[PGRN] = headroomLimit / max(vec4(0.0001f), luma - rgbTest[PGRN]);
    rgbRatios[TBLU] = headroomLimit / max(vec4(0.0001f), luma - rgbTest[TBLU]);
    ratio = min(vec4(1.0), max(vec4(0.0), min(minRatios, ratio)));
    yuvColor[PGRN] = ((yuvColor[PGRN] - vec4(U_GAMUT_MID_CHROMA)) * ratio) + vec4(U_GAMUT_MID_CHROMA);
    yuvColor[TBLU] = ((yuvColor[TBLU] - vec4(U_GAMUT_MID_CHROMA)) * ratio) + vec4(U_GAMUT_MID_CHROMA);

    return yuvColor;
}

vec3 Zebra(vec3 pixel, vec2 fCoords)
{
    float modulo = mod((fCoords.x - fCoords.y) / 8.0, 2.0) - 1.0;
    bool stripe = modulo >= 0.0;
    if (stripe)
    {
        vec3 yuv = editYUV(pixel);
        if (yuv.x > U_GAMUT_MAX_LUMA)
            pixel.rgb = vec3(1.0, 0.0, 0.0);
        else if (yuv.x < U_GAMUT_MIN_LUMA)
            pixel.rgb = vec3(0.0, 0.0, 1.0);
        else if (any(greaterThan(pixel.rgb, vec3(U_GAMUT_MAX_RGB))) || any(lessThan(pixel.rgb, vec3(U_GAMUT_MIN_RGB))))
            pixel.rgb = vec3(0.0, 1.0, 0.0);
    }
    return pixel;
}

vec2 Transform(mat4 matrix, vec2 coords)
{
    vec4 result = matrix * vec4(coords.x, coords.y, 0.0f, 0.0f);
    return result.xy;
}

//////////////////////////////////////////////
// Edit Function

// Four pixels are sorted by component, not by pixel, in the mat4.

mat4 editPixel(mat4 originalPixel, mat4 compositePixel, mat2x4 vidCoords, mat4x2 fCoords)
{
    vec4 alpha = originalPixel[AKEY]; // Save off alpha key
    originalPixel = icsc(originalPixel); // Input YUV? convert to RGB

#ifdef EDIT_REC_2020
    // Convert from Rec709 RGB to Rec 2020 RGB
//    mat4 linear = sRGB2L(compositePixel.rgb);
    mat4 linear = compositePixel;
    linear = ColorSpaceConversion(RgbRec709xyzRec2020, linear);
    compositePixel = L2HLG(linear);
#endif // EDIT_REC_2020

    // Perform RGB gain and offset
    mat4 texColor = originalPixel;
    texColor[IRED] = fma(texColor[IRED], vec4(U_GAIN), vec4(U_OFFSET));
    texColor[PGRN] = fma(texColor[PGRN], vec4(U_GAIN), vec4(U_OFFSET));
    texColor[TBLU] = fma(texColor[TBLU], vec4(U_GAIN), vec4(U_OFFSET));

    // Convert to YUV and perform YUV processing
    mat4 yuvColor = editYUV(texColor);
    yuvColor[IRED] = fma(yuvColor[IRED] - vec4(0.5f), vec4(U_CONTRAST), vec4(U_SHIFT_Y + 0.5f));
    yuvColor[PGRN] = fma(yuvColor[PGRN] - vec4(0.5f), vec4(U_SATURATION), vec4(U_SHIFT_U + 0.5f));
    yuvColor[TBLU] = fma(yuvColor[TBLU] - vec4(0.5f), vec4(U_SATURATION), vec4(U_SHIFT_V + 0.5f));

    // Gamut restriction
    if (U_GAMUT_ENABLE > 0.5) yuvColor = Gamut(yuvColor);

    // Force Overrides
    if (U_OVERRIDE_Y >= 0.0) yuvColor[IRED] = vec4(U_OVERRIDE_Y);
    if (U_OVERRIDE_U >= 0.0) yuvColor[PGRN] = vec4(U_OVERRIDE_U);
    if (U_OVERRIDE_V >= 0.0) yuvColor[TBLU] = vec4(U_OVERRIDE_V);
    if (U_OVERRIDE_A >= 0.0) yuvColor[AKEY] = vec4(U_OVERRIDE_A);

    // Convert to RGB, apply composite texture
    texColor = editRGB(yuvColor);
    if (U_COMPOSITOR_ENABLE > 0.5f)
    {
        texColor[IRED] = mix(texColor[IRED], compositePixel[IRED], compositePixel[AKEY]);
        texColor[PGRN] = mix(texColor[PGRN], compositePixel[PGRN], compositePixel[AKEY]);
        texColor[TBLU] = mix(texColor[TBLU], compositePixel[TBLU], compositePixel[AKEY]);
    }

    // Wiper between original and processed
    vec4 orig = vec4(lessThan(vidCoords[0], vec4(U_WIPER_LEFT)));
    orig += vec4(greaterThan(vidCoords[0], vec4(U_WIPER_RIGHT)));
    orig += vec4(lessThan(vidCoords[1], vec4(U_WIPER_TOP)));
    orig += vec4(greaterThan(vidCoords[1], vec4(U_WIPER_BOTTOM)));
    orig = min(vec4(1.0f), orig);
    texColor[IRED] = mix(texColor[IRED], originalPixel[IRED], orig);
    texColor[PGRN] = mix(texColor[PGRN], originalPixel[PGRN], orig);
    texColor[TBLU] = mix(texColor[TBLU], originalPixel[TBLU], orig);

    // Zebra
    if (U_ZEBRA_ENABLE > 0.5)
    {
        texColor = transpose(texColor);
        for (int i = 0; i < 4; i++) texColor[i].rgb = Zebra(texColor[i].rgb, fCoords[i]);
        texColor = transpose(texColor);
    }

    texColor = ocsc(texColor); // Output YUV? convert from RGB
    texColor[AKEY] = alpha; // restore alpha
    return texColor;
}
)";

const char EditShaders::mEditComputeShaderDeclarations[] =
    R"(
//////////////////////////////////////////////
// Compute edit shader preamble with version precedes these declarations

precision mediump int;
precision mediump float;
uniform mat4 inputTextureTransform;
uniform mat4 compositorTextureTransform;
uniform __samplerExternal2DY2YEXT inputTexture;
uniform highp sampler3D lut3dtex;
#ifdef COMPOSITING_SAMPLER
uniform sampler2D compositingTexture;
#else // !COMPOSITING_SAMPLER
layout (binding = 5, rgba8) uniform readonly highp image2D compositingTexture; // For compositing buffer
#endif // !COMPOSITING_SAMPLER
layout (binding = 4, rgba16f) uniform writeonly highp image2D transferTexture;

#define OUTPUT_DYXDY vec2(1.0f / (OUTPUT_WIDTH + 1.0f), 1.0f / (OUTPUT_HEIGHT + 1.0f))
#define OUTPUT_OFFSET vec2(1.0f / OUTPUT_WIDTH, 1.0f / OUTPUT_HEIGHT)
)";

const char EditShaders::mEditComputeShaderBody[] =
    R"(
//////////////////////////////////////////////
// Compute shader inclusions precede this body

// Processes one pixel at a time

void main()
{
    ivec2 icoords = ivec2(gl_GlobalInvocationID.xy);
    vec2 fcoords = vec2(gl_GlobalInvocationID.xy);
    vec2 outVidCoords = fma(fcoords, OUTPUT_DYXDY, OUTPUT_OFFSET);

    vec4 originalPixel = vec4(outVidCoords.x, 0.0f, 1.0f - outVidCoords.x, 1.0f);

    originalPixel = texture(inputTexture, Transform(inputTextureTransform, outVidCoords * U_INPUT_VIDEO_SCALE_XY));

    // Get text compositing pixel
    vec4 compositePixel = vec4(1.0f - outVidCoords.x, outVidCoords.x, 0.0f, outVidCoords.y);
#ifdef COMPOSITING_SAMPLER
    if (U_COMPOSITOR_ENABLE > 0.5f) compositePixel = texture(compositingTexture, Transform(compositorTextureTransform, outVidCoords));
#else // !COMPOSITING_SAMPLER
    if (U_COMPOSITOR_ENABLE > 0.5f) compositePixel = imageLoad(compositingTexture, icoords);
#endif // !COMPOSITING_SAMPLER

//    originalPixel = vec4(1.0f - outVidCoords.x, outVidCoords.y, outVidCoords.x, 1.0f);
    originalPixel = editPixel(originalPixel, compositePixel, outVidCoords, fcoords);
    imageStore(transferTexture, icoords, originalPixel);
}
)";

const char EditShaders::mEditComputeShaderQuadBody[] =
    R"(
//////////////////////////////////////////////
// Compute shader inclusions precede this body

// Processes four pixels at a time

// Note that we're using a mat4 as a convenient container to
// hold four pixels at a time. Since OpenGL is column major
// that means the (color) components are adjacent forming nice
// vec4 vectors, e.g., mtx[0] is a vec4 of the red (or x)
// components for the four pixels. This makes it easy to toss
// four pixels around as inputs and output to/from functions.

// Note that we read four pixels at a time into the mat4 which is sorted by pixel.
// We then transpose the matrix to become sorted by component for processing.
// Lastly we transpose back to sorted by pixel to write the pixels out.

void main()
{
    const ivec2 offsets[4] = ivec2[4](ivec2(0, 0), ivec2(1, 0), ivec2(2, 0), ivec2(3, 0));
    mat4 pixels = mat4(0.0f), compositingPixels = mat4(0.0f);
    mat4x2 vcrds, fcrds;
    ivec2 icrds[4];

    // Get the input pixels
    for (int i = 0; i < 4; i++)
    {
        icrds[i] = ivec2(gl_GlobalInvocationID.xy) * ivec2(4, 1) + offsets[i];
        fcrds[i] = vec2(icrds[i]);
        vcrds[i] = fma(vec2(icrds[i]), OUTPUT_DYXDY, OUTPUT_OFFSET);
        pixels[i] = texture(inputTexture, Transform(inputTextureTransform, vcrds[i] * U_INPUT_VIDEO_SCALE_XY));
//        pixels[i] = vec4(vcrds[i].x, vcrds[i].y, 1.0f - vcrds[i].x, 1.0f);
    }

    // Is compositor enabled?
    if (U_COMPOSITOR_ENABLE > 0.5f) for (int i = 0; i < 4; i++)
    {
        compositingPixels[i] = imageLoad(compositingTexture, icrds[i]);
    }

// Note that we can use either the quad processor or single pixel processor
// depending on which has been included ahead of us.
#ifdef QUAD_EDIT
    pixels = transpose(editPixel(transpose(pixels), transpose(compositingPixels), transpose(vcrds), fcrds));
#elif defined SINGLE_EDIT
    for (int i = 0; i < 4; i++)
        pixels[i] = editPixel(pixels[i], compositingPixels[i], vcrds[i], fcrds[i]);
#else // !SINGLE_EDIT
#endif // Passthough

    // Output the results
    for (int i = 0; i < 4; i++)
    {
        imageStore(transferTexture, icrds[i], pixels[i]);
    }
}
)";

const char EditShaders::mVersionAndExtensions[] =
    R"(#version 320 es
#extension GL_OES_EGL_image_external_essl3 : require
#extension GL_OES_EGL_image_external : require
#extension GL_EXT_YUV_target : require
)";

} // namespace Simulation
