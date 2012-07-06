## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS=subdir-objects
AM_CFLAGS=$(PLOT_CFLAGS)
ACLOCAL_AMFLAGS=${ACLOCAL_FLAGS}
lib_LTLIBRARIES=libgtkplot-@PLOT_API_VERSION@.la
libgtkplot_@PLOT_API_VERSION@_la_SOURCES=	\
	gtkplot/plotlinear.c
libgtkplot_@PLOT_API_VERSION@_la_LDFLAGS=	\
	-version-info $(PLOT_SO_VERSION)	\
	$(PLOT_LIBS)
gtkplot_includedir=$(includedir)/gtkplot-$(PLOT_API_VERSION)
gtkplot_include_HEADERS=	\
	gtkplot/plotlinear.h
gtkplot_libincludedir=$(libdir)/gtkplot-$(PLOT_API_VERSION)/include
nodist_gtkplot_libinclude_HEADERS=gtkplotconfig.h

bin_PROGRAMS=TestPlotLinear
TestPlotLinear_SOURCES=			\
	gtkplot/testplotlinear.c
TestPlotLinear_LDADD=$(PLOT_LIBS) libgtkplot-@PLOT_API_VERSION@.la

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=gtkplot-${PLOT_API_VERSION}.pc
dist_noinst_SCRIPTS=autogen.sh
