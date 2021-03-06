/***************************************************************************
 *            gtkplot.c
 *
 *  A GTK+ widget that plots data on polar axes
 *  version 0.1.0
 *  Features:
 *            optimal visualisation of axes
 *            zoomable ranges
 *            signal emission for mouse movement
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

#include <gtk/gtk.h>
#include "gtkplot.h"

G_DEFINE_TYPE(GtkPlot, gtk_plot, GTK_TYPE_DRAWING_AREA);

void gtk_plot_set_font(GtkPlot *plot, PangoFontDescription *lf, PangoFontDescription *af, PangoFontDescription *kf) {
  if (plot->afont) pango_font_description_free(plot->afont);
  if (plot->lfont) pango_font_description_free(plot->lfont);
  if (plot->kfont) pango_font_description_free(plot->kfont);
  plot->afont    = pango_font_description_copy(af);
  plot->lfont    = pango_font_description_copy(lf);
  plot->kfont    = pango_font_description_copy(kf);
}

void gtk_plot_set_colour(GtkPlot *plot, GArray *rd, GArray *gr, GArray *bl, GArray *al, gchar **ky) {
  if (plot->rd) g_array_unref(plot->rd);
  if (plot->gr) g_array_unref(plot->gr);
  if (plot->bl) g_array_unref(plot->bl);
  if (plot->al) g_array_unref(plot->al);
  if (plot->ky) g_strfreev(plot->ky);
  plot->rd    = g_array_ref(rd);
  plot->gr    = g_array_ref(gr);
  plot->bl    = g_array_ref(bl);
  plot->al    = g_array_ref(al);
  plot->ky    = g_strdupv(ky);
}

void gtk_plot_set_indices(GtkPlot *plot, GArray *nx, GArray *sz, GArray *st) {
  if (plot->ind)    g_array_unref(plot->ind);
  if (plot->sizes)  g_array_unref(plot->sizes);
  if (plot->stride) g_array_unref(plot->stride);
  plot->ind       = g_array_ref(nx);
  plot->sizes     = g_array_ref(sz);
  plot->stride    = g_array_ref(st);
}

void gtk_plot_set_index(GtkPlot *plot, GArray *nx) {
  if (plot->ind) g_array_unref(plot->ind);
  plot->ind    = g_array_ref(nx);
}

static void gtk_plot_finalise(GtkPlot *plot) {
  if (plot->ind) {
    g_array_unref(plot->ind);
    plot->ind=NULL;
  }
  if (plot->sizes) {
    g_array_unref(plot->sizes);
    plot->sizes=NULL;
  }
  if (plot->stride) {
    g_array_unref(plot->stride);
    plot->stride=NULL;
  }
  if (plot->afont) {
    pango_font_description_free(plot->afont);
    plot->afont=NULL;
  }
  if (plot->lfont) {
    pango_font_description_free(plot->lfont);
    plot->lfont=NULL;
  }
  if (plot->kfont) {
    pango_font_description_free(plot->kfont);
    plot->kfont=NULL;
  }
  if (plot->rd) {
    g_array_unref(plot->rd);
    plot->rd=NULL;
  }
  if (plot->gr) {
    g_array_unref(plot->gr);
    plot->gr=NULL;
  }
  if (plot->bl) {
    g_array_unref(plot->bl);
    plot->bl=NULL;
  }
  if (plot->al) {
    g_array_unref(plot->al);
    plot->al=NULL;
  }
  if (plot->ky) {
    g_strfreev(plot->ky);
    plot->ky=NULL;
  }
}

static void gtk_plot_class_init(GtkPlotClass *klass) {G_OBJECT_CLASS(klass)->finalize=(GObjectFinalizeFunc) gtk_plot_finalise;}

static void gtk_plot_init(GtkPlot *plot) {
	gdouble val;

  plot->afont=pango_font_description_new();
  plot->lfont=pango_font_description_new();
  plot->kfont=pango_font_description_new();
  pango_font_description_set_family(plot->afont, "sans");
  pango_font_description_set_family(plot->lfont, "sans");
  pango_font_description_set_family(plot->kfont, "sans");
  pango_font_description_set_style(plot->afont, PANGO_STYLE_NORMAL);
  pango_font_description_set_style(plot->lfont, PANGO_STYLE_NORMAL);
  pango_font_description_set_style(plot->kfont, PANGO_STYLE_NORMAL);
  pango_font_description_set_size(plot->afont, 12*PANGO_SCALE);
  pango_font_description_set_size(plot->lfont, 12*PANGO_SCALE);
  pango_font_description_set_size(plot->kfont, 12*PANGO_SCALE);
  plot->sizes=plot->ind=plot->stride=NULL;
  plot->ky=NULL;
  plot->rd=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 10);
  plot->gr=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 10);
  plot->bl=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 10);
  plot->al=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 10);
  val=0;
  g_array_append_val(plot->rd, val);
  g_array_append_val(plot->gr, val);
  g_array_append_val(plot->bl, val);
  g_array_append_val(plot->gr, val);
  g_array_append_val(plot->bl, val);
  g_array_append_val(plot->bl, val);
  val=1;
  g_array_append_val(plot->rd, val);
  g_array_append_val(plot->gr, val);
  g_array_append_val(plot->bl, val);
  val=0;
  g_array_append_val(plot->rd, val);
  g_array_append_val(plot->gr, val);
  g_array_append_val(plot->bl, val);
  g_array_append_val(plot->rd, val);
  val=1.0;
  g_array_append_val(plot->gr, val);
  g_array_append_val(plot->bl, val);
  g_array_append_val(plot->rd, val);
  g_array_append_val(plot->gr, val);
  g_array_append_val(plot->bl, val);
  val=0;
  g_array_append_val(plot->rd, val);
  g_array_append_val(plot->gr, val);
  val=1;
  g_array_append_val(plot->rd, val);
  val=0.8;
  g_array_append_val(plot->al, val);
  g_array_append_val(plot->al, val);
  g_array_append_val(plot->al, val);
  g_array_append_val(plot->al, val);
  g_array_append_val(plot->al, val);
  g_array_append_val(plot->al, val);
  g_array_append_val(plot->al, val);
}

GtkWidget *gtk_plot_new(void) {return g_object_new(GTK_TYPE_PLOT, NULL);}
