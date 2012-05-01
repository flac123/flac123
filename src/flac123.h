/*
 *  flac123 a command-line flac player
 *  Copyright (C) 2003-2007  Jake Angerman
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <ao/ao.h>
#include <limits.h>
#include <FLAC/all.h>

/* by LEGACY_FLAC we mean pre-1.1.3 before FLAC__FileDecoder was merged into FLAC__StreamDecoder */
#if !defined(FLAC_API_VERSION_CURRENT) || FLAC_API_VERSION_CURRENT < 8
#define LEGACY_FLAC
#else
#undef LEGACY_FLAC
#endif

/* string widths for printing ID3 (vorbis) data in remote mode */
#define VORBIS_TAG_LEN 30
#define VORBIS_YEAR_LEN 4

/* the main data structure of the program */
typedef struct {
#ifdef LEGACY_FLAC
    FLAC__FileDecoder *decoder;
#else
    FLAC__StreamDecoder *decoder;
#endif

    /* bits, rate, channels, byte_format */
    ao_sample_format sam_fmt; /* input sample's true format */
    ao_sample_format ao_fmt;  /* libao output format */

    ao_device *ao_dev;
    char filename[PATH_MAX];
    unsigned long total_samples;
    unsigned long current_sample;
    float total_time;        /* seconds */
    float elapsed_time;      /* seconds */
    FLAC__bool is_loaded;    /* loaded or not? */
    FLAC__bool is_playing;   /* playing or not? */
    char title[VORBIS_TAG_LEN+1]; 
    char artist[VORBIS_TAG_LEN+1];
    char album[VORBIS_TAG_LEN+1];    /* +1 for \0 */
    char genre[VORBIS_TAG_LEN+1];
    char comment[VORBIS_TAG_LEN+1];
    char year[VORBIS_YEAR_LEN+1];
} file_info_struct;

extern file_info_struct file_info;

extern FLAC__bool decoder_constructor(const char *filename);
extern void decoder_destructor(void);
extern int remote_get_input_wait(void);
extern int remote_get_input_nowait(void);
extern FLAC__bool get_vorbis_comments(const char *filename);

extern float scale;
