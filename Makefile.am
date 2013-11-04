
EXTRA_DIST=bootstrap.sh

AM_LDFLAGS=-avoid-version -module -export-dynamic

lib_LTLIBRARIES = libflashsupport.la

libflashsupport_la_SOURCES = flashsupport.c
libflashsupport_la_LIBADD = $(AM_LIBADD) $(JACK_LIBS) $(SRC_LIBS) $(LIBSSL_LIBS)
libflashsupport_la_CFLAGS = $(AM_CFLAGS) $(JACK_CFLAGS) $(SRC_LIBS) $(LIBSSL_CFLAGS)


