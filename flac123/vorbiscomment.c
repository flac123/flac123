/* 
 *  flac123 a command-line flac player
 *  Copyright (C) 2003-2007  Jake Angerman
 *
 *  This vorbiscomment.c module heavily based upon:
 *  plugin_common - Routines common to several xmms plugins
 *  Copyright (C) 2002,2003  Josh Coalson
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include "flac123.h"

static int local__vcentry_matches(const char *field_name, const FLAC__StreamMetadata_VorbisComment_Entry *entry)
{
    const FLAC__byte *eq = memchr(entry->entry, '=', entry->length);
    const unsigned field_name_length = strlen(field_name);
    return (0 != eq && (unsigned)(eq-entry->entry) == field_name_length && 0 == strncasecmp(field_name, entry->entry, field_name_length));
}

/* parse a string entry and put it into dest which is of size len.
 * len is the width of non-null bytes.
 */
static void local__vcentry_parse_value(const FLAC__StreamMetadata_VorbisComment_Entry *entry, char *dest, int len)
{
    const FLAC__byte * const eq = memchr(entry->entry, '=', entry->length);

    if(!eq || !dest)
	return;
    else {
	unsigned value_length = entry->length - (unsigned)((eq+1) - entry->entry);
	if(value_length > len) {
	    value_length = len;
	}

	memset(dest, ' ', len); 
	memcpy(dest, eq+1, value_length);
	dest[len] = '\0'; /* assumes char dest[len+1] */
    }
}

FLAC__bool get_vorbis_comments(const char *filename)
{
    FLAC__Metadata_SimpleIterator *iterator = FLAC__metadata_simple_iterator_new();
    FLAC__bool got_vorbis_comments = false;

    if(0 != iterator) {
	if(FLAC__metadata_simple_iterator_init(iterator, filename, /*read_only=*/true, /*preserve_file_stats=*/true)) {
	    do {
		if(FLAC__metadata_simple_iterator_get_block_type(iterator) == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
		    FLAC__StreamMetadata *block = FLAC__metadata_simple_iterator_get_block(iterator);
		    if(0 != block) {
			int i;
			const FLAC__StreamMetadata_VorbisComment *vc = &block->data.vorbis_comment;
			for(i = 0; i < vc->num_comments; i++) {
			    if(local__vcentry_matches("artist", &vc->comments[i]))
			    {
				local__vcentry_parse_value(&vc->comments[i], file_info.artist, VORBIS_TAG_LEN);
				got_vorbis_comments = true;
			    }
			    else if(local__vcentry_matches("album", &vc->comments[i]))
			    {
				local__vcentry_parse_value(&vc->comments[i], file_info.album, VORBIS_TAG_LEN);
				got_vorbis_comments = true;
			    }
			    else if(local__vcentry_matches("title", &vc->comments[i]))
			    {
				local__vcentry_parse_value(&vc->comments[i], file_info.title, VORBIS_TAG_LEN);
				got_vorbis_comments = true;
			    }
			    else if(local__vcentry_matches("genre", &vc->comments[i]))
			    {
				local__vcentry_parse_value(&vc->comments[i], file_info.genre, VORBIS_TAG_LEN);
				got_vorbis_comments = true;
			    }
			    else if(local__vcentry_matches("description", &vc->comments[i]))
			    {
				local__vcentry_parse_value(&vc->comments[i], file_info.comment, VORBIS_TAG_LEN);
				got_vorbis_comments = true;
			    }
			    else if(local__vcentry_matches("date", &vc->comments[i]))
			    {
				local__vcentry_parse_value(&vc->comments[i], file_info.year, VORBIS_YEAR_LEN);
				got_vorbis_comments = true;
			    }
			}
			FLAC__metadata_object_delete(block);
		    }
		}
	    } while (!got_vorbis_comments && FLAC__metadata_simple_iterator_next(iterator));
	}
	FLAC__metadata_simple_iterator_delete(iterator);
    }
    return got_vorbis_comments;
}
