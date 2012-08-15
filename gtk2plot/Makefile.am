AM_CFLAGS=$(PLOT_CFLAGS)
MAINTAINERCLEANFILES=Makefile.in

gtkplot_libincludedir=$(libdir)/gtkplot-$(PLOT_API_VERSION)/include
nodist_gtkplot_libinclude_HEADERS=../gtkplotconfig.h

lib_LTLIBRARIES=libgtkplot-@PLOT_API_VERSION@.la
libgtkplot_@PLOT_API_VERSION@_la_SOURCES=			\
	gtkplotlinear.c						\
	gtkplotpolar.c
libgtkplot_@PLOT_API_VERSION@_la_LDFLAGS=			\
	@lt_enable_auto_import@					\
	-no-undefined						\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	$(PLOT_LIBS)
gtkplotincludedir=$(includedir)/gtkplot-$(PLOT_API_VERSION)
gtkplotinclude_HEADERS=						\
	gtkplotlinear.h						\
	gtkplotpolar.h

bin_PROGRAMS=TestPlotLinear TestPlotPolar
TestPlotLinear_SOURCES=testplotlinear.c
TestPlotLinear_LDADD=$(PLOT_LIBS) libgtkplot-@PLOT_API_VERSION@.la
TestPlotPolar_SOURCES=testplotpolar.c
TestPlotPolar_LDADD=$(PLOT_LIBS) libgtkplot-@PLOT_API_VERSION@.la
