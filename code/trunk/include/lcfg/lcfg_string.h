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

#ifndef LCFG_STRING_H
#define LCFG_STRING_H

struct lcfg_string *  lcfg_string_new();
struct lcfg_string *  lcfg_string_new_copy(struct lcfg_string *);
int                   lcfg_string_set(struct lcfg_string *, const char *);
int                   lcfg_string_cat_char(struct lcfg_string *, char);
int                   lcfg_string_cat_cstr(struct lcfg_string *, const char *);
int                   lcfg_string_cat_uint(struct lcfg_string *, unsigned int);
int                   lcfg_string_find(struct lcfg_string *, char);
int                   lcfg_string_rfind(struct lcfg_string *, char);
void                  lcfg_string_trunc(struct lcfg_string *, unsigned int);
const char *          lcfg_string_cstr(struct lcfg_string *);
unsigned int          lcfg_string_len(struct lcfg_string *);
void                  lcfg_string_delete(struct lcfg_string *);

#endif
