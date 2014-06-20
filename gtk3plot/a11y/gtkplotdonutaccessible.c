/***************************************************************************
 *            gtkplotdonutaccessible.c
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
#include "gtkplotdonutaccessible.h"
#include "../gtkplotdonut.h"
#define ZS 0.5 /* zoom scale */
#define ZSC 0.5 /* 1 minus this */
#define UZ 2 /* inverse of this */
#define UZC 1 /* this minus 1 */

static void gtk_plot_donut_accessible_get_image_position(AtkImage* img, gint* x, gint* y, AtkCoordType coord_type) {atk_component_get_position(ATK_COMPONENT(img), x, y, coord_type);}

static const gchar* gtk_plot_donut_accessible_get_image_description(AtkImage* img) {return (GTK_PLOT_DONUT_ACCESSIBLE(img))->image_description;}

static gboolean gtk_plot_donut_accessible_set_image_description(AtkImage* img, const gchar* desc)
{
	GtkPlotDonutAccessible* access=GTK_PLOT_DONUT_ACCESSIBLE(img);

	if (access->image_description) g_free(access->image_description);
	access->image_description=g_strdup(desc);
	return TRUE;
}

static void gtk_plot_donut_accessible_get_image_size(AtkImage* img, gint* width, gint* height)
{
	GtkWidget* widget;

	widget=gtk_accessible_get_widget(GTK_ACCESSIBLE(img));
	if (widget==NULL) {*height=-1; *width=-1;}
	else {*width=gtk_widget_get_allocated_width(widget); *height=gtk_widget_get_allocated_height(widget);}
}

static void atk_image_interface_init(AtkImageIface* iface)
{
	iface->get_image_position=gtk_plot_donut_accessible_get_image_position;
	iface->get_image_description=gtk_plot_donut_accessible_get_image_description;
	iface->set_image_description=gtk_plot_donut_accessible_set_image_description;
	iface->get_image_size=gtk_plot_donut_accessible_get_image_size;
}

G_DEFINE_TYPE_WITH_CODE(GtkPlotDonutAccessible, _gtk_plot_donut_accessible, GTK_TYPE_PLOT_ACCESSIBLE, G_IMPLEMENT_INTERFACE(ATK_TYPE_IMAGE, atk_image_interface_init))

static void gtk_plot_donut_accessible_initialise(AtkObject* obj, gpointer data) {(ATK_OBJECT_CLASS(_gtk_plot_donut_accessible_parent_class))->initialize(obj, data);}

static void gtk_plot_donut_accessible_finalise(GObject* obj)
{
	if (GTK_PLOT_DONUT_ACCESSIBLE(obj)->image_description) g_free(GTK_PLOT_DONUT_ACCESSIBLE(obj)->image_description);
	GTK_PLOT_DONUT_ACCESSIBLE(obj)->image_description=NULL;
	(G_OBJECT_CLASS(_gtk_plot_donut_accessible_parent_class))->finalize(obj);
}

static void _gtk_plot_donut_accessible_class_init(GtkPlotDonutAccessibleClass* klass)
{
	(ATK_OBJECT_CLASS(klass))->initialize=gtk_plot_donut_accessible_initialise;
	(G_OBJECT_CLASS(klass))->finalize=gtk_plot_donut_accessible_finalise;
}

static void _gtk_plot_donut_accessible_init(GtkPlotDonutAccessible* access) {access->image_description=g_strdup_printf("Pi/Donut plot");}
