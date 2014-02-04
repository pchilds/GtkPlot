/***************************************************************************
 *            gtkplotsurf3.c
 *
 *  A GTK+ widget that plots data
 *  version 0.1.0
 *  Features:
 *            automatic wiggle insertion
 *            multiple plot capability
 *            optimal visualisation of axes
 *            zoomable ranges
 *            signal emission for mouse movement
 *
 *  < Mon Jan  14 07:28:00 2014
 *  Copyright  2014  Paul Childs
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
#include "gtkplotsurf3.h"

#define GTK_PLOT_SURF3_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_PLOT_TYPE_SURF3, GtkPlotSurf3Private))
G_DEFINE_TYPE (GtkPlotSurf3, gtk_plot_surf3, GTK_TYPE_PLOT);
enum {PROP_0, };
enum {MOVED, LAST_SIGNAL};
static guint gtk_plot_surf3_signals[LAST_SIGNAL]={0};
typedef struct _GtkPlotSurf3Private GtkPlotSurf3Private;
struct xs {gdouble xmin, ymin, xmax, ymax;};
struct tk {guint xj, yj, xn, yn;};
struct _GtkPlotSurf3Private {struct xs bounds, rescale; struct tk ticks, range; guint flaga, flagr;};

static void drawz(GtkWidget *widget, cairo_t *cr) {
    gint xw;
    gdouble dt;
    GtkPlotLinear *plot;

    xw=widget->allocation.width;
    plot=GTK_PLOT_LINEAR(widget);
    cairo_set_source_rgba(cr, 0, 0, 0, 1);
    cairo_set_line_width(cr, 1);
    cairo_rectangle(cr, xw-21.5, 0.5, 10, 10);
    cairo_rectangle(cr, xw-10.5, 0.5, 10, 10);
    cairo_move_to(cr, xw-9, 5.5);
    cairo_line_to(cr, xw-2, 5.5);
    if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_DRG)!=0) {
        cairo_move_to(cr, xw-2.5, 4.5);
        cairo_line_to(cr, xw-2, 5.5);
        cairo_line_to(cr, xw-2.5, 6.5);
    } else {
        cairo_move_to(cr, xw-5.5, 2);
        cairo_line_to(cr, xw-5.5, 9);
        if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_OUT)==0) {
            cairo_move_to(cr, xw-6.5, 2.5);
            cairo_line_to(cr, xw-5.5, 2);
            cairo_line_to(cr, xw-4.5, 2.5);
            cairo_move_to(cr, xw-2.5, 4.5);
            cairo_line_to(cr, xw-2, 5.5);
            cairo_line_to(cr, xw-2.5, 6.5);
            cairo_move_to(cr, xw-6.5, 8.5);
            cairo_line_to(cr, xw-5.5, 9);
            cairo_line_to(cr, xw-4.5, 8.5);
            cairo_move_to(cr, xw-8.5, 4.5);
            cairo_line_to(cr, xw-9, 5.5);
            cairo_line_to(cr, xw-8.5, 6.5);
        } else {
            cairo_move_to(cr, xw-7.5, 3.5);
            cairo_line_to(cr, xw-3.5, 7.5);
            cairo_move_to(cr, xw-7.5, 7.5);
            cairo_line_to(cr, xw-3.5, 3.5);
        }
    }
    cairo_stroke(cr);
    if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_SGL)==0) {
        cairo_save(cr);
        dt=1;
        cairo_set_dash(cr, &dt, 1, 0);
        if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_VRT)!=0) {
            cairo_move_to(cr, xw-20, 2.5);
            cairo_line_to(cr, xw-13, 2.5);
            cairo_move_to(cr, xw-20, 8.5);
            cairo_line_to(cr, xw-13, 8.5);
            cairo_stroke(cr);
        } if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_HZT)!=0) {
            cairo_move_to(cr, xw-19.5, 2);
            cairo_line_to(cr, xw-19.5, 9);
            cairo_move_to(cr, xw-13.5, 2);
            cairo_line_to(cr, xw-13.5, 9);
            cairo_stroke(cr);
        }
        cairo_restore(cr);
    }
}






static void gtk_plot_surf3_class_init(GtkPlotSurf3Class *klass)
{
	GObjectClass *obj_klass;
	GtkWidgetClass *widget_klass;

	obj_klass=G_OBJECT_CLASS(klass);
	g_type_class_add_private(obj_klass, sizeof(GtkPlotSurf3Private));
	(obj_klass->finalize)=(GObjectFinalizeFunc) gtk_plot_linear_finalise;
	(obj_klass->set_property)=gtk_plot_linear_set_property;
	(obj_klass->get_property)=gtk_plot_linear_get_property;
	g_object_class_install_property(obj_klass, PROP_BXN, g_param_spec_double("xmin", "Minimum x value", "Minimum value for the horizontal scale", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_BXX, g_param_spec_double("xmax", "Maximum x value", "Maximum value for the horizontal scale", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_BYN, g_param_spec_double("ymin", "Minimum y value", "Minimum value for the vertical scale", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_BYX, g_param_spec_double("ymax", "Maximum y value", "Maximum value for the vertical scale", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_XTJ, g_param_spec_uint("xbigticks", "Major x ticks-1", "Number of gaps between major ticks for the horizontal scale-1", 1, G_MAXINT, 4, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_YTJ, g_param_spec_uint("ybigticks", "Major y ticks-1", "Number of gaps between major ticks for the vertical scale-1", 1, G_MAXINT, 4, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_XTN, g_param_spec_uint("xsmallticks", "Minor x ticks+1", "Number of unlabelled tick divisions between major ticks for the horizontal scale+1", 1, G_MAXINT, 5, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_YTN, g_param_spec_uint("ysmallticks", "Minor y ticks+1", "Number of unlabelled ticks divisions between major ticks for the vertical scale+1", 1, G_MAXINT, 5, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_FA, g_param_spec_flags("aflag", "Axis Flags", "Flags for axes behaviour: 32 = Labels right, 16 = Labels above, 8 = Wiggle on top, 4 = Wiggle underneath, 2 = Wiggle on Right, 1 = Wiggle on left", G_TYPE_FLAGS, 0, G_PARAM_READWRITE));
	widget_klass=GTK_WIDGET_CLASS(klass);
	(widget_klass->button_press_event)=gtk_plot_surf3_button_press;
	(widget_klass->motion_notify_event)=gtk_plot_surf3_motion_notify;
	(widget_klass->button_release_event)=gtk_plot_surf3_button_release;
	(widget_klass->expose_event)=gtk_plot_surf3_expose;
	gtk_plot_surf3_signals[MOVED]=g_signal_new("moved", G_OBJECT_CLASS_TYPE(obj_klass), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkPlotSurf3Class, moved), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void gtk_plot_surf3_init(GtkPlotSurf3 *plot)
{
	GtkPlotSurf3Private *priv;
	gdouble val;

	gtk_widget_add_events(GTK_WIDGET(plot), GDK_BUTTON_PRESS_MASK|GDK_POINTER_MOTION_MASK|GDK_BUTTON_RELEASE_MASK);
	priv=GTK_PLOT_SURF3_GET_PRIVATE(plot);
	{(priv->bounds.xmin)=0; (priv->bounds.xmax)=1; (priv->bounds.ymin)=0; (priv->bounds.ymax)=1;}
	{(priv->ticks.xj)=4; (priv->ticks.yj)=4; (priv->ticks.xn)=5; (priv->ticks.yn)=5;}
	{(priv->range.xj)=0; (priv->range.yj)=0; (priv->range.xn)=1; (priv->range.yn)=1;}
	{(plot->xdp)=2; (plot->ydp)=2;}
	{(priv->flaga)=0; (priv->flagr)=0;}
	{(plot->xdata)=NULL; (plot->ydata)=NULL;}
	{(plot->xlab)=g_strdup("Domain"); (plot->ylab)=g_strdup("Amplitude");}
	{(plot->flagd)=GTK_PLOT_LINEAR_DISP_LIN; (plot->ptsize)=5; (plot->linew)=2;}
	(plot->zmode)=(GTK_PLOT_LINEAR_ZOOM_VRT|GTK_PLOT_LINEAR_ZOOM_HZT);
	{(plot->xps)=0; (plot->yps)=0;}
}

GtkWidget *gtk_plot_surf3_new(void) {return g_object_new(GTK_PLOT_TYPE_SURF3, NULL);}
