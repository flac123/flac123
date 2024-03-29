/*
 *  flac123 a command-line flac player
 *  Copyright (C) 2003-2023  Jake Angerman
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

#include <popt.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include "flac123.h"
#include "version.h"

file_info_struct file_info = { NULL, {0,0,0,0}, {0,0,0,0}, NULL, "", 0,0,0,0, false };

static int ao_output_id;

typedef struct {
    char *driver;
    char *buffer_time;
    char *wavfile;
    int remote;
    int quiet;
    int version;
} cli_var_struct;

cli_var_struct cli_args = { NULL, NULL, NULL, 0, 0, 0 };

struct poptOption cli_options[] = {
    /* longName, shortName, argInfo, arg, val, descrip, argDescrip */
    { "driver", 'd', POPT_ARG_STRING, (void *)&(cli_args.driver), 0, "set libao output driver (pulse, macosx, oss, etc).  Default is " AUDIO_DEFAULT, NULL },
    { "wav", 'w', POPT_ARG_STRING, (void *)&(cli_args.wavfile), 0, "send output to wav file (use --wav=- and -q for stdout)", "FILENAME" },
    { "remote", 'R', POPT_ARG_NONE, (void *)&(cli_args.remote), 0, "set remote mode for programmatic control", NULL },
    { "buffer-time", 'b', POPT_ARG_STRING, (void *)&(cli_args.buffer_time), 0, "override default hardware buffer size (in milliseconds)", "INT" },
    { "quiet", 'q', POPT_ARG_NONE, (void *)&(cli_args.quiet), 0, "suppress text output", NULL },
    { "version", 'v', POPT_ARG_NONE, (void *)&(cli_args.version), 0, "version info", NULL},
    POPT_AUTOHELP
    POPT_TABLEEND
};

static void play_file(const char *);
static void play_remote_file(void);
void flac_error_hdl(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus, void *);
void flac_metadata_hdl(const FLAC__StreamDecoder *, const FLAC__StreamMetadata *, void *);
FLAC__StreamDecoderWriteStatus flac_write_hdl(const FLAC__StreamDecoder *,
	const FLAC__Frame *, const FLAC__int32 * const buf[], void *);

static void signal_handler(int);
static int quit_now = 0;
static int interrupted = 0;

float scale = 1;
ao_option **ao_options = NULL;

int main(int argc, const char **argv)
{
    poptContext pc;
    int rc;
    const char *filename;

    setvbuf(stdout, NULL, _IONBF, 0);

    pc = poptGetContext("flac123", argc, argv, cli_options, 0);
    poptSetOtherOptionHelp(pc, "[OPTIONS] FILES...");
    while ((rc=poptGetNextOpt(pc)) >= 0)
    {
    }

    if (rc != -1) {
	fprintf(stderr, "%s: %s\n", poptBadOption(pc, POPT_BADOPTION_NOALIAS), poptStrerror(rc));
	exit(1);
    }

    if (cli_args.version) {
	printf("flac123 version %s\n", FLAC123_VERSION);
        exit(0);
    }

    if (!(cli_args.quiet || cli_args.remote)) {
        printf("flac123 version %s   'flac123 --help' for more info\n", FLAC123_VERSION);
    }

    ao_initialize();

    ao_options = malloc(1024);
    *ao_options = NULL;
    if (cli_args.buffer_time) {
	ao_append_option(ao_options, "buffer_time", cli_args.buffer_time);
    }

    if (! cli_args.wavfile) {
      if (cli_args.driver) {
	ao_output_id = ao_driver_id(cli_args.driver);
	if(ao_output_id < 0) {
	  fprintf(stderr, "Error identifying libao driver %s\n", cli_args.driver);
	  ao_output_id = ao_default_driver_id();
	}
      } else {
	ao_output_id = ao_default_driver_id();
      }

      if (ao_output_id < 0) {
	fprintf(stderr, "No fallback libao driver found, exiting.\n");
	ao_shutdown();
	exit(1);
      }
    }

    if (cli_args.remote)
    {
	play_remote_file();
    }
    else
    {
	if (signal(SIGINT, signal_handler) == SIG_ERR)
	    fprintf(stderr, "signal handler setup failed.\n");

	do {
	    if ((filename = poptGetArg(pc)))
		play_file(filename);
	} while (filename != NULL && !quit_now);
    }

    if (file_info.ao_dev)
	ao_close(file_info.ao_dev);
    ao_shutdown();

    return 0;
}

static void print_file_info(const char *filename)
{
    FLAC__bool got_vorbis = get_vorbis_comments(filename);

    if (cli_args.remote)
    {
	if (got_vorbis)
	{
	  fprintf(stderr, "@I ID3:%s%s%s%s%s%s\n",
		   file_info.title,
		   file_info.artist,
		   file_info.album,
		   file_info.year,
		   file_info.comment,
		   file_info.genre
		);
	}
	else
	{
	    /* print filename without suffix */
	    char *dupe = strdup(filename);
	    char *dot = strrchr(dupe, '.');

	    if (dot && dot == strstr(dupe, ".flac"))
		*dot = '\0';

	    fprintf(stderr, "@I %s\n", dupe);
	    free(dupe);
	}
    }
    else if (!cli_args.quiet)
    {
        printf("\n"
	       "Title  : %s Artist: %s\n"
	       "Album  : %s Year  : %s\n"
	       "Comment: %s Genre : %s\n",
	       file_info.title, file_info.artist,
	       file_info.album, file_info.year,
	       file_info.comment, file_info.genre);
	printf("\nPlaying FLAC stream from %s\n", 
	       strncasecmp(filename, "http://", 7) != 0 && 
	       strchr(filename, '/') ? strrchr(filename, '/')+1 : filename);
	printf("%d bit, %d Hz, %d channels, %lu total samples, "
	       "%.2f total seconds\n", 
	       file_info.sam_fmt.bits, file_info.ao_fmt.rate, 
	       file_info.ao_fmt.channels, file_info.total_samples, 
	       file_info.total_time);
    }
}

FLAC__bool decoder_constructor(const char *filename)
{
    int len = strlen(filename);
    int max_len = len < PATH_MAX ? len : PATH_MAX-1;
    static ao_sample_format previous_bitrate;
    static int first_time = true;

    file_info.filename[max_len] = '\0';
    strncpy(file_info.filename, filename, max_len);

    memset(file_info.title, ' ', VORBIS_TAG_LEN);
    file_info.title[VORBIS_TAG_LEN] = '\0';
    memset(file_info.artist, ' ', VORBIS_TAG_LEN);
    file_info.artist[VORBIS_TAG_LEN] = '\0';
    memset(file_info.album, ' ', VORBIS_TAG_LEN);
    file_info.album[VORBIS_TAG_LEN] = '\0';
    memset(file_info.genre, ' ', VORBIS_TAG_LEN);
    file_info.genre[VORBIS_TAG_LEN] = '\0';
    memset(file_info.comment, ' ', VORBIS_TAG_LEN);
    file_info.comment[VORBIS_TAG_LEN] = '\0';
    memset(file_info.year, ' ', VORBIS_YEAR_LEN);
    file_info.year[VORBIS_YEAR_LEN] = '\0';

    /* create and initialize flac decoder object */
    file_info.decoder = FLAC__stream_decoder_new();
    FLAC__stream_decoder_set_md5_checking(file_info.decoder, true);

    /* read metadata */
    if ((FLAC__stream_decoder_init_file(file_info.decoder, filename, flac_write_hdl, flac_metadata_hdl, flac_error_hdl, (void *)&file_info) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
	|| (!FLAC__stream_decoder_process_until_end_of_metadata(file_info.decoder)))
    {
	FLAC__stream_decoder_delete(file_info.decoder);
	return false;
    }

    /* open libao output device */
    if (cli_args.wavfile) {
	if (!(file_info.ao_dev = ao_open_file(ao_driver_id("wav"), cli_args.wavfile, /*overwrite*/ 1, &(file_info.ao_fmt), NULL)))
	{
	    fprintf(stderr, "Error opening wav file %s\n", cli_args.wavfile);
	    FLAC__stream_decoder_delete(file_info.decoder);
	    return false;
	}
    }
    else if (first_time) {
	if (!(file_info.ao_dev = ao_open_live(ao_output_id, &(file_info.ao_fmt), *ao_options)))
	{
	    fprintf(stderr, "Error opening ao device %d\n", ao_output_id);
	    FLAC__stream_decoder_delete(file_info.decoder);
	    return false;
	}
    }
    else if (previous_bitrate.bits != file_info.ao_fmt.bits ||
	previous_bitrate.rate != file_info.ao_fmt.rate ||
	previous_bitrate.channels != file_info.ao_fmt.channels) 
    {
        /* close the ao_device and re-open */
	ao_close(file_info.ao_dev);
	if (!(file_info.ao_dev = ao_open_live(ao_output_id, &(file_info.ao_fmt), *ao_options)))
	{
	    fprintf(stderr, "Error opening ao device %d\n", ao_output_id);
	    FLAC__stream_decoder_delete(file_info.decoder);
	    return false;
	}
    }

    print_file_info(filename);

    previous_bitrate.bits = file_info.ao_fmt.bits;
    previous_bitrate.rate = file_info.ao_fmt.rate;
    previous_bitrate.channels = file_info.ao_fmt.channels;

    first_time = false;
    file_info.is_loaded  = true;
    file_info.is_playing = true;

    return true;
}

void decoder_destructor(void)
{
    FLAC__stream_decoder_finish(file_info.decoder);
    FLAC__stream_decoder_delete(file_info.decoder);
    file_info.is_loaded  = false;
    file_info.is_playing = false;
    file_info.filename[0] = '\0';
}

static void play_file(const char *filename)
{
    if (!decoder_constructor(filename))
    {
	fprintf(stderr, "Error opening %s\n", filename);
	return;
    }

    while (FLAC__stream_decoder_process_single(file_info.decoder) == true &&
	   FLAC__stream_decoder_get_state(file_info.decoder) <
	   FLAC__STREAM_DECODER_END_OF_STREAM && !interrupted)
    {
    }
    interrupted = 0; /* more accurate feedback if placed after loop */

    decoder_destructor();
}

static void play_remote_file(void)
{
    int status = 0;

    printf("@R FLAC123\n");

    while (status == 0)
    {
	if (file_info.is_playing == true)
	{
	    if (FLAC__stream_decoder_get_state(file_info.decoder) ==
		FLAC__STREAM_DECODER_END_OF_STREAM) 
	    {
		decoder_destructor();
		printf("@P 0\n");
	    }
	    else if (!FLAC__stream_decoder_process_single(file_info.decoder)) 
	    {
		fprintf(stderr, "error decoding single frame!\n");
	    }

	    /* get the next command, no wait */
	    status = remote_get_input_nowait();
	}
	else
	{
	    /* get the next command, wait */
	    status = remote_get_input_wait();
	}
    }
}

void flac_error_hdl(const FLAC__StreamDecoder *dec, 
		    FLAC__StreamDecoderErrorStatus status, void *data)
{
    fprintf(stderr, "error handler called!\n");
}

void flac_metadata_hdl(const FLAC__StreamDecoder *dec, 
		       const FLAC__StreamMetadata *meta, void *data)
{
    file_info_struct *p = (file_info_struct *) data;

    if(meta->type == FLAC__METADATA_TYPE_STREAMINFO) {
	p->sam_fmt.bits = p->ao_fmt.bits = meta->data.stream_info.bits_per_sample;
#ifdef DARWIN
	if (meta->data.stream_info.bits_per_sample == 8 && !cli_args.wavfile)
	    p->ao_fmt.bits = 16;
#endif
	p->ao_fmt.rate = meta->data.stream_info.sample_rate;
	p->ao_fmt.channels = meta->data.stream_info.channels;
	p->ao_fmt.byte_format = AO_FMT_NATIVE;
	FLAC__ASSERT(meta->data.stream_info.total_samples <
		     0x100000000); /* we can handle < 4 gigasamples */
	p->total_samples = (unsigned) 
	    (meta->data.stream_info.total_samples & 0xffffffff);
	p->current_sample = 0;
	p->total_time = (((float) p->total_samples) / p->ao_fmt.rate);
	p->elapsed_time = 0;
    }
}

FLAC__StreamDecoderWriteStatus flac_write_hdl(const FLAC__StreamDecoder *dec, 
					      const FLAC__Frame *frame, 
					      const FLAC__int32 * const buf[], 
					      void *data)
{
    int sample, channel, i;
    unsigned long remaining_samples;
    uint_32 num_samples = frame->header.blocksize;
    file_info_struct *p = (file_info_struct *) data;
    uint_32 decoded_size = frame->header.blocksize * frame->header.channels * (p->ao_fmt.bits / 8);
    float elapsed, remaining_time;
    static uint_8 aobuf[FLAC__MAX_BLOCK_SIZE * FLAC__MAX_CHANNELS * sizeof(sint_32)]; /*oink!*/
    sint_16 *s16aobuf = (sint_16 *) aobuf;
    sint_32 *s32aobuf = (sint_32 *) aobuf;
    uint_8   *u8aobuf = (uint_8  *) aobuf;

    if (p->sam_fmt.bits == 8) {
        for (sample = i = 0; sample < num_samples; sample++) {
	    for(channel = 0; channel < frame->header.channels; channel++,i++) {
		if (cli_args.wavfile) {
		    /* 8 bit wav data is unsigned */
		    u8aobuf[i] = buf[channel][sample] + 0x80;
		} else {
#ifdef DARWIN
		    /* macosx libao expects 16 bit samples */
		    s16aobuf[i] = (sint_16)(buf[channel][sample] << 8);
#else
		    u8aobuf[i] = buf[channel][sample] + 0x80;
#endif
		}
	    }
	} 
    } else if (p->sam_fmt.bits == 16) {
        for (sample = i = 0; sample < num_samples; sample++) {
	    for(channel = 0; channel < frame->header.channels; channel++,i++) {
		s16aobuf[i] = (sint_16)(buf[channel][sample] * scale);
	    }
        }
    } else if (p->sam_fmt.bits == 24) {
        for (sample = i = 0; sample < num_samples; sample++) {
	    for(channel = 0; channel < frame->header.channels; channel++,i+=3) {
                sint_32 scaled_sample = (sint_32)(buf[channel][sample] * scale);

                u8aobuf[i]   = (scaled_sample >>  0) & 0xFF;
                u8aobuf[i+1] = (scaled_sample >>  8) & 0xFF;
                u8aobuf[i+2] = (scaled_sample >> 16) & 0xFF;
	    }
	} 
    }

    ao_play(p->ao_dev, (char *)aobuf, decoded_size);

    p->current_sample += num_samples;
    elapsed = ((float) num_samples) / frame->header.sample_rate;
    p->elapsed_time += elapsed;

    if (cli_args.remote) {
	remaining_samples = p->total_samples > 0 ? p->total_samples - p->current_sample : p->current_sample;
    	if ((remaining_time = p->total_time - p->elapsed_time) < 0)
	    remaining_time = 0;

 	printf("@F %lu %lu %.2f %.2f\n", p->current_sample, remaining_samples, p->elapsed_time, remaining_time);
    }

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void signal_handler(int sig) {
    static struct timeval last_time = { 0, 0 };
    struct timeval current_time;

    gettimeofday(&current_time, /*no timezone*/ NULL);

    if (sig == SIGINT) {
	interrupted = 1;

	if (current_time.tv_sec - last_time.tv_sec < 1) {
	    /* multiple SIGINT received in short succession */
	    quit_now = 1;
	}

	last_time.tv_sec = current_time.tv_sec;
	last_time.tv_usec = current_time.tv_usec;
    }
}
