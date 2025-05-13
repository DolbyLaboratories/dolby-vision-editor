/******************************************************************************
 * The Clear BSD License
 * Copyright (c) 2025 Dolby Laboratories
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
#ifndef HLG_33_LUT_H
#define HLG_33_LUT_H
#define LUT_33_SIZE (33)
#define LUT_33_VALUE_COUNT (LUT_33_SIZE * LUT_33_SIZE * LUT_33_SIZE * 3)
namespace Simulation
{
    template <class TYPE>
    void Indentity3dLut(int size, TYPE *data)
    {
        for (int x = 0; x < size; x++)
        {
            TYPE fx = ((TYPE)x) / ((TYPE)(size - 1));
            for (int y = 0; y < size; y++)
            {
                TYPE fy = ((TYPE)y) / ((TYPE)(size - 1));
                for (int z = 0; z < size; z++)
                {
                    TYPE fz = ((TYPE)z) / ((TYPE)(size - 1));
                    *data++ = fz;
                    *data++ = fy;
                    *data++ = fx;
                }
            }
        }
    }

    extern float hlg_lut_1000_bt2020_pq_33[LUT_33_VALUE_COUNT];
} // namespace Simulation
#endif