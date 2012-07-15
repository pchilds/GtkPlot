## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS=subdir-objects
AM_CFLAGS=$(PLOT_CFLAGS)
ACLOCAL_AMFLAGS=${ACLOCAL_FLAGS}
lib_LTLIBRARIES=libgtk2plot-@PLOT_API_VERSION@.la
libgtk2plot_@PLOT_API_VERSION@_la_SOURCES=	\
	gtk2plot/gtkplotlinear.c			\
	gtk2plot/gtkplotpolar.c
libgtk2plot_@PLOT_API_VERSION@_la_LDFLAGS=	\
	-version-info $(PLOT_SO_VERSION)	\
	$(PLOT_LIBS)
gtk2plot_includedir=$(includedir)/gtk2plot-$(PLOT_API_VERSION)
gtk2plot_include_HEADERS=	\
	gtk2plot/gtkplotlinear.h	\
	gtk2plot/gtkplotpolar.h
gtk2plot_libincludedir=$(libdir)/gtk2plot-$(PLOT_API_VERSION)/include
nodist_gtk2plot_libinclude_HEADERS=gtk2plotconfig.h
##notrans_dist_man3_MANS=plot:linear.3 plot:polar.3

bin_PROGRAMS=TestPlotLinear TestPlotPolar
TestPlotLinear_SOURCES=gtk2plot/testplotlinear.c
TestPlotLinear_LDADD=$(PLOT_LIBS) libgtk2plot-@PLOT_API_VERSION@.la
TestPlotPolar_SOURCES=gtk2plot/testplotpolar.c
TestPlotPolar_LDADD=$(PLOT_LIBS) libgtk2plot-@PLOT_API_VERSION@.la

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=gtk2plot-${PLOT_API_VERSION}.pc
