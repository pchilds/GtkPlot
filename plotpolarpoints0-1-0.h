/***************************************************************************
 *            plotpolarpoints.h
 *
 *  A GTK+ widget that plots data (points only) on polar axes
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

#ifndef __PLOT_POLAR_POINTS_H__
#define __PLOT_POLAR_POINTS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PLOT_TYPE_POLAR_POINTS (plot_polar_points_get_type())
#define PLOT_POLAR_POINTS(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), PLOT_TYPE_POLAR_POINTS, PlotPolarPoints))
#define PLOT_IS_POLAR_POINTS(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PLOT_TYPE_POLAR_POINTS))
#define PLOT_POLAR_POINTS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), PLOT_POLAR_POINTS, PlotPolarPointsClass))
#define PLOT_IS_POLAR_POINTS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PLOT_TYPE_POLAR_POINTS))
#define PLOT_GET_POLAR_POINTS_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), PLOT_TYPE_POLAR_POINTS, PlotPolarPointsClass))

typedef struct _PlotPolarPoints PlotPolarPoints;
typedef struct _PlotPolarPointsClass PlotPolarPointsClass;

struct _PlotPolarPoints
  {
  GtkDrawingArea parent;
  GArray *rdata; /* radial data values */
  GArray *thdata; /* azimuthal data values */
  gint size; /* number of data points to display */
  gchar *rlab; /* label for the r axis */
  gchar *thlab; /* label for the theta axis */
  guint afsize; /* font size for the tick mark labels */
  guint lfsize; /* font size for the axis labels */
  guint ptsize; /* radii of the points */
  gint zmode; /* zoom mode flags xxxx0b/xxxx1b = zoom in/out,1001xb = zoom radial only, 1010xb = zoom azimuthal only, 1011xb = zoom both, 0xxxxb = cartesian shift mode, x1xxxb = single click zoom */
  gdouble rps; /* radial position of mouse */
  gdouble thps; /* azimuthal position of mouse */
  };

struct _PlotPolarPointsClass
  {
  GtkDrawingAreaClass parent_class;
  void (*moved) (PlotPolarPoints *plot);
  };

GType plot_polar_points_get_type (void);
gboolean plot_polar_points_update_scale(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx);
gboolean plot_polar_points_update_scale_pretty(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx);
gboolean plot_polar_points_print_eps(GtkWidget *widget, gchar *fout);
GtkWidget *plot_polar_points_new (void);

G_END_DECLS

#endif
