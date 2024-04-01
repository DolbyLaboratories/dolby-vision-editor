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
#define LOG_TAG "EGLContext"
//#define LOG_NDEBUG 0

#include "EGLContext.h"
#include<string.h>

#define METADATA_SCALE(x) (static_cast<EGLint>(x * EGL_METADATA_SCALING_EXT))
#define EGL_GL_COLORSPACE_BT2020_HLG_EXT  0x3540
#define  TRANSFER_HLG  2
#define  TRANSFER_SDR  0

Simulation::Context::Context(EGLContext toShare, ANativeWindow *window, bool isDPUSolution, bool isPreviewMode, int transfer) {
    LOGI("CTOR");

    nativeWindow = window;
    const EGLint configRGB1010102Attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 0,
            EGL_STENCIL_SIZE, 0,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_NONE
    };

    const EGLint configRGB8888Attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 10,
            EGL_BLUE_SIZE, 10,
            EGL_GREEN_SIZE, 10,
            EGL_ALPHA_SIZE, 2,
            EGL_DEPTH_SIZE, 0,
            EGL_STENCIL_SIZE, 0,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_NONE
    };

    const EGLint configYUV10bitAttribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_COLOR_BUFFER_TYPE, EGL_YUV_BUFFER_EXT,
            EGL_YUV_NUMBER_OF_PLANES_EXT, 2,
            EGL_YUV_SUBSAMPLE_EXT, EGL_YUV_SUBSAMPLE_4_2_0_EXT,
            EGL_YUV_DEPTH_RANGE_EXT, EGL_YUV_DEPTH_RANGE_LIMITED_EXT,
            EGL_YUV_CSC_STANDARD_EXT, EGL_YUV_CSC_STANDARD_2020_EXT,
            EGL_YUV_PLANE_BPP_EXT, EGL_YUV_PLANE_BPP_10_EXT,
            EGL_NONE
    };
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        LOGE("EGL default display acquisition: FAIL");
        if (valid) {
            error = Simulation::EGLContextError::DISPLAY_ACQUISITION_FAILED;
            valid = false;
        }
    } else {
        LOGI("default display acquisition: PASS");
    }

    EGLint versionMajor;
    EGLint versionMinor;
    if (!eglInitialize(display, &versionMajor, &versionMinor)) {
        LOGE("EGL display initialization: FAIL (major: %d, minor: %d)", versionMajor, versionMinor);

        if (valid) {
            error = Simulation::EGLContextError::DISPLAY_INITIALIZATION_FAILED;
            valid = false;
        }
    } else {
        LOGI("EGL display initialization: PASS (major: %d, minor: %d)", versionMajor, versionMinor);
    }

    int numConfigs;
    if (!eglChooseConfig(display, isPreviewMode ? (transfer == TRANSFER_HLG ? configRGB1010102Attribs
                                                                      : configRGB8888Attribs)
                                          : configYUV10bitAttribs, &config, 1, &numConfigs)) {
        LOGE("EGL configuration acquisition: FAIL");

        if (valid) {
            error = Simulation::EGLContextError::CONFIG_ACQUISITION_FAILED;
            valid = false;
        }
    } else {
        LOGI("EGL configuration acquisition: PASS");
    }

    int contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
            EGL_NONE
    };

#ifdef SHARE_CURRENT_CONTEXT
    context = eglCreateContext(display, config, eglGetCurrentContext(), contextAttribs);
#else
    if (toShare == nullptr) {
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    } else {
        context = eglCreateContext(display, config, toShare, contextAttribs);
    }
#endif

    if (!context) {
        LOGE("EGL context acquisition: FAIL");

        if (valid) {
            error = Simulation::EGLContextError::CONTEXT_ACQUISITION_FAILED;
            valid = false;
        }
    } else {
        LOGI("EGL context acquisition: PASS");
    }

    int surfaceAttribs[] = {
            (transfer == TRANSFER_HLG) ? EGL_GL_COLORSPACE_KHR : EGL_NONE,
            (transfer == TRANSFER_HLG) ? EGL_GL_COLORSPACE_BT2020_HLG_EXT : EGL_NONE,
            EGL_NONE
    };

    eglSurface = eglCreateWindowSurface(display, config, nativeWindow, surfaceAttribs);
    if (eglSurface == EGL_NO_SURFACE) {
        LOGE("create egl surface failed");
        if (valid) {
            error = Simulation::EGLContextError::CONFIG_ACQUISITION_FAILED;
            valid = false;
        }
    } else {
        LOGI("EGLSurface acquisition: PASS");
    }
    if (transfer == TRANSFER_HLG && isPreviewMode && isDPUSolution && hasEglExtension(display, "EGL_EXT_surface_CTA861_3_metadata")) {
        //Dolby DPU platform, HLG preview through Dolby clstc if having the following specified metadata,
        // otherwise Qcom handle it.
        int maxContentLL = 'd' * 100 + 'v';
        eglSurfaceAttrib(display, eglSurface, EGL_CTA861_3_MAX_CONTENT_LIGHT_LEVEL_EXT,
                         METADATA_SCALE(maxContentLL));
        eglSurfaceAttrib(display, eglSurface, EGL_CTA861_3_MAX_FRAME_AVERAGE_LEVEL_EXT,
                         METADATA_SCALE(12));
        LOGI("Setting specified metadata for DPU platform preview");
    }
    CHECK_GL_ERROR;
}

void Simulation::Context::setPresentationTime(long nsecs) {
    eglPresentationTimeANDROID(display, eglSurface, nsecs);
}

bool Simulation::Context::hasEglExtension(EGLDisplay dpy, const char *extensionName) {
    const char *exts = eglQueryString(dpy, EGL_EXTENSIONS);
    size_t cropExtLen = strlen(extensionName);
    size_t extsLen = strlen(exts);
    bool equal = !strcmp(extensionName, exts);
    std::string extString(extensionName);
    std::string space(" ");
    bool atStart = !strncmp((extString + space).c_str(), exts, cropExtLen + 1);
    bool atEnd = (cropExtLen + 1) < extsLen &&
                 !std::strcmp((space + extString).c_str(), exts + extsLen - (cropExtLen + 1));
    bool inMiddle = std::strstr(exts, (space + extString + space).c_str());
    return equal || atStart || atEnd || inMiddle;
}

Simulation::Context::~Context() {
    LOGI("Context DTOR");

    eglDestroyContext(display, context);
    makeUncurrent();

    if (display != EGL_NO_DISPLAY) {
        eglDestroySurface(display, eglSurface);
        eglTerminate(display);
    }

    display = EGL_NO_DISPLAY;
    context = EGL_NO_CONTEXT;
    eglSurface = EGL_NO_SURFACE;
}


void Simulation::Context::makeUncurrent() {
    LOGI("makeUncurrent");

    if (valid) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    } else {
        LOGE("cannot make invalid context uncurrent");
    }
}

void Simulation::Context::makeCurrent() {
    LOGI("makeCurrent");

    if (valid) {
        if (eglSurface == EGL_NO_SURFACE) {
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
        } else {
            eglMakeCurrent(display, eglSurface, eglSurface, context);
        }
    } else {
        LOGE("cannot make invalid context current");
    }
}

EGLDisplay Simulation::Context::getDisplay() {
    LOGI("getDisplay");

    return display;
}

bool Simulation::Context::swapBuffers() {
    LOGI("swapBuffers");

    return eglSwapBuffers(display, eglSurface);
}

bool Simulation::Context::isContextValid(Simulation::EGLContextError *rete) const {
    LOGI("isContextValid");

    if (rete) {
        *rete = error;
    }

    return valid;
}
