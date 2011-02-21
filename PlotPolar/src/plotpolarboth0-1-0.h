/***************************************************************************
 *            plotpolarboth.h
 *
 *  A GTK+ widget that plots data (points and lines) on polar axes
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

#ifndef __PLOT_POLAR_BOTH_H__
#define __PLOT_POLAR_BOTH_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PLOT_TYPE_POLAR_BOTH (plot_polar_both_get_type())
#define PLOT_POLAR_BOTH(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), PLOT_TYPE_POLAR_BOTH, PlotPolarBoth))
#define PLOT_IS_POLAR_BOTH(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PLOT_TYPE_POLAR_BOTH))
#define PLOT_POLAR_BOTH_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), PLOT_POLAR_BOTH, PlotPolarBothClass))
#define PLOT_IS_POLAR_BOTH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PLOT_TYPE_POLAR_BOTH))
#define PLOT_GET_POLAR_BOTH_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), PLOT_TYPE_POLAR_BOTH, PlotPolarBothClass))

typedef struct _PlotPolarBoth PlotPolarBoth;
typedef struct _PlotPolarBothClass PlotPolarBothClass;

struct _PlotPolarBoth
  {
  GtkDrawingArea parent;
  GArray *rdata, *thdata; /* radial and azimuthal data values */
  gchar *rlab, *thlab; /* labels for the radial and azimuthal axis */
  gdouble rps, thps; /* radial and azimuthal position of mouse */
  guint afsize, lfsize, ptsize, linew; /* font size for the tick mark and axis labels, the point radii and line width */
  gint zmode; /* zoom mode flags xxxx0b/xxxx1b = zoom in/out,1001xb = zoom radial only, 1010xb = zoom azimuthal only, 1011xb = zoom both, 0xxxxb = cartesian shift mode, x1xxxb = single click zoom */
  gint size; /* number of data points to display */
  gint flags; /* flags: 1 = Removed Centre */
  };

struct _PlotPolarBothClass
  {
  GtkDrawingAreaClass parent_class;
  void (*moved) (PlotPolarBoth *plot);
  };

GType plot_polar_both_get_type (void);
gboolean plot_polar_both_update_scale(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx);
gboolean plot_polar_both_update_scale_pretty(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx);
GtkWidget *plot_polar_both_new (void);

G_END_DECLS

#endif
