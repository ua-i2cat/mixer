SUBDIRS = libmixer

#include libmixer/Makefile.am

bin_PROGRAMS = controller
controller_SOURCES = src/controller.cpp src/mixer.cpp
controller_CPPFLAGS = -D__STDC_CONSTANT_MACROS -Ilibmixer -Iug-modules/io_mngr -Iug-modules/src -Wall
controller_LDFLAGS= -Lug-modules/lib -lrtp -lvcompress -lvdecompress -liomanager
controller_LDADD= libmixer/libmixer.la
