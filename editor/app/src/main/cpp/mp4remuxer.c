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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mp4remuxer.h"


#ifdef xx_DEBUG
#if defined(DOLBY_ANDROID)
#define DPRINTF(x) LOGI x
#else
#define DPRINTF(x) printf x
#endif
#else
#define DPRINTF(x) printf_fake x
#endif

#ifdef WIN32
#define FSEEK _fseeki64
#define FTELL _ftelli64
#define PRIu64 "Lu"
#else
#define FSEEK fseeko
#define FTELL ftello
#endif

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) x = x
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

#define TAGLEN 256
#define IO_BLOCK 1024

/*
 * File and memory I/O routines.
 */

static void printf_fake(const char * _format, ...){
    UNUSED(_format);
}

static uint32_t
GET_BE32(uint32_t x)
{
    return ( ( ( ( (x) >> 24 ) & 0xff ) << 0  ) | ( ( ( (x) >> 16 ) & 0xff ) << 8  ) | ( ( ( (x) >> 8  ) & 0xff ) << 16 ) |  ( ( ( (x) >> 0  ) & 0xff ) << 24 ) );
}

static uint64_t
GET_BE64(uint64_t x)
{
    uint32_t u1 = (x >> 32);
    uint32_t u2 = x & 0xffffffff;

    uint64_t o1 = GET_BE32(u1);
    uint64_t o2 = GET_BE32(u2);

    uint64_t res = (o2 << 32) | o1;

    return res;
}

static uint64_t
src_read_u64(io_t *io)
{
    uint64_t u64 = 0;
    int i;

    for (i = 0; i < 8; i++) {
        uint8_t u8;
        fread((char *)&u8, 1, 1, io->fp);
        u64 <<= 8;
        u64 |= u8;
    }
    return u64;
}

static uint32_t
src_read_u32(io_t *io)
{
    uint32_t u32 = 0;
    int i;

    for (i = 0; i < 4; i++) {
        uint8_t u8;
        fread((char *)&u8, 1, 1, io->fp);
        u32 <<= 8;
        u32 |= u8;
    }
    return u32;
}

/* static uint16_t
src_read_u16(io_t *io)
{
    uint16_t u16 = 0;
    int i;

    for (i = 0; i < 2; i++) {
        uint8_t u8;
        fread((char *)&u8, 1, 1, io->fp);
        u16 <<= 8;
        u16 |= u8;
    }
    return u16;
}

static uint8_t
src_read_u8(io_t *io)
{
    uint8_t u8;
    fread((char *)&u8, 1, 1, io->fp);
    return u8;
}
*/
static void
src_read(io_t *io, char *buf, int size)
{
    fread(buf, 1, (size_t)size, io->fp);
}

static void
src_skip(io_t *io, offset_t nbytes)
{
    FSEEK(io->fp, nbytes, SEEK_CUR);
}

static offset_t
src_get_position(io_t *io)
{
    return FTELL(io->fp);
}

static uint32_t
buff_read_u32(char *io)
{
    /*unsigned char * io = (unsigned char * )s_io;*/
    return (uint32_t)(((io[0] & 0xff) << 24) | ((io[1] & 0xff) << 16) | ((io[2] & 0xff) << 8) | ((io[3] << 0) & 0xff));
}

static void
buff_read(char *src, char *buf, int size)
{
    while(size) {
    *buf++ = *src++;
    size--;
    }
}

static void
dst_write(io_t *io, char *buf, int size)
{
    fwrite(buf, 1, (uint32_t)size, io->fp);
}

static void
dst_write_u32(io_t *io, uint32_t value)
{
    uint32_t be_value = GET_BE32(value);

    fwrite(&be_value, 1, sizeof(uint32_t), io->fp);
}

static void
dst_write_u64(io_t *io, uint64_t value)
{
    uint64_t be_value = GET_BE64(value);
    fwrite(&be_value, 1, sizeof(uint64_t), io->fp);
}


/*
 * MPEG-4 File remuxer
 */

typedef struct _box {
    offset_t size;
    char type[5];
} box_t;

static int copy_box_content(mp4_remuxer_t *remuxer, io_t *src, io_t *dst, box_t *parent);
static int parse_box_stco(mp4_remuxer_t *remuxer, io_t *src, io_t *dst, box_t *parent);
static int parse_box_stsd(mp4_remuxer_t *remuxer, io_t *src, io_t *dst, box_t *parent);
static int parse_box_audio(mp4_remuxer_t *remuxer, io_t *src, io_t *dst,  box_t *parent);
static int parse_box_video(mp4_remuxer_t *remuxer, io_t *src, io_t *dst,  box_t *parent);
static int parse_edit_box(mp4_remuxer_t *remuxer, io_t *src, io_t *dst, box_t *parent);
static int insert_dovi_box(mp4_remuxer_t *remuxer, io_t *src, io_t *dst, box_t *parent);

struct {
    char type[5];
    int (*func)(mp4_remuxer_t *, io_t *, io_t *, box_t *);
} DispatcherTable[] = {
    { "ftyp", copy_box_content },
    { "mdat", copy_box_content },
    { "moov", parse_edit_box },
    { "mvhd", copy_box_content },
    { "trak", parse_edit_box },
    { "tkhd", copy_box_content },
    { "edts", copy_box_content },
    { "stco", parse_box_stco },
    { "co64", parse_box_stco },
    { "stsz", copy_box_content },
    { "stsd", parse_box_stsd },
    { "stsc", copy_box_content },
    { "stts", copy_box_content },
    { "stss", copy_box_content },
    { "ctts", copy_box_content },
    { "mdhd", copy_box_content },
    { "stbl", parse_edit_box },

    { "encv", parse_box_video },
    { "enca", parse_box_audio },
    { "sinf", parse_edit_box },
    { "frma", copy_box_content },

    { "mp4a", parse_box_audio },
    { "avc1", parse_box_video },
    { "avc3", parse_box_video },
    { "dvav", parse_box_video },
    { "hvc1", parse_box_video },
    { "hev1", parse_box_video },
    { "dvhe", parse_box_video },
	{ "dvh1", parse_box_video },
    { "s263", parse_box_video },
    { "tx3g", copy_box_content },
    { "metx", copy_box_content },
    { "ac-3", parse_box_audio },
    { "ec-3", parse_box_audio },
    { "ac-4", parse_box_audio },
    { "mlpa", parse_box_audio },

    { "esds", copy_box_content },
    { "d263", copy_box_content },
    { "avcC", insert_dovi_box },
    { "hvcC", insert_dovi_box },
    { "dvcC", copy_box_content },
    { "dvvC", copy_box_content },
    { "dac3", copy_box_content },
    { "dec3", copy_box_content },
    { "dmlp", copy_box_content },
    { "dac4", copy_box_content },

    { "hdlr", copy_box_content },
    { "mdia", parse_edit_box },
    { "minf", parse_edit_box },
    { "vmhd", copy_box_content },
    { "dinf", copy_box_content },
    { "udta", parse_edit_box },
    { "meta", copy_box_content },
    { "ilst", copy_box_content },
    { "uuid", copy_box_content },
    { "free", copy_box_content },
    { "pasp", copy_box_content },
    { "colr", copy_box_content },

    /* fragments */
    { "mvex", parse_edit_box },
    { "moof", parse_edit_box },
    { "traf", parse_edit_box },
    { "trex", copy_box_content },
    { "tfhd", copy_box_content },
    { "trun", copy_box_content },
	{ "mfhd", copy_box_content },
    { "iods", copy_box_content },
    { "smhd", copy_box_content },
};

#define NDISPATCHERS (int)(sizeof(DispatcherTable)/sizeof(DispatcherTable[0]))
#define FOURCC_COMPARE(x,y) (memcmp(x, y, 4) == 0)

static int
dispatch(mp4_remuxer_t *remuxer, io_t *src, io_t *dst, box_t *box)
{
    offset_t offset = src_get_position(src);
    offset_t end = offset + box->size;
    int i;

    DPRINTF(("parent %.4s, size=%lld, offset=%lld\n",
        (const char *)&box->type, (size_t)box->size, (size_t)offset));

    for (i = 0; i < NDISPATCHERS; i++) {
        if (FOURCC_COMPARE(DispatcherTable[i].type, box->type)) {
            int ret = DispatcherTable[i].func(remuxer, src, dst, box);
            offset_t pos = src_get_position(src);
            if (pos != end) {
                DPRINTF(("-------> %lld extra bytes in box %.4s\n", (size_t)(end - pos), DispatcherTable[i].type));
                src_skip(src, end - pos);
            }
            return ret;
        }
    }

    DPRINTF(("unable to parse box %.4s\n", box->type));
    src_skip(src, box->size);

    return 0;
}

static int
copy_box_content(mp4_remuxer_t *remuxer, io_t *src, io_t *dst, box_t *parent)
{
    int i;
    for (i = 0; i < parent->size/IO_BLOCK; i++)
    {
        src_read(src, (char *)remuxer->box_data, IO_BLOCK);
        dst_write(dst, (char *)remuxer->box_data, IO_BLOCK);
    }

    if (parent->size % IO_BLOCK) {
        src_read(src, (char *)remuxer->box_data, parent->size % IO_BLOCK);
        dst_write(dst, (char *)remuxer->box_data, parent->size % IO_BLOCK);
    }

    return 0;
}

static int
insert_dovi_box(mp4_remuxer_t *remuxer, io_t *src, io_t *dst, box_t *parent)
{
    char name[5] = {0,};
    char reserved[19] = {0,};
    int temp = 0;
    int size = remuxer->insert_size;

    /* copy avcC/hvcC contents */
    copy_box_content(remuxer, src, dst, parent);

    /* Dovi config box size */
    dst_write_u32(dst, (uint32_t)size);

    /* Dovi config box name */
    if (remuxer->dv_profile == 8 ||
        remuxer->dv_profile == 9) {             /*dvvC*/
        memcpy(&name,"dvvC", 5);
    } else if (remuxer->dv_profile == 32) {     /*dvwC*/
        memcpy(&name,"dvwC", 5);
    } else {                                    /*dvcC*/
        memcpy(&name,"dvcC", 5);
    }
    dst_write(dst, name, 4);

    /* dv_version_major = 1; maybe should be 2?*/
    temp = 1;
    dst_write(dst, (char *)&temp, 1);

    /* dv_version_minor = 0 */
    temp = 0;
    dst_write(dst, (char *)&temp, 1);

    /* dv_profile && the 6th bit of dv_level */
    temp = ((remuxer->dv_profile << 1) & (0xfe)) | ((remuxer->dv_level >> 5) & (0x1));
    dst_write(dst, (char *)&temp, 1);

    /* 5 bits of dv_level && rpu == 1, el == 0, bl == 1 */
    temp = ((remuxer->dv_level << 3) & 0xf8) | (1 << 2) | (0 << 1) | (1 << 0);
    dst_write(dst, (char *)&temp, 1);

    /* 4 bits of dv_bl_comp_id && zeros */
    temp = (remuxer->dv_bl_comp_id << 4) & 0xf0;
    dst_write(dst, (char *)&temp, 1);

    /* reserved 0s */
    dst_write(dst, (char *)reserved, 19);

    return 0;
}

static int
parse_box_stsd(mp4_remuxer_t *remuxer, io_t *src, io_t *dst, box_t *parent)
{
    uint32_t count;

    dst_write_u32(dst, src_read_u32(src)); /* version/flags */

    count = src_read_u32(src);
    dst_write_u32(dst, count);

    if (count != 1) {
        DPRINTF(("don't support more than one entry in stsd\n"));
        return 1;
    }

    parent->size -= 8;
    return parse_edit_box(remuxer, src, dst, parent);
}

static int
parse_box_stco(mp4_remuxer_t *remuxer, io_t *src, io_t *dst, box_t *parent)
{
    int is64bit = FOURCC_COMPARE("co64", parent->type);
    uint32_t i;
    uint32_t count;

    /* version/flags */
    dst_write_u32(dst, src_read_u32(src));

    /* entry count */
    count = src_read_u32(src);
    dst_write_u32(dst, count);

    if (remuxer->is_mdat_at_begining) {
        for (i = 0; i < count; i++) {
            if (is64bit) {
                dst_write_u64(dst, src_read_u64(src));
            }
            else {
                dst_write_u32(dst, src_read_u32(src));
            }
        }
    }
    else {
        for (i = 0; i < count; i++) {
            if (is64bit) {
                dst_write_u64(dst, src_read_u64(src) + remuxer->insert_size);
            }
            else {
                dst_write_u32(dst, src_read_u32(src) + remuxer->insert_size);
            }
        }
    }

    return 0;
}

static int
parse_box_audio(mp4_remuxer_t *remuxer, io_t *src, io_t *dst, box_t *parent)
{
    src_read(src, (char *)(remuxer->box_data), 28);
    dst_write(dst, (char *)(remuxer->box_data), 28);

    parent->size -= 28;
    return parse_edit_box(remuxer, src, dst, parent);
}

static int
parse_box_video(mp4_remuxer_t *remuxer, io_t *src, io_t *dst, box_t *parent)
{
    src_read(src, (char *)(remuxer->box_data), 78);
    dst_write(dst, (char *)(remuxer->box_data), 78);
    parent->size -= 78;
    return parse_edit_box(remuxer, src, dst, parent);
}

static int
parse_edit_box(mp4_remuxer_t *remuxer, io_t *src, io_t *dst, box_t *parent)
{
    offset_t left = parent->size;
    box_t box;
    int ret = 0;

    while (left > 0) {

        offset_t size = src_read_u32(src);
        offset_t new_size = size;
        if (size == 0) {
            size = left;
        }

        src_read(src, box.type, 4);

        if (size == 1) {
            new_size = (offset_t)src_read_u64(src);
        }

        if (FOURCC_COMPARE(box.type, "trak")) {
            remuxer->current_track_index++;
        }

        if (FOURCC_COMPARE(box.type, "moov") ||
            ((FOURCC_COMPARE(box.type, "trak") ||
              FOURCC_COMPARE(box.type, "mdia") ||
              FOURCC_COMPARE(box.type, "minf") ||
              FOURCC_COMPARE(box.type, "stbl") ||
              FOURCC_COMPARE(box.type, "stsd") ||
              FOURCC_COMPARE(box.type, "avc1") ||
              FOURCC_COMPARE(box.type, "avc3") ||
              FOURCC_COMPARE(box.type, "hvc1") ||
              FOURCC_COMPARE(box.type, "hev1")) &&
              remuxer->insert_track_index == remuxer->current_track_index)
           )
        {
            new_size += remuxer->insert_size;
        }

        if (size != 1) {
            dst_write_u32(dst, (uint32_t)new_size);
        } else {
            dst_write_u32(dst, (uint32_t)size);
        }

        dst_write(dst, box.type, 4);

        if (size == 1) {
            dst_write_u64(dst, new_size);
            size = new_size;
            box.size = size - 16;
        } else {
            box.size = size - 8;
        }

        if (box.size < 0 || box.size > left) {
            return 1;
        }

        ret = dispatch(remuxer, src, dst, &box);
        if (ret) {
            return ret;
        }

        left -= size;
    }

    return ret;
}

int
mp4remux_parse_boxes(mp4_remuxer_t *remuxer, const char *infilename, const char *outfilename)
{
    io_t in, out;
    box_t box;
    int ret;

    in.fp = fopen(infilename, "rb");
    if (in.fp == NULL) {
        return 1;
    }

    out.fp = fopen(outfilename, "wb");
    if (out.fp == NULL) {
        fclose(in.fp);
        return 1;
    }

	if (remuxer->offset_size == 0){
		FSEEK(in.fp, 0, SEEK_END);
		box.size = (offset_t)FTELL(in.fp);
		memcpy(box.type,"file",5);
		rewind(in.fp);
	} else {
		box.size = remuxer->offset_size;
		memcpy(box.type,"file",5);
		if (remuxer->start_offset) {
			FSEEK(in.fp, remuxer->start_offset, SEEK_SET);
        }
	}

    ret = parse_edit_box(remuxer, (io_t *)&in, (io_t *)&out, &box);

    fclose(in.fp);
    fclose(out.fp);

    return ret;
}

int
mp4remux_search_insert_track(mp4_remuxer_t *remuxer, const char *infilename)
{
    io_t in;
    offset_t left = 0;
    box_t box;
    io_t *src = NULL;
    char *moov_buf = NULL;
    int i = 0;
    int hasMoov = 0;
    int ret = 0;
    int track_index = -1;
    char type[5][5] = {"trak", "mdia", "minf", "stbl", "stsd"};

    in.fp = fopen(infilename, "rb");
    if (in.fp) {
	    FSEEK(in.fp, 0, SEEK_END);
	    left = (offset_t)FTELL(in.fp);
	    rewind(in.fp);
    }

    src = (io_t *)&in;

    while (left > 0) {
        offset_t size = src_read_u32(src);
        if (size == 0) {
            size = left;
        }

        if (size == 1) {
            src_read(src, box.type, 4);
            size = (offset_t)src_read_u64(src);
            left -= 8;
            src_skip(src, size - 16);
            left -= (size - 16);
            if (FOURCC_COMPARE(box.type, "mdat")) {
                if (!hasMoov) {
                    remuxer->is_mdat_at_begining = 1;
                }
                else {
                    remuxer->is_mdat_at_begining = 0;
                }
            }

            continue;
        }

        src_read(src, box.type, 4);

        if (FOURCC_COMPARE(box.type, "mdat")) {
            if (!hasMoov) {
                remuxer->is_mdat_at_begining = 1;
            }
            else {
                remuxer->is_mdat_at_begining = 0;
            }
        }

        if (!FOURCC_COMPARE(box.type, "moov")) {
            src_skip(src, size - 8);
            left -= size;
        } else {
            /* moov */
            moov_buf = (char *)malloc((size_t)(size - 8));
            fread(moov_buf, 1, (size_t)(size - 8), in.fp);
            left = size - 8;
            hasMoov = 1;
            break;
        }
    }

    if (hasMoov) {
        char *temp = moov_buf;
        while (left > 0) {
            box.size = buff_read_u32(temp);
            temp += 4;
            buff_read(temp, box.type, 4);
            temp += 4;
            if (FOURCC_COMPARE(box.type, "avc1") ||
                FOURCC_COMPARE(box.type, "avc3") ||
                FOURCC_COMPARE(box.type, "hvc1") ||
                FOURCC_COMPARE(box.type, "hev1")) {
                    remuxer->insert_track_index = track_index;
                    break;
            }

            if (!FOURCC_COMPARE(box.type, type[i])) {
                /* skip */
                temp += (box.size - 8);
                left -= box.size;
            } else {
                if (i == 0)
                {
                    track_index ++;
                }
                i++;
                if ( i > 4 ) {
                    i = 0;
                    temp += 8;
                    left -= 8;
                }
            }

        }

        free(moov_buf);
    } else {
        ret =  1;
    }

    if (remuxer->insert_track_index == -1)
    {
        ret = 1;
    }

    fclose(in.fp);
    return ret;
}

mp4_remuxer_t *
mp4remux_remuxer_create(void)
{
    mp4_remuxer_t *remuxer = (mp4_remuxer_t*)malloc(sizeof(mp4_remuxer_t));
    if (remuxer == NULL) {
        return NULL;
    }

    memset(remuxer, 0, sizeof(mp4_remuxer_t));

    remuxer->current_track_index = -1;
    remuxer->insert_track_index = -1;
    remuxer->is_mdat_at_begining = 0;
    /* sizeof 'dvcC/dvvC/dvwC'*/
    remuxer->insert_size = 32;

    return remuxer;
}

void
mp4remux_remuxer_destroy(mp4_remuxer_t *remuxer)
{
    free(remuxer);
}
