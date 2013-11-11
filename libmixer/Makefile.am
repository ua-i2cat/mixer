AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libmixer.la
libmixer_la_SOURCES = stream.cpp layout.cpp crop.cpp
libmixer_la_CPPFLAGS = -I/usr/local/include/opencv -Wall
libmixer_la_LDFLAGS = -shared -fPIC -lopencv_core -lopencv_imgproc -lpthread 

bin_PROGRAMS = test
test_SOURCES = testlibmixer.cpp
test_CPPFLAGS = -D__STDC_CONSTANT_MACROS -Ilibmixer -Wall
test_LDFLAGS = -lavformat -lavcodec -lswscale -lmixer 
test_DEPENDENCIES= libmixer.la