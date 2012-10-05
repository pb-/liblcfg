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
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include "lcfg/lcfg_scanner.h"
#include "lcfg/lcfg_token.h"

#define BUFFER_SIZE 0xff

struct lcfg_scanner
{
	struct lcfg *lcfg;

	int fd;
	char buffer[BUFFER_SIZE];
	int offset;
	int size;
	int eof;

	short line;
	short col;

	struct lcfg_token prepared_token;
	int token_eof;
};


static enum lcfg_status lcfg_scanner_buffer_fill(struct lcfg_scanner *s)
{
	if( (s->size = read(s->fd, s->buffer, BUFFER_SIZE)) < 0 )
	{
		lcfg_error_set(s->lcfg, "read(): %s", strerror(errno));
		return lcfg_status_error;
	}
	else if( s->size == 0 )
	{
		s->eof = !0;
	}
	else
	{
		s->offset = 0;
	}

	return lcfg_status_ok;
}

static inline int lcfg_scanner_char_eof(struct lcfg_scanner *s)
{
	if( s->eof )
	{
		return !0;
	}
	else
	{
		if( s->size == 0 || s->offset == BUFFER_SIZE )
		{
			lcfg_scanner_buffer_fill(s);
		}
		if( s->size < BUFFER_SIZE && s->offset == s->size )
		{
			s->eof = !0;
		}

		return s->eof;
	}
}

static enum lcfg_status lcfg_scanner_char_read(struct lcfg_scanner *s, char *c)
{
	if( lcfg_scanner_char_eof(s) )
	{
		lcfg_error_set(s->lcfg, "%s", "cannot read beyond eof");
		return lcfg_status_error;
	}

	*c = s->buffer[s->offset++];

	return lcfg_status_ok;
}

static enum lcfg_status lcfg_scanner_char_peek(struct lcfg_scanner *s, char *c)
{
	if( lcfg_scanner_char_eof(s) )
	{
		lcfg_error_set(s->lcfg, "%s", "cannot peek beyond eof");
		return lcfg_status_error;
	}

	*c = s->buffer[s->offset];

	return lcfg_status_ok;
}

/* the beautiful lowlevel fsm */
static enum lcfg_status lcfg_scanner_token_read(struct lcfg_scanner *s)
{
	enum scanner_state { start = 0, comm_start, in_oneline, in_multiline, multiline_end, in_identifier, in_str, in_esc, esc_hex_exp_first, esc_hex_exp_second, invalid };
	enum scanner_state state = start;
	char c = '\0';
	char hex[3];

	s->prepared_token.type = lcfg_null_token;

	while( !lcfg_scanner_char_eof(s) )
	{
		int consume = !0;
		lcfg_scanner_char_peek(s, &c);

		switch( state )
		{
			case start:
				switch( c )
				{
					case ' ':
					case '\t':
					case '\r':
					case '\n':
						break;
					case '=':
						s->prepared_token.type = lcfg_equals;
						break;
					case '[':
						s->prepared_token.type = lcfg_sbracket_open;
						break;
					case ']':
						s->prepared_token.type = lcfg_sbracket_close;
						break;
					case '{':
						s->prepared_token.type = lcfg_brace_open;
						break;
					case '}':
						s->prepared_token.type = lcfg_brace_close;
						break;
					case ',':
						s->prepared_token.type = lcfg_comma;
						break;
					case '/':
						state = comm_start;
						break;
					case '"':
						state = in_str;
						lcfg_string_trunc(s->prepared_token.string, 0);
						break;
					default:
						if( isalpha(c) )
						{
							lcfg_string_trunc(s->prepared_token.string, 0);
							lcfg_string_cat_char(s->prepared_token.string, c);
							state = in_identifier;
						}
						else
						{
							lcfg_error_set(s->lcfg, "parse error: invalid input character `%c' (0x%02x) near line %d, col %d", isprint(c) ? c : '.', c, s->line, s->col);
							state = invalid;
						}
				}
				break;
			case comm_start:
				if( c == '/' )
				{
					state = in_oneline;
				}
				else if( c == '*' )
				{
					state = in_multiline;
				}
				else
				{
					lcfg_error_set(s->lcfg, "parse error: invalid input character `%c' (0x%02x) near line %d, col %d", isprint(c) ? c : '.', c, s->line, s->col);
					state = invalid;
				}
				break;
			case in_oneline:
				if( c == '\n' )
				{
					state = start;
				}
				break;
			case in_multiline:
				if( c == '*' )
				{
					state = multiline_end;
				}
				break;
			case multiline_end:
				if( c == '/' )
				{
					state = start;
				}
				else if( c != '*' )
				{
					state = in_multiline;
				}
				break;
			case in_identifier:
				if( isalnum(c) || c == '-' || c == '_' )
				{
					lcfg_string_cat_char(s->prepared_token.string, c);
				}
				else
				{
					s->prepared_token.type = lcfg_identifier;
					consume = 0;
					state = start;
				}
				break;
			case in_str:
				if( c == '"' )
				{
					s->prepared_token.type = lcfg_string;
					state = start;
				}
				else if( c == '\\' )
				{
					state = in_esc;
				}
				else
				{
					lcfg_string_cat_char(s->prepared_token.string, c);
				}
				break;
			case in_esc:
				state = in_str;
				switch( c )
				{
					case '"':
						lcfg_string_cat_char(s->prepared_token.string, '"');
						break;
					case 'n':
						lcfg_string_cat_char(s->prepared_token.string, '\n');
						break;
					case 't':
						lcfg_string_cat_char(s->prepared_token.string, '\t');
						break;
					case 'r':
						lcfg_string_cat_char(s->prepared_token.string, '\r');
						break;
					case '0':
						lcfg_string_cat_char(s->prepared_token.string, '\0');
						break;
					case '\\':
						lcfg_string_cat_char(s->prepared_token.string, '\\');
						break;
					case 'x':
						state = esc_hex_exp_first;
						break;
					default:
						lcfg_error_set(s->lcfg, "invalid string escape sequence `%c' near line %d, col %d", c, s->line, s->col);
						state = invalid;
				}
				break;
			case esc_hex_exp_first:
				if( !isxdigit(c) )
				{
					lcfg_error_set(s->lcfg, "invalid hex escape sequence `%c' on line %d column %d", c, s->line, s->col);
					state = invalid;
				}
				hex[0] = c;
				state = esc_hex_exp_second;
				break;
			case esc_hex_exp_second:
				if( !isxdigit(c) )
				{
					lcfg_error_set(s->lcfg, "invalid hex escape sequence `%c' on line %d column %d", c, s->line, s->col);
					state = invalid;
				}
				hex[1] = c;
				hex[2] = '\0';
				lcfg_string_cat_char(s->prepared_token.string, strtoul(hex, NULL, 16));
				state = in_str;
				break;
			case invalid:
				break;
		}
		/*#include <stdio.h>
		printf("read %c at line %d column %d, new state is %d\n", isprint(c) ? c : '.', s->line, s->col, state);*/

		/* this is technically not optimal (token position identified by last char), but it will suffice for now */
		s->prepared_token.line = s->line;
		s->prepared_token.col = s->col;

		if( consume )
		{
			lcfg_scanner_char_read(s, &c);
			if( c == '\n' )
			{
				s->line++;
				s->col = 1;
			}
			else
			{
				s->col++;
			}
		}

		if( s->prepared_token.type != lcfg_null_token || state == invalid )
		{
			break;
		}
	}

	if( state != start )
	{
		if( state != invalid )
		{
			lcfg_error_set(s->lcfg, "parse error: premature end of file near line %d, col %d", s->line, s->col);
		}

		return lcfg_status_error;
	}

	return lcfg_status_ok;
}

enum lcfg_status lcfg_scanner_init(struct lcfg_scanner *s)
{
	/* prepare the first token */
	return lcfg_scanner_token_read(s);
}

int lcfg_scanner_has_next(struct lcfg_scanner *s)
{
	return s->prepared_token.type != lcfg_null_token;
}

enum lcfg_status lcfg_scanner_next_token(struct lcfg_scanner *s, struct lcfg_token *t)
{
	if( !lcfg_scanner_has_next(s) )
	{
		lcfg_error_set(s->lcfg, "%s", "cannot access tokenstream beyond eof");
		return lcfg_status_error;
	}

	memcpy(t, &s->prepared_token, sizeof(struct lcfg_token));
	t->string = lcfg_string_new_copy(s->prepared_token.string);

	/* prepare the next token */
	return lcfg_scanner_token_read(s);
}

struct lcfg_scanner *lcfg_scanner_new(struct lcfg *c, int fd)
{
	struct lcfg_scanner *s = malloc(sizeof(struct lcfg_scanner));
	assert(s);

	memset(s, 0, sizeof(struct lcfg_scanner));

	s->lcfg = c;
	s->fd = fd;

	s->line = s->col = 1;

	s->prepared_token.string = lcfg_string_new();

	return s;
}

void lcfg_scanner_delete(struct lcfg_scanner *s)
{
	lcfg_string_delete(s->prepared_token.string);
	free(s);
}

