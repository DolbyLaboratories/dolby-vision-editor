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



Simulation::Context::Context(EGLContext toShare)
{
    LOGI("CTOR");

    const EGLint configAttribs[] = {
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_NONE
    };

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY)
    {
        LOGE("EGL default display acquisition: FAIL");

        if (valid)
        {
            error = Simulation::EGLContextError::DISPLAY_ACQUISITION_FAILED;
            valid = false;
        }
    }
    else
    {
        LOGI("default display acquisition: PASS");
    }

    EGLint versionMajor;
    EGLint versionMinor;
    if (!eglInitialize(display, &versionMajor, &versionMinor))
    {
        LOGE("EGL display initialization: FAIL (major: %d, minor: %d)", versionMajor, versionMinor);

        if (valid)
        {
            error = Simulation::EGLContextError::DISPLAY_INITIALIZATION_FAILED;
            valid = false;
        }
    }
    else
    {
        LOGI("EGL display initialization: PASS (major: %d, minor: %d)", versionMajor, versionMinor);
    }

    int numConfigs;
    if (!eglChooseConfig(display, configAttribs, &config, 1, &numConfigs))
    {

        LOGE("EGL configuration acquisition: FAIL");

        if (valid)
        {
            error = Simulation::EGLContextError::CONFIG_ACQUISITION_FAILED;
            valid = false;
        }
    }
    else
    {
        LOGI("EGL configuration acquisition: PASS");
    }

    int contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            //EGL_PROTECTED_CONTENT_EXT, EGL_TRUE,
            EGL_NONE
    };

#ifdef SHARE_CURRENT_CONTEXT
    context = eglCreateContext(display, config, eglGetCurrentContext(), contextAttribs);
#else
    if (toShare == nullptr)
    {
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    }
    else
    {
        context = eglCreateContext(display, config, toShare, contextAttribs);
    }
#endif

    if (!context)
    {
        LOGE("EGL context acquisition: FAIL");

        if (valid)
        {
            error = Simulation::EGLContextError::CONTEXT_ACQUISITION_FAILED;
            valid = false;
        }
    }
    else
    {
        LOGI("EGL context acquisition: PASS");
    }
}

Simulation::Context::~Context()
{
    LOGI("DTOR");

    eglDestroyContext(display, context);
}


void Simulation::Context::makeUncurrent()
{
    LOGI("makeUncurrent");

    if (valid)
    {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
    else
    {
        LOGE("cannot make invalid context uncurrent");
    }
}

void Simulation::Context::makeCurrent()
{
    LOGI("makeCurrent");

    if (valid)
    {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
    }
    else
    {
        LOGE("cannot make invalid context current");
    }
}

EGLDisplay Simulation::Context::getDisplay()
{
    LOGI("getDisplay");

    return display;
}

bool Simulation::Context::isContextValid(Simulation::EGLContextError* rete) const
{
    LOGI("isContextValid");

    if (rete)
    {
        *rete = error;
    }

    return valid;
}
