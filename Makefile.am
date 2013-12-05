ACLOCAL_AMFLAGS = -I m4
SUBDIRS = libmixer

#include libmixer/Makefile.am

bin_PROGRAMS = controller
controller_SOURCES = src/controller.cpp src/mixer.cpp src/Jzon.cpp src/stat_manager.cpp
controller_CPPFLAGS = -D__STDC_CONSTANT_MACROS -Ilibmixer -I. -I../src -I../src/rtp -I../src/utils -Wall
controller_LDFLAGS=  -Llibmixer -lmixer -lrtp -lvcompress -lvdecompress -liomanager
controller_DEPENDENCIES= libmixer/libmixer.la
