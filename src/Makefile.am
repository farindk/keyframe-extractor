
bin_PROGRAMS = extractor

AM_CPPFLAGS = # -I../libde265

extractor_DEPENDENCIES = # ../libde265/libde265.la
extractor_CXXFLAGS = -std=c++0x -fPIC
extractor_LDFLAGS =
extractor_LDADD = -lstdc++ -lpthread #../libde265/libde265.la
extractor_SOURCES = \
  extractor.cc \
  cmdline.c \
  decoder.cc decoder.hh \
  libcvalgo/histogram.hh libcvalgo/histogram.cc \
  libcvalgo/histogram_diff.hh libcvalgo/histogram_diff.cc

extractor_CXXFLAGS += $(VIDEOGFX_CFLAGS)
extractor_LDFLAGS += $(VIDEOGFX_LIBS)
extractor_LDFLAGS += -ljpeg

extractor_CXXFLAGS += $(SWSCALE_CFLAGS) $(AVUTIL_CFLAGS) $(AVFORMAT_CFLAGS) $(AVCODEC_CFLAGS) $(X264_CFLAGS)
extractor_LDFLAGS += $(SWSCALE_LIBS) $(AVUTIL_LIBS) $(AVFORMAT_LIBS) $(AVCODEC_LIBS) $(X264_LIBS)

EXTRA_DIST = \
  README


extractor.cc: cmdline.c

cmdline.c: cmdline.ggo
	gengetopt -u -i cmdline.ggo
