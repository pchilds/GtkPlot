/***************************************************************************
 *            plotpolar.h
 *
 *  A GTK+ widget that plots data on polar axes
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

#ifndef __PLOT_POLAR_H__
#	define __PLOT_POLAR_H__
#	include <gtk/gtk.h>
	G_BEGIN_DECLS
#	define PLOT_TYPE_POLAR (plot_polar_get_type())
#	define PLOT_POLAR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), PLOT_TYPE_POLAR, PlotPolar))
#	define PLOT_IS_POLAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PLOT_TYPE_POLAR))
#	define PLOT_POLAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), PLOT_POLAR, PlotPolarClass))
#	define PLOT_IS_POLAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PLOT_TYPE_POLAR))
#	define PLOT_GET_POLAR_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), PLOT_TYPE_POLAR, PlotPolarClass))
	typedef struct _PlotPolar PlotPolar;
	typedef struct _PlotPolarClass PlotPolarClass;
	typedef enum
	{
		PLOT_POLAR_ZOOM_OUT = 1 << 0,
		PLOT_POLAR_ZOOM_RDL = 1 << 1,
		PLOT_POLAR_ZOOM_AZM = 1 << 2,
		PLOT_POLAR_ZOOM_SGL = 1 << 3,
		PLOT_POLAR_ZOOM_ZOM = 1 << 4
	} PlotPolarZoom;
	typedef enum
	{
		PLOT_POLAR_DISP_RDN = 1 << 0,
		PLOT_POLAR_DISP_LIN = 1 << 1,
		PLOT_POLAR_DISP_PTS = 1 << 2,
		PLOT_POLAR_DISP_PNT = 1 << 3
	} PlotPolarDisp;
	struct _PlotPolar
	{
		GtkDrawingArea parent;
		GArray *rdata, *thdata; /* radial and azimuthal data values */
		GArray *ind, *sizes; /* indices of first element and number of elements for each trace */
		GArray *rd, *gr, *bl, *al; /* colour and alpha of the plots */
		gchar *rlab, *thlab; /* labels for the radial and azimuthal axis */
		PangoFontDescription *afont, *lfont; /* font descriptions for the tick mark and axis labels */
		gdouble rps, thps; /* radial and azimuthal position of mouse */
		guint ptsize, linew; /* point radii and line width */
		gint zmode; /* zoom mode flags xxxx0b/xxxx1b = zoom in/out,1001xb = zoom radial only, 1010xb = zoom azimuthal only, 1011xb = zoom both, 0xxxxb = cartesian shift mode, x1xxxb = single click zoom */
		gint flagd; /* data displayflags: x01xb = lines only, x10xb = points only, x11x = both, xxx0b/xxx1b = Degrees/Radians 0xxxb/1xxxb = Connect the dots/Polar interpolation */
	};
	struct _PlotPolarClass
	{
		GtkDrawingAreaClass parent_class;
		void (*moved) (PlotPolar *plot);
	};
	gboolean plot_polar_update_scale(GtkWidget *widget, gdouble rn, gdouble rx, gdouble thn, gdouble thx, gdouble tcn, gdouble thc);
	gboolean plot_polar_update_scale_pretty(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx);
	gboolean plot_polar_print_eps(GtkWidget *widget, gchar *fout);
	gboolean plot_polar_print_png(GtkWidget *widget, gchar *fout);
	gboolean plot_polar_print_svg(GtkWidget *widget, gchar *fout);
	GtkWidget *plot_polar_new(void);
	G_END_DECLS
#endif
