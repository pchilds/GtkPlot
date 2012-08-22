#!/bin/bash
if [ $# -ne 5 ]
then
	echo "usage - $0 v w x y z"
	echo "        Where v.w is the GTK API version (e.g. 2.0),"
	echo "        and x,y,z is the library current, age, and revision versions"
	exit 1
fi
if [ $1 -eq 3 ]
then
	gmx=3
	gmn=0
	gdp="-3"
else
	gmx=2
	gmn=14
	gdp="2.$2"
fi
gar=$2
lcr=$3
lag=$4
lrv=$5
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
echo "AC_SUBST([PLOT_GTK_VERSION], [gtk+-${gmx}.${gar} >= ${gmx}.${gmn}])"
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
echo "AC_CONFIG_FILES([debian/Makefile])"
echo "AC_OUTPUT()"
} >configure.ac
{
echo "## Process this file with automake to produce Makefile.in"
echo "SUBDIRS=gtk${gmx}plot debian"
echo "ACLOCAL_AMFLAGS=-I m4 \${ACLOCAL_FLAGS}"
echo "MAINTAINERCLEANFILES=Makefile.in configure"
echo "DISTCLEANFILES= ="
echo ""
echo "pkgconfigdir=\$(libdir)/pkgconfig"
echo "pkgconfig_DATA=gtkplot-${gmx}.${gar}.pc"
} >Makefile.am
{
echo "AM_CFLAGS=\$(PLOT_CFLAGS) -g"
echo "MAINTAINERCLEANFILES=Makefile.in"
echo ""
echo "gtkplot_libincludedir=\$(libdir)/gtkplot-${gmx}.${gar}/include"
echo "nodist_gtkplot_libinclude_HEADERS=../gtkplotconfig.h"
echo ""
echo "lib_LTLIBRARIES=libgtkplot-@PLOT_API_VERSION@.la"
echo "libgtkplot_@PLOT_API_VERSION@_la_SOURCES=			\\"
echo "	gtkplotlinear.c						\\"
echo "	gtkplotpolar.c"
echo "libgtkplot_@PLOT_API_VERSION@_la_LDFLAGS=			\\"
echo "	@lt_enable_auto_import@					\\"
echo "	-no-undefined						\\"
echo "	-version-info ${lcr}:${lrv}:${lag}	\\"
echo "	\$(PLOT_LIBS)"
echo "gtkplotincludedir=\$(includedir)/gtkplot-${gmx}.${gar}"
echo "gtkplotinclude_HEADERS=						\\"
echo "	gtkplotlinear.h						\\"
echo "	gtkplotpolar.h"
echo ""
echo "bin_PROGRAMS=TestPlotLinear TestPlotPolar"
echo "TestPlotLinear_SOURCES=testplotlinear.c"
echo "TestPlotLinear_LDADD=\$(PLOT_LIBS) libgtkplot-@PLOT_API_VERSION@.la"
echo "TestPlotPolar_SOURCES=testplotpolar.c"
echo "TestPlotPolar_LDADD=\$(PLOT_LIBS) libgtkplot-@PLOT_API_VERSION@.la"
} >gtk${gmx}plot/Makefile.am
{
echo "Source: libgtkplot"
echo "Priority: optional"
echo "Maintainer: Paul Childs <pchilds@physics.org>"
echo "Build-Depends: debhelper (>= 7.0.50), autotools-dev, dh-autoreconf, pkg-config, libgtk${gdp}-dev"
echo "Standards-Version: 3.8.3"
echo "Section: libs"
echo "Homepage: <https://github.com/pchilds/GtkPlot>"
echo ""
echo "Package: libgtkplot-${gmx}-dev"
echo "Section: libdevel"
echo "Architecture: any"
echo "Depends: libgtkplot-${gmx} (= \${binary:Version})"
echo "Description: Provides headers for the libgtkplot-${gmx} package"
echo " libgtkplot-${gmx} makes available to gtk+${gmx} programs, a linear and polar"
echo " plotting widget. The widget features multiplot capability, zoom"
echo " control, smart range setting and mouse location tracking."
echo " Processing is optimised towards speed for large data sets as"
echo " commonly occurs in scientific applications and outputs to a neat"
echo " publication ready format."
echo ""
echo "Package: libgtkplot-${gmx}-doc"
echo "Section: doc"
echo "Architecture: any"
echo "Depends: libgtkplot-${gmx} (= \${binary:Version})"
echo "Description: Provides examples for the libgtkplot-${gmx} package"
echo " libgtkplot-${gmx} makes available to gtk+${gmx} programs, a linear and polar"
echo " plotting widget. The widget features multiplot capability, zoom"
echo " control, smart range setting and mouse location tracking."
echo " Processing is optimised towards speed for large data sets as"
echo " commonly occurs in scientific applications and outputs to a neat"
echo " publication ready format."
echo ""
echo "Package: libgtkplot-${gmx}-dbg"
echo "Section: debug"
echo "Architecture: any"
echo "Depends: libgtkplot-${gmx} (= \${binary:Version})"
echo "Description: Provides debug symbols for the libgtkplot-${gmx} package"
echo " libgtkplot-${gmx} makes available to gtk+${gmx} programs, a linear and polar"
echo " plotting widget. The widget features multiplot capability, zoom"
echo " control, smart range setting and mouse location tracking."
echo " Processing is optimised towards speed for large data sets as"
echo " commonly occurs in scientific applications and outputs to a neat"
echo " publication ready format."
echo ""
echo "Package: libgtkplot-${gmx}"
echo "Section: libs"
echo "Architecture: any"
echo "Depends: \${shlibs:Depends}, \${misc:Depends}"
echo "Description: Provides shared libraries for plotting with gtk+${gmx}"
echo " libgtkplot-${gmx} makes available to gtk+${gmx} programs, a linear and polar"
echo " plotting widget. The widget features multiplot capability, zoom"
echo " control, smart range setting and mouse location tracking."
echo " Processing is optimised towards speed for large data sets as"
echo " commonly occurs in scientific applications and outputs to a neat"
echo " publication ready format."
} >debian/control
{
echo "This work was packaged for Debian by:"
echo "    Paul Childs <pchilds@physics.org> on Sat, 21 Jul 2012 13:34:41 +1000"
echo ""
echo "It was downloaded from <https://github.com/pchilds/GtkPlot>"
echo ""
echo "Upstream Author(s):"
echo "    Paul Childs <pchilds@physics.org>"
echo ""
echo "Copyright:"
echo "    Copyright (C) 2010 Paul Childs <pchilds@physics.org>"
echo ""
echo "License:"
echo "    This package is free software; you can redistribute it and/or"
echo "    modify it under the terms of the GNU Lesser General Public"
echo "    License as published by the Free Software Foundation; either"
echo "    version ${gmx} of the License, or (at your option) any later version."
echo ""
echo "    This package is distributed in the hope that it will be useful,"
echo "    but WITHOUT ANY WARRANTY; without even the implied warranty of"
echo "    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU"
echo "    Lesser General Public License for more details."
echo ""
echo "    You should have received a copy of the GNU Lesser General Public"
echo "    License along with this package; if not, write to the Free Software"
echo "    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA"
echo ""
echo "On Debian systems, the complete text of the GNU Lesser General"
echo "Public License can be found in \`/usr/share/common-licenses/LGPL-${gmx}'."
echo ""
echo "The Debian packaging is:"
echo "    Copyright (C) 2012 Paul Childs <pchilds@physics.org>"
echo "and is licensed under the LGPL version ${gmx}," 
echo "see \`/usr/share/common-licenses/LGPL-${gmx}'."
} >debian/copyright
{
echo "#!/usr/bin/make -f"
echo "export DH_VERBOSE=1"
echo ""
echo "%:"
echo "	dh  \$@ --with autoreconf"
echo ".PHONY: override_dh_strip"
echo "override_dh_strip:"
echo "	dh_strip --dbg-package=libgtkplot-${gmx}-dbg"
} >debian/rules
{
echo "libgtkplot ${gmx}.${gar} libgtkplot-${gmx}"
} >debian/shlibs.local
