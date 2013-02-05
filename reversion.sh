#!/bin/bash
if [ $# -ne 5 ]
then
	echo "usage - $0 v w x y z"
	echo "        Where v.w is the GTK API version (e.g. 2.0),"
	echo "        and x,y,z is the library current, age, and revision versions"
	exit 1
fi
gar=$2
lcr=$3
lag=$4
lrv=$5
if [ $1 -eq 3 ]
then
	gmx=3
	gmn=8
	gdp="-3"
	{
	echo "AM_CFLAGS=\$(PLOT_CFLAGS) -g"
	echo "MAINTAINERCLEANFILES=Makefile.in"
	echo ""
	echo "gtkplot_libincludedir=\$(libdir)/gtkplot-3.${gar}/include"
	echo "nodist_gtkplot_libinclude_HEADERS=../gtkplotconfig.h"
	echo ""
	echo "lib_LTLIBRARIES=libgtkplot-@PLOT_API_VERSION@.la"
	echo "libgtkplot_@PLOT_API_VERSION@_la_SOURCES=	\\"
	echo "	gtkplot.c				\\"
	echo "	gtkplotlinear.c				\\"
	echo "	gtkplotpolar.c				\\"
	echo "	a11y/gtkplotaccessible.c		\\"
	echo "	a11y/gtkplotlinearaccessible.c		\\"
	echo "	a11y/gtkplotpolaraccessible.c"
	echo "libgtkplot_@PLOT_API_VERSION@_la_LDFLAGS=	\\"
	echo "	@lt_enable_auto_import@			\\"
	echo "	-no-undefined				\\"
	echo "	-version-info ${lcr}:${lrv}:${lag}			\\"
	echo "	\$(PLOT_LIBS) -lm"
	echo "gtkplotincludedir=\$(includedir)/gtkplot-3.${gar}"
	echo "gtkplotinclude_HEADERS=				\\"
	echo "	gtkplot.h				\\"
	echo "	gtkplotlinear.h				\\"
	echo "	gtkplotpolar.h				\\"
	echo "	a11y/gtkplotaccessible.h		\\"
	echo "	a11y/gtkplotlinearaccessible.h		\\"
	echo "	a11y/gtkplotpolaraccessible.h"
	echo ""
	echo "bin_PROGRAMS=TestPlotLinear TestPlotPolar"
	echo "TestPlotLinear_SOURCES=testplotlinear.c"
	echo "TestPlotLinear_LDADD=\$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la"
	echo "TestPlotPolar_SOURCES=testplotpolar.c"
	echo "TestPlotPolar_LDADD=\$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la"
	} >gtk3plot/Makefile.am
else
	gmx=2
	gmn=18
	gdp="2.$2"
	{
	echo "AM_CFLAGS=\$(PLOT_CFLAGS) -g"
	echo "MAINTAINERCLEANFILES=Makefile.in"
	echo ""
	echo "gtkplot_libincludedir=\$(libdir)/gtkplot-2.${gar}/include"
	echo "nodist_gtkplot_libinclude_HEADERS=../gtkplotconfig.h"
	echo ""
	echo "lib_LTLIBRARIES=libgtkplot-@PLOT_API_VERSION@.la"
	echo "libgtkplot_@PLOT_API_VERSION@_la_SOURCES=	\\"
	echo "	gtkplot.c				\\"
	echo "	gtkplotlinear.c				\\"
	echo "	gtkplotpolar.c"
	echo "libgtkplot_@PLOT_API_VERSION@_la_LDFLAGS=	\\"
	echo "	@lt_enable_auto_import@			\\"
	echo "	-no-undefined				\\"
	echo "	-version-info ${lcr}:${lrv}:${lag}			\\"
	echo "	\$(PLOT_LIBS) -lm"
	echo "gtkplotincludedir=\$(includedir)/gtkplot-2.${gar}"
	echo "gtkplotinclude_HEADERS=				\\"
	echo "	gtkplot.h				\\"
	echo "	gtkplotlinear.h				\\"
	echo "	gtkplotpolar.h"
	echo ""
	echo "bin_PROGRAMS=TestPlotLinear TestPlotPolar"
	echo "TestPlotLinear_SOURCES=testplotlinear.c"
	echo "TestPlotLinear_LDADD=\$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la"
	echo "TestPlotPolar_SOURCES=testplotpolar.c"
	echo "TestPlotPolar_LDADD=\$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la"
	} >gtk2plot/Makefile.am
fi
{
echo "AC_PREREQ([2.65])"
echo "AC_INIT([Gtk${gmx}Plot],[${gmx}.${lcr}.${lrv}],[pchilds@physics.org],[gtk${gmx}plot],[https://github.com/pchilds/GtkPlot])"
echo "AC_CONFIG_HEADERS([config.h gtkplotconfig.h])"
echo "AC_CONFIG_SRCDIR([gtk${gmx}plot/gtkplotlinear.h])"
echo "AC_CONFIG_AUX_DIR([confsupp])"
echo "AC_CONFIG_MACRO_DIR([m4])"
echo "AM_INIT_AUTOMAKE([-Wall -Werror foreign])"
echo "AC_PROG_CC([gcc cc])"
echo "LT_PREREQ([2.2])"
echo "LT_INIT([win32-dll])"
echo "AC_SUBST([PLOT_API_VERSION], [${gmx}.${gar}])"
echo "AC_SUBST([PLOT_GTK_MAJOR], [${gmx}])"
echo "AC_SUBST([PLOT_GTK_MINOR], [${gmn}])"
echo "AC_SUBST([PLOT_SDIR], [gtk${gmx}plot])"
echo "lt_enable_auto_import="""
echo "case \"\$host_os\" in"
echo "   mingw*)"
echo "      lt_enable_auto_import=\"-Wl,--enable-auto-import\""
echo "esac"
echo "AC_SUBST([lt_enable_auto_import])"
echo "PKG_CHECK_MODULES([PLOT], [gtk+-${gmx}.${gar} >= ${gmx}.${gmn}])"
echo "AC_CONFIG_FILES([Makefile gtk${gmx}plot/Makefile	\\"
echo "	gtkplot-${gmx}.${gar}.pc:gtkplot.pc.in		\\"
echo "	COPYING:COPYING-${gmx}.${gar}.in])"
echo "AC_OUTPUT()"
} >configure.ac
{
echo "## Process this file with automake to produce Makefile.in"
echo "SUBDIRS=gtk${gmx}plot"
echo "ACLOCAL_AMFLAGS=-I m4 \${ACLOCAL_FLAGS}"
echo "MAINTAINERCLEANFILES=Makefile.in configure"
echo "DISTCLEANFILES= ="
echo ""
echo "pkgconfigdir=\$(libdir)/pkgconfig"
echo "pkgconfig_DATA=gtkplot-${gmx}.${gar}.pc"
} >Makefile.am
