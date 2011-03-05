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
#define __PLOT_LINEAR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PLOT_TYPE_LINEAR (plot_linear_get_type())
#define PLOT_LINEAR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), PLOT_TYPE_LINEAR, PlotLinear))
#define PLOT_IS_LINEAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PLOT_TYPE_LINEAR))
#define PLOT_LINEAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), PLOT_LINEAR, PlotLinearClass))
#define PLOT_IS_LINEAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PLOT_TYPE_LINEAR))
#define PLOT_GET_LINEAR_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), PLOT_TYPE_LINEAR, PlotLinearClass))

typedef struct _PlotLinear PlotLinear;
typedef struct _PlotLinearClass PlotLinearClass;

struct _PlotLinear
{
	GtkDrawingArea parent;
	GArray *xdata, *ydata; /* x and y data sets */
	GArray *ind, *sizes; /* indices of first element and number of elements for each trace */
	gchar *xlab, *ylab; /* labels for the x and y axes */
	guint afsize, lfsize; /* font size for the tick mark and axis labels */
	guint ptsize, linew; /* radii of the points and line width of the plot line */
	gint zmode; /* zoom mode flags xxx0b/xxx1b = zoom in/out, 001xb = zoom horizontal only, 010xb = zoom vertical only, 011xb = zoom both, 1xxxb = single click zoom */
	gdouble xps, yps; /* x and y position of mouse */
	guint flagd; /* data display flags 1=lines only, 2=points only, 3=both */
};

struct _PlotLinearClass
{
	GtkDrawingAreaClass parent_class;
	void (*moved) (PlotLinear *plot);
};

GType plot_linear_get_type(void);
gboolean plot_linear_update_scale(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx);
gboolean plot_linear_update_scale_pretty(GtkWidget *widget, gdouble xl, gdouble xu, gdouble yl, gdouble yu);
gboolean plot_linear_print_eps(GtkWidget *widget, gchar *fout);
GtkWidget *plot_linear_new(void);

G_END_DECLS

#endif
