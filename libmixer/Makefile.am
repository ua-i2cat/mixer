AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libmixer.la
libmixer_la_SOURCES = libmixer/stream.cpp libmixer/layout.cpp
libmixer_la_CPPFLAGS = -D__STDC_CONSTANT_MACROS -Isrc/include -Wall
libmixer_la_LDFLAGS = -shared -fPIC -lavcodec -lswscale -lpthread 
