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
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <string.h>
#include <lcfg/lcfg.h>
#include <lcfgx/lcfgx_tree.h>

const char *help =
                   "Usage: %s [OPTION] CONFIGFILE\n"
                   "Read all or specific key/value-pairs from the lcfg configuration file CONFIGFILE.\n"
                   "The default is to read and print all values found in CONFIGFILE, non-printable\n"
                   "characters are substituted with a dot.\n"
                   "\n"
                   "Mandatory arguments to long options are mandatory for short options too.\n"
                   "  -k, --key=KEY              only read the (possibly binary) value of KEY\n"
                   "                               and print it unfiltered to stdout\n"
                   "  -n, --newline              print a newline character (\\n) after KEY value\n"
                   "\n"
                   "SELINUX options:\n"
                   "\n"
                   "      --help     display this help and exit\n"
                   "      --version  output version information and exit\n"
                   "\n"
                   "Exit status is 0 if OK, 1 if the requested KEY was not found, 2 if some serious\n"
                   "error occured (parse error, file not found, etc.)\n"
                   "\n"
                   "Report bugs to <pb_remove_everything_except_pb@carnivore.it>.\n";

int get_revision()
{
	/*                    0123456*/
	const char *revstr = "$Rev: 1453 $";
	return atoi(revstr + 6);
}
enum lcfg_status print_all_visitor(const char *key, void *data, size_t len, void *user_data)
{
	int i;
	char c;

	printf("%s ", key);
	for( i = 0; i < len; i++ )
	{
		c = *((const char *)(data + i));
		printf("%c", isprint(c) ? c : '.');
	}
	puts("");

	return lcfg_status_ok;
}

enum lcfg_status dump_key_visitor(const char *key, void *data, size_t len, void *user_data)
{
	const char *search_key = user_data;
	int i;

	if( !strcmp(search_key, key) )
	{
		for( i = 0; i < len; i++ )
		{
			fprintf(stdout, "%c", *((char *)(data + i)));
		}

		/* abuse the error handling to indicate that we found the key */
		return lcfg_status_error;
	}

	return lcfg_status_ok;
}

int main(int argc, char **argv)
{
	const char *filename;
	const char *key = NULL;

	enum lcfg_mode
	{
		lcfg_mode_visitor,
		lcfg_mode_tree,
	} mode;

	mode = lcfg_mode_visitor;
	enum lcfgx_type type = lcfgx_string;
	int print_nl = 0;


	int c;

	for( ;; )
	{
		int option_index = 0;
		static struct option long_options[] =
		{
			{ "newline", no_argument, NULL, 'n'},
			{ "key", required_argument, NULL, 'k'},
			{ "help", no_argument, NULL, 'h'},
			{ "version", no_argument, NULL, 'v'}
		};

		c = getopt_long (argc, argv, "k:tT:hvn", long_options, &option_index);
		if( c == -1 )
			break;

		switch( c )
		{
			case 'k':
				key = optarg;
				if( key == '\0' )
					key = NULL;
				break;
			case 'h':
				fprintf(stdout, help, argv[0]);
				return 0;
				break;
			case 'n':
				print_nl = 1;
				break;
			case 'v':
				fprintf(stdout, "%s 10.01.%d (c) 2007--2010 Paul Baecher\n", argv[0], get_revision());
				return 0;
				break;

			case 't':
				mode = lcfg_mode_tree;
				break;

			case 'T':
				if( strcmp("string", optarg) == 0 )
					type = lcfgx_string;
				else
					if( strcmp("map", optarg) == 0 )
						type = lcfgx_map;
					else
						if( strcmp("list", optarg) == 0 )
							type = lcfgx_list;
						else
						{

							fprintf(stdout, help, argv[0]);
							return 0;
						}
				break;

			default:
				return 2;
		}
	}

	if( optind != (argc - 1) )
	{
		fprintf(stderr, help, argv[0]);
		return 2;
	}
	else
	{
		filename = argv[optind];

		struct lcfg *c = lcfg_new(filename);

		if( c == NULL )
		{
			fprintf(stderr, "%s: out of memory\n", argv[0]);
			lcfg_delete(c);
			return 2;
		}

		if( lcfg_parse(c) != lcfg_status_ok )
		{
			fprintf(stderr, "%s: liblcfg error: %s\n", argv[0], lcfg_error_get(c));
			lcfg_delete(c);
			return 2;
		}
		else
		{
			if( mode == lcfg_mode_visitor )
			{
				if( key == NULL )
				{
					lcfg_accept(c, print_all_visitor, 0);
				}
				else
				{
					void *data;
					size_t len;

					if( lcfg_value_get(c, key, &data, &len) != lcfg_status_ok )
					{
						fprintf(stderr, "%s: key %s not found in %s\n", argv[0], key, filename);
						return 1;
					}
					else
					{
						int i;
						for( i = 0; i < len; i++ )
						{
							fprintf(stdout, "%c", *((char *)(data + i)));
						}

						if( print_nl )
						{
							fprintf(stdout, "%c", '\n');
						}
					}
				}
			}
			else
				if( mode == lcfg_mode_tree )
				{
					struct lcfgx_tree_node *root = lcfgx_tree_new(c);
					if( key == NULL )
					{
						lcfgx_tree_dump(root, 0);
					}
					else
					{
						struct lcfgx_tree_node *n;
						enum lcfgx_path_access axs = lcfgx_get(root, &n, key, type);
						printf("PATH %s %s\n", lcfgx_path_access_strings[axs], key);
						if( axs == LCFGX_PATH_FOUND_TYPE_OK )
							lcfgx_tree_dump(n, 0);
					}

					lcfgx_tree_delete(root);
				}

		}

		lcfg_delete(c);
	}

	return 0;
}
