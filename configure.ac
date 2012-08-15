AC_PREREQ([2.65])
m4_define(gtkplot_configure_ac)
m4_define([gtk_major_dependency], [2])
m4_define([gtk_minor_dependency], [14])
m4_define([gtk_api_revision], [0])
m4_define([gtkplot_lt_current], [0])
m4_define([gtkplot_lt_age], [0])
m4_define([gtkplot_lt_revision], [0])
m4_define([gtk_req], [gtk+-gtk_major_dependency.gtk_api_revision >= gtk_major_dependency.gtk_minor_dependency])
m4_append([Sdir], [Gtk])
m4_append([sdir], [gtk])
m4_append([Sdir], m4_unquote([gtk_major_dependency]))
m4_append([sdir], m4_unquote([gtk_major_dependency]))
m4_append([Sdir], [Plot])
m4_append([sdir], [plot])
AC_INIT([Sdir],[gtk_major_dependency.gtkplot_lt_current.gtkplot_lt_revision],[pchilds@physics.org],[sdir],[https://github.com/pchilds/GtkPlot])
AC_CONFIG_HEADERS([config.h gtkplotconfig.h])
AC_CONFIG_SRCDIR([sdir/gtkplotlinear.h])
AC_CONFIG_AUX_DIR([confsupp])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC([gcc cc])
LT_PREREQ([2.2])
LT_INIT([win32-dll])
AC_SUBST([LT_CURRENT], [gtkplot_lt_current])
AC_SUBST([LT_REVISION], [gtkplot_lt_revision])
AC_SUBST([LT_AGE], [gtkplot_lt_age])
AC_SUBST([PLOT_API_VERSION], [gtk_major_dependency.gtk_api_revision])
AC_SUBST([PLOT_GTK_MAJOR], [gtk_major_dependency])
AC_SUBST([PLOT_GTK_VERSION], [gtk_req])
AC_SUBST([PLOT_SDIR], [sdir])
lt_enable_auto_import=""
case "$host_os" in
   mingw*)
      lt_enable_auto_import="-Wl,--enable-auto-import"
esac
AC_SUBST([lt_enable_auto_import])
PKG_CHECK_MODULES([PLOT], [gtk_req])
AC_CONFIG_FILES([Makefile ${PLOT_SDIR}/Makefile					\
	gtkplot-${PLOT_API_VERSION}.pc:gtkplot.pc.in				\
	COPYING:COPYING-${PLOT_API_VERSION}.in					\
	debian/Makefile								\
	debian/libgtkplot${PLOT_GTK_MAJOR}.install:debian/libgtkplot.install.in	\
	debian/libgtkplot${PLOT_GTK_MAJOR}.dirs:debian/libgtkplot.dirs.in	\
	debian/libgtkplot-dev.install						\
	debian/libgtkplot-dev.dirs						\
	debian/shlibs.local							\
	debian/control								\
	debian/copyright])
AC_OUTPUT()
