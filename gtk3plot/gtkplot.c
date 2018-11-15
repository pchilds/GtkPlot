/***************************************************************************
 *            gtkplot.c
 *
 *  A GTK+ base class for data plotting
 *
 *  Sat Dec  4 17:18:14 2010
 *  Copyright  2011  Paul Childs
 *  <pchilds@physics.org>
 ****************************************************************************/

/*  GtkPlot3 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include "gtkplot.h"
#include "a11y/gtkplotaccessible.h"
G_DEFINE_TYPE(GtkPlot, gtk_plot, GTK_TYPE_DRAWING_AREA);

void gtk_plot_set_font(GtkPlot *plot, PangoFontDescription *lf, PangoFontDescription *af)
{
	if (plot->afont) pango_font_description_free(plot->afont);
	if (plot->lfont) pango_font_description_free(plot->lfont);
	{(plot->afont)=pango_font_description_copy(af); (plot->lfont)=pango_font_description_copy(lf);}
}

void gtk_plot_set_colour(GtkPlot *plot, GArray *cl)
{
	if (plot->cl) g_array_unref(plot->cl);
	(plot->cl)=g_array_ref(cl);
}

void gtk_plot_set_indices(GtkPlot *plot, GArray *nd, GArray *sz, GArray *st)
{
	if (plot->ind) g_array_unref(plot->ind);
	if (plot->sizes) g_array_unref(plot->sizes);
	if (plot->stride) g_array_unref(plot->stride);
	{(plot->ind)=g_array_ref(nd); (plot->sizes)=g_array_ref(sz); (plot->stride)=g_array_ref(st);}
}

void gtk_plot_set_index(GtkPlot *plot, GArray *nd)
{
	if (plot->ind) g_array_unref(plot->ind);
	(plot->ind)=g_array_ref(nd);
}

static void gtk_plot_finalise(GtkPlot *plot)
{
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
	if (plot->cl) {
		g_array_unref(plot->cl);
		plot->cl=NULL;
	}
	if (plot->afont) {
		pango_font_description_free(plot->afont);
		plot->afont=NULL;
	}
	if (plot->lfont) {
		pango_font_description_free(plot->lfont);
		plot->lfont=NULL;
	}
}

static void gtk_plot_class_init(GtkPlotClass *klass)
{
	GObjectClass *obj_klass;

	obj_klass=G_OBJECT_CLASS(klass);
	(obj_klass->finalize)=(GObjectFinalizeFunc) gtk_plot_finalise;
	gtk_widget_class_set_accessible_type(GTK_WIDGET_CLASS(klass), GTK_TYPE_PLOT_ACCESSIBLE);
}

static void gtk_plot_init(GtkPlot *plot)
{
	GdkRGBA cl;
	gdouble val;

	{(plot->afont)=pango_font_description_new(); (plot->lfont)=pango_font_description_new();}
	{pango_font_description_set_family((plot->afont), "sans"); pango_font_description_set_family((plot->lfont), "sans");}
	{pango_font_description_set_style((plot->afont), PANGO_STYLE_NORMAL); pango_font_description_set_style((plot->lfont), PANGO_STYLE_NORMAL);}
	{pango_font_description_set_size((plot->afont), 12*PANGO_SCALE); pango_font_description_set_size((plot->lfont), 12*PANGO_SCALE);}
	(plot->cl)=g_array_sized_new(FALSE, FALSE, sizeof(GdkRGBA), 7);
	{(plot->ind)=NULL; (plot->sizes)=NULL; (plot->stride)=NULL;}
	{cl.red=0; cl.green=0; cl.blue=0; cl.alpha=0.8;}
	g_array_append_val((plot->cl), cl);
	{cl.red=1; cl.green=0; cl.blue=0; cl.alpha=0.8;}
	g_array_append_val((plot->cl), cl);
	{cl.red=0; cl.green=1; cl.blue=0; cl.alpha=0.8;}
	g_array_append_val((plot->cl), cl);
	{cl.red=0; cl.green=0; cl.blue=1; cl.alpha=0.8;}
	g_array_append_val((plot->cl), cl);
	{cl.red=1; cl.green=1; cl.blue=0; cl.alpha=0.8;}
	g_array_append_val((plot->cl), cl);
	{cl.red=0; cl.green=1; cl.blue=1; cl.alpha=0.8;}
	g_array_append_val((plot->cl), cl);
	{cl.red=1; cl.green=0; cl.blue=1; cl.alpha=0.8;}
	g_array_append_val((plot->cl), cl);
}

GtkWidget *gtk_plot_new(void) {return g_object_new(GTK_TYPE_PLOT, NULL);}
