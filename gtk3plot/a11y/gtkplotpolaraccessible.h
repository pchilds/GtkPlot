/***************************************************************************
 *            gtkplotpolaraccessible.h
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

#ifndef __GTK_PLOT_POLAR_ACCESSIBLE_H__
#	define __GTK_PLOT_POLAR_ACCESSIBLE_H__
#	include "gtkplotaccessible.h"
	G_BEGIN_DECLS
#	define GTK_TYPE_PLOT_POLAR_ACCESSIBLE (_gtk_plot_linear_accessible_get_type())
#	define GTK_PLOT_POLAR_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_PLOT_POLAR_ACCESSIBLE, GtkPlotPolarAccessible))
#	define GTK_IS_PLOT_POLAR_ACCESSIBLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_PLOT_POLAR_ACCESSIBLE))
#	define GTK_PLOT_POLAR_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GTK_TYPE_PLOT_POLAR_ACCESSIBLE, GtkPlotPolarAccessibleClass))
#	define GTK_IS_PLOT_POLAR_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GTK_TYPE_PLOT_POLAR_ACCESSIBLE))
#	define GTK_PLOT_POLAR_ACCESSIBLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_PLOT_POLAR_ACCESSIBLE, GtkPlotPolarAccessibleClass))
	typedef struct _GtkPlotPolarAccessible GtkPlotPolarAccessible;
	typedef struct _GtkPlotPolarAccessibleClass GtkPlotPolarAccessibleClass;
	struct _GtkPlotPolarAccessible
	{
		GtkPlotAccessible parent;
		gchar *image_description;
	};
	struct _GtkPlotPolarAccessibleClass
	{
		GtkPlotAccessibleClass parent_class;
	};
	GType _gtk_plot_polar_accessible_get_type(void);
	G_END_DECLS
#endif
