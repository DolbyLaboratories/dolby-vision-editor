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

#ifndef _MP4REMUXER_H_
#define _MP4REMUXER_H_

#ifdef _WIN32
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long long int64_t;
typedef int64_t offset_t;
#else
#include <inttypes.h>
#include <sys/types.h>        /* for off_t */
#include <stdio.h>
typedef int64_t offset_t;
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _io
{
    FILE *fp;
} io_t;


struct _mp4_remuxer
{
#define IO_BLOCK 1024
    char box_data[IO_BLOCK];

    int insert_size;
    int insert_track_index;
    int current_track_index;

    int dv_profile;
    int dv_level;
    int dv_bl_comp_id;

    offset_t start_offset;
    uint32_t offset_size;
    int is_mdat_at_begining;
};
typedef struct _mp4_remuxer mp4_remuxer_t;

int mp4remux_search_insert_track(mp4_remuxer_t *remuxer, const char *infilename);
int mp4remux_parse_boxes(mp4_remuxer_t *remuxer, const char *infilename, const char *outfilename);
mp4_remuxer_t *mp4remux_remuxer_create(void);
void mp4remux_remuxer_destroy(mp4_remuxer_t *remuxer);

#ifdef __cplusplus
}
#endif
#endif
