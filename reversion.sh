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
	echo "	gtkplotdonut.c				\\"
	echo "	gtkplotlinear.c				\\"
	echo "	gtkplotpolar.c				\\"
	echo "	a11y/gtkplotaccessible.c		\\"
	echo "	a11y/gtkplotdonutaccessible.c		\\"
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
	echo "	gtkplotdonut.h				\\"
	echo "	gtkplotlinear.h				\\"
	echo "	gtkplotpolar.h				\\"
	echo "	a11y/gtkplotaccessible.h		\\"
	echo "	a11y/gtkplotdonutaccessible.h		\\"
	echo "	a11y/gtkplotlinearaccessible.h		\\"
	echo "	a11y/gtkplotpolaraccessible.h"
	echo ""
	echo "if TEST_BIN"
	echo "bin_PROGRAMS=testPlotLinear testPlotPolar"
	echo "testPlotLinear_SOURCES=testplotlinear.c"
	echo "testPlotLinear_LDADD=\$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la"
	echo "testPlotPolar_SOURCES=testplotpolar.c"
	echo "testPlotPolar_LDADD=\$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la"
	echo "endif"
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
	echo "	gtkplotdonut.c				\\"
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
	echo "	gtkplotdonut.h				\\"
	echo "	gtkplotlinear.h				\\"
	echo "	gtkplotpolar.h"
	echo ""
	echo "if TEST_BIN"
	echo "bin_PROGRAMS=testPlotDonut testPlotLinear testPlotPolar"
	echo "testPlotDonut_SOURCES=testplotdonut.c"
	echo "testPlotDonut_LDADD=\$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la"
	echo "testPlotLinear_SOURCES=testplotlinear.c"
	echo "testPlotLinear_LDADD=\$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la"
	echo "testPlotPolar_SOURCES=testplotpolar.c"
	echo "testPlotPolar_LDADD=\$(PLOT_LIBS) -lm libgtkplot-@PLOT_API_VERSION@.la"
	echo "endif"
	} >gtk2plot/Makefile.am
fi
{
echo "AC_PREREQ([2.63])"
echo "AC_INIT([Gtk${gmx}Plot],[${gmx}.${lcr}.${lrv}],[pchilds@physics.org],[gtk${gmx}plot],[https://github.com/pchilds/GtkPlot])"
echo "AC_CONFIG_HEADERS([config.h gtkplotconfig.h])"
echo "AC_CONFIG_SRCDIR([gtk${gmx}plot/gtkplot.h])"
echo "AC_CONFIG_AUX_DIR([confsupp])"
echo "AC_CONFIG_MACRO_DIR([m4])"
echo "AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])"
echo "m4_ifdef([AM_PROG_AR], [AM_PROG_AR])"
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
echo "AC_ARG_ENABLE([testbinaries],AS_HELP_STRING([--disable-testbinaries],[Disable building test examples -incompatible with some packaging  @<:@yes/no@:>@]),,[enable_testbinaries=yes])"
echo "AM_CONDITIONAL([TEST_BIN], [test \"\$enable_testbinaries\" = \"yes\"])"
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
echo "libgtkplot ($gmx.$gar) unstable; urgency=low"
echo ""
echo "  * Initial Release."
echo ""
echo " -- Paul Childs <pchilds@physics.org>  Sat, 21 Jul 2012 13:34:41 +1000"
} >debian/changelog
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
echo " libgtkplot-${gmx} makes available to gtk+${gmx} programs, a pi/donut,"
echo " linear and polar plotting widgets. Features include: multiplot"
echo " capability, zoom control, smart range setting and mouse location"
echo " tracking. Processing is optimised towards speed for large data sets as"
echo " commonly occurs in scientific applications and outputs to a neat"
echo " publication ready format."
echo ""
echo "Package: libgtkplot-${gmx}-doc"
echo "Section: doc"
echo "Architecture: any"
echo "Depends: libgtkplot-${gmx} (= \${binary:Version})"
echo "Description: Provides examples for the libgtkplot-${gmx} package"
echo " libgtkplot-${gmx} makes available to gtk+${gmx} programs, a pi/donut,"
echo " linear and polar plotting widgets. Features include: multiplot"
echo " capability, zoom control, smart range setting and mouse location"
echo " tracking. Processing is optimised towards speed for large data sets as"
echo " commonly occurs in scientific applications and outputs to a neat"
echo " publication ready format."
echo ""
echo "Package: libgtkplot-${gmx}-dbg"
echo "Section: debug"
echo "Architecture: any"
echo "Depends: libgtkplot-${gmx} (= \${binary:Version})"
echo "Description: Provides debug symbols for the libgtkplot-${gmx} package"
echo " libgtkplot-${gmx} makes available to gtk+${gmx} programs, a pi/donut,"
echo " linear and polar plotting widgets. Features include: multiplot"
echo " capability, zoom control, smart range setting and mouse location"
echo " tracking. Processing is optimised towards speed for large data sets as"
echo " commonly occurs in scientific applications and outputs to a neat"
echo " publication ready format."
echo ""
echo "Package: libgtkplot-${gmx}"
echo "Section: libs"
echo "Architecture: any"
echo "Depends: \${shlibs:Depends}, \${misc:Depends}"
echo "Description: Provides shared libraries for plotting with gtk+${gmx}"
echo " libgtkplot-${gmx} makes available to gtk+${gmx} programs, a pi/donut,"
echo " linear and polar plotting widgets. Features include: multiplot"
echo " capability, zoom control, smart range setting and mouse location"
echo " tracking. Processing is optimised towards speed for large data sets as"
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
echo "  dh  \$@ --with autoreconf"
echo ".PHONY: override_dh_strip"
echo "override_dh_strip:"
echo "  dh_strip --dbg-package=libgtkplot-${gmx}-dbg"
} >debian/rules
{
echo "libgtkplot ${gmx}.${gar} libgtkplot-${gmx}"
} >debian/shlibs.local
{
echo "Summary:	Provides shared libraries for plotting with gtk+${gmx}"
echo "Name:		gtk${gmx}plot"
echo "Version:	${gmx}.${lcr}.${lrv}"
echo "Release:	1%{?dist}"
echo "License:	LGPLv${gmx}+"
echo "Group:		Development/Libraries"
echo "Source:		%{name}-%{version}.tar.gz"
echo "BuildRequires:	automake, autoconf, libtool, gtk${gmx} >= ${gmx}.${gmn}, pkgconfig"
echo "Requires:	gtk+${gmx} >= ${gmx}.${gmn}"
echo ""
echo "%description"
echo "%{name} makes available to gtk+${gmx} programs, a pi/donut, "
echo "linear and polar plotting widgets. Features include: multiplot "
echo "capability, zoom control, smart range setting and mouse location "
echo "tracking. Processing is optimised towards speed for large data sets as "
echo "commonly occurs in scientific applications and outputs to a neat "
echo "publication ready format."
echo ""
echo "%package devel"
echo "Summary:	Provides headers for the %{name} package"
echo "Group:		Development/Libraries"
echo "Requires:	%{name} = %{version}-%{release}, gtk${gmx} >= ${gmx}.${gmn}, pkgconfig"
echo ""
echo "%description devel"
echo "This package contains the header files and libraries needed to develop "
echo "applications that use %{name}."
echo ""
echo "%package static"
echo "Summary:	Static development files for %{name}"
echo "Group:		Development/Libraries"
echo "Requires:	%{name} = %{version}-%{release}"
echo ""
echo "%description static"
echo "This package contains static libraries to develop applications that use %{name}."
echo ""
echo "%prep"
echo "%setup -q"
echo ""
echo "%build"
echo "%configure --disable-testbinaries"
echo "make %{?_smp_mflags}"
echo ""
echo "%install"
echo "%make_install"
echo "rm -f \$RPM_BUILD_ROOT/%{_libdir}/*.la"
echo ""
echo "%post -p /sbin/ldconfig"
echo "%postun -p /sbin/ldconfig"
echo ""
echo "%files"
echo "%defattr(-,root,root)"
echo "%doc AUTHORS COPYING README NEWS ChangeLog"
echo "%{_libdir}/libgtkplot-${gmx}.${gar}.so.*"
echo ""
echo "%files devel"
echo "%defattr(-,root,root)"
echo "%{_includedir}/gtkplot-${gmx}.${gar}/gtkplot*.h"
echo "%{_libdir}/gtkplot-${gmx}.${gar}/include/gtkplotconfig.h"
echo "%{_libdir}/libgtkplot-${gmx}.${gar}.so"
echo "%{_libdir}/pkgconfig/gtkplot-${gmx}.${gar}.pc"
echo ""
echo "%files static"
echo "%defattr(-,root,root)"
echo "%{_libdir}/libgtkplot-${gmx}.${gar}.a"
echo ""
echo "%changelog"
echo "* Sun Jun 22 2014 Paul Childs <pchilds@physics.org> ${gmx}.0.0-1"
echo "- Initial build"
} >SPECS/gtk${gmx}plot-${gmx}.${lcr}.${lrv}.spec
