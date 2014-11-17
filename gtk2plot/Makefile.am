AM_CFLAGS=$(PLOT_CFLAGS) -g -Wall -Werror
MAINTAINERCLEANFILES=Makefile.in

gtkplot_libincludedir=$(libdir)/gtkplot-2.0/include
nodist_gtkplot_libinclude_HEADERS=../gtkplotconfig.h

lib_LTLIBRARIES=libgtkplot-@PLOT_API_VERSION@.la
libgtkplot_@PLOT_API_VERSION@_la_SOURCES=	\
	gtkplot.c
libgtkplot_@PLOT_API_VERSION@_la_LDFLAGS=	\
	@lt_enable_auto_import@					\
	-no-undefined							\
	-version-info 0:0:0						\
	$(PLOT_LIBS) -lm
gtkplotincludedir=$(includedir)/gtkplot-2.0
gtkplotinclude_HEADERS=						\
	gtkplot.h
if BASE
libgtkplot_@PLOT_API_VERSION@_la_SOURCES+=	\
	gtkplotlinear.c
gtkplotinclude_HEADERS+=					\
	gtkplotlinear.h
endif
if POLARS
libgtkplot_@PLOT_API_VERSION@_la_SOURCES+=	\
	gtkplotpolar.c
gtkplotinclude_HEADERS+=					\
	gtkplotpolar.h
endif
if LOGARS
libgtkplot_@PLOT_API_VERSION@_la_SOURCES+=	\
	gtkplotloglinear.c						\
	gtkplotloglog.c
gtkplotinclude_HEADERS+=					\
	gtkplotloglinear.h						\
	gtkplotloglog.h
endif
if MISCS
libgtkplot_@PLOT_API_VERSION@_la_SOURCES+=	\
	gtkplotdonut.c
gtkplotinclude_HEADERS+=					\
	gtkplotdonut.h
endif

if TEST_BIN
bin_PROGRAMS=
if BASE
bin_PROGRAMS+=testPlotLinear
testPlotLinear_SOURCES=testplotlinear.c
testPlotLinear_LDADD=$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la
endif
if POLARS
bin_PROGRAMS+=testPlotPolar
testPlotPolar_SOURCES=testplotpolar.c
testPlotPolar_LDADD=$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la
endif
if LOGARS
bin_PROGRAMS+=testPlotLogLinear
testPlotLogLinear_SOURCES=testplotloglinear.c
testPlotLogLinear_LDADD=$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la
endif
if MISCS
bin_PROGRAMS+=testPlotDonut
testPlotDonut_SOURCES=testplotdonut.c
testPlotDonut_LDADD=$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la
endif
endif
