AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -I../include -I ../.. -Werror -Wall -g
AM_LDFLAGS = -llcfg -L../src 

bin_PROGRAMS = lcfg

lcfg_LDADD = ../src/liblcfg.la
lcfg_SOURCES = lcfg.c
