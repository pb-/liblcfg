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

#ifndef LCFG_TOKEN_H
#define LCFG_TOKEN_H

#include "lcfg/lcfg_string.h"

enum lcfg_token_type
{
	lcfg_null_token = 0,
	lcfg_identifier,
	lcfg_equals,
	lcfg_string,
	lcfg_sbracket_open,
	lcfg_sbracket_close,
	lcfg_comma,
	lcfg_brace_open,
	lcfg_brace_close
};

extern const char *lcfg_token_map[];

struct lcfg_token
{
	enum lcfg_token_type type;
	struct lcfg_string *string;
	short line;
	short col;
};


#endif
