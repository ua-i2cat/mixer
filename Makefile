all: controller


libmixer:
	g++ -c -D__STDC_CONSTANT_MACROS -Isrc/include -Wall -fpic src/layout.cpp
	g++ -c -D__STDC_CONSTANT_MACROS -Isrc/include -Wall -fpic src/stream.cpp

	g++ -shared -o libmixer.so layout.o stream.o -lavcodec -lswscale -lpthread

controller: libmixer
	g++ -D__STDC_CONSTANT_MACROS -c -Isrc/include -Iug-modules/reciever -Iug_modules/src -Wall src/mixer.cpp
	g++ -D__STDC_CONSTANT_MACROS -c -Isrc/include -Iug-modules/reciever -Iug_modules/src -Wall src/controller.cpp

	g++ -Iug-modules/src -Isrc/include -Iug-modules/reciever ug-modules/reciever/transmitter.o ug-modules/reciever/reciever.o ug-modules/reciever/participants.o ug-modules/src/debug.o ug-modules/src/ntp.o ug-modules/src/pdb.o ug-modules/src/perf.o ug-modules/src/tfrc.o ug-modules/src/tile.o ug-modules/src/tv.o ug-modules/src/video.o ug-modules/src/video_codec.o ug-modules/src/video_compress.o ug-modules/src/video_decompress.o ug-modules/src/compat/drand48.o ug-modules/src/compat/gettimeofday.o ug-modules/src/compat/inet_ntop.o ug-modules/src/compat/inet_pton.o ug-modules/src/compat/platform_semaphore.o ug-modules/src/compat/vsnprintf.o ug-modules/src/crypto/crypt_aes.o ug-modules/src/crypto/crypt_aes_impl.o ug-modules/src/crypto/crypt_des.o ug-modules/src/crypto/md5.o ug-modules/src/crypto/random.o ug-modules/src/rtp/net_udp.o ug-modules/src/rtp/pbuf.o ug-modules/src/rtp/ptime.o ug-modules/src/rtp/rtp.o ug-modules/src/rtp/rtp_callback.o ug-modules/src/rtp/rtpdec.o ug-modules/src/rtp/rtpdec_h264.o ug-modules/src/rtp/rtpenc.o ug-modules/src/rtp/rtpenc_h264.o ug-modules/src/video_compress/libavcodec.o ug-modules/src/video_compress/none.o ug-modules/src/video_decompress/libavcodec.o ug-modules/src/video_decompress/null.o ug-modules/src/utils/resource_manager.o ug-modules/src/utils/worker.o layout.o stream.o mixer.o controller.o -o controller -lrt -lpthread -ldl -lavcodec -lavutil -lieee -lm -Lug-modules/lib -lrtp -lvcompress -lvdecompress -lavformat -lswscale

clean:
	rm -f libmixer.so
	rm -f src/*.o
	rm -f *.o
	rm -f controller
