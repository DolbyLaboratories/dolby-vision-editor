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

#ifndef TOOLS_H
#define TOOLS_H

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
#include <functional>

#include <android/log.h>

#include <GLES3/gl32.h>

#ifndef LOG_TAG
#define LOG_TAG "tools"
#endif // LOG_TAG

#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#if defined(LOG_NDEBUG) && (LOG_NDEBUG==0)
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#else
#define LOGI(...)
#define LOGV(...)
#endif


using namespace std;
using namespace std::chrono;

namespace Simulation
{
///////////////////////////////////////////////////////////////////////////////
// Tools

// Integer division where any remainder rounds up
template <class TYPE>
inline TYPE DivUp(TYPE a, TYPE b)
{
    assert(std::is_integral<TYPE>());
    return (a + b - 1) / b;
}

// Clamps a value between two limits
template <class TYPE>
inline TYPE ClampVal(TYPE value, TYPE lower_limit, TYPE upper_limit)
{
    return max(lower_limit, min(upper_limit, value));
}

// Zeros out an object
template <class TYPE>
inline void MemClr(TYPE &object, unsigned char val = 0)
{
    memset(&object, (int)val, sizeof(TYPE));
}

// Copies an object
template <class TYPE>
inline void MemCopy(TYPE &dst, const TYPE &src)
{
    memcpy(&dst, &src, sizeof(TYPE));
}

// Compares two objects
template <class TYPE>
inline int MemCompare(const TYPE &src1, const TYPE &src2)
{
    return memcmp(&src1, &src2, sizeof(TYPE));
}

// Safe size for a fixed size text string array
// Example: char text[100]; snprintf(text, SafeSize(text),"%s", some_string_of_indeterminant_size);
template <size_t size>
inline size_t SafeSize(const char (&text)[size])
{
    return size - 1;
}

// Returns the number of elements in a fixed size scope array, static or allocated on the stack
// Won't work with arrays on the heap
template <size_t size, class TYPE>
inline size_t GetLength(TYPE (&array)[size])
{
    return size;
}

extern void DumpEmbeddedLines(const char *text, const char *error_label = nullptr);

///////////////////////////////////////////////////////////////////////////////
// ScopeExitFunction

// Allows user to specify a Lambda function to be executed when the
// ScopeExitFunction goes out of scope, such as function exit
// or exit from a conditional.
// The Lambda function should return void with no input arguments: void().
// The Lambda function can use its closure to capture local variables.

class ScopeExitFunction
{
protected:
    ScopeExitFunction(){}; // Disallow default constructor
public:
    ScopeExitFunction(std::function<void()> func) { mExitFunction = func; }
    virtual ~ScopeExitFunction() { if (mExitFunction) mExitFunction(); }

protected:
    std::function<void()> mExitFunction = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
// Some OpenGL tools

extern bool CompileAndLinkShader(GLuint &program, char const *fragment_or_compute_text, char const *vertex_text = nullptr);
extern bool mDumpShaderErrors;

// Note that the argument is passed by reference to these functions
// These functions use the convention that unallocated handles are initlialized
// to GL_INVALID_VALUE, and when release are reset to GL_INVALID_VALUE

extern void DeleteTexture(GLuint &handle);
extern void DeleteProgram(GLuint &handle);
extern void DeleteShader(GLuint &handle);
extern void DeleteBuffer(GLuint &handle);

///////////////////////////////////////////////////////////////////////////////
// CheckGlError

// use #define for debug code trace
#ifdef _DEBUG
#define CHECK_GL_ERROR CheckGlError(__FUNCTION__, __FILE__, __LINE__)
#else // ! _DEBUG
//#define CHECK_GL_ERROR CheckGlError()
#define CHECK_GL_ERROR Simulation::CheckGlError(__FUNCTION__, __FILE__, __LINE__)

// Inline in release mode for CPU speed in inner loops of lower resolution images
inline bool CheckGlError()
{
    GLenum last_code = GL_NO_ERROR;
    bool guilty = false; // Innocent until proven guilty
    for (GLenum code = glGetError(); code != GL_NO_ERROR && code != last_code; code = glGetError())
    {
        last_code = code;
        LOGE("GL error: 0x%X", code);
        guilty = true;
    }
    if (guilty) assert(false);
    return guilty;
}
#endif // ! _DEBUG

extern bool CheckGlError();
extern bool CheckGlError(const char *function, const char *file, int line);

///////////////////////////////////////////////////////////////////////////////
// ScopeTimer & ScopeTimerGPU

// These are primarily designed be used as a scope object.
// They finish and report when going out of scope.
// Note the mBusy flag to prevent timing re-entrancy issues.
// Running a nested timing test within an already running timing test
// can affect the upper level test. This re-entrancy test disables testing
// in the nested call when the upper test is enabled.
// In case of conflicts, an upper test has priority over any nested test.
// One side effect is that then these cannot be used to simultaneoulsy measure
// scopes in two different threads, perhaps on different cores.

class ScopeTimer
{
protected:
    ScopeTimer(){}; // Disallow default constructor
public:
    inline ScopeTimer(const char *label, int loop_count, double *accumulator = nullptr)
    {
        if ((loop_count > 0) && mBusy.test_and_set())
            loop_count = 0; // Disable if called recursively
        mLoops          = loop_count;
        mAccumulatorPtr = accumulator;
        if (TimerActive())
        {
            mLabel = label;
            Start();
        }
    };
    inline ~ScopeTimer()
    {
        if (TimerActive())
        {
            Finish();
            Report();
            mBusy.clear();
        }
    }
    inline bool TimerActive() const { return mLoops > 0; }
    inline void Start()
    {
        if (TimerActive())
            mFinish = mStart = high_resolution_clock::now();
    }
    inline void Finish()
    {
        if (TimerActive())
            mFinish = high_resolution_clock::now();
    }
    void Report()
    {
        if (TimerActive())
        {
            auto   elapsed = duration_cast<microseconds>(mFinish - mStart);
            double time    = (double)elapsed.count() / (double)mLoops;
            time /= 1000.0; // Time in milliseconds
            LOGI("%sTime: %g milliseconds (loops: %d)", mLabel.c_str(), time, mLoops);
            if (mAccumulatorPtr)
                *mAccumulatorPtr += time;
        }
    };
    inline int LoopCount() const { return std::max(1, mLoops); } // Always return at least one for loop counters

    int                                                         mLoops; // Timer only active when mLoops > 0
    double *                                                    mAccumulatorPtr;
    string                                                      mLabel;
    std::chrono::time_point<std::chrono::high_resolution_clock> mStart;
    std::chrono::time_point<std::chrono::high_resolution_clock> mFinish;
    static std::atomic_flag                                     mBusy;
};

// GPU version
// Performs glFinish() before Start() and Finish()
class ScopeTimerGPU : public ScopeTimer
{
protected:
    ScopeTimerGPU(){}; // Disallow default constructor
public:
    inline ScopeTimerGPU(const char *label, int loop_count, double *accumulator = nullptr)
    {
        if ((loop_count > 0) && mBusy.test_and_set())
            loop_count = 0; // Disable if called recursively
        mLoops          = loop_count;
        mAccumulatorPtr = accumulator;
        if (TimerActive())
        {
            mLabel = label;
            Start();
        }
    };
    inline ~ScopeTimerGPU()
    {
        if (TimerActive())
            glFinish();
    } // Let subclass report
    inline void Start()
    {
        if (TimerActive())
        {
            glFinish();
            ScopeTimer::Start();
        }
    }
    inline void Finish()
    {
        if (TimerActive())
        {
            glFinish();
            ScopeTimer::Finish();
        }
    }
};

} // namespace dovi
#endif // TOOLS_H
