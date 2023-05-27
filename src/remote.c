/*
 *  flac123 a command-line flac player
 *  Copyright (C) 2003-2023  Jake Angerman
 *
 *  This remote.c module heavily based upon:
 *  mpg321 - a fully free clone of mpg123.  Copyright (C) 2001 Joe Drew
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

#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "flac123.h"

/* for filename plus command and a space */
#define BUF_SIZE (PATH_MAX + 5)

static char remote_input_buf[BUF_SIZE];

static void trim_whitespace(char *str)
/* logic from stackoverflow 122616 */
{
    size_t len = 0;
    char *frontp = str;
    char *endp = NULL;

    if (NULL == str || '\0' == *str)
    {
        return;
    }

    len = strlen(str);
    endp = str + len;

    /* Move the front and back pointers to address the first non-whitespace
     * characters from each end.
     */
    while (isspace( (unsigned char) *frontp))
    {
        frontp++;
    }
    if (endp != frontp)
    {
        while( isspace((unsigned char) *(--endp)) && endp != frontp ) {}
    }

    if (frontp != str && endp == frontp)
    {
        *str = '\0'; /* it was all whitespace */
    }
    else if (str + len - 1 != endp)
    {
       *(endp + 1) = '\0'; /* there was trailing whitespace */
    }

    /* Shift the string so that it starts at str so that if it's dynamically
     * allocated, we can still free it on the returned pointer.  Note the reuse
     * of endp to mean the front of the string buffer now.
     */
    endp = str;
    if (frontp != str)
    {
        while (*frontp)
        {
            *endp++ = *frontp++;
        }
        *endp = '\0';
    }
}

/* returns 0 on success (keep decoding), or -1 on QUIT (stop decoding) */
static int remote_parse_input(void)
{
    char input[BUF_SIZE]; /* full line of a command plus argument */
    char tmp_remote_input_buf[BUF_SIZE];
    char *arg, *newline;
    int num_read = 0;
    int already_read;
    int linelen;

    fd_set fd;
    struct timeval tv = { 0, 0 };
    FD_ZERO(&fd);
    FD_SET(0,&fd); /* stdin */

    already_read = strlen(remote_input_buf);

    if (select (1, &fd, NULL, NULL, &tv))  /* returns immediately */
    {
	if ((num_read = read(0, remote_input_buf + already_read, (sizeof(input)-1)-already_read)) < 0)
	{
            num_read = 0; /* should never happen.  read() blocks */
	} else if (0 == num_read && 0 == already_read) {
            return 0; /* EOF */
	}
    } else {
	num_read = 0;
    }

    remote_input_buf[already_read + num_read] = '\0';

    if ((newline = strchr(remote_input_buf, '\n')))
    {
        *(newline) = '\0';
    }


    linelen = strlen(remote_input_buf);

    strcpy(input, remote_input_buf);
    strcpy(tmp_remote_input_buf, remote_input_buf + linelen + 1); /* +1 skips \0 */
    strcpy(remote_input_buf, tmp_remote_input_buf);

    trim_whitespace(input);

    if (strlen(input) == 0)
        return 0;

    arg = strchr(input, ' ');

    if (arg)
    {
        *(arg++) = '\0';  /* separate command from argument */
        arg = strdup(arg);
        trim_whitespace(arg);
    }

    if (strcasecmp(input, "L") == 0 || strcasecmp(input, "LOAD") == 0)
    {
        if (arg)
        {
	    if (file_info.is_loaded == true)
		decoder_destructor();

	    if (!decoder_constructor(arg))
	    {
		fprintf(stderr, "@E Error opening %s\n", arg);
		free(arg);
		return 0; 
	    }
        }
        else
        {
            fprintf(stderr, "@E Missing argument to '%s'\n", input);
            return 0;
        }
    }

    else if (strcasecmp(input, "J") == 0 || strcasecmp(input, "JUMP") == 0)
    {
        if (file_info.is_playing && arg)
        {
            /* relative seek */
            if (arg[0] == '-' || arg[0] == '+')
            {
		/* assumption: fixed bit-rate encoding */
		signed long delta_time = atol(arg);
		signed long delta_frames = delta_time * file_info.ao_fmt.rate;
		    
		if ((delta_time < 0) && (labs(delta_time) > file_info.elapsed_time))
		{
		    file_info.elapsed_time = 0;
		    file_info.current_sample = 0;
		}
		else if ((delta_time > 0) && (delta_time > file_info.total_time - file_info.elapsed_time))
		{
		    /* don't do anything */
		    if (arg)
			free (arg);
		    return 0;
		}
		else
		{
		    file_info.elapsed_time += delta_time;
		    file_info.current_sample += delta_frames;
		}

		FLAC__stream_decoder_seek_absolute(file_info.decoder,
						 file_info.current_sample);
            }
	    /* absolute seek */
            else
            {
		long absolute_time = atol(arg);
		long absolute_frame = absolute_time * file_info.ao_fmt.rate;
		file_info.elapsed_time = absolute_time;
		file_info.current_sample = absolute_frame;

		FLAC__stream_decoder_seek_absolute(file_info.decoder, absolute_frame);
            }

        }
        else
        {
            /* mpg123 does no error checking, so we should emulate that */
        }                        
    }

    else if (strcasecmp(input, "S") == 0 || strcasecmp(input, "STOP") == 0)
    {
	if (file_info.is_loaded == true)
	{
	    printf("@P 0\n");
	    decoder_destructor();
	}
    }

    else if (strcasecmp(input, "V") == 0 || strcasecmp(input, "VOLUME") == 0)
    {
        if (arg)
	{
	    scale = atof(arg);
	    printf("@V %f\n", scale);
	}
    }

    else if (strcasecmp(input, "P") == 0 || strcasecmp(input, "PAUSE") == 0)
    {
	if (file_info.is_loaded == true)
	{
	    if (file_info.is_playing == true)
	    {
		file_info.is_playing = false;
		printf("@P 1\n");
	    }
	    else
	    {
		file_info.is_playing = true;
		printf("@P 2\n");
	    }
	}
    }

    else if (strcasecmp(input, "Q") == 0 || strcasecmp(input, "QUIT") == 0)
    {
	if (arg)
	    free(arg);

	if (file_info.is_loaded)
	    decoder_destructor();
      
	return -1;
    }

    else 
    {
        fprintf(stderr, "@E Unknown command '%s'\n", input);
    }

    if (arg) 
        free(arg);

    return 0;
}

int remote_get_input_wait(void)
{
    fd_set fd;
    FD_ZERO(&fd);
    FD_SET(0,&fd);
    
    if (strlen(remote_input_buf) == 0)
    {
	select(1, &fd, NULL, NULL, NULL); /* block indefinitely */
    }

    return remote_parse_input();
}

int remote_get_input_nowait(void)
{
    fd_set fd;
    struct timeval tv = { 0, 0 };
    FD_ZERO(&fd);
    FD_SET(0,&fd);

    if (strlen(remote_input_buf) == 0)
    {
	if (select(1, &fd, NULL, NULL, &tv)) /* return immediately */
	    return remote_parse_input();
	else
	    return 0;
    } 
    else 
    {
	return remote_parse_input();
    }
}

