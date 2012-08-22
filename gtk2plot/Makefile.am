AM_CFLAGS=$(PLOT_CFLAGS) -g
MAINTAINERCLEANFILES=Makefile.in

gtkplot_libincludedir=$(libdir)/gtkplot-2.0/include
nodist_gtkplot_libinclude_HEADERS=../gtkplotconfig.h

lib_LTLIBRARIES=libgtkplot-2.0.la
libgtkplot_2.0_la_SOURCES=			\
	gtkplotlinear.c						\
	gtkplotpolar.c
libgtkplot_2.0_la_LDFLAGS=			\
	@lt_enable_auto_import@					\
	-no-undefined						\
	-version-info 0:0:0	\
	$(PLOT_LIBS)
gtkplotincludedir=$(includedir)/gtkplot-2.0
gtkplotinclude_HEADERS=						\
	gtkplotlinear.h						\
	gtkplotpolar.h

bin_PROGRAMS=TestPlotLinear TestPlotPolar
TestPlotLinear_SOURCES=testplotlinear.c
TestPlotLinear_LDADD=$(PLOT_LIBS) libgtkplot-2.0.la
TestPlotPolar_SOURCES=testplotpolar.c
TestPlotPolar_LDADD=$(PLOT_LIBS) libgtkplot-2.0.la
