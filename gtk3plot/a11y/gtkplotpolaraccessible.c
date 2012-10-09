/***************************************************************************
 *            gtkplotpolaraccessible.c
 *
 *  An ATK class for data plotting
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
#include "gtkplotpolaraccessible.h"
#include "../gtkplotpolar.h"
#define ZS 0.5 /* zoom scale */
#define ZSC 0.5 /* 1 minus this */
#define UZ 2 /* inverse of this */
#define UZC 1 /* this minus 1 */

static gboolean gtk_plot_polar_accessible_do_action(AtkAction* action, gint i)
{
	gint tp, xf, xi, yf, yi;
	GtkPlotPolar *plt;
	GtkWidget *plot;

	plot=gtk_accessible_get_widget(GTK_ACCESSIBLE(action));
	plt=GTK_PLOT_POLAR(plot);
	if (plot==NULL||!gtk_widget_get_sensitive(plot)) return FALSE;
	g_object_get(G_OBJECT(plot), "thmin", &yi, "thmax", &yf, "rmin", &xi, "rmax", &xf, NULL);
	switch i
	{
		case 3:
		tp=-UZC*(plt->thps);
		{yi*=UZ; yf*=UZ;}
		{yi+=tp; yf+=tp;}
		break;
		case 2:
		tp=ZS*(plt->thps);
		{yi*=ZSC; yf*=ZSC;}
		{yi+=tp; yf+=tp;}
		break;
		case 1:
		tp=-UZC*(plt->rps);
		{xi*=UZ; xf*=UZ;}
		{xi+=tp; xf+=tp;}
		break;
		case 0:
		tp=ZS*(plt->rps);
		{xi*=ZSC; xf*=ZSC;}
		{xi+=tp; xf+=tp;}
		break;
		default:
		return FALSE;
	}
	gtk_plot_polar_update_scale_pretty(plot, xi, xf, yi, yf);
	return TRUE;
}

static gint gtk_plot_polar_accessible_get_n_actions(AtkAction* action)
{
	GtkWidget *plot;

	plot=gtk_accessible_get_widget(GTK_ACCESSIBLE(action));
	if (plot==NULL) return 0;
	return 4;
}

static const gchar* gtk_plot_polar_accessible_action_get_name(AtkAction* action, gint i)
{
	if (gtk_plot_polar_accessible_get_n_actions(action)==0) return NULL;
	switch i
	{
		case 5: return "out";
		case 4: return "in";
		case 3: return "open";
		case 2: return "close";
		default: return NULL;
	}
}

static const gchar* gtk_plot_polar_accessible_action_get_description(AtkAction* action, gint i)
{
	if (gtk_plot_polar_accessible_get_n_actions(action)==0) return NULL;
	switch i
	{
		case 5: return "zoom out radially";
		case 4: return "zoom in radially";
		case 3: return "zoom out azimuthally";
		case 2: return "zoom in azimuthally";
		default: return NULL;
	}
}

static const gchar* gtk_plot_polar_accessible_get_keybinding(AtkAction* action, gint i) {}

static void atk_action_interface_init(AtkActionIface* iface)
{
	iface->do_action=gtk_plot_polar_accessible_do_action;
	iface->get_n_actions=gtk_plot_polar_accessible_get_n_actions;
	iface->get_name=gtk_plot_polar_accessible_action_get_name;
	iface->get_name=gtk_plot_polar_accessible_action_get_description;
	iface->get_keybinding=gtk_plot_polar_accessible_get_keybinding;
}

static void gtk_plot_polar_accessible_get_image_position(AtkImage* img, gint* x, gint* y, AtkCoordType coord_type) {atk_component_get_position(ATK_COMPONENT(img), x, y, coord_type);}

static const gchar* gtk_plot_polar_accessible_get_image_description(AtkImage* img) {return (GTK_IMAGE_ACCESSIBLE(img))->image_description;}

static gboolean gtk_plot_polar_accessible_set_image_description(AtkImage* img, const gchar* desc)
{
	GtkPlotPolarAccessible* access=GTK_PLOT_POLAR_ACCESSIBLE(img);

	g_free(access->image_description);
	access->image_description=g_strdup(desc);
	return TRUE;
}

static void gtk_plot_polar_accessible_get_image_size(AtkImage* img, gint* width, gint* height)
{
	GtkWidget* widget;

	widget=gtk_accessible_get_widget(GTK_ACCESSIBLE(img));
	if (widget==NULL) {*height=-1; *width=-1;}
	else {*width=gtk_widget_get_allocated_width(widget); *height=gtk_widget_get_allocated_height(widget);}
}

static void atk_image_interface_init(AtkImageIface* iface)
{
	iface->get_image_position=gtk_plot_polar_accessible_get_image_position;
	iface->get_image_description=gtk_plot_polar_accessible_get_image_description;
	iface->set_image_description=gtk_plot_polar_accessible_set_image_description;
	iface->get_image_size=gtk_plot_polar_accessible_get_image_size;
}

G_DEFINE_TYPE_WITH_CODE(GtkPlotPolarAccessible, _gtk_plot_polar_accessible, GTK_TYPE_WIDGET_ACCESSIBLE,
	G_IMPLEMENT_INTERFACE(ATK_TYPE_ACTION, atk_action_interface_init);
	G_IMPLEMENT_INTERFACE(ATK_TYPE_IMAGE, atk_image_interface_init))

static void gtk_plot_polar_accessible_initialise(AtkObject *obj, gpointer data) {(ATK_OBJECT_CLASS(_gtk_plot_polar_accessible_parent_class))->initialize(obj, data);}

static void gtk_plot_polar_accessible_finalise(GObject *obj)
{
	g_free(GTK_PLOT_POLAR_ACCESSIBLE(obj)->image_description);
	(G_OBJECT_CLASS(_gtk_plot_polar_accessible_parent_class))->finalize(obj);
}

static void _gtk_plot_polar_accessible_class_init(GtkPlotPolarAccessibleClass *klass)
{
	(ATK_OBJECT_CLASS(klass))->initialize=gtk_plot_polar_accessible_initialise;
	(G_OBJECT_CLASS(klass))->finalize=gtk_plot_polar_accessible_finalise;
}

static void _gtk_plot_polar_accessible_init(GtkPlotPolarAccessible* access)
{
	GtkPlotPolar* plt;

	plt=GTK_PLOT_POLAR(gtk_accessible_get_widget(GTK_ACCESSIBLE(access)));
	access->image_description=g_strdup_printf("Polar plot of %s versus %s", plt->rlab, plt->thlab);
}
