/*
 * liblcfg - lightweight configuration file library
 * Copyright (c) 2007--2009  Paul Baecher
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *    $Id$
 *
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "lcfg/lcfg_string.h"

struct lcfg_string
{
	char *str;
	unsigned int size;
	unsigned int capacity;
};

int lcfg_string_set(struct lcfg_string *s, const char *cstr)
{
	lcfg_string_trunc(s, 0);
	return lcfg_string_cat_cstr(s, cstr);
}


/* make sure new_size bytes fit into the string */
inline static void lcfg_string_grow(struct lcfg_string *s, unsigned int new_size)
{
	/* always allocate one byte more than needed
	 * to make _cstr() working in any case without realloc. */
	while( (new_size + 1) > s->capacity )
	{
		s->capacity *= 2;
		s->str = realloc(s->str, s->capacity);
	}
}

struct lcfg_string *lcfg_string_new()
{
	struct lcfg_string *s = malloc(sizeof(struct lcfg_string));
	assert(s);

	s->capacity = 8;
	s->size = 0;
	s->str = malloc(s->capacity);

	assert(s->str);

	return s;
}

struct lcfg_string *lcfg_string_new_copy(struct lcfg_string *s)
{
	struct lcfg_string *s_new = malloc(sizeof(struct lcfg_string));
	assert(s_new);

	s_new->capacity = s->capacity;
	s_new->size = s->size;
	s_new->str = malloc(s_new->capacity);

	memcpy(s_new->str, s->str, s_new->size);

	return s_new;
}

int lcfg_string_cat_uint(struct lcfg_string *s, unsigned int i)
{
	unsigned int size_needed = 1;
	unsigned int ii = i;
	char c;

	while( ii >= 10 )
	{
		size_needed++;
		ii /= 10;
	}

	lcfg_string_grow(s, s->size + size_needed);

	ii = size_needed - 1;
	do
	{
		c = '0' + i % 10;
		s->str[s->size + ii--] = c;
		i /= 10;
	}
	while( i != 0 );

	s->size += size_needed;

	return s->size;
}

int lcfg_string_find(struct lcfg_string *s, char c)
{
	int i;

	for( i = 0; i < s->size; i++ )
	{
		if( s->str[i] == c )
		{
			return i;
		}
	}

	return -1;
}

int lcfg_string_rfind(struct lcfg_string *s, char c)
{
	int i;

	for( i = s->size - 1; i >= 0; i-- )
	{
		if( s->str[i] == c )
		{
			return i;
		}
	}

	return -1;
}

void lcfg_string_trunc(struct lcfg_string *s, unsigned int max_size)
{
	if( max_size < s->size )
	{
		s->size = max_size;
	}
}

int lcfg_string_cat_cstr(struct lcfg_string *s, const char *cstr)
{
	size_t len = strlen(cstr);

	lcfg_string_grow(s, s->size + len);

	memcpy(s->str + s->size, cstr, len);

	s->size += len;

	return s->size;
}

int lcfg_string_cat_char(struct lcfg_string *s, char c)
{
	lcfg_string_grow(s, s->size + 1);

	s->str[s->size++] = c;

	return s->size;
}

const char *lcfg_string_cstr(struct lcfg_string *s)
{
	s->str[s->size] = '\0';
	return s->str;
}

unsigned int lcfg_string_len(struct lcfg_string *s)
{
	return s->size;
}

void lcfg_string_delete(struct lcfg_string *s)
{
	free(s->str);
	free(s);
}
