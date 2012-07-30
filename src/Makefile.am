AM_CFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/config

# headers to be installed in /include subdirectory
#include_HEADERS =

# headers that should not be installed into /include
noinst_HEADERS = \
	mpileaks.h \
	callpath2count.h

lib_LTLIBRARIES = \
	libmpileaks.la

INCLUDES = \
	$(DEFS) \
	$(DEFAULT_INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) \
	$(AM_CFLAGS) $(CFLAGS) \
	$(MPI_CFLAGS) $(CALLPATH_CFLAGS)

# source files that are used by scr commands and runtime library
libmpileaks_la_SOURCES = \
  mpileaks.cpp \
  comm.cpp \
  datatype.cpp \
  errhandler.cpp \
  fileio.cpp \
  group.cpp \
  info.cpp \
  keyval.cpp \
  mem.cpp \
  op.cpp \
  request.cpp \
  win.cpp
libmpileaks_la_CFLAGS = $(INCLUDES)
libmpileaks_la_LIBADD = $(CALLPATH_LDFLAGS) $(CALLPATH_LIBS)
libmpileaks_la_LDFLAGS = -avoid-version
