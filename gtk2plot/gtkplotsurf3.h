/***************************************************************************
 *            gtkplotsurf3.h
 *
 *  A GTK+ widget that plots data
 *
 *  Mon Jan  14 07:28:00 2014
 *  Copyright  2014  Paul Childs
 *  <pchilds@physics.org>
 ****************************************************************************/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef __GTK_PLOT_SURF3_H__
#	define __GTK_PLOT_SURF3_H__
#	include <gtk/gtk.h>
#	include "gtkplot.h"
	G_BEGIN_DECLS
#	define GTK_PLOT_TYPE_SURF3 (gtk_plot_surf3_get_type())
#	define GTK_PLOT_SURF3(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_PLOT_TYPE_SURF3, GtkPlotSurf3))
#	define GTK_PLOT_IS_SURF3(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_PLOT_TYPE_SURF3))
#	define GTK_PLOT_SURF3_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GTK_PLOT_TYPE_SURF3, GtkPlotSurf3Class))
#	define GTK_PLOT_IS_SURF3_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GTK_PLOT_TYPE_SURF3))
#	define GTK_PLOT_SURF3_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_PLOT_TYPE_SURF3, GtkPlotSurf3Class))
	typedef struct _GtkPlotSurf3 GtkPlotSurf3;
	typedef struct _GtkPlotSurf3Class GtkPlotSurf3Class;
	typedef struct {gdouble x, y, z;} GtkPlotSurf3Tuple;
	typedef struct {gdouble x, y, z, latitude, longitude, roll;} GtkPlotSurf3View;
	typedef enum
	{
		GTK_PLOT_SURF3_NULL,
		GTK_PLOT_SURF3_TRIANGLE, /* defined by three tuples*/
		GTK_PLOT_SURF3_RECTANGLE, /* defined by two lengths and a view*/
		GTK_PLOT_SURF3_CYLINDER_SEGMENT, /* defined by two lengths, an angle and a view*/
		GTK_PLOT_SURF3_CONE_SEGMENT,/* defined by two lengths, two angles and a view*/
		GTK_PLOT_SURF3_SPHERE_SEGMENT/* defined by a length, three angles and a view*/
	} GtkPlotSurf3Shape;
	typedef enum
	{
		GTK_PLOT_SURF3_ZOOM_SGL = 1 << 0,
		GTK_PLOT_SURF3_ZOOM_IN  = 1 << 1,
		GTK_PLOT_SURF3_ZOOM_OUT = 2 << 1,
		GTK_PLOT_SURF3_ZOOM_DRG = 3 << 1,
		GTK_PLOT_SURF3_ZOOM_PAN = 4 << 1,
		GTK_PLOT_SURF3_ZOOM_ROT = 5 << 1
	} GtkPlotSurf3Zoom;
	struct _GtkPlotSurf3
	{
		GtkPlot parent;
		GArray *xdata, *ydata; /* x and y data sets */
		gint zmode; /* zoom mode flags */
	};
	struct _GtkPlotSurf3Class
	{
		GtkPlotClass parent_class;
	};
	gboolean gtk_plot_surf3_update_scale(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx);
	gboolean gtk_plot_surf3_update_scale_pretty(GtkWidget *widget, gdouble xl, gdouble xu, gdouble yl, gdouble yu);
	gboolean gtk_plot_surf3_print(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer data);
	gboolean gtk_plot_surf3_print_eps(GtkWidget *widget, gchar *fout);
	gboolean gtk_plot_surf3_print_png(GtkWidget *widget, gchar *fout);
	gboolean gtk_plot_surf3_print_svg(GtkWidget *widget, gchar *fout);
	void gtk_plot_surf3_set_data(GtkPlotSurf3 *plot, GArray *xd, GArray *yd, GArray *nd, GArray *sz, GArray *st);
	GtkWidget *gtk_plot_surf3_new(void);
	extern GType gtk_plot_surf3_get_type(void);
	G_END_DECLS
#endif
