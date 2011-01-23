/***************************************************************************
 *            plotloglog.h
 *
 *  A GTK+ widget that plots data on a log-log scale
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

#ifndef __PLOT_LOGLOG_H__
#define __PLOT_LOGLOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PLOT_TYPE_LOGLOG (plot_loglog_get_type())
#define PLOT_LOGLOG(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), PLOT_TYPE_LOGLOG, PlotLoglog))
#define PLOT_IS_LOGLOG(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PLOT_TYPE_LOGLOG))
#define PLOT_LOGLOG_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), PLOT_LOGLOG, PlotLoglogClass))
#define PLOT_IS_LOGLOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PLOT_TYPE_LOGLOG))
#define PLOT_GET_LOGLOG_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), PLOT_TYPE_LOGLOG, PlotLoglogClass))

typedef struct _PlotLoglog PlotLoglog;
typedef struct _PlotLoglogClass PlotLoglogClass;

struct _PlotLoglog
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
  guint linew; /* line width of the plot line */
  gint zmode; /* zoom mode flags xxx0b/xxx1b = zoom in/out, 001xb = zoom horizontal only, 010xb = zoom vertical only, 011xb = zoom both, 1xxxb = single click zoom */
  gdouble xps; /* x position of mouse */
  gdouble yps; /* y position of mouse */
  guint flagd; /* data display flags 1=lines only, 2=points only, 3=both */
  };

struct _PlotLoglogClass
  {
  GtkDrawingAreaClass parent_class;
  void (*moved) (PlotLoglog *plot);
  };

GType plot_loglog_get_type (void);
gboolean plot_loglog_update_scale(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx);
gboolean plot_loglog_update_scale_pretty(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx);
GtkWidget *plot_loglog_new (void);

G_END_DECLS

#endif
