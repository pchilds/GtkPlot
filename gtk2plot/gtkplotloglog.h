/***************************************************************************
 *            gtkplotloglog.h
 *
 *  A GTK+ widget that plots data
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

#ifndef __GTK_PLOT_LOG_LOG_H__
# define __GTK_PLOT_LOG_LOG_H__
# include <gtk/gtk.h>
# include "gtkplot.h"
  G_BEGIN_DECLS
# define GTK_PLOT_TYPE_LOG_LOG (gtk_plot_log_log_get_type())
# define GTK_PLOT_LOG_LOG(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_PLOT_TYPE_LOG_LOG, GtkPlotLogLog))
# define GTK_PLOT_IS_LOG_LOG(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_PLOT_TYPE_LOG_LOG))
# define GTK_PLOT_LOG_LOG_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GTK_PLOT_TYPE_LOG_LOG, GtkPlotLogLogClass))
# define GTK_PLOT_IS_LOG_LOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GTK_PLOT_TYPE_LOG_LOG))
# define GTK_PLOT_LOG_LOG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_PLOT_TYPE_LOG_LOG, GtkPlotLogLogClass))
  typedef struct _GtkPlotLogLog GtkPlotLogLog;
  typedef struct _GtkPlotLogLogClass GtkPlotLogLogClass;
  typedef enum {
    GTK_PLOT_LOG_LOG_ZOOM_OUT = 1 << 0,
    GTK_PLOT_LOG_LOG_ZOOM_DRG = 1 << 1,
    GTK_PLOT_LOG_LOG_ZOOM_HZT = 1 << 2,
    GTK_PLOT_LOG_LOG_ZOOM_VRT = 1 << 3,
    GTK_PLOT_LOG_LOG_ZOOM_SGL = 1 << 4
  } GtkPlotLogLogZoom;
  typedef enum {
    GTK_PLOT_LOG_LOG_DISP_LIN = 1 << 0,
    GTK_PLOT_LOG_LOG_DISP_PTS = 1 << 1
  } GtkPlotLogLogDisp;
  typedef enum {
    GTK_PLOT_LOG_LOG_KEY_CLR = 1 << 0,
    GTK_PLOT_LOG_LOG_KEY_DSH = 3 << 1
  } GtkPlotLogLogKey;
  struct _GtkPlotLogLog {
    GtkPlot parent;
    GArray *xdata, *ydata; /* x and y data sets */
    gchar *xlab, *ylab; /* labels for the x and y axes */
    GArray *kdata; /* opacity and style flags for plots */
    guint ptsize, linew; /* radii of the points and line width of the plot line */
    gint zmode; /* zoom mode flags */
    gdouble xps, yps; /* x and y position of mouse */
    guint flagd; /* data display flags */
  };
  struct _GtkPlotLogLogClass {
    GtkPlotClass parent_class;
    void (*moved) (GtkPlotLogLog *plot);
    void (*key_changed) (GtkPlotLogLog *plot);
  };
  gboolean gtk_plot_log_log_update_scale(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx);
  gboolean gtk_plot_log_log_update_scale_pretty(GtkWidget *widget, gdouble xl, gdouble xu, gdouble yl, gdouble yu);
  gboolean gtk_plot_log_log_print(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer data);
  gboolean gtk_plot_log_log_print_eps(GtkWidget *widget, gchar *fout);
  gboolean gtk_plot_log_log_print_png(GtkWidget *widget, gchar *fout);
  gboolean gtk_plot_log_log_print_svg(GtkWidget *widget, gchar *fout);
  void gtk_plot_log_log_set_label(GtkPlotLogLog *plot, gchar *xl, gchar *yl);
  void gtk_plot_log_log_set_data(GtkPlotLogLog *plot, GArray *xd, GArray *yd, GArray *nd, GArray *sz, GArray *st, GArray *kd);
  GtkWidget *gtk_plot_log_log_new(void);
  extern GType gtk_plot_log_log_get_type(void);
  G_END_DECLS
#endif
