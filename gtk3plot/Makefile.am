AM_CFLAGS=$(PLOT_CFLAGS) -g
MAINTAINERCLEANFILES=Makefile.in

gtkplot_libincludedir=$(libdir)/gtkplot-3.0/include
nodist_gtkplot_libinclude_HEADERS=../gtkplotconfig.h

lib_LTLIBRARIES=libgtkplot-@PLOT_API_VERSION@.la
libgtkplot_@PLOT_API_VERSION@_la_SOURCES=	\
	gtkplot.c				\
	gtkplotdonut.c				\
	gtkplotlinear.c				\
	gtkplotpolar.c				\
	a11y/gtkplotaccessible.c		\
	a11y/gtkplotdonutaccessible.c		\
	a11y/gtkplotlinearaccessible.c		\
	a11y/gtkplotpolaraccessible.c
libgtkplot_@PLOT_API_VERSION@_la_LDFLAGS=	\
	@lt_enable_auto_import@			\
	-no-undefined				\
	-version-info 0:0:0			\
	$(PLOT_LIBS) -lm
gtkplotincludedir=$(includedir)/gtkplot-3.0
gtkplotinclude_HEADERS=				\
	gtkplot.h				\
	gtkplotdonut.h				\
	gtkplotlinear.h				\
	gtkplotpolar.h				\
	a11y/gtkplotaccessible.h		\
	a11y/gtkplotdonutaccessible.h		\
	a11y/gtkplotlinearaccessible.h		\
	a11y/gtkplotpolaraccessible.h

if TEST_BIN
bin_PROGRAMS=testPlotLinear testPlotPolar
testPlotLinear_SOURCES=testplotlinear.c
testPlotLinear_LDADD=$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la
testPlotPolar_SOURCES=testplotpolar.c
testPlotPolar_LDADD=$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la
endif
