all: controller


libmixer:
	g++ -c -D__STDC_CONSTANT_MACROS -Isrc/include -Wall -fpic src/layout.cpp
	g++ -c -D__STDC_CONSTANT_MACROS -Isrc/include -Wall -fpic src/stream.cpp

	g++ -shared -o libmixer.so layout.o stream.o -lavcodec -lswscale -lpthread

controller: libmixer
	g++ -D__STDC_CONSTANT_MACROS -c -Isrc/include -Iug-modules/io_mngr -Iug-modules/src -Wall src/mixer.cpp
	g++ -D__STDC_CONSTANT_MACROS -c -Isrc/include -Iug-modules/io_mngr -Iug-modules/src -Wall src/controller.cpp

	g++ mixer.o controller.o -o controller -L. -Lug-modules/lib -lrtp -lvcompress -lvdecompress -liomanager -lmixer -lavformat -lswscale

clean:
	rm -f libmixer.so
	rm -f src/*.o
	rm -f *.o
	rm -f controller
