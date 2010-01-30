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

#ifndef LCFG_PARSER_H
#define LCFG_PARSER_H

#include "lcfg/lcfg.h"

struct lcfg_parser;

struct lcfg_parser *  lcfg_parser_new(struct lcfg *, const char *);
enum lcfg_status      lcfg_parser_run(struct lcfg_parser *);
enum lcfg_status      lcfg_parser_accept(struct lcfg_parser *, lcfg_visitor_function, void *);
void                  lcfg_parser_delete(struct lcfg_parser *);
enum lcfg_status      lcfg_parser_get(struct lcfg_parser *, const char *, void **, size_t *);

#endif
