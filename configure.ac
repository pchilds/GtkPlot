AC_PREREQ([2.65])
AC_INIT([Gtk3Plot],[3.0.0],[pchilds@physics.org],[gtk3plot],[https://github.com/pchilds/GtkPlot])
AC_CONFIG_HEADERS([config.h gtkplotconfig.h])
AC_CONFIG_SRCDIR([gtk3plot/gtkplotlinear.h])
AC_CONFIG_AUX_DIR([confsupp])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC([gcc cc])
LT_PREREQ([2.2])
LT_INIT([win32-dll])
AC_SUBST([PLOT_API_VERSION], [3.0])
AC_SUBST([PLOT_GTK_MAJOR], [3])
AC_SUBST([PLOT_GTK_MINOR], [2])
AC_SUBST([PLOT_SDIR], [gtk3plot])
lt_enable_auto_import=
case "$host_os" in
   mingw*)
      lt_enable_auto_import="-Wl,--enable-auto-import"
esac
AC_SUBST([lt_enable_auto_import])
PKG_CHECK_MODULES([PLOT], [gtk+-3.0 >= 3.2])
AC_CONFIG_FILES([Makefile gtk3plot/Makefile	\
	gtkplot-3.0.pc:gtkplot.pc.in		\
	COPYING:COPYING-3.0.in])
AC_CONFIG_FILES([debian/Makefile])
AC_OUTPUT()
