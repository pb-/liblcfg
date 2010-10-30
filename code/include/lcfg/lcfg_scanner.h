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
 *    
 *
 */

#ifndef LCFG_SCANNER_H
#define LCFG_SCANNER_H

#include "lcfg/lcfg.h"

struct lcfg_scanner;
struct lcfg_token;

struct lcfg_scanner *    lcfg_scanner_new(struct lcfg *, int fd);
enum lcfg_status         lcfg_scanner_init(struct lcfg_scanner *);
enum lcfg_status         lcfg_scanner_next_token(struct lcfg_scanner *, struct lcfg_token *);
int                      lcfg_scanner_has_next(struct lcfg_scanner *);
void                     lcfg_scanner_delete(struct lcfg_scanner *);

#endif
