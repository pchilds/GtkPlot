/***************************************************************************
 *            gtkplotpolar.h
 *
 *  A GTK+ widget that plots data on polar axes
 *
 *  Sat Dec  4 17:18:14 2010
 *  Copyright  2011  Paul Childs
 *  <pchilds@physics.org>
 ****************************************************************************/

/*  GtkPlot3 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GTK_PLOT_POLAR_H__
#	define __GTK_PLOT_POLAR_H__
#	include <gtk/gtk.h>
#	include "gtkplot.h"
	G_BEGIN_DECLS
#	define GTK_PLOT_TYPE_POLAR (gtk_plot_polar_get_type())
#	define GTK_PLOT_POLAR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_PLOT_TYPE_POLAR, GtkPlotPolar))
#	define GTK_PLOT_IS_POLAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_PLOT_TYPE_POLAR))
#	define GTK_PLOT_POLAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GTK_PLOT_TYPE_POLAR, GtkPlotPolarClass))
#	define GTK_PLOT_IS_POLAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GTK_PLOT_TYPE_POLAR))
#	define GTK_PLOT_GET_POLAR_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_PLOT_TYPE_POLAR, GtkPlotPolarClass))
	typedef struct _GtkPlotPolar GtkPlotPolar;
	typedef struct _GtkPlotPolarClass GtkPlotPolarClass;
	typedef enum
	{
		GTK_PLOT_POLAR_ZOOM_OUT = 1 << 0,
		GTK_PLOT_POLAR_ZOOM_RDL = 1 << 1,
		GTK_PLOT_POLAR_ZOOM_AZM = 1 << 2,
		GTK_PLOT_POLAR_ZOOM_SGL = 1 << 3,
		GTK_PLOT_POLAR_ZOOM_ZOM = 1 << 4
	} GtkPlotPolarZoom;
	typedef enum
	{
		GTK_PLOT_POLAR_DISP_RDN = 1 << 0,
		GTK_PLOT_POLAR_DISP_LIN = 1 << 1,
		GTK_PLOT_POLAR_DISP_PTS = 1 << 2,
		GTK_PLOT_POLAR_DISP_PNT = 1 << 3
	} GtkPlotPolarDisp;
	struct _GtkPlotPolar
	{
		GtkPlot parent;
		GArray *rdata, *thdata; /* radial and azimuthal data values */
		gchar *rlab, *thlab; /* labels for the radial and azimuthal axis */
		gdouble rps, thps; /* radial and azimuthal position of mouse */
		guint ptsize, linew; /* point radii and line width */
		guint rdp, thdp; /* number of decimal points for axes */
		gint zmode; /* zoom mode flags xxxx0b/xxxx1b = zoom in/out,1001xb = zoom radial only, 1010xb = zoom azimuthal only, 1011xb = zoom both, 0xxxxb = cartesian shift mode, x1xxxb = single click zoom */
		gint flagd; /* data displayflags: x01xb = lines only, x10xb = points only, x11x = both, xxx0b/xxx1b = Degrees/Radians 0xxxb/1xxxb = Connect the dots/Polar interpolation */
	};
	struct _GtkPlotPolarClass
	{
		GtkPlotClass parent_class;
		void (*moved) (GtkPlotPolar *plot);
	};
	gboolean gtk_plot_polar_update_scale(GtkWidget *widget, gdouble rn, gdouble rx, gdouble thn, gdouble thx, gdouble tcn, gdouble thc);
	gboolean gtk_plot_polar_update_scale_pretty(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx);
	gboolean gtk_plot_polar_print(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer data);
	gboolean gtk_plot_polar_print_eps(GtkWidget *widget, gchar *fout);
	gboolean gtk_plot_polar_print_png(GtkWidget *widget, gchar *fout);
	gboolean gtk_plot_polar_print_svg(GtkWidget *widget, gchar *fout);
	void gtk_plot_polar_set_label(GtkPlotPolar *plot, gchar *rl, gchar *tl);
	void gtk_plot_polar_set_data(GtkPlotPolar *plot, GArray *rd, GArray *td, GArray *nd, GArray *sz, GArray *st);
	GtkWidget *gtk_plot_polar_new(void);
	extern GType gtk_plot_polar_get_type(void);
	G_END_DECLS
#endif
