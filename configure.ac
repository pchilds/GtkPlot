AC_PREREQ([2.63])
AC_INIT([Gtk3Plot],[3.0.0],[pchilds@physics.org],[gtk3plot],[https://github.com/pchilds/GtkPlot])
AC_CONFIG_HEADERS([config.h gtkplotconfig.h])
AC_CONFIG_SRCDIR([gtk3plot/gtkplot.h])
AC_CONFIG_AUX_DIR([confsupp])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_CC([gcc cc])
LT_PREREQ([2.2])
LT_INIT([win32-dll])
AC_SUBST([PLOT_API_VERSION], [3.0])
AC_SUBST([PLOT_GTK_MAJOR], [3])
AC_SUBST([PLOT_GTK_MINOR], [8])
AC_SUBST([PLOT_SDIR], [gtk3plot])
lt_enable_auto_import=
case "$host_os" in
   mingw*)
      lt_enable_auto_import="-Wl,--enable-auto-import"
esac
AC_SUBST([lt_enable_auto_import])
AC_ARG_ENABLE([testbinaries],AS_HELP_STRING([--disable-testbinaries],[Disable building test examples -incompatible with some packaging  @<:@yes/no@:>@]),,[enable_testbinaries=yes])
AM_CONDITIONAL([TEST_BIN], [test "$enable_testbinaries" = "yes"])
PKG_CHECK_MODULES([PLOT], [gtk+-3.0 >= 3.8])
AC_CONFIG_FILES([Makefile gtk3plot/Makefile	\
	gtkplot-3.0.pc:gtkplot.pc.in		\
	COPYING:COPYING-3.0.in])
AC_CONFIG_FILES([debian/Makefile])
AC_OUTPUT()
