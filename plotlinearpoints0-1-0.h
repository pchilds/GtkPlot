/***************************************************************************
 *            plotlinearpoints.h
 *
 *  A GTK+ widget that plots data (points only)
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

#ifndef __PLOT_LINEAR_POINTS_H__
#define __PLOT_LINEAR_POINTS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PLOT_TYPE_LINEAR_POINTS (plot_linear_points_get_type ())
#define PLOT_LINEAR_POINTS(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), PLOT_TYPE_LINEAR_POINTS, PlotLinearPoints))
#define PLOT_IS_LINEAR_POINTS(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PLOT_TYPE_LINEAR_POINTS))
#define PLOT_LINEAR_POINTS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), PLOT_LINEAR_POINTS, PlotLinearPointsClass))
#define PLOT_IS_LINEAR_POINTS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PLOT_TYPE_LINEAR_POINTS))
#define PLOT_GET_LINEAR_POINTS_CLASS (G_TYPE_INSTANCE_GET_CLASS((obj), PLOT_TYPE_LINEAR_POINTS, PlotLinearPointsClass))

typedef struct _PlotLinearPoints PlotLinearPoints;
typedef struct _PlotLinearPointsClass PlotLinearPointsClass;

struct _PlotLinearPoints
  {
  GtkDrawingArea parent;
  GArray *xdata; /* x data set */
  GArray *ydata; /* y data set */
  gint size; /* number of data points to display */
  gchar *xlab; /* label for the x axis */
  gchar *ylab; /* label for the y axis */
  guint afsize; /* font size for the tick mark labels */
  guint lfsize; /* font size for the axis labels */
  guint ptsize; /* radii of the points */
  gint zmode; /* zoom mode flags xxx0b/xxx1b = zoom in/out, 001xb = zoom horizontal only, 010xb = zoom vertical only, 011xb = zoom both, 1xxxb = single click zoom */
  gdouble xps; /* x position of mouse */
  gdouble yps; /* y position of mouse */
  };

struct _PlotLinearPointsClass
  {
  GtkDrawingAreaClass parent_class;
  void (*moved) (PlotLinearPoints *plot);
  };

GType plot_linear_points_get_type (void);
gboolean plot_linear_points_update_scale(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx);
gboolean plot_linear_points_update_scale_pretty(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx);
GtkWidget *plot_linear_points_new (void);

G_END_DECLS

#endif
