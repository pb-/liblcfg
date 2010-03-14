/*
 * liblcfg - lightweight configuration file library
 * Copyright (c) 2007--2010  Paul Baecher
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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "lcfg/lcfg.h"
#include "lcfg/lcfg_parser.h"

struct lcfg
{
	char error[0xff];
	struct lcfg_parser *parser;
};

struct lcfg *lcfg_new(const char *filename)
{
	struct lcfg *c = malloc(sizeof(struct lcfg));
	assert(c);
	memset(c, 0, sizeof(struct lcfg));

	c->parser = lcfg_parser_new(c, filename);
	assert(c->parser);

	return c;
}

void lcfg_delete(struct lcfg *c)
{
	lcfg_parser_delete(c->parser);
	free(c);
}

const char *lcfg_error_get(struct lcfg *c)
{
	return c->error;
}

enum lcfg_status lcfg_parse(struct lcfg *c)
{
	return lcfg_parser_run(c->parser);
}

enum lcfg_status lcfg_accept(struct lcfg *c, lcfg_visitor_function fn, void *user_data)
{
	return lcfg_parser_accept(c->parser, fn, user_data);
}

enum lcfg_status lcfg_value_get(struct lcfg *c, const char *key, void **data, size_t *len)
{
	return lcfg_parser_get(c->parser, key, data, len);
}

void lcfg_error_set(struct lcfg *c, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(c->error, sizeof(c->error), fmt, ap);
	va_end(ap);
}

