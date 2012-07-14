/***************************************************************************
 *            plotlinear.h
 *
 *  A GTK+ widget that plots data
 *
 *  Sat Dec  4 17:18:14 2010
 *  Copyright  2011  Paul Childs
 *  <pchilds@physics.org>
 ****************************************************************************/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef __PLOT_LINEAR_H__
#	define __PLOT_LINEAR_H__
#	include <gtk/gtk.h>
	G_BEGIN_DECLS
#	define PLOT_TYPE_LINEAR (plot_linear_get_type())
#	define PLOT_LINEAR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), PLOT_TYPE_LINEAR, PlotLinear))
#	define PLOT_IS_LINEAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PLOT_TYPE_LINEAR))
#	define PLOT_LINEAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), PLOT_LINEAR, PlotLinearClass))
#	define PLOT_IS_LINEAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PLOT_TYPE_LINEAR))
#	define PLOT_LINEAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), PLOT_TYPE_LINEAR, PlotLinearClass))
	typedef struct _PlotLinear PlotLinear;
	typedef struct _PlotLinearClass PlotLinearClass;
	typedef enum
	{
		PLOT_LINEAR_ZOOM_OUT = 1 << 0,
		PLOT_LINEAR_ZOOM_HZT = 1 << 1,
		PLOT_LINEAR_ZOOM_VRT = 1 << 2,
		PLOT_LINEAR_ZOOM_SGL = 1 << 3
	} PlotLinearZoom;
	typedef enum
	{
		PLOT_LINEAR_DISP_LIN = 1 << 0,
		PLOT_LINEAR_DISP_PTS = 1 << 1
	} PlotLinearDisp;
	struct _PlotLinear
	{
		GtkDrawingArea parent;
		GArray *xdata, *ydata; /* x and y data sets */
		GArray *ind, *sizes; /* indices of first element and number of elements for each trace */
		GArray *rd, *gr, *bl, *al; /* colour and alpha of the plots */
		gchar *xlab, *ylab; /* labels for the x and y axes */
		PangoFontDescription *afont, *lfont; /* font descriptions for the tick mark and axis labels */
		guint ptsize, linew; /* radii of the points and line width of the plot line */
		guint xdp, ydp; /* number of decimal points for axes */
		gint zmode; /* zoom mode flags */
		gdouble xps, yps; /* x and y position of mouse */
		guint flagd; /* data display flags */
	};
	struct _PlotLinearClass
	{
		GtkDrawingAreaClass parent_class;
		void (*moved) (PlotLinear *plot);
	};
	gboolean plot_linear_update_scale(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx);
	gboolean plot_linear_update_scale_pretty(GtkWidget *widget, gdouble xl, gdouble xu, gdouble yl, gdouble yu);
	gboolean plot_linear_print_eps(GtkWidget *widget, gchar *fout);
	gboolean plot_linear_print_png(GtkWidget *widget, gchar *fout);
	gboolean plot_linear_print_svg(GtkWidget *widget, gchar *fout);
	GtkWidget *plot_linear_new(void);
	G_END_DECLS
#endif
