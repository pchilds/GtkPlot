AM_CFLAGS=$(PLOT_CFLAGS) -g
MAINTAINERCLEANFILES=Makefile.in

gtkplot_libincludedir=$(libdir)/gtkplot-2.0/include
nodist_gtkplot_libinclude_HEADERS=../gtkplotconfig.h

lib_LTLIBRARIES=libgtkplot-@PLOT_API_VERSION@.la
libgtkplot_@PLOT_API_VERSION@_la_SOURCES=	\
	gtkplot.c				\
	gtkplotlinear.c				\
	gtkplotpolar.c
libgtkplot_@PLOT_API_VERSION@_la_LDFLAGS=	\
	@lt_enable_auto_import@			\
	-no-undefined				\
	-version-info 0:0:0			\
	$(PLOT_LIBS) -lm
gtkplotincludedir=$(includedir)/gtkplot-2.0
gtkplotinclude_HEADERS=				\
	gtkplot.h				\
	gtkplotlinear.h				\
	gtkplotpolar.h

bin_PROGRAMS=TestPlotLinear TestPlotPolar
TestPlotLinear_SOURCES=testplotlinear.c
TestPlotLinear_LDADD=$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la
TestPlotPolar_SOURCES=testplotpolar.c
TestPlotPolar_LDADD=$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la
