/*
  Copyright (c) 2012, Paul Baecher
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.
  
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <THE COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

