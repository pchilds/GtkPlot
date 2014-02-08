/***************************************************************************
 *            gtkplotdonut.c
 *
 *  A GTK+ widget that plots data on polar axes
 *  version 0.1.0
 *  Features:
 *            optimal visualisation of axes
 *            zoomable ranges
 *            signal emission for mouse movement
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

#include <gtk/gtk.h>
#include <math.h>
#include <cairo-ps.h>
#include <cairo-svg.h>
#include "gtkplotdonut.h"

#define GTK_PLOT_DONUT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_PLOT_TYPE_DONUT, GtkPlotDonutPrivate))
#define DZE 0.0001 /* divide by zero threshold */
#define DV_CRIT 0.02 /* critical percent to display any text */
#define NDV_CRIT -0.02 /* critical percent to display any text */
#define MY_SEP 4 /* separation between KEY and VALUE lines*/
#define MY_FIT 0.85 /* percent limit to allow text in segment */
#define MY_2PI 6.2831853071795864769252867665590057683943387987502
#define MY_BFL 4 /*buffer length for axes*/
G_DEFINE_TYPE (GtkPlotDonut, gtk_plot_donut, GTK_TYPE_PLOT);
enum {PROP_0, PROP_THT};
typedef struct _GtkPlotDonutPrivate GtkPlotDonutPrivate;
struct _GtkPlotDonutPrivate {gdouble theta; gdouble rescale; gint flagr;};

static void draw(GtkWidget *widget, cairo_t *cr)
{
	gboolean ccw;
	gchar lbl[MY_BFL];
	gdouble av, cv, dv, prm, r, sv, vv, wv, xv, yv;
	gint ct, ft, hg1, hg2, j, k, lt, st, wd1, wd2, xw, yw;
	GtkPlot *plt;
	GtkPlotDonut *plot;
	PangoLayout *lyt1, *lyt2;

	xw=widget->allocation.width/2;
	yw=widget->allocation.height/2;
	plot=GTK_PLOT_DONUT(widget);
	plt=GTK_PLOT(widget);
	cairo_set_line_width(cr, plot->linew);
	if (plot->kdata && plot->vdata) /* plot data */
	{
		r = (fmin(xw,yw)-(plot->linew))/(plt->ind->len);
		for (k=0; k<(plt->ind->len); k++)
		{
			ft=g_array_index((plt->ind), gint, k);
			if (ft>=(plot->vdata->len)) continue;
			st=g_array_index((plt->stride), gint, k);
			lt=ft+(st*g_array_index((plt->sizes), gint, k));
			if (lt>(plot->vdata->len)) lt=(plot->vdata->len);
			{sv=0; cv=(GTK_PLOT_DONUT_GET_PRIVATE(plot))->theta;}
			for (j=ft; j<lt; j+=st) sv+=g_array_index((plot->vdata), gdouble, j);
			if (sv>0) ccw=TRUE;
			else {sv=-sv; ccw=FALSE;}
			if (sv<DZE) continue;
			sv=-1/sv;
			for (j=ft; j<lt; j+=st)
			{
				dv=g_array_index((plot->vdata), gdouble, j)*sv;
				cairo_move_to(cr, xw+(r*cos(cv)*(gdouble)k), yw+(r*sin(cv)*(gdouble)k));
				cairo_arc_negative(cr, xw, yw, r*(gdouble)(k+1), cv, cv+(dv*MY_2PI));
				cairo_arc(cr, xw, yw, r*(gdouble)k, cv+(dv*MY_2PI), cv);
				cairo_set_source_rgba(cr, 0, 0, 0, 1);
				cairo_stroke_preserve(cr);
				ct=fmod(j,(plt->rd->len));
				{vv=g_array_index((plt->rd), gdouble, ct); wv=g_array_index((plt->gr), gdouble, ct); xv=g_array_index((plt->bl), gdouble, ct); yv=g_array_index((plt->al), gdouble, ct);}
				cairo_set_source_rgba(cr, vv, wv, xv, yv);
				cairo_fill(cr);
				if (ccw?(dv>NDV_CRIT):(dv<DV_CRIT))
				{
					cv+=(dv*MY_2PI);
					continue;
				}
				cairo_set_source_rgba(cr, 1-vv, 1-wv, 1-xv, 1);
				cv+=(dv*G_PI);
				if (((plot->flagd)&GTK_PLOT_DONUT_DISP_KEY)!=0)
				{
					av=(0.35+(gdouble)k)*fabs(dv)*MY_2PI;
					lyt1=pango_cairo_create_layout(cr);
					pango_layout_set_font_description(lyt1, (plt->lfont));
					pango_layout_set_text(lyt1, g_array_index((plot->kdata), gchar*, j), -1);
					pango_layout_get_pixel_size(lyt1, &wd1, &hg1);
					if (((plot->flagd)&GTK_PLOT_DONUT_DISP_VAL)!=0) /* KEYs and VALUEs */
					{
						lyt2=pango_cairo_create_layout(cr);
						pango_layout_set_font_description(lyt2, (plt->afont));
						g_snprintf(lbl, MY_BFL, "%2d%%", (gint)round(100*fabs(dv)));
						pango_layout_set_text(lyt2, lbl, -1);
						pango_layout_get_pixel_size(lyt2, &wd2, &hg2);
						if (2*(wd1+hg1+hg2+MY_SEP)<MY_FIT*r*((1+av)+((1-av)*cos(2*cv))+(sin(2*cv)*sin(2*cv))))
						{
							if (k==0) prm=r*(0.6+(gdouble)k);
							else prm=r*(0.5+(gdouble)k);
							cairo_move_to(cr, xw+(prm*cos(cv))-(wd2/2), yw+(prm*sin(cv))-((hg2+MY_SEP+hg1)/2));
							pango_cairo_show_layout(cr, lyt2);
							cairo_move_to(cr, xw+(prm*cos(cv))-(wd1/2), yw+(prm*sin(cv))+((hg2+MY_SEP-hg1)/2));
							pango_cairo_show_layout(cr, lyt1);
						}
						g_object_unref(lyt2);
					}
					else /* KEYs only */
					{
						if (2*(wd1+hg1)<MY_FIT*r*((1+av)+((1-av)*cos(2*cv))+(sin(2*cv)*sin(2*cv))))
						{
							if (k==0) prm=r*(0.6+(gdouble)k);
							else prm=r*(0.5+(gdouble)k);
							cairo_move_to(cr, xw+(prm*cos(cv))-(wd1/2), yw+(prm*sin(cv))-(hg1/2));
							pango_cairo_show_layout(cr, lyt1);
						}
					}
					g_object_unref(lyt1);
				}
				else if (((plot->flagd)&GTK_PLOT_DONUT_DISP_VAL)!=0) /* VALUEs only */
				{
					av=(0.3+(gdouble)k)*fabs(sin(dv*MY_2PI));
					lyt2=pango_cairo_create_layout(cr);
					pango_layout_set_font_description(lyt2, (plt->afont));
					g_snprintf(lbl, MY_BFL, "%2d%%", (gint)round(100*fabs(dv)));
					pango_layout_set_text(lyt2, lbl, -1);
					pango_layout_get_pixel_size(lyt2, &wd2, &hg2);
					if (2*(wd2+hg2)<MY_FIT*r*((1+av)+((1-av)*cos(2*cv))+(sin(2*cv)*sin(2*cv))))
					{
						if (k==0) prm=r*(0.6+(gdouble)k);
						else prm=r*(0.5+(gdouble)k);
						cairo_move_to(cr, xw+(prm*cos(cv))-(wd2/2), yw+(prm*sin(cv))-(hg2/2));
						pango_cairo_show_layout(cr, lyt2);
					}
					g_object_unref(lyt2);
				} else  g_print("skipped - no print\n");
				cv+=(dv*G_PI);
			}
		}
	}
}

gboolean gtk_plot_donut_refresh(GtkWidget *widget)
{
	GdkRegion *region;
	GdkWindow *wdw;

	wdw=gtk_widget_get_window(widget);
	if (!wdw) return;
	region=gdk_drawable_get_clip_region(wdw);
	gdk_window_invalidate_region(wdw, region, TRUE);
	gdk_window_process_updates(wdw, TRUE);
	gdk_region_destroy(region);
	return FALSE;
}

gboolean gtk_plot_donut_print(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer data)
{
	cairo_t* cr=gtk_print_context_get_cairo_context(context);
	draw(GTK_WIDGET(data), cr);
	return FALSE;
}

gboolean gtk_plot_donut_print_eps(GtkWidget *plot, gchar* fout)
{
	cairo_t *cr;
	cairo_surface_t *surface;

	surface=cairo_ps_surface_create(fout, (gdouble) (plot->allocation.width), (gdouble) (plot->allocation.height));
	cairo_ps_surface_set_eps(surface, TRUE);
	cairo_ps_surface_restrict_to_level(surface, CAIRO_PS_LEVEL_2);
	cr=cairo_create(surface);
	draw(plot, cr);
	cairo_surface_show_page(surface);
	cairo_destroy(cr);
	cairo_surface_finish(surface);
	cairo_surface_destroy(surface);
	return FALSE;
}

gboolean gtk_plot_donut_print_png(GtkWidget *plot, gchar* fout)
{
	cairo_t *cr;
	cairo_surface_t *surface;

	surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (gdouble) (plot->allocation.width), (gdouble) (plot->allocation.height));
	cr=cairo_create(surface);
	draw(plot, cr);
	cairo_surface_write_to_png(surface, fout);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	return FALSE;
}

gboolean gtk_plot_donut_print_svg(GtkWidget *plot, gchar* fout)
{
	cairo_t *cr;
	cairo_surface_t *surface;

	surface=cairo_svg_surface_create(fout, (gdouble) (plot->allocation.width), (gdouble) (plot->allocation.height));
	cr=cairo_create(surface);
	draw(plot, cr);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	return FALSE;
}

static gboolean gtk_plot_donut_button_press(GtkWidget *widget, GdkEventButton *event)
{
	GtkPlotDonutPrivate *priv;
	gdouble dy, dx;
	
	priv=GTK_PLOT_DONUT_GET_PRIVATE(widget);
	if ((priv->flagr)==0)
	{
		dy=(widget->allocation.height/2)-(event->y);
		dx=(event->x)-(widget->allocation.width/2);
		if (4*((dy*dy)+(dx*dx))<fmin(widget->allocation.width,widget->allocation.height)*fmin(widget->allocation.width,widget->allocation.height))
		{
			(priv->rescale)=atan2(dy, dx);
			(priv->flagr)=1;
		}
	}
	return FALSE;
}

static gboolean gtk_plot_donut_button_release(GtkWidget *widget, GdkEventButton *event)
{
	GtkPlotDonutPrivate *priv;
	gdouble dy, dx;

	priv=GTK_PLOT_DONUT_GET_PRIVATE(widget);
	if ((priv->flagr)==1)
	{
		dy=(widget->allocation.height/2)-(event->y);
		dx=(event->x)-(widget->allocation.width/2);
		if (4*((dy*dy)+(dx*dx))<fmin(widget->allocation.width,widget->allocation.height)*fmin(widget->allocation.width,widget->allocation.height)) (priv->theta)-=atan2(dy, dx)-(priv->rescale);
		gtk_plot_donut_refresh(widget);
		(priv->flagr)=0;
	}
	return FALSE;
}

static void gtk_plot_donut_finalise(GtkPlotDonut *plot)
{
	if (plot->kdata)
	{
		g_array_free((plot->kdata), TRUE);
		plot->kdata=NULL;
	}
	if (plot->vdata)
	{
		g_array_free((plot->vdata), FALSE);
		plot->vdata=NULL;
	}
}

void gtk_plot_donut_set_data(GtkPlotDonut *plot, GArray *kdata, GArray *vdata, GArray *nd, GArray *sz, GArray *st)
{
    gtk_plot_donut_finalise(plot);
	{(plot->kdata)=g_array_ref(kdata); (plot->vdata)=g_array_ref(vdata);}
	gtk_plot_set_indices(GTK_PLOT(plot), nd, sz, st);
}

static void gtk_plot_donut_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch (prop_id)
	{
		case PROP_THT: (GTK_PLOT_DONUT_GET_PRIVATE(object)->theta)=g_value_get_double(value);
		break;
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gtk_plot_donut_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch (prop_id)
	{
		case PROP_THT: g_value_set_double(value, (GTK_PLOT_DONUT_GET_PRIVATE(object)->theta));
		break;
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static gboolean gtk_plot_donut_expose(GtkWidget *widget, GdkEventExpose *event)
{
	cairo_t *cr;

	cr=gdk_cairo_create(gtk_widget_get_window(widget));
	cairo_rectangle(cr, (event->area.x), (event->area.y), (event->area.width), (event->area.height));
	cairo_clip(cr);
	draw(widget, cr);
	cairo_destroy(cr);
	return FALSE;
}

static void gtk_plot_donut_class_init(GtkPlotDonutClass *klass)
{
	GObjectClass *obj_klass;
	GtkWidgetClass *widget_klass;

	obj_klass=G_OBJECT_CLASS(klass);
	g_type_class_add_private(obj_klass, sizeof(GtkPlotDonutPrivate));
	(obj_klass->finalize)=(GObjectFinalizeFunc) gtk_plot_donut_finalise;
	(obj_klass->set_property)=gtk_plot_donut_set_property;
	(obj_klass->get_property)=gtk_plot_donut_get_property;
	g_object_class_install_property(obj_klass, PROP_THT, g_param_spec_double("theta", "Rotation", "Rotation of the chart", -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
	widget_klass=GTK_WIDGET_CLASS(klass);
	(widget_klass->button_press_event)=gtk_plot_donut_button_press;
	(widget_klass->button_release_event)=gtk_plot_donut_button_release;
	(widget_klass->expose_event)=gtk_plot_donut_expose;
}

static void gtk_plot_donut_init(GtkPlotDonut *plot)
{
	GtkPlotDonutPrivate *priv;

	gtk_widget_add_events(GTK_WIDGET(plot), GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);
	priv=GTK_PLOT_DONUT_GET_PRIVATE(plot);
	{(priv->theta)=0; (priv->rescale)=0; (priv->flagr)=0;}
	{(plot->kdata)=NULL; (plot->vdata)=NULL;}
	{(plot->flagd)=(GTK_PLOT_DONUT_DISP_KEY|GTK_PLOT_DONUT_DISP_VAL); (plot->linew)=2;}
}

GtkWidget *gtk_plot_donut_new(void) {return g_object_new(GTK_PLOT_TYPE_DONUT, NULL);}
