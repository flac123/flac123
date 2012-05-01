/*
 *  flac123 a command-line flac player
 *  Copyright (C) 2003-2007  Jake Angerman
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
#include "flac123.h"

#define BUF_SIZE (PATH_MAX + 5)

char remote_input_buf[BUF_SIZE];

static void trim_whitespace(char *str)
{
    char *ptr = str;
    register int pos = strlen(str)-1;

    while(isspace(ptr[pos]))
        ptr[pos--] = '\0';
    
    while(isspace(*ptr))
        ptr++;
    
    strncpy(str, ptr, pos+1);
}

/* returns 0 on success (keep decoding), or -1 on QUIT (stop decoding) */
static int remote_parse_input(void)
{
    char input[BUF_SIZE]; /* for filename as well as input and space */
    char new_remote_input_buf[BUF_SIZE];
    char *arg, *newline;
    int numread;
    int alreadyread;
    int linelen;

    fd_set fd;
    struct timeval tv = { 0, 0 };
    FD_ZERO(&fd);
    FD_SET(0,&fd);

    alreadyread = strlen (remote_input_buf);

    if (select (1, &fd, NULL, NULL, &tv))  /* return immediately */
    {
	if (!(numread = read(0, remote_input_buf + alreadyread, (sizeof(input)-1)-alreadyread)) > 0)
	{
	    numread = 0; /* should never happen.  read() blocks */
	}
    } else {
	numread = 0;
    }

    remote_input_buf[numread+alreadyread] = '\0';
    
    if ((newline = strchr(remote_input_buf, '\n')))
    {
        *(newline) = '\0';
    }

    linelen = strlen (remote_input_buf);

    strcpy (input, remote_input_buf);
    /* input[linelen] = '\0'; */
    strcpy (new_remote_input_buf, remote_input_buf + linelen + 1);
    /* don't copy the \0... */
    strcpy (remote_input_buf, new_remote_input_buf);

    trim_whitespace(input); /* from left and right */

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

#ifdef LEGACY_FLAC
		FLAC__file_decoder_seek_absolute(file_info.decoder,
						 file_info.current_sample);
#else
		FLAC__stream_decoder_seek_absolute(file_info.decoder,
						 file_info.current_sample);
#endif
            }
	    /* absolute seek */
            else
            {
		long absolute_time = atol(arg);
		long absolute_frame = absolute_time * file_info.ao_fmt.rate;
		file_info.elapsed_time = absolute_time;
		file_info.current_sample = absolute_frame;

#ifdef LEGACY_FLAC
		FLAC__file_decoder_seek_absolute(file_info.decoder, absolute_frame);
#else
		FLAC__stream_decoder_seek_absolute(file_info.decoder, absolute_frame);
#endif
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
	    fprintf(stderr, "@P 0\n");
	    decoder_destructor();
	}
    }

    else if (strcasecmp(input, "V") == 0 || strcasecmp(input, "VOLUME") == 0)
    {
        if (arg)
	{
	    scale = atof(arg);
	    fprintf(stderr, "@V %f\n", scale);
	}
    }

    else if (strcasecmp(input, "P") == 0 || strcasecmp(input, "PAUSE") == 0)
    {
	if (file_info.is_loaded == true)
	{
	    if (file_info.is_playing == true)
	    {
		file_info.is_playing = false;
		fprintf(stderr, "@P 1\n");
	    }
	    else
	    {
		file_info.is_playing = true;
		fprintf(stderr, "@P 2\n");
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

