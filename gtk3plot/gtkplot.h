/***************************************************************************
 *            gtkplot.h
 *
 *  A GTK+ base class for data plotting
 *
 *  Sat Dec  4 17:18:14 2010
 *  Copyright  2011  Paul Childs
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

#ifndef __GTK_PLOT_H__
#	define __GTK_PLOT_H__
#	include <gtk/gtk.h>
	G_BEGIN_DECLS
#	define GTK_TYPE_PLOT (gtk_plot_get_type())
#	define GTK_PLOT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_PLOT, GtkPlot))
#	define GTK_IS_PLOT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_PLOT))
#	define GTK_PLOT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GTK_PLOT, GtkPlotClass))
#	define GTK_IS_PLOT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GTK_TYPE_PLOT))
#	define GTK_PLOT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_PLOT, GtkPlotClass))
	typedef struct _GtkPlot GtkPlot;
	typedef struct _GtkPlotClass GtkPlotClass;
	struct _GtkPlot
	{
		GtkDrawingArea parent;
		GArray *ind, *sizes; /* indices of first element and number of elements for each trace */
		GArray *cl; /* colour and alpha of the plots */
		PangoFontDescription *afont, *lfont; /* font descriptions for the tick mark and axis labels */
	};
	struct _GtkPlotClass
	{
		GtkDrawingAreaClass parent_class;
	};
	void gtk_plot_set_font(GtkPlot *plot, PangoFontDescription *lf, PangoFontDescription *af);
	void gtk_plot_set_colour(GtkPlot *plot, GArray *cl);
	void gtk_plot_polar_set_indices(GtkPlot *plot, GArray *nd, GArray *sz);
	void gtk_plot_polar_set_index(GtkPlot *plot, GArray *nd);
	GtkWidget *gtk_plot_new(void);
	G_END_DECLS
#endif
