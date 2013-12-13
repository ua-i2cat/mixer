AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libmixer.la
libmixer_la_SOURCES = stream.cpp layout.cpp crop.cpp
libmixer_la_CPPFLAGS = -I/usr/local/include/opencv -Wall
libmixer_la_LDFLAGS = -shared -fPIC -lopencv_core -lopencv_imgproc -lpthread 