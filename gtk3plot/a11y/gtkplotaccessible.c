/***************************************************************************
 *            gtkplotaccessible.c
 *
 *  An ATK base class for data plotting
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
#include "gtkplotaccessible.h"
G_DEFINE_TYPE(GtkPlotAccessible, _gtk_plot_accessible, GTK_TYPE_WIDGET_ACCESSIBLE);

static void gtk_plot_accessible_initialise(AtkObject *obj, gpointer data)
{
	GtkWidget *widget;

	(ATK_OBJECT_CLASS(_gtk_plot_accessible_parent_class))->initialize(obj, data);
	obj->role=ATK_ROLE_CHART;
}

static void gtk_plot_accessible_finalise(GObject *obj) {(G_OBJECT_CLASS(_gtk_plot_accessible_parent_class))->finalize(obj);}

static void _gtk_plot_accessible_class_init(GtkPlotAccessibleClass *klass)
{
	(ATK_OBJECT_CLASS(klass))->initialize=gtk_plot_accessible_initialise;
	(G_OBJECT_CLASS(klass))->finalize=gtk_plot_accessible_finalise;
}

static void _gtk_plot_accessible_init(GtkPlotAccessible *plot) {}
