# $Id: Makefile.am 1445 2009-01-15 22:43:09Z dp $

AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -pipe -D _GNU_SOURCE -I../include -Werror -Wall -g --std=c99

pkglib_LTLIBRARIES = liblcfg.la

liblcfg_la_SOURCES = lcfg.c
liblcfg_la_SOURCES += lcfg_parser.c
liblcfg_la_SOURCES += lcfg_scanner.c
liblcfg_la_SOURCES += lcfg_string.c
liblcfg_la_SOURCES += lcfg_token.c
liblcfg_la_SOURCES += lcfgx_tree.c

liblcfg_la_LDFLAGS = -no-undefined -version-info @liblcfg_soname@ -export-symbols-regex "^lcfgx?_"
