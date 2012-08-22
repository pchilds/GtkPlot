AC_PREREQ([2.65])
AC_INIT([Gtk2Plot],[2.0.0],[pchilds@physics.org],[gtk2plot],[https://github.com/pchilds/GtkPlot])
AC_CONFIG_HEADERS([config.h gtkplotconfig.h])
AC_CONFIG_SRCDIR([gtk2plot/gtkplotlinear.h])
AC_CONFIG_AUX_DIR([confsupp])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC([gcc cc])
LT_PREREQ([2.2])
LT_INIT([win32-dll])
AC_SUBST([PLOT_API_VERSION], [2.0])
AC_SUBST([PLOT_GTK_MAJOR], [2])
AC_SUBST([PLOT_GTK_VERSION], [gtk+-2.0 >= 2.18])
AC_SUBST([PLOT_SDIR], [gtk2plot])
lt_enable_auto_import=
case "$host_os" in
   mingw*)
      lt_enable_auto_import="-Wl,--enable-auto-import"
esac
AC_SUBST([lt_enable_auto_import])
PKG_CHECK_MODULES([PLOT], [gtk+-2.0 >= 2.18])
AC_CONFIG_FILES([Makefile gtk2plot/Makefile	\
	gtkplot-2.0.pc:gtkplot.pc.in		\
	COPYING:COPYING-2.0.in			\
	])
AC_OUTPUT()
