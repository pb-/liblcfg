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

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "lcfg/lcfg_token.h"
#include "lcfg/lcfg_scanner.h"
#include "lcfg/lcfg_parser.h"

#ifndef strdup
char *strdup(const char *s)
{
	char *sdup;

	size_t len = strlen(s) + 1;
	sdup = malloc(len);

	if( sdup != NULL )
	{
		memcpy(sdup, s, len);
	}

	return sdup;
}
#endif

struct lcfg_parser_value_pair
{
	char *key;
	struct lcfg_string *value;
};


struct lcfg_parser
{
	struct lcfg *lcfg;
	char *filename;
	struct lcfg_scanner *scanner;

	struct lcfg_parser_value_pair *values;
	unsigned int value_length;
	unsigned int value_capacity;
};

static int lcfg_parser_add_value(struct lcfg_parser *p, const char *key, struct lcfg_string *value)
{
	if( p->value_length == p->value_capacity )
	{
		p->value_capacity *= 2;
		p->values = realloc(p->values, sizeof(struct lcfg_parser_value_pair) * p->value_capacity);
		assert(p->values);
	}

	p->values[p->value_length].key = strdup(key);
	p->values[p->value_length].value = lcfg_string_new_copy(value);

	return ++p->value_length;
}

struct lcfg_parser *lcfg_parser_new(struct lcfg *c, const char *filename)
{
	struct lcfg_parser *p = malloc(sizeof(struct lcfg_parser));
	assert(p);

	memset(p, 0, sizeof(struct lcfg_parser));

	p->filename = strdup(filename);
	p->lcfg = c;

	p->value_length = 0;
	p->value_capacity = 8;
	p->values = malloc(sizeof(struct lcfg_parser_value_pair) * p->value_capacity);
	assert(p->values);

	return p;
}

/* this is a basic push down automata */
static enum lcfg_status lcfg_parser_parse(struct lcfg_parser *p)
{
	enum state { top_level = 0, exp_equals, exp_value, in_list, in_map, invalid };
	/*const char *state_map[] = { "top_level", "exp_equals", "exp_value", "in_list", "in_map", "invalid" };*/

	struct state_element
	{
		enum state s;
		int list_counter;
	};

	/* start of ugly preproc stuff */
#define STATE_STACK_PUSH(t) \
	if( ssi + 1 == state_stack_size ) \
	{ \
		state_stack_size *= 2; \
		state_stack = realloc(state_stack, state_stack_size * sizeof(struct state_element)); \
	} \
	state_stack[++ssi].s = t; \
	state_stack[ssi].list_counter = 0
#define STATE_STACK_POP() ssi--
#define PATH_PUSH_STR(s) \
	if( lcfg_string_len(current_path) != 0 ) \
	{ \
		lcfg_string_cat_char(current_path, '.'); \
	} \
	lcfg_string_cat_cstr(current_path, s);
#define PATH_PUSH_INT(i) \
	if( lcfg_string_len(current_path) != 0 ) \
	{ \
		lcfg_string_cat_char(current_path, '.'); \
	} \
	lcfg_string_cat_uint(current_path, i);
#define PATH_POP() \
	if( lcfg_string_rfind(current_path, '.') != -1 ) \
	{ \
		lcfg_string_trunc(current_path, lcfg_string_rfind(current_path, '.')); \
	} \
	else \
	{ \
		lcfg_string_trunc(current_path, 0); \
	}
	/* end of ugly preproc stuff */

	if( lcfg_scanner_init(p->scanner) != lcfg_status_ok )
	{
		return lcfg_status_error;
	}

	int state_stack_size = 8;
	int ssi = 0; /* ssi = state stack index */
	struct state_element *state_stack = malloc(sizeof(struct state_element) * state_stack_size);

	state_stack[ssi].s = top_level;
	state_stack[ssi].list_counter = 0;

	struct lcfg_token t;
	struct lcfg_string *current_path = lcfg_string_new();

	while( lcfg_scanner_has_next(p->scanner) && state_stack[ssi].s != invalid )
	{
		if( lcfg_scanner_next_token(p->scanner, &t) != lcfg_status_ok )
		{
			free(state_stack);
			lcfg_string_delete(t.string);
			lcfg_string_delete(current_path);
			return lcfg_status_error;
		}

		switch( state_stack[ssi].s )
		{
			case top_level:
			case in_map:
				if( t.type == lcfg_identifier )
				{
					PATH_PUSH_STR(lcfg_string_cstr(t.string));
					STATE_STACK_PUSH(exp_equals);
				}
				else if( state_stack[ssi].s == in_map && t.type == lcfg_brace_close )
				{
					STATE_STACK_POP();
					PATH_POP();
				}
				else
				{
					lcfg_error_set(p->lcfg, "invalid token (%s) near line %d column %d: expected identifier%s", lcfg_token_map[t.type], t.line, t.col, state_stack[ssi].s == in_map ? " or `}'" : "");
					state_stack[ssi].s = invalid;
				}
				break;
			case exp_equals:
				if( t.type == lcfg_equals )
					state_stack[ssi].s = exp_value;
				else
				{
					lcfg_error_set(p->lcfg, "invalid token (%s) near line %d column %d: expected `='", lcfg_token_map[t.type], t.line, t.col);
					state_stack[ssi].s = invalid;
				}
				break;
			case exp_value:
				if( t.type == lcfg_string )
				{
					lcfg_parser_add_value(p, lcfg_string_cstr(current_path), t.string);
					/*printf("adding string value for single statement\n");*/
					STATE_STACK_POP();
					PATH_POP();
				}
				else if( t.type == lcfg_sbracket_open )
				{
					state_stack[ssi].s = in_list;
				}
				else if( t.type == lcfg_brace_open )
				{
					state_stack[ssi].s = in_map;
				}
				else
				{
					lcfg_error_set(p->lcfg, "invalid token (%s) near line %d column %d: expected string, `[' or `{'", lcfg_token_map[t.type], t.line, t.col);
					state_stack[ssi].s = invalid;
				}
				break;
			case in_list:
				if( t.type == lcfg_comma ); /* ignore comma */
				else if( t.type == lcfg_string )
				{
					PATH_PUSH_INT(state_stack[ssi].list_counter);
					lcfg_parser_add_value(p, lcfg_string_cstr(current_path), t.string);
					PATH_POP();
					/*printf("adding string to list pos %d\n", state_stack[ssi].list_counter);*/
					state_stack[ssi].list_counter++;
				}
				else if( t.type == lcfg_sbracket_open )
				{
					PATH_PUSH_INT(state_stack[ssi].list_counter);
					/*printf("adding list to list pos %d\n", state_stack[ssi].list_counter);*/
					state_stack[ssi].list_counter++;
					STATE_STACK_PUSH(in_list);
				}
				else if( t.type == lcfg_brace_open )
				{
					PATH_PUSH_INT(state_stack[ssi].list_counter);
					/*printf("adding map to list pos %d\n", state_stack[ssi].list_counter);*/
					state_stack[ssi].list_counter++;
					STATE_STACK_PUSH(in_map);
				}
				else if( t.type == lcfg_sbracket_close )
				{
					PATH_POP();
					STATE_STACK_POP();
				}
				else
				{
					lcfg_error_set(p->lcfg, "invalid token (%s) near line %d column %d: expected string, `[', `{', `,' or `]'", lcfg_token_map[t.type], t.line, t.col);
					state_stack[ssi].s = invalid;
				}
				break;
			case invalid: /* unreachable */
				assert(0);
				break;
		}

		lcfg_string_delete(t.string);

		/*printf(" *** pda: read %s, state is now %s\n", lcfg_token_map[t.type], state_map[state_stack[ssi].s]);*/
	}

	lcfg_string_delete(current_path);

	if( state_stack[ssi].s == top_level && ssi == 0 )
	{
		free(state_stack);
		return lcfg_status_ok;
	}
	else
	{
		free(state_stack);
		lcfg_error_set(p->lcfg, "%s", "unexpected end of file: unterminated list/map?");
		return lcfg_status_error;
	}
}

enum lcfg_status lcfg_parser_run(struct lcfg_parser *p)
{
	int fd = open(p->filename, 0);
	enum lcfg_status status;

	if( fd < 0 )
	{
		lcfg_error_set(p->lcfg, "open(): %s", strerror(errno));
		return lcfg_status_error;
	}

	p->scanner = lcfg_scanner_new(p->lcfg, fd);

	status = lcfg_parser_parse(p);

	close(fd);

	return status;
}
enum lcfg_status lcfg_parser_accept(struct lcfg_parser *p, lcfg_visitor_function fn, void *user_data)
{
	int i;

	for( i = 0; i < p->value_length; i++ )
	{
		if( fn(p->values[i].key, (void *)lcfg_string_cstr(p->values[i].value), lcfg_string_len(p->values[i].value), user_data) != lcfg_status_ok )
		{
			lcfg_error_set(p->lcfg, "%s", "configuration value traversal aborted upon user request");
			return lcfg_status_error;
		}
	}

	return lcfg_status_ok;
}

enum lcfg_status lcfg_parser_get(struct lcfg_parser *p, const char *key, void **data, size_t *len)
{
	int i;

	for( i = 0; i < p->value_length; i++ )
	{
		if( !strcmp(p->values[i].key, key) )
		{
			*data = (void *)lcfg_string_cstr(p->values[i].value);
			*len = lcfg_string_len(p->values[i].value);
			return lcfg_status_ok;
		}
	}

	return lcfg_status_error;
}


void lcfg_parser_delete(struct lcfg_parser *p)
{
	if( p->scanner != NULL )
	{
		lcfg_scanner_delete(p->scanner);
	}

	int i;

	for( i = 0; i < p->value_length; i++ )
	{
		free(p->values[i].key);
		lcfg_string_delete(p->values[i].value);
	}
	free(p->values);
	free(p->filename);
	free(p);
}
