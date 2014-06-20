/***************************************************************************
 *            gtkplotdonut.h
 *
 *  A GTK+ widget that plots a pi/donut graph
 *
 *  Sat Dec  4 17:18:14 2013
 *  Copyright  2013  Paul Childs
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

#ifndef __GTK_PLOT_DONUT_H__
#	define __GTK_PLOT_DONUT_H__
#	include <gtk/gtk.h>
#	include "gtkplot.h"
	G_BEGIN_DECLS
#	define GTK_PLOT_TYPE_DONUT (gtk_plot_donut_get_type())
#	define GTK_PLOT_DONUT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_PLOT_TYPE_DONUT, GtkPlotDonut))
#	define GTK_PLOT_IS_DONUT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_PLOT_TYPE_DONUT))
#	define GTK_PLOT_DONUT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GTK_PLOT_TYPE_DONUT, GtkPlotDonutClass))
#	define GTK_PLOT_IS_DONUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GTK_PLOT_TYPE_DONUT))
#	define GTK_PLOT_GET_DONUT_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_PLOT_TYPE_DONUT, GtkPlotDonutClass))
	typedef struct _GtkPlotDonut GtkPlotDonut;
	typedef struct _GtkPlotDonutClass GtkPlotDonutClass;
	typedef enum
	{
		GTK_PLOT_DONUT_DISP_KEY = 1 << 0,
		GTK_PLOT_DONUT_DISP_VAL = 1 << 1
	} GtkPlotDonutDisp;
	struct _GtkPlotDonut
	{
		GtkPlot parent;
		GArray *kdata, *vdata; /* key and value data */
		guint linew; /* line width */
		gint flagd;
	};
	struct _GtkPlotDonutClass
	{
		GtkPlotClass parent_class;
	};
	gboolean gtk_plot_donut_refresh(GtkWidget *widget);
	gboolean gtk_plot_donut_print(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer data);
	gboolean gtk_plot_donut_print_eps(GtkWidget *widget, gchar *fout);
	gboolean gtk_plot_donut_print_png(GtkWidget *widget, gchar *fout);
	gboolean gtk_plot_donut_print_svg(GtkWidget *widget, gchar *fout);
	void gtk_plot_donut_set_data(GtkPlotDonut *plot, GArray *kdata, GArray *vdata, GArray *nd, GArray *sz, GArray *st);
	GtkWidget *gtk_plot_donut_new(void);
	extern GType gtk_plot_donut_get_type(void);
	G_END_DECLS
#endif
