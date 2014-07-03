/***************************************************************************
 *            gtkplotlinear.c
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
 *  Sat Dec  4 17:18:14 2010
 *  Copyright  2010  Paul Childs
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
#include <math.h>
#include <cairo-ps.h>
#include <cairo-svg.h>
#include "gtkplot.h"
#include "gtkplotlinear.h"
#include "a11y/gtkplotlinearaccessible.h"
#define GTK_PLOT_LINEAR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_PLOT_TYPE_LINEAR, GtkPlotLinearPrivate))
#define ARP 0.05 /* Proportion of the graph occupied by arrows */
#define IRTR 0.577350269 /* 1/square root 3 */
#define MY_2PI 6.2831853071795864769252867665590057683943387987502
#define WGP 0.08 /* Proportion of the graph the wiggles occupy */
#define WFP 0.01 /* Proportion of wiggle that is flat to axis */
#define WMP 0.045 /* the mean of these */
#define WHP 0.020207259 /* wiggle height proportion */
#define DZE 0.00001 /* divide by zero threshold */
#define NZE -0.00001 /* negative of this */
#define FAC 0.05 /* floating point accuracy check for logarithms etc */
#define NAC 0.95 /* conjugate of this */
#define JT 5 /* major tick length */
#define JTI 6 /* this incremented */
#define NT 3 /* minor tick length */
#define ZS 0.5 /* zoom scale */
#define ZSC 0.5 /* 1 minus this */
#define UZ 2 /* inverse of this */
#define UZC 1 /* this minus 1 */
#define BFL 10 /*buffer length for axes*/
#define BF3 7 /*buffer length -3*/
#define BF4 6 /*buffer length -4*/
typedef enum
{
	GTK_PLOT_LINEAR_BORDERS_LT = 1 << 0,
	GTK_PLOT_LINEAR_BORDERS_RT = 1 << 1,
	GTK_PLOT_LINEAR_BORDERS_DN = 1 << 2,
	GTK_PLOT_LINEAR_BORDERS_UP = 1 << 3
} GtkPlotLinearBorders;
typedef enum
{
	GTK_PLOT_LINEAR_AXES_LW = 1 << 0,
	GTK_PLOT_LINEAR_AXES_RW = 1 << 1,
	GTK_PLOT_LINEAR_AXES_DW = 1 << 2,
	GTK_PLOT_LINEAR_AXES_UW = 1 << 3,
	GTK_PLOT_LINEAR_AXES_LR = 1 << 4,
	GTK_PLOT_LINEAR_AXES_LT = 1 << 5
} GtkPlotLinearAxes;
G_DEFINE_TYPE (GtkPlotLinear, gtk_plot_linear, GTK_TYPE_PLOT);
enum {PROP_0, PROP_BXN, PROP_BXX, PROP_BYN, PROP_BYX, PROP_XTJ, PROP_YTJ, PROP_XTN, PROP_YTN, PROP_FA};
enum {MOVED, LAST_SIGNAL};
static guint gtk_plot_linear_signals[LAST_SIGNAL]={0};
typedef struct _GtkPlotLinearPrivate GtkPlotLinearPrivate;
struct xs {gdouble xmin, ymin, xmax, ymax;};
struct tk {guint xj, yj, xn, yn;};
struct _GtkPlotLinearPrivate {struct xs bounds, rescale; struct tk ticks, range; guint flaga, flagr;};

static void drawz(GtkWidget *widget, cairo_t *cr)
{
	gint xw;
	gdouble dt;
	GtkPlotLinear *plot;

	xw=gtk_widget_get_allocated_width(widget);
	plot=GTK_PLOT_LINEAR(widget);
	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, xw-21.5, 0.5, 10, 10);
	cairo_rectangle(cr, xw-10.5, 0.5, 10, 10);
	cairo_move_to(cr, xw-9, 5.5);
	cairo_line_to(cr, xw-2, 5.5);
	if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_DRG)!=0)
	{
		cairo_move_to(cr, xw-2.5, 4.5);
		cairo_line_to(cr, xw-2, 5.5);
		cairo_line_to(cr, xw-2.5, 6.5);
	}
	else
	{
		cairo_move_to(cr, xw-5.5, 2);
		cairo_line_to(cr, xw-5.5, 9);
		if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_OUT)==0)
		{
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
		}
		else
		{
			cairo_move_to(cr, xw-7.5, 3.5);
			cairo_line_to(cr, xw-3.5, 7.5);
			cairo_move_to(cr, xw-7.5, 7.5);
			cairo_line_to(cr, xw-3.5, 3.5);
		}
	}
	cairo_stroke(cr);
	if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_SGL)==0)
	{
		cairo_save(cr);
		dt=1;
		cairo_set_dash(cr, &dt, 1, 0);
		if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_VRT)!=0)
		{
			cairo_move_to(cr, xw-20, 2.5);
			cairo_line_to(cr, xw-13, 2.5);
			cairo_move_to(cr, xw-20, 8.5);
			cairo_line_to(cr, xw-13, 8.5);
			cairo_stroke(cr);
		}
		if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_HZT)!=0)
		{
			cairo_move_to(cr, xw-19.5, 2);
			cairo_line_to(cr, xw-19.5, 9);
			cairo_move_to(cr, xw-13.5, 2);
			cairo_line_to(cr, xw-13.5, 9);
			cairo_stroke(cr);
		}
		cairo_restore(cr);
	}
}

static void draw(GtkWidget *widget, cairo_t *cr)
{
	cairo_matrix_t mtr2, mtr3;
	gchar *str1=NULL, *str2=".", *str3=NULL;
	gchar lbl[BFL];
	GdkRGBA vv;
	gdouble dt, lr1, lr2, delx, dely;
	gint dtt, ft, hg, j, k, lt, st, tf, to, tn, tnn, tx, tz, wd, xa, xl, xr, xr2, xt, xu, xv, xvn, xw, ya, yl, yr, yr2, yu, yv, yvn, yw;
	GtkPlot *plt;
	GtkPlotLinear *plot;
	GtkPlotLinearPrivate *priv;
	guint lr3;
	PangoLayout *lyt;

	{mtr2.xx=0; mtr2.xy=1; mtr2.yx=-1; mtr2.yy=0; mtr2.x0=0; mtr2.y0=0;}/* initialise */
	{mtr3.xx=0; mtr3.xy=-1; mtr3.yx=1; mtr3.yy=0; mtr3.x0=0; mtr3.y0=0;}
	plot=GTK_PLOT_LINEAR(widget);
	plt=GTK_PLOT(widget);
	{xw=gtk_widget_get_allocated_width(widget); yw=gtk_widget_get_allocated_height(widget);}
	priv=GTK_PLOT_LINEAR_GET_PRIVATE(plot);
	(priv->flaga)&=(GTK_PLOT_LINEAR_AXES_LT|GTK_PLOT_LINEAR_AXES_LR);
	delx=((priv->bounds.xmax)-(priv->bounds.xmin))/(priv->ticks.xj);
	dely=((priv->bounds.ymax)-(priv->bounds.ymin))/(priv->ticks.yj);
	lyt=pango_cairo_create_layout(cr);
	pango_layout_set_font_description(lyt, (plt->lfont));
	str1=g_strconcat((plot->xlab), (plot->ylab), NULL);
	pango_layout_set_text(lyt, str1, -1);
	pango_layout_get_pixel_size(lyt, &wd, &dtt);
	g_free(str1);
	g_object_unref(lyt);
	lyt=pango_cairo_create_layout(cr);
	pango_layout_set_font_description(lyt, (plt->afont));
	str1=g_strdup("27");
	pango_layout_set_text(lyt, str1, -1);
	pango_layout_get_pixel_size(lyt, &wd, &hg);
	dtt+=hg;
	g_free(str1);
	g_object_unref(lyt);
	xr=MIN(xw*ARP,dtt);
	xr2=(xr-2)*IRTR;
	yr=MIN(yw*ARP,dtt);
	yr2=(yr-2)*IRTR;
	dtt+=JTI;
	lyt=pango_cairo_create_layout(cr);
	pango_layout_set_font_description(lyt, (plt->afont));
	if ((priv->bounds.ymax)<=DZE) /* determine positions of axes */
	{
		(priv->flaga)|=GTK_PLOT_LINEAR_AXES_LT;
		lr3=3;
		if ((priv->bounds.ymin)<-1) lr3+=floor(log10(-(priv->bounds.ymin)));
		if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
		if (lr3<BFL) g_snprintf(lbl, lr3, "%f", (priv->bounds.ymin));
		else g_snprintf(lbl, BFL, "%f", (priv->bounds.ymin));
		pango_layout_set_text(lyt, lbl, -1);
		pango_layout_get_pixel_size(lyt, &wd, &hg);
		yl=yw-(wd/2)-1; /* allow space for lower label */
		ya=dtt;
		yu=(yl-ya)*((priv->bounds.ymax)/(priv->bounds.ymin));
		if (yu>(yw*WGP)) {(priv->flaga)|=GTK_PLOT_LINEAR_AXES_DW; yu=yw*WGP;}
		yu+=ya;
	}
	else if ((priv->bounds.ymin)>=NZE)
	{
		yu=yr;
		ya=yw-dtt;
		yl=(ya-yu)*((priv->bounds.ymin)/(priv->bounds.ymax));
		if (yl>yw*WGP) {(priv->flaga)|=GTK_PLOT_LINEAR_AXES_UW; yl=(yw*WGP);}
		yl=-yl;
		yl+=ya;
	}
	else if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_LT)!=0)
	{
		if ((priv->bounds.ymin)+(priv->bounds.ymax)<=0)
		{
			lr3=3;
			if ((priv->bounds.ymin)<-1) lr3+=floor(log10(-(priv->bounds.ymin)));
			if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
			if (lr3<BFL) g_snprintf(lbl, lr3, "%f", (priv->bounds.ymin));
			else g_snprintf(lbl, BFL, "%f", (priv->bounds.ymin));
			pango_layout_set_text(lyt, lbl, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			yl=yw-(wd/2)-1; /* allow space for lower label */
			yu=(yl-dtt)*((priv->bounds.ymax)/(priv->bounds.ymin));
			yu+=dtt;
			if (yu<yr) {yu=yr; ya=((yl*(priv->bounds.ymax))-(yr*(priv->bounds.ymin)))/((priv->bounds.ymax)-(priv->bounds.ymin));}
			else ya=dtt;
		}
		else
		{
			yl=yw;
			yu=yr;
			ya=((yl*(priv->bounds.ymax))-(yu*(priv->bounds.ymin)))/((priv->bounds.ymax)-(priv->bounds.ymin));
			if (ya>(yw-xr2))
			{
				yl*=(yw-xr2)/ya;
				ya=yw-xr2;
			}
		}
	}
	else
	{
		yu=yr;
		ya=yw-dtt;
		yl=(yu-ya)*((priv->bounds.ymin)/(priv->bounds.ymax));
		yl+=ya;
		if (yl>yw)
		{
			yl=yw;
			if ((priv->bounds.ymin)+(priv->bounds.ymax)<=0)
			{
				lr3=3;
				if ((priv->bounds.ymin)<-1) lr3+=floor(log10(-(priv->bounds.ymin)));
				if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
				if (lr3<BFL) g_snprintf(lbl, lr3, "%f", (priv->bounds.ymin));
				else g_snprintf(lbl, BFL, "%f", (priv->bounds.ymin));
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				yl+=wd/2;
				yl--; /* allow space for lower label */
			}
			ya=((yl*(priv->bounds.ymax))-(yu*(priv->bounds.ymin)))/((priv->bounds.ymax)-(priv->bounds.ymin));
		}
	}
	g_object_unref(lyt);
	lyt=pango_cairo_create_layout(cr);
	pango_layout_set_font_description(lyt, (plt->afont));
	if (priv->bounds.xmax<=DZE)
	{
		(priv->flaga)|=GTK_PLOT_LINEAR_AXES_LR;
		lr3=3;
		if ((priv->bounds.xmin)<-1) lr3+=floor(log10(-(priv->bounds.xmin)));
		if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
		if (lr3<BFL) g_snprintf(lbl, lr3, "%f", (priv->bounds.xmin));
		else g_snprintf(lbl, BFL, "%f", (priv->bounds.xmin));
		pango_layout_set_text(lyt, lbl, -1);
		pango_layout_get_pixel_size(lyt, &wd, &hg);
		xl=wd/2; /* allow space for left label */
		xa=xw-dtt;
		xu=(xa-xl)*((priv->bounds.xmax)/(priv->bounds.xmin));
		if (xu>(xw*WGP)) {(priv->flaga)|=GTK_PLOT_LINEAR_AXES_LW; xu=xw*WGP;}
		xu=-xu;
		xu+=xa;
	}
	else if (priv->bounds.xmin>=NZE)
	{
		xu=xw-xr;
		xa=dtt;
		xl=(xu-xa)*((priv->bounds.xmin)/(priv->bounds.xmax));
		if (xl>(xw*WGP)) {(priv->flaga)|=GTK_PLOT_LINEAR_AXES_RW; xl=(xw*WGP);}
		xl+=xa;
	}
	else if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_LR)!=0)
	{
		if ((priv->bounds.xmin)+(priv->bounds.xmax)<=0)
		{
			lr3=3;
			if ((priv->bounds.xmin)<-1) lr3+=floor(log10(-(priv->bounds.xmin)));
			if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
			if (lr3<BFL) g_snprintf(lbl, lr3, "%f", (priv->bounds.xmin));
			else g_snprintf(lbl, BFL, "%f", (priv->bounds.xmin));
			pango_layout_set_text(lyt, lbl, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			xl=wd/2; /* allow space for left label */
			xa=xw-dtt;
			xu=(xa-xl)*((priv->bounds.xmax)/(priv->bounds.xmin));
			xu+=dtt;
			if (xu<xr)
			{
				xu=xw-xr;
				xa=((xl*(priv->bounds.xmax))-(xu*(priv->bounds.xmin)))/((priv->bounds.xmax)-(priv->bounds.xmin));
			}
			else xu=xw-xu;
		}
		else
		{
			xu=xw-xr;
			xa=-xu*((priv->bounds.xmin)/((priv->bounds.xmax)-(priv->bounds.xmin)));
			if (xa<yr2)
			{
				xl=xu*((yr2-xa)/(xu-xa));
				xa=yr2;
			}
			else xl=0;
		}
	}
	else
	{
		xu=xw-xr;
		xa=dtt;
		xl=(xu-xa)*((priv->bounds.xmin)/(priv->bounds.xmax));
		xl+=xa;
		if (xl<0)
		{
			if ((priv->bounds.xmin)+(priv->bounds.xmax)<=0)
			{
				lr3=3;
				if ((priv->bounds.xmin)<-1) lr3+=floor(log10(-(priv->bounds.xmin)));
				if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
				if (lr3<BFL) g_snprintf(lbl, lr3, "%f", (priv->bounds.xmin));
				else g_snprintf(lbl, BFL, "%f", (priv->bounds.xmin));
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				xl=wd/2; /* allow space for left label */
				xa=((xl*(priv->bounds.xmax))-(xu*(priv->bounds.xmin)))/((priv->bounds.xmax)-(priv->bounds.xmin));
			}
			else {xl=0; xa=-xu*((priv->bounds.xmin)/((priv->bounds.xmax)-(priv->bounds.xmin)));}
		}
	}
	g_object_unref(lyt);
	(priv->range.xj)=xl;
	(priv->range.yj)=yu;
	(priv->range.xn)=xu;
	(priv->range.yn)=yl;
	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_set_line_width(cr, 2);
	cairo_move_to(cr, 0, ya);
	if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_LW)!=0) /* draw x wiggles */
	{
		cairo_line_to(cr, xa-(WGP*xw), ya);
		cairo_curve_to(cr, xa-(WMP*xw), ya-(WHP*xw), xa-(WMP*xw), ya+(WHP*xw), xa-(WFP*xw), ya);
	}
	else if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_RW)!=0)
	{
		cairo_line_to(cr, xa+(WFP*xw), ya);
		cairo_curve_to(cr, xa+(WMP*xw), ya-(WHP*xw), xa+(WMP*xw), ya+(WHP*xw), xa+(WGP*xw), ya);
	}
	xr--;
	xr--;
	cairo_line_to(cr, xw, ya); /* draw x axis */
	cairo_line_to(cr, xw-xr, ya-xr2); /* draw x arrow */
	cairo_move_to(cr, xw, ya);
	cairo_line_to(cr, xw-xr, ya+xr2);
	cairo_move_to(cr, xa, yw);
	if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_DW)!=0) /* draw y wiggles */
	{
		cairo_line_to(cr, xa, ya+(WGP*yw));
		cairo_curve_to(cr, xa-(WHP*yw), ya+(WMP*yw), xa+(WHP*yw), ya+(WMP*yw), xa, ya+(WFP*yw));
	}
	else if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_UW)!=0)
	{
		cairo_line_to(cr, xa, ya-(WFP*yw));
		cairo_curve_to(cr, xa-(WHP*yw), ya-(WMP*yw), xa+(WHP*yw), ya-(WMP*yw), xa, ya-(WGP*yw));
	}
	{yr--; yr--;}
	cairo_line_to(cr, xa, 0); /* draw y axis */
	cairo_line_to(cr, xa-yr2, yr); /* draw y arrow */
	cairo_move_to(cr, xa, 0);
	cairo_line_to(cr, xa+yr2, yr);
	cairo_stroke(cr);
	lyt=pango_cairo_create_layout(cr); /* draw ticks, grid and labels */
	pango_layout_set_font_description(lyt, (plt->lfont));
	pango_layout_set_text(lyt, (plot->xlab), -1);
	pango_layout_get_pixel_size(lyt, &wd, &hg);
	dt=5;
	if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_LW)!=0)
	{
		to=xl;
		if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_LT)!=0)
		{
			cairo_move_to(cr, (xu+xl-wd)/2, ya-dtt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_move_to(cr, to, ya);
			cairo_line_to(cr, to, ya-JT);
			cairo_stroke(cr);
			lyt=pango_cairo_create_layout(cr);
			pango_layout_set_font_description(lyt, (plt->afont));
			lr1=round((priv->bounds.xmin)*exp(G_LN10*(plot->xdp)));
			if ((priv->bounds.xmin)>DZE)
			{
				lr1+=0.1;
				lr1*=exp(-G_LN10*(plot->xdp));
				if (lr1>=1)
				{
					{lr2=log10(lr1); lr3=2;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else if ((priv->bounds.xmin)<NZE)
			{
				lr1-=0.1;
				lr1*=exp(-G_LN10*(plot->xdp));
				if (lr1<=-1)
				{
					{lr2=log10(-lr1); lr3=3;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else g_snprintf(lbl, 2, "%f", 0.0);
			pango_layout_set_text(lyt, lbl, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			cairo_move_to(cr, to-(wd/2), ya-JTI-hg);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, to, yu);
			cairo_line_to(cr, to, yl);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<=(priv->ticks.xj); j++)
			{
				tn=xl+(((xu-xl)*j)/(priv->ticks.xj));
				for (k=1; k<(priv->ticks.xn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.xn));
					cairo_move_to(cr, tnn, ya);
					cairo_line_to(cr, tnn, ya-NT);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, tn, ya);
				cairo_line_to(cr, tn, ya-JT);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.xmin)+(j*delx);
				lr1=round(lr2*exp(G_LN10*(plot->xdp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, tn-(wd/2), ya-JTI-hg);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, tn, yu);
				cairo_line_to(cr, tn, yl);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
		}
		else
		{
			cairo_move_to(cr, (xu+xl-wd)/2, ya+dtt-hg);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_move_to(cr, to, ya);
			cairo_line_to(cr, to, ya+JT);
			cairo_stroke(cr);
			lyt=pango_cairo_create_layout(cr);
			pango_layout_set_font_description(lyt, (plt->afont));
			lr1=round((priv->bounds.xmin)*exp(G_LN10*(plot->xdp)));
			if ((priv->bounds.xmin)>DZE)
			{
				lr1+=0.1;
				lr1*=exp(-G_LN10*(plot->xdp));
				if (lr1>=1)
				{
					{lr2=log10(lr1); lr3=2;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else if ((priv->bounds.xmin)<NZE)
			{
				lr1-=0.1;
				lr1*=exp(-G_LN10*(plot->xdp));
				if (lr1<=-1)
				{
					{lr2=log10(-lr1); lr3=3;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else g_snprintf(lbl, 2, "%f", 0.0);
			pango_layout_set_text(lyt, lbl, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			cairo_move_to(cr, to-(wd/2), ya+JTI);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, to, yu);
			cairo_line_to(cr, to, yl);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<=(priv->ticks.xj); j++)
			{
				tn=xl+(((xu-xl)*j)/(priv->ticks.xj));
				for (k=1; k<(priv->ticks.xn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.xn));
					cairo_move_to(cr, tnn, ya);
					cairo_line_to(cr, tnn, ya+NT);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, tn, ya);
				cairo_line_to(cr, tn, ya+JT);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.xmin)+(j*delx);
				lr1=round(lr2*exp(G_LN10*(plot->xdp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, tn-(wd/2), ya+JTI);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, tn, yu);
				cairo_line_to(cr, tn, yl);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
		}
	}
	else if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_RW)!=0)
	{
		to=xl;
		if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_LT)!=0)
		{
			cairo_move_to(cr, (xu+xl-wd)/2, ya-dtt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_move_to(cr, to, ya);
			cairo_line_to(cr, to, ya-JT);
			cairo_stroke(cr);
			lyt=pango_cairo_create_layout(cr);
			pango_layout_set_font_description(lyt, (plt->afont));
			lr1=round((priv->bounds.xmin)*exp(G_LN10*(plot->xdp)));
			if ((priv->bounds.xmin)>DZE)
			{
				lr1+=0.1;
				lr1*=exp(-G_LN10*(plot->xdp));
				if (lr1>=1)
				{
					{lr2=log10(lr1); lr3=2;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else if ((priv->bounds.xmin)<NZE)
			{
				lr1-=0.1;
				lr1*=exp(-G_LN10*(plot->xdp));
				if (lr1<=-1)
				{
					{lr2=log10(-lr1); lr3=3;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else g_snprintf(lbl, 2, "%f", 0.0);
			pango_layout_set_text(lyt, lbl, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			cairo_move_to(cr, to-(wd/2), ya-JTI-hg);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, to, yu);
			cairo_line_to(cr, to, yl);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<(priv->ticks.xj); j++)
			{
				tn=xl+(((xu-xl)*j)/(priv->ticks.xj));
				for (k=1; k<(priv->ticks.xn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.xn));
					cairo_move_to(cr, tnn, ya);
					cairo_line_to(cr, tnn, ya-NT);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, tn, ya);
				cairo_line_to(cr, tn, ya-JT);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.xmin)+(j*delx);
				lr1=round(lr2*exp(G_LN10*(plot->xdp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, tn-(wd/2), ya-JTI-hg);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, tn, yu);
				cairo_line_to(cr, tn, yl);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			for (k=1; k<(priv->ticks.xn); k++)
			{
				tnn=to+(((xu-to)*k)/(priv->ticks.xn));
				cairo_move_to(cr, tnn, ya);
				cairo_line_to(cr, tnn, ya-NT);
			}
			cairo_stroke(cr);
			cairo_set_line_width(cr, 2);
			cairo_move_to(cr, xu, ya);
			cairo_line_to(cr, xu, ya-JT);
			cairo_stroke(cr);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, xu, yu);
			cairo_line_to(cr, xu, yl);
			cairo_stroke(cr);
			cairo_restore(cr);
		}
		else
		{
			cairo_move_to(cr, (xu+xl-wd)/2, ya+dtt-hg);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_move_to(cr, to, ya);
			cairo_line_to(cr, to, ya+JT);
			cairo_stroke(cr);
			lyt=pango_cairo_create_layout(cr);
			pango_layout_set_font_description(lyt, (plt->afont));
			lr1=round((priv->bounds.xmin)*exp(G_LN10*(plot->xdp)));
			if ((priv->bounds.xmin)>DZE)
			{
				lr1+=0.1;
				lr1*=exp(-G_LN10*(plot->xdp));
				if (lr1>=1)
				{
					{lr2=log10(lr1); lr3=2;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else if ((priv->bounds.xmin)<NZE)
			{
				lr1-=0.1;
				lr1*=exp(-G_LN10*(plot->xdp));
				if (lr1<=-1)
				{
					{lr2=log10(-lr1); lr3=3;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else g_snprintf(lbl, 2, "%f", 0.0);
			pango_layout_set_text(lyt, lbl, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			cairo_move_to(cr, to-(wd/2), ya+JTI);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, to, yu);
			cairo_line_to(cr, to, yl);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<(priv->ticks.xj); j++)
			{
				tn=xl+(((xu-xl)*j)/(priv->ticks.xj));
				for (k=1; k<(priv->ticks.xn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.xn));
					cairo_move_to(cr, tnn, ya);
					cairo_line_to(cr, tnn, ya+NT);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, tn, ya);
				cairo_line_to(cr, tn, ya+JT);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.xmin)+(j*delx);
				lr1=round(lr2*exp(G_LN10*(plot->xdp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, tn-(wd/2), ya+JTI);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, tn, yu);
				cairo_line_to(cr, tn, yl);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			for (k=1; k<(priv->ticks.xn); k++)
			{
				tnn=to+(((xu-to)*k)/(priv->ticks.xn));
				cairo_move_to(cr, tnn, ya);
				cairo_line_to(cr, tnn, ya+NT);
			}
			cairo_stroke(cr);
			cairo_set_line_width(cr, 2);
			cairo_move_to(cr, xu, ya);
			cairo_line_to(cr, xu, ya+JT);
			cairo_stroke(cr);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, xu, yu);
			cairo_line_to(cr, xu, yl);
			cairo_stroke(cr);
			cairo_restore(cr);
		}
	}
	else if ((xu+xl)>=(2*xa))
	{
		to=xu;
		if (xu==xl) {tz=(priv->ticks.xj); tf=xu;}
		else
		{
			tz=((xu+1-xa)*(priv->ticks.xj))/(xu-xl);
			if (tz==0) tf=xu;
			else tf=xu-(((xu-xa)*(priv->ticks.xj))/tz);
		}
		if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_LT)!=0)
		{
			cairo_move_to(cr, (xu+xa-wd)/2, ya-dtt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_move_to(cr, to, ya);
			cairo_line_to(cr, to, ya-JT);
			cairo_stroke(cr);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt,1, 0);
			cairo_move_to(cr, to, yu);
			cairo_line_to(cr, to, yl);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<tz; j++)
			{
				tn=xu-(((xu-tf)*j)/(priv->ticks.xj));
				for (k=1; k<(priv->ticks.xn); k++)
				{
					tnn=to-(((to-tn)*k)/(priv->ticks.xn));
					cairo_move_to(cr, tnn, ya);
					cairo_line_to(cr, tnn, ya-NT);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, tn, ya);
				cairo_line_to(cr, tn, ya-JT);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.xmax)-(j*delx*(xu-tf)/(xu-xl));
				lr1=round(lr2*exp(G_LN10*(plot->xdp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, tn-(wd/2), ya-JTI-hg);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, tn, yu);
				cairo_line_to(cr, tn, yl);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tn=xu-(((xu-tf)*tz)/(priv->ticks.xj));
			for (k=1; k<(priv->ticks.xn); k++)
			{
				tnn=to-(((to-tn)*k)/(priv->ticks.xn));
				cairo_move_to(cr, tnn, ya);
				cairo_line_to(cr, tnn, ya-NT);
			}
			cairo_stroke(cr);
			to=tn;
			for (j=(tz+1); j<(priv->ticks.xj); j++)
			{
				tn=xu-(((xu-tf)*j)/(priv->ticks.xj));
				for (k=1; k<(priv->ticks.xn); k++)
				{
					tnn=to-(((to-tn)*k)/(priv->ticks.xn));
					cairo_move_to(cr, tnn, ya);
					cairo_line_to(cr, tnn, ya-NT);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, tn, ya);
				cairo_line_to(cr, tn, ya-JT);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.xmax)-(j*delx*(xu-tf)/(xu-xl));
				lr1=round(lr2*exp(G_LN10*(plot->xdp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, tn-(wd/2), ya-JTI-hg);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, tn, yu);
				cairo_line_to(cr, tn, yl);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tnn=to-(((to-tf)*k)/(priv->ticks.xn));
			for (k=1; (tnn>=xl)&&(k<=(priv->ticks.xn)); k++)
			{
				cairo_move_to(cr, tnn, ya);
				cairo_line_to(cr, tnn, ya-NT);
				tnn=to-(((to-tf)*k)/(priv->ticks.xn));
			}
		}
		else
		{
			cairo_move_to(cr, (xu+xa-wd)/2, ya+dtt-hg);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_move_to(cr, to, ya);
			cairo_line_to(cr, to, ya+JT);
			cairo_stroke(cr);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, to, yu);
			cairo_line_to(cr, to, yl);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<tz; j++)
			{
				tn=xu-(((xu-tf)*j)/(priv->ticks.xj));
				for (k=1; k<(priv->ticks.xn); k++)
				{
					tnn=to-(((to-tn)*k)/(priv->ticks.xn));
					cairo_move_to(cr, tnn, ya);
					cairo_line_to(cr, tnn, ya+NT);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, tn, ya);
				cairo_line_to(cr, tn, ya+JT);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.xmax)-(j*delx*(xu-tf)/(xu-xl));
				lr1=round(lr2*exp(G_LN10*(plot->xdp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, tn-(wd/2), ya+JTI);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, tn, yu);
				cairo_line_to(cr, tn, yl);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tn=xu-(((xu-tf)*tz)/(priv->ticks.xj));
			for (k=1; k<(priv->ticks.xn); k++)
			{
				tnn=to-(((to-tn)*k)/(priv->ticks.xn));
				cairo_move_to(cr, tnn, ya);
				cairo_line_to(cr, tnn, ya+NT);
			}
			cairo_stroke(cr);
			to=tn;
			for (j=(tz+1); j<(priv->ticks.xj); j++)
			{
				tn=xu-(((xu-tf)*j)/(priv->ticks.xj));
				for (k=1; k<(priv->ticks.xn); k++)
				{
					tnn=to-(((to-tn)*k)/(priv->ticks.xn));
					cairo_move_to(cr, tnn, ya);
					cairo_line_to(cr, tnn, ya+NT);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, tn, ya);
				cairo_line_to(cr, tn, ya+JT);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.xmax)-(j*delx*(xu-tf)/(xu-xl));
				lr1=round(lr2*exp(G_LN10*(plot->xdp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, tn-(wd/2), ya+JTI);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, tn, yu);
				cairo_line_to(cr, tn, yl);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tnn=to-(((to-tf)*k)/(priv->ticks.xn));
			for (k=1; (tnn>=xl)&&(k<=(priv->ticks.xn)); k++)
			{
				cairo_move_to(cr, tnn, ya);
				cairo_line_to(cr, tnn, ya+NT);
				tnn=to-(((to-tf)*k)/(priv->ticks.xn));
			}
		}
	}
	else
	{
		to=xl;
		tz=((xa+1-xl)*(priv->ticks.xj))/(xu-xl);
		if (tz==0) tf=xl;
		else tf=xl+(((xa-xl)*(priv->ticks.xj))/tz);
		if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_LT)!=0)
		{
			cairo_move_to(cr, (xa+xl-wd)/2, ya-dtt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_move_to(cr, to, ya);
			cairo_line_to(cr, to, ya-JT);
			cairo_stroke(cr);
			lyt=pango_cairo_create_layout(cr);
			pango_layout_set_font_description(lyt, (plt->afont));
			lr1=round((priv->bounds.xmin)*exp(G_LN10*(plot->xdp)));
			if ((priv->bounds.xmin)>DZE)
			{
				lr1+=0.1;
				lr1*=exp(-G_LN10*(plot->xdp));
				if (lr1>=1)
				{
					{lr2=log10(lr1); lr3=2;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else if ((priv->bounds.xmin)<NZE)
			{
				lr1-=0.1;
				lr1*=exp(-G_LN10*(plot->xdp));
				if (lr1<=-1)
				{
					{lr2=log10(-lr1); lr3=3;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else g_snprintf(lbl, 2, "%f", 0.0);
			pango_layout_set_text(lyt, lbl, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			cairo_move_to(cr, to-(wd/2), ya-JTI-hg);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, to, yu);
			cairo_line_to(cr, to, yl);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<tz; j++)
			{
				tn=xl+(((tf-xl)*j)/(priv->ticks.xj));
				for (k=1; k<(priv->ticks.xn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.xn));
					cairo_move_to(cr, tnn, ya);
					cairo_line_to(cr, tnn, ya-NT);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, tn, ya);
				cairo_line_to(cr, tn, ya-JT);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.xmin)+(j*delx*(tf-xl)/(xu-xl));
				lr1=round(lr2*exp(G_LN10*(plot->xdp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, tn-(wd/2), ya-JTI-hg);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, tn, yu);
				cairo_line_to(cr, tn, yl);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tn=xl+(((tf-xl)*tz)/(priv->ticks.xj));
			for (k=1; k<(priv->ticks.xn); k++)
			{
				tnn=to+(((tn-to)*k)/(priv->ticks.xn));
				cairo_move_to(cr, tnn, ya);
				cairo_line_to(cr, tnn, ya-NT);
			}
			cairo_stroke(cr);
			to=tn;
			for (j=(tz+1); j<(priv->ticks.xj); j++)
			{
				tn=xl+(((tf-xl)*j)/(priv->ticks.xj));
				for (k=1; k<(priv->ticks.xn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.xn));
					cairo_move_to(cr, tnn, ya);
					cairo_line_to(cr, tnn, ya-NT);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, tn, ya);
				cairo_line_to(cr, tn, ya-JT);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.xmin)+(j*delx*(tf-xl)/(xu-xl));
				lr1=round(lr2*exp(G_LN10*(plot->xdp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, tn-(wd/2), ya-JTI-hg);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, tn, yu);
				cairo_line_to(cr, tn, yl);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tnn=to+(((tf-to)*k)/(priv->ticks.xn));
			for (k=1; (tnn<=xu)&&(k<=(priv->ticks.xn)); k++)
			{
				cairo_move_to(cr, tnn, ya);
				cairo_line_to(cr, tnn, ya-NT);
				tnn=to+(((tf-to)*k)/(priv->ticks.xn));
			}
		}
		else
		{
			cairo_move_to(cr, (xa+xl-wd)/2, ya+dtt-hg);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_move_to(cr, to, ya);
			cairo_line_to(cr, to, ya+JT);
			cairo_stroke(cr);
			lyt=pango_cairo_create_layout(cr);
			pango_layout_set_font_description(lyt, (plt->afont));
			lr1=round((priv->bounds.xmin)*exp(G_LN10*(plot->xdp)));
			if ((priv->bounds.xmin)>DZE)
			{
				lr1+=0.1;
				lr1*=exp(-G_LN10*(plot->xdp));
				if (lr1>=1)
				{
					{lr2=log10(lr1); lr3=2;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else if ((priv->bounds.xmin)<NZE)
			{
				lr1-=0.1;
				lr1*=exp(-G_LN10*(plot->xdp));
				if (lr1<=-1)
				{
					{lr2=log10(-lr1); lr3=3;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else g_snprintf(lbl, 2, "%f", 0.0);
			pango_layout_set_text(lyt, lbl, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			cairo_move_to(cr, to-(wd/2), ya+JTI);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, to, yu);
			cairo_line_to(cr, to, yl);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<tz; j++)
			{
				tn=xl+(((tf-xl)*j)/(priv->ticks.xj));
				for (k=1; k<(priv->ticks.xn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.xn));
					cairo_move_to(cr, tnn, ya);
					cairo_line_to(cr, tnn, ya+NT);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, tn, ya);
				cairo_line_to(cr, tn, ya+JT);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.xmin)+(j*delx*(tf-xl)/(xu-xl));
				lr1=round(lr2*exp(G_LN10*(plot->xdp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, tn-(wd/2), ya+JTI);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, tn, yu);
				cairo_line_to(cr, tn, yl);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tn=xl+(((tf-xl)*tz)/(priv->ticks.xj));
			for (k=1; k<(priv->ticks.xn); k++)
			{
				tnn=to+(((tn-to)*k)/(priv->ticks.xn));
				cairo_move_to(cr, tnn, ya);
				cairo_line_to(cr, tnn, ya+NT);
			}
			cairo_stroke(cr);
			to=tn;
			for (j=(tz+1); j<(priv->ticks.xj); j++)
			{
				tn=xl+(((tf-xl)*j)/(priv->ticks.xj));
				for (k=1; k<(priv->ticks.xn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.xn));
					cairo_move_to(cr, tnn, ya);
					cairo_line_to(cr, tnn, ya+NT);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, tn, ya);
				cairo_line_to(cr, tn, ya+JT);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.xmin)+(j*delx*(tf-xl)/(xu-xl));
				lr1=round(lr2*exp(G_LN10*(plot->xdp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF3) g_snprintf(lbl, (plot->xdp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->xdp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->xdp)!=0) lr3+=(plot->xdp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->xdp)<BF4) g_snprintf(lbl, (plot->xdp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, tn-(wd/2), ya+JTI);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, tn, yu);
				cairo_line_to(cr, tn, yl);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tnn=to+(((tf-to)*k)/(priv->ticks.xn));
			for (k=1; (tnn<=xu)&&(k<=(priv->ticks.xn)); k++)
			{
				cairo_move_to(cr, tnn, ya);
				cairo_line_to(cr, tnn, ya+NT);
				tnn=to+(((tf-to)*k)/(priv->ticks.xn));
			}
		}
	}
	cairo_stroke(cr);
	lyt=pango_cairo_create_layout(cr);
	pango_layout_set_font_description(lyt, (plt->lfont));
	pango_layout_set_text(lyt, (plot->ylab), -1);
	pango_layout_get_pixel_size(lyt, &wd, &hg);
	if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_DW)!=0)
	{
		to=yu;
		if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_LR)!=0)
		{
			cairo_move_to(cr, xa+dtt, (yl+yu-wd)/2);
			cairo_set_matrix(cr, &mtr3);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			lyt=pango_cairo_create_layout(cr);
			pango_layout_set_font_description(lyt, (plt->afont));
			lr1=round((priv->bounds.ymax)*exp(G_LN10*(plot->ydp)));
			if ((priv->bounds.ymax)>DZE)
			{
				lr1+=0.1;
				lr1*=exp(-G_LN10*(plot->ydp));
				if (lr1>=1)
				{
					{lr2=log10(lr1); lr3=2;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else if ((priv->bounds.ymax)<NZE)
			{
				lr1-=0.1;
				lr1*=exp(-G_LN10*(plot->ydp));
				if (lr1<=-1)
				{
					{lr2=log10(-lr1); lr3=3;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else g_snprintf(lbl, 2, "%f", 0.0);
			pango_layout_set_text(lyt, lbl, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			cairo_move_to(cr, xa+JTI+hg, to-(wd/2));
			cairo_set_matrix(cr, &mtr3);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			cairo_move_to(cr, xa, to);
			cairo_line_to(cr, xa+JT, to);
			cairo_stroke(cr);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, xl, to);
			cairo_line_to(cr, xu, to);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<=(priv->ticks.yj); j++)
			{
				tn=yu+(((yl-yu)*j)/(priv->ticks.yj));
				for (k=1; k<(priv->ticks.yn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.yn));
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa+NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa+JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.ymax)-(j*dely);
				lr1=round(lr2*exp(G_LN10*(plot->ydp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa+JTI+hg, tn-(wd/2));
				cairo_set_matrix(cr, &mtr3);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, xl, tn);
				cairo_line_to(cr, xu, tn);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
		}
		else
		{
			cairo_move_to(cr, xa-dtt, (yl+yu+wd)/2);
			cairo_set_matrix(cr, &mtr2);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			lyt=pango_cairo_create_layout(cr);
			pango_layout_set_font_description(lyt, (plt->afont));
			lr1=round((priv->bounds.ymax)*exp(G_LN10*(plot->ydp)));
			if ((priv->bounds.ymax)>DZE)
			{
				lr1+=0.1;
				lr1*=exp(-G_LN10*(plot->ydp));
				if (lr1>=1)
				{
					{lr2=log10(lr1); lr3=2;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else if ((priv->bounds.ymax)<NZE)
			{
				lr1-=0.1;
				lr1*=exp(-G_LN10*(plot->ydp));
				if (lr1<=-1)
				{
					{lr2=log10(-lr1); lr3=3;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else g_snprintf(lbl, 2, "%f", 0.0);
			pango_layout_set_text(lyt, lbl, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			cairo_move_to(cr, xa-JTI-hg, to+(wd/2));
			cairo_set_matrix(cr, &mtr2);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			cairo_move_to(cr, xa, to);
			cairo_line_to(cr, xa-JT, to);
			cairo_stroke(cr);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, xl, to);
			cairo_line_to(cr, xu, to);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<=(priv->ticks.yj); j++)
			{
				tn=yu+(((yl-yu)*j)/(priv->ticks.yj));
				for (k=1; k<(priv->ticks.yn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.yn));
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa-NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa-JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.ymax)-(j*dely);
				lr1=round(lr2*exp(G_LN10*(plot->ydp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa-JTI-hg, tn+(wd/2));
				cairo_set_matrix(cr, &mtr2);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, xl, tn);
				cairo_line_to(cr, xu, tn);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
		}
	}
	else if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_UW)!=0)
	{
		to=yu;
		if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_LR)!=0)
		{
			cairo_move_to(cr, xa+dtt, (yl+yu-wd)/2);
			cairo_set_matrix(cr, &mtr3);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			cairo_move_to(cr, xa, to);
			cairo_line_to(cr, xa+JT, to);
			cairo_stroke(cr);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, xl, to);
			cairo_line_to(cr, xu, to);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<=(priv->ticks.yj); j++)
			{
				tn=yu+(((yl-yu)*j)/(priv->ticks.yj));
				for (k=1; k<(priv->ticks.yn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.yn));
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa+NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa+JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.ymax)-(j*dely);
				lr1=round(lr2*exp(G_LN10*(plot->ydp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa+JTI+hg, tn-(wd/2));
				cairo_set_matrix(cr, &mtr3);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, xl, tn);
				cairo_line_to(cr, xu, tn);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
		}
		else
		{
			cairo_move_to(cr, xa-dtt, (yl+yu+wd)/2);
			cairo_set_matrix(cr, &mtr2);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			cairo_move_to(cr, xa, to);
			cairo_line_to(cr, xa-JT, to);
			cairo_stroke(cr);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, xl, to);
			cairo_line_to(cr, xu, to);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<=(priv->ticks.yj); j++)
			{
				tn=yu+(((yl-yu)*j)/(priv->ticks.yj));
				for (k=1; k<(priv->ticks.yn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.yn));
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa-NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa-JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.ymax)-(j*dely);
				lr1=round(lr2*exp(G_LN10*(plot->ydp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa-JTI-hg, tn+(wd/2));
				cairo_set_matrix(cr, &mtr2);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, xl, tn);
				cairo_line_to(cr, xu, tn);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
		}
	}
	else if ((yu+yl)<=(2*ya))
	{
		to=yu;
		tz=((ya+1-yu)*(priv->ticks.yj))/(yl-yu);
		if (tz==0) tf=yu;
		else tf=yu+(((ya-yu)*(priv->ticks.yj))/tz);
		if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_LR)!=0)
		{
			cairo_move_to(cr, xa+dtt, (ya+yu-wd)/2);
			cairo_set_matrix(cr, &mtr3);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			cairo_move_to(cr, xa, to);
			cairo_line_to(cr, xa+JT, to);
			cairo_stroke(cr);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, xl, to);
			cairo_line_to(cr, xu, to);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<tz; j++)
			{
				tn=yu+(((yl-yu)*j)/(priv->ticks.yj));
				for (k=1; k<(priv->ticks.yn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.yn));
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa+NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa+JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.ymax)-(j*dely*(tf-yu)/(yl-yu));
				lr1=round(lr2*exp(G_LN10*(plot->ydp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa+JTI+hg, tn-(wd/2));
				cairo_set_matrix(cr, &mtr3);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, xl, tn);
				cairo_line_to(cr, xu, tn);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tn=yu+(((yl-yu)*tz)/(priv->ticks.yj));
			for (k=1; k<(priv->ticks.yn); k++)
			{
				tnn=to+(((tn-to)*k)/(priv->ticks.yn));
				cairo_move_to(cr, xa, tnn);
				cairo_line_to(cr, xa+NT, tnn);
			}
			cairo_stroke(cr);
			to=tn;
			for (j=(tz+1); j<(priv->ticks.yj); j++)
			{
				tn=yu+(((yl-yu)*j)/(priv->ticks.yj));
				for (k=1; k<(priv->ticks.yn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.yn));
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa+NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa+JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.ymax)-(j*dely*(tf-yu)/(yl-yu));
				lr1=round(lr2*exp(G_LN10*(plot->ydp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa+JTI+hg, tn-(wd/2));
				cairo_set_matrix(cr, &mtr3);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, xl, tn);
				cairo_line_to(cr, xu, tn);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tnn=to+(((tf-to)*k)/(priv->ticks.yn));
			for (k=1; (tnn<=yl)&&(k<=(priv->ticks.yn)); k++)
			{
				cairo_move_to(cr, xa, tnn);
				cairo_line_to(cr, xa+NT, tnn);
				tnn=to+(((tf-to)*k)/(priv->ticks.yn));
			}
		}
		else
		{
			cairo_move_to(cr, xa-dtt, (ya+yu+wd)/2);
			cairo_set_matrix(cr, &mtr2);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			cairo_move_to(cr, xa, to);
			cairo_line_to(cr, xa-JT, to);
			cairo_stroke(cr);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, xl, to);
			cairo_line_to(cr, xu, to);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<tz; j++)
			{
				tn=yu+(((yl-yu)*j)/(priv->ticks.yj));
				for (k=1; k<(priv->ticks.yn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.yn));
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa-NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa-JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.ymax)-(j*dely*(tf-yu)/(yl-yu));
				lr1=round(lr2*exp(G_LN10*(plot->ydp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa-JTI-hg, tn+(wd/2));
				cairo_set_matrix(cr, &mtr2);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, xl, tn);
				cairo_line_to(cr, xu, tn);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tn=yu+(((yl-yu)*tz)/(priv->ticks.yj)); /*SIGFPE here?*/
			for (k=1; k<(priv->ticks.yn); k++)
			{
				tnn=to+(((tn-to)*k)/(priv->ticks.yn));
				cairo_move_to(cr, xa, tnn);
				cairo_line_to(cr, xa-NT, tnn);
			}
			cairo_stroke(cr);
			to=tn;
			for (j=(tz+1); j<(priv->ticks.yj); j++)
			{
				tn=yu+(((yl-yu)*j)/(priv->ticks.yj));
				for (k=1; k<(priv->ticks.yn); k++)
				{
					tnn=to+(((tn-to)*k)/(priv->ticks.yn));
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa-NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa-JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.ymax)-(j*dely*(tf-yu)/(yl-yu));
				lr1=round(lr2*exp(G_LN10*(plot->ydp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa-JTI-hg, tn+(wd/2));
				cairo_set_matrix(cr, &mtr2);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, xl, tn);
				cairo_line_to(cr, xu, tn);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tnn=to+(((tf-to)*k)/(priv->ticks.yn));
			for (k=1; (tnn<=yl)&&(k<=(priv->ticks.yn)); k++)
			{
				cairo_move_to(cr, xa, tnn);
				cairo_line_to(cr, xa-NT, tnn);
				tnn=to+(((tf-to)*k)/(priv->ticks.yn));
			}
		}
	}
	else
	{
		to=yl;
		tz=((yl+1-ya)*(priv->ticks.yj))/(yl-yu);
		if (tz==0) tf=yl;
		else tf=yl-(((yl-ya)*(priv->ticks.yj))/tz);
		if (((priv->flaga)&GTK_PLOT_LINEAR_AXES_LR)!=0)
		{
			cairo_move_to(cr, xa+dtt, (yl+ya-wd)/2);
			cairo_set_matrix(cr, &mtr3);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			lyt=pango_cairo_create_layout(cr);
			pango_layout_set_font_description(lyt, (plt->afont));
			lr1=round((priv->bounds.ymin)*exp(G_LN10*(plot->ydp)));
			if ((priv->bounds.ymin)>DZE)
			{
				lr1+=0.1;
				lr1*=exp(-G_LN10*(plot->ydp));
				if (lr1>=1)
				{
					{lr2=log10(lr1); lr3=2;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else if ((priv->bounds.ymin)<NZE)
			{
				lr1-=0.1;
				lr1*=exp(-G_LN10*(plot->ydp));
				if (lr1<=-1)
				{
					{lr2=log10(-lr1); lr3=3;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else g_snprintf(lbl, 2, "%f", 0.0);
			pango_layout_set_text(lyt, lbl, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			cairo_move_to(cr, xa+JTI+hg, to-(wd/2));
			cairo_set_matrix(cr, &mtr3);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			cairo_move_to(cr, xa, to);
			cairo_line_to(cr, xa+JT, to);
			cairo_stroke(cr);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, xl, to);
			cairo_line_to(cr, xu, to);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<tz; j++)
			{
				tn=yl-(((yl-yu)*j)/(priv->ticks.yj));
				for (k=1; k<(priv->ticks.yn); k++)
				{
					tnn=to-(((to-tn)*k)/(priv->ticks.yn));
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa+NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa+JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.ymin)+(j*dely*(yl-tf)/(yl-yu));
				lr1=round(lr2*exp(G_LN10*(plot->ydp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa+JTI+hg, tn-(wd/2));
				cairo_set_matrix(cr, &mtr3);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, xl, tn);
				cairo_line_to(cr, xu, tn);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tn=yl-(((yl-yu)*tz)/(priv->ticks.yj));
			for (k=1; k<(priv->ticks.yn); k++)
			{
				tnn=to-(((to-tn)*k)/(priv->ticks.yn));
				cairo_move_to(cr, xa, tnn);
				cairo_line_to(cr, xa+NT, tnn);
			}
			cairo_stroke(cr);
			to=tn;
			for (j=(tz+1); j<(priv->ticks.yj); j++)
			{
				tn=yl-(((yl-yu)*j)/(priv->ticks.yj));
				for (k=1; k<(priv->ticks.yn); k++)
				{
					tnn=to-(((to-tn)*k)/(priv->ticks.yn));
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa+NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa+JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.ymin)+(j*dely*(yl-tf)/(yl-yu));
				lr1=round(lr2*exp(G_LN10*(plot->ydp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa+JTI+hg, tn-(wd/2));
				cairo_set_matrix(cr, &mtr3);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, xl, tn);
				cairo_line_to(cr, xu, tn);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tnn=to-(((to-tf)*k)/(priv->ticks.yn));
			for (k=1; (tnn>=yu)&&(k<=(priv->ticks.yn)); k++)
			{
				cairo_move_to(cr, xa, tnn);
				cairo_line_to(cr, xa+NT, tnn);
				tnn=to-(((to-tf)*k)/(priv->ticks.yn));
			}
		}
		else
		{
			cairo_move_to(cr, xa-dtt, (yl+ya+wd)/2);
			cairo_set_matrix(cr, &mtr2);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			lyt=pango_cairo_create_layout(cr);
			pango_layout_set_font_description(lyt, (plt->afont));
			lr1=round((priv->bounds.ymin)*exp(G_LN10*(plot->ydp)));
			if ((priv->bounds.ymin)>DZE)
			{
				lr1+=0.1;
				lr1*=exp(-G_LN10*(plot->ydp));
				if (lr1>=1)
				{
					{lr2=log10(lr1); lr3=2;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else if ((priv->bounds.ymin)<NZE)
			{
				lr1-=0.1;
				lr1*=exp(-G_LN10*(plot->ydp));
				if (lr1<=-1)
				{
					{lr2=log10(-lr1); lr3=3;}
					if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
					else lr3+=(guint)ceil(lr2);
					if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			}
			else g_snprintf(lbl, 2, "%f", 0.0);
			pango_layout_set_text(lyt, lbl, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			cairo_move_to(cr, xa-JTI-hg, to+(wd/2));
			cairo_set_matrix(cr, &mtr2);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			cairo_move_to(cr, xa, to);
			cairo_line_to(cr, xa-JT, to);
			cairo_stroke(cr);
			cairo_set_line_width(cr, 1);
			cairo_save(cr);
			cairo_set_dash(cr, &dt, 1, 0);
			cairo_move_to(cr, xl, to);
			cairo_line_to(cr, xu, to);
			cairo_stroke(cr);
			cairo_restore(cr);
			for (j=1; j<tz; j++)
			{
				tn=yl-(((yl-yu)*j)/(priv->ticks.yj));
				for (k=1; k<(priv->ticks.yn); k++)
				{
					tnn=to-(((to-tn)*k)/(priv->ticks.yn));
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa-NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa-JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.ymin)+(j*dely*(yl-tf)/(yl-yu));
				lr1=round(lr2*exp(G_LN10*(plot->ydp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa-JTI-hg, tn-(wd/2));
				cairo_set_matrix(cr, &mtr2);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, xl, tn);
				cairo_line_to(cr, xu, tn);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tn=yl-(((yl-yu)*tz)/(priv->ticks.yj));
			for (k=1; k<(priv->ticks.yn); k++)
			{
				tnn=to-(((to-tn)*k)/(priv->ticks.yn));
				cairo_move_to(cr, xa, tnn);
				cairo_line_to(cr, xa-NT, tnn);
			}
			cairo_stroke(cr);
			to=tn;
			for (j=(tz+1); j<(priv->ticks.yj); j++)
			{
				tn=yl-(((yl-yu)*j)/(priv->ticks.yj));
				for (k=1; k<(priv->ticks.yn); k++)
				{
					tnn=to-(((to-tn)*k)/(priv->ticks.yn));
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa-NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa-JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				lr2=(priv->bounds.ymin)+(j*dely*(yl-tf)/(yl-yu));
				lr1=round(lr2*exp(G_LN10*(plot->ydp)));
				if (lr2>DZE)
				{
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF3) g_snprintf(lbl, (plot->ydp)+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else if (lr2<NZE)
				{
					lr1-=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint)lr2;
						else lr3+=(guint)ceil(lr2);
						if ((plot->ydp)!=0) lr3+=(plot->ydp)+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if ((plot->ydp)<BF4) g_snprintf(lbl, (plot->ydp)+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa-JTI-hg, tn-(wd/2));
				cairo_set_matrix(cr, &mtr2);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
				cairo_set_line_width(cr, 1);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, xl, tn);
				cairo_line_to(cr, xu, tn);
				cairo_stroke(cr);
				cairo_restore(cr);
				to=tn;
			}
			tnn=to-(((to-tf)*k)/(priv->ticks.yn));
			for (k=1; (tnn>=yu)&&(k<=(priv->ticks.yn)); k++)
			{
				cairo_move_to(cr, xa, tnn);
				cairo_line_to(cr, xa-NT, tnn);
				tnn=to-(((to-tf)*k)/(priv->ticks.yn));
			}
		}
	}
	cairo_stroke(cr);
	if ((plot->xdata)&&(plot->ydata)) /* plot data */
	{
		if (((plot->flagd)&GTK_PLOT_LINEAR_DISP_LIN)!=0)
		{
			cairo_set_line_width(cr, (plot->linew));
			if (((plot->flagd)&GTK_PLOT_LINEAR_DISP_PTS)!=0) /* lines and points */
			{
				for (k=0; k<(plt->ind->len); k++)
				{
					ft=fmod(k,(plt->cl->len));
					vv=g_array_index((plt->cl), GdkRGBA, ft);
					cairo_set_source_rgba(cr, (vv.red), (vv.green), (vv.blue), (vv.alpha));
					ft=g_array_index((plt->ind), gint, k);
					if (ft>=(plot->ydata->len)) break;
					st=g_array_index((plt->stride), gint, k);
					lt=(g_array_index((plt->sizes), gint, k)*st)+ft;
					if (lt>(plot->ydata->len)) lt=(plot->ydata->len);
					xv=xl+((xu-xl)*(g_array_index((plot->xdata), gdouble, ft)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
					yv=yl+((yu-yl)*(g_array_index((plot->ydata), gdouble, ft)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
					if (xv<xl)
					{
						if (yv>yl) xt=(GTK_PLOT_LINEAR_BORDERS_LT|GTK_PLOT_LINEAR_BORDERS_DN);
						else if (yv<yu) xt=(GTK_PLOT_LINEAR_BORDERS_LT|GTK_PLOT_LINEAR_BORDERS_UP);
						else xt=GTK_PLOT_LINEAR_BORDERS_LT;
					}
					else if (xv>xu)
					{
						if (yv>yl) xt=(GTK_PLOT_LINEAR_BORDERS_RT|GTK_PLOT_LINEAR_BORDERS_DN);
						else if (yv<yu) xt=(GTK_PLOT_LINEAR_BORDERS_RT|GTK_PLOT_LINEAR_BORDERS_DN);
						else xt=GTK_PLOT_LINEAR_BORDERS_RT;
					}
					else if (yv>yl)xt=GTK_PLOT_LINEAR_BORDERS_DN;
					else if (yv<yu) xt=GTK_PLOT_LINEAR_BORDERS_UP;
					else
					{
						xt=0;
						cairo_arc(cr, xv, yv, (plot->ptsize), 0, MY_2PI);
						cairo_fill(cr);
						cairo_move_to(cr, xv, yv);
					}
					for (j=st+ft; j<lt; j+=st)
					{
						xvn=xl+((xu-xl)*(g_array_index((plot->xdata), gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
						yvn=yl+((yu-yl)*(g_array_index((plot->ydata), gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
						if (xvn<xl)
						{
							if (yvn>yl)
							{
								if (xt==0)
								{
									tx=((yvn-yv)*(xl-xvn))/(xvn-xv);
									tx+=yvn;
									if (tx>yl)
									{
										tx=((xvn-xv)*(yl-yvn))/(yvn-yv);
										tx+=xvn;
										cairo_line_to(cr, tx, yl);
									}
									else cairo_line_to(cr, xl, tx);
									cairo_stroke(cr);
								}
								xt=(GTK_PLOT_LINEAR_BORDERS_LT|GTK_PLOT_LINEAR_BORDERS_DN);
							}
							else if (yvn<yu)
							{
								if (xt==0)
								{
									tx=((yvn-yv)*(xl-xvn))/(xvn-xv);
									tx+=yvn;
									if (tx<yu)
									{
										tx=((xvn-xv)*(yu-yvn))/(yvn-yv);
										tx+=xvn;
										cairo_line_to(cr, tx, yu);
									}
									else cairo_line_to(cr, xl, tx);
									cairo_stroke(cr);
								}
								xt=(GTK_PLOT_LINEAR_BORDERS_LT|GTK_PLOT_LINEAR_BORDERS_UP);
							}
							else if (xt==0)
							{
								tx=((yvn-yv)*(xl-xvn))/(xvn-xv);
								tx+=yvn;
								cairo_line_to(cr, xl, tx);
								cairo_stroke(cr);
								xt=GTK_PLOT_LINEAR_BORDERS_LT;
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_DN)
							{
								tx=((yvn-yv)*(xl-xvn))/(xvn-xv);
								tx+=yvn;
								if (tx>yl)
								{
									cairo_move_to(cr, xl, tx);
									tx=((xvn-xv)*(yl-yv))/(yvn-yv);
									tx+=xv;
									cairo_line_to(cr, tx, yl);
									cairo_stroke(cr);
								}
								xt=GTK_PLOT_LINEAR_BORDERS_LT;
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_UP)
							{
								tx=((yvn-yv)*(xl-xvn))/(xvn-xv);
								tx+=yvn;
								if (tx<yu)
								{
									cairo_move_to(cr, xl, tx);
									tx=((xvn-xv)*(yu-yv))/(yvn-yv);
									tx+=xv;
									cairo_line_to(cr, tx, yu);
									cairo_stroke(cr);
								}
								xt=GTK_PLOT_LINEAR_BORDERS_LT;
							}
							else xt=GTK_PLOT_LINEAR_BORDERS_LT;
						}
						else if (xvn>xu)
						{
							if (yvn>yl)
							{
								if (xt==0)
								{
									tx=((yvn-yv)*(xu-xv))/(xvn-xv);
									tx+=yv;
									if (tx>yl)
									{
										tx=((xvn-xv)*(yl-yv))/(yvn-yv);
										tx+=xv;
										cairo_line_to(cr, tx, yl);
									}
									else cairo_line_to(cr, xu, tx);
									cairo_stroke(cr);
								}
								xt=(GTK_PLOT_LINEAR_BORDERS_RT|GTK_PLOT_LINEAR_BORDERS_DN);
							}
							else if (yvn<yu)
							{
								if (xt==0)
								{
									tx=((yvn-yv)*(xu-xv))/(xvn-xv);
									tx+=yv;
									if (tx<yu)
									{
										tx=((xvn-xv)*(yu-yv))/(yvn-yv);
										tx+=xv;
										cairo_line_to(cr, tx, yu);
									}
									else cairo_line_to(cr, xu, tx);
									cairo_stroke(cr);
								}
								xt=(GTK_PLOT_LINEAR_BORDERS_RT|GTK_PLOT_LINEAR_BORDERS_UP);
							}
							if (xt==0)
							{
								tx=((yvn-yv)*(xu-xv))/(xvn-xv);
								tx+=yv;
								cairo_line_to(cr, xu, tx);
								cairo_stroke(cr);
								xt=GTK_PLOT_LINEAR_BORDERS_RT;
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_DN)
							{
								tx=((yvn-yv)*(xu-xv))/(xvn-xv);
								tx+=yv;
								if (tx>yl)
								{
									cairo_move_to(cr, xu, tx);
									tx=((xvn-xv)*(yl-yv))/(yvn-yv);
									tx+=xv;
									cairo_line_to(cr, tx, yl);
									cairo_stroke(cr);
								}
								xt=GTK_PLOT_LINEAR_BORDERS_RT;
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_UP)
							{
								tx=((yvn-yv)*(xu-xv))/(xvn-xv);
								tx+=yv;
								if (tx>yu)
								{
									cairo_move_to(cr, xu, tx);
									tx=((xvn-xv)*(yu-yv))/(yvn-yv);
									tx+=xv;
									cairo_line_to(cr, tx, yu);
									cairo_stroke(cr);
								}
								xt=GTK_PLOT_LINEAR_BORDERS_RT;
							}
							else xt=GTK_PLOT_LINEAR_BORDERS_RT;
						}
						else if (yvn>yl)
						{
							if (xt==0)
							{
								tx=((xvn-xv)*(yl-yvn))/(yvn-yv);
								tx+=xvn;
								cairo_line_to(cr, tx, yl);
								cairo_stroke(cr);
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_LT)
							{
								tx=((xvn-xv)*(yl-yvn))/(yvn-yv);
								tx+=xvn;
								if (tx<xl)
								{
									cairo_move_to(cr, tx, yl);
									tx=((yvn-yv)*(xl-xv))/(xvn-xv);
									tx+=yv;
									cairo_line_to(cr, xl, tx);
									cairo_stroke(cr);
								}
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_RT)
							{
								tx=((xvn-xv)*(yl-yvn))/(yvn-yv);/* SIGFPE here. change? */
								tx+=xvn;
								if (tx>xu)
								{
									cairo_move_to(cr, tx, yl);
									tx=((yvn-yv)*(xu-xvn))/(xvn-xv);
									tx+=yvn;
									cairo_line_to(cr, xu, tx);
									cairo_stroke(cr);
								}
							}
							xt=GTK_PLOT_LINEAR_BORDERS_DN;
						}
						else if (yvn<yu)
						{
							if (xt==0)
							{
								tx=((xvn-xv)*(yu-yvn))/(yvn-yv);
								tx+=xvn;
								cairo_line_to(cr, tx, yu);	
								cairo_stroke(cr);
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_LT)
							{
								tx=((xvn-xv)*(yu-yvn))/(yvn-yv);
								tx+=xvn;
								if (tx<xl)
								{
									cairo_move_to(cr, tx, yu);
									tx=((yvn-yv)*(xl-xv))/(xvn-xv);
									tx+=yv;
									cairo_line_to(cr, xl, tx);
									cairo_stroke(cr);
								}
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_RT)
							{
								tx=((xvn-xv)*(yu-yvn))/(yvn-yv);
								tx+=xvn;
								if (tx>xu)
								{
									cairo_move_to(cr, tx, yu);
									tx=((yvn-yv)*(xu-xvn))/(xvn-xv);
									tx+=yvn;
									cairo_line_to(cr, xu, tx);
									cairo_stroke(cr);
								}
							}
							xt=GTK_PLOT_LINEAR_BORDERS_UP;
						}
						else /* within range */
						{
							if ((xt&GTK_PLOT_LINEAR_BORDERS_LT)!=0)
							{
								if ((xt&GTK_PLOT_LINEAR_BORDERS_DN)!=0)
								{
									tx=((yvn-yv)*(xl-xv))/(xvn-xv);
									tx+=yv;
									if (tx>yl)
									{
										tx=((xvn-xv)*(yl-yv))/(yvn-yv);
										tx+=xv;
										cairo_move_to(cr, tx, yl);
									}
									else cairo_move_to(cr, xl, tx);
								}
								else if ((xt&GTK_PLOT_LINEAR_BORDERS_UP)!=0)
								{
									tx=((yvn-yv)*(xl-xv))/(xvn-xv);
									tx+=yv;
									if (tx<yu)
									{
										tx=((xvn-xv)*(yu-yv))/(yvn-yv);
										tx+=xv;
										cairo_move_to(cr, tx, yu);
									}
									else cairo_move_to(cr, xl, tx);
								}
								else
									{
									tx=((yvn-yv)*(xl-xv))/(xvn-xv);
									tx+=yv;
									cairo_move_to(cr, xl, tx);
								}
							}
							else if ((xt&GTK_PLOT_LINEAR_BORDERS_RT)!=0)
							{
								if ((xt&GTK_PLOT_LINEAR_BORDERS_DN)!=0)
								{
									tx=((yvn-yv)*(xu-xvn))/(xvn-xv);
									tx+=yvn;
									if (tx>yl)
									{
										tx=((xvn-xv)*(yl-yvn))/(yvn-yv);
										tx+=xvn;
										cairo_move_to(cr, tx, yl);
									}
									else cairo_move_to(cr, xl, tx);
								}
								else if ((xt&GTK_PLOT_LINEAR_BORDERS_DN)!=0)
								{
									tx=((yvn-yv)*(xu-xvn))/(xvn-xv);
									tx+=yvn;
									if (tx<yu)
									{
										tx=((xvn-xv)*(yu-yvn))/(yvn-yv);
										tx+=xvn;
										cairo_move_to(cr, tx, yu);
									}
									else cairo_move_to(cr, xl, tx);
								}
								else
								{
									tx=((yvn-yv)*(xu-xvn))/(xvn-xv);
									tx+=yvn;
									cairo_move_to(cr, xu, tx);
								}
							}
							else if ((xt&GTK_PLOT_LINEAR_BORDERS_DN)!=0)
							{
								tx=((xvn-xv)*(yl-yv))/(yvn-yv);
								tx+=xv;
								cairo_move_to(cr, tx, yl);
							}
							else if ((xt&GTK_PLOT_LINEAR_BORDERS_UP)!=0)
							{
								tx=((xvn-xv)*(yu-yv))/(yvn-yv);
								tx+=xv;
								cairo_move_to(cr, tx, yu);
							}
							cairo_line_to(cr, xvn, yvn);
							cairo_stroke(cr);
							cairo_arc(cr, xvn, yvn, (plot->ptsize), 0, MY_2PI);
							cairo_fill(cr);
							cairo_move_to(cr, xvn, yvn);
							xt=0;
						}
						xv=xvn;
						yv=yvn;
					}
				}
			}
			else /* lines only */
			{
				for (k=0;k<(plt->ind->len);k++)
				{
					ft=fmod(k,(plt->cl->len));
					vv=g_array_index((plt->cl), GdkRGBA, ft);
					cairo_set_source_rgba(cr, (vv.red), (vv.green), (vv.blue), (vv.alpha));
					ft=g_array_index((plt->ind), gint, k);
					if (ft>=(plot->ydata->len)) break;
					st=g_array_index((plt->stride), gint, k);
					lt=(g_array_index((plt->sizes), gint, k)*st)+ft;
					if (lt>(plot->ydata->len)) lt=(plot->ydata->len);
					xv=xl+((xu-xl)*(g_array_index((plot->xdata), gdouble, ft)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));/*segfault here when adding a plot*/
					yv=yl+((yu-yl)*(g_array_index((plot->ydata), gdouble, ft)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
					if (xv<xl)
					{
						if (yv>yl) xt=(GTK_PLOT_LINEAR_BORDERS_LT|GTK_PLOT_LINEAR_BORDERS_DN);
						else if (yv<yu) xt=(GTK_PLOT_LINEAR_BORDERS_LT|GTK_PLOT_LINEAR_BORDERS_UP);
						else xt=GTK_PLOT_LINEAR_BORDERS_LT;
					}
					else if (xv>xu)
					{
						if (yv>yl) xt=(GTK_PLOT_LINEAR_BORDERS_RT|GTK_PLOT_LINEAR_BORDERS_DN);
						else if (yv<yu) xt=(GTK_PLOT_LINEAR_BORDERS_RT|GTK_PLOT_LINEAR_BORDERS_UP);
						else xt=GTK_PLOT_LINEAR_BORDERS_RT;
					}
					else if (yv>yl)xt=GTK_PLOT_LINEAR_BORDERS_DN;
					else if (yv<yu) xt=GTK_PLOT_LINEAR_BORDERS_UP;
					else {xt=0; cairo_move_to(cr, xv, yv);}
					for (j=st+ft; j<lt; j+=st)
					{
						xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
						yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
						if (xvn<xl)
						{
							if (yvn>yl)
							{
								if (xt==0)
								{
									tx=xvn-xv;
									if ((tx<DZE)&&(tx>NZE)) cairo_line_to(cr, xvn, yl);
									else
									{
										tx=((yvn-yv)*(xl-xvn))/tx;
										tx+=yvn;
										if (tx>yl)
										{
											tx=((xvn-xv)*(yl-yvn))/(yvn-yv);
											tx+=xvn;
											cairo_line_to(cr, tx, yl);
										}
										else cairo_line_to(cr, xl, tx);
									}
									cairo_stroke(cr);
								}
								xt=(GTK_PLOT_LINEAR_BORDERS_LT|GTK_PLOT_LINEAR_BORDERS_DN);
							}
							else if (yvn<yu)
							{
								if (xt==0)
								{
									tx=xvn-xv;
									if ((tx<DZE)&&(tx>NZE)) cairo_line_to(cr, xvn, yu);
									else
									{
										tx=((yvn-yv)*(xl-xvn))/tx;
										tx+=yvn;
										if (tx<yu)
										{
											tx=((xvn-xv)*(yu-yvn))/(yvn-yv);
											tx+=xvn;
											cairo_line_to(cr, tx, yu);
										}
										else cairo_line_to(cr, xl, tx);
									}
									cairo_stroke(cr);
								}
								xt=(GTK_PLOT_LINEAR_BORDERS_LT|GTK_PLOT_LINEAR_BORDERS_UP);
							}
							else if (xt==0)
							{
								tx=xvn-xv;
								if ((tx<DZE)&&(tx>NZE)) cairo_line_to(cr, xl, yvn);
								else
								{
									tx=((yvn-yv)*(xl-xvn))/tx;
									tx+=yvn;
									cairo_line_to(cr, xl, tx);
								}
								cairo_stroke(cr);
								xt=GTK_PLOT_LINEAR_BORDERS_LT;
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_DN)
							{
								tx=((yvn-yv)*(xl-xvn))/(xvn-xv);
								tx+=yvn;
								if (tx>yl)
								{
									cairo_move_to(cr, xl, tx);
									tx=((xvn-xv)*(yl-yv))/(yvn-yv);
									tx+=xv;
									cairo_line_to(cr, tx, yl);
									cairo_stroke(cr);
								}
								xt=GTK_PLOT_LINEAR_BORDERS_LT;
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_UP)
							{
								tx=((yvn-yv)*(xl-xvn))/(xvn-xv);
								tx+=yvn;
								if (tx<yu)
								{
									cairo_move_to(cr, xl, tx);
									tx=((xvn-xv)*(yu-yv))/(yvn-yv);
									tx+=xv;
									cairo_line_to(cr, tx, yu);
									cairo_stroke(cr);
								}
								xt=GTK_PLOT_LINEAR_BORDERS_LT;
							}
							else xt=GTK_PLOT_LINEAR_BORDERS_LT;
						}
						else if (xvn>xu)
						{
							if (yvn>yl)
							{
								if (xt==0)
								{
									tx=((yvn-yv)*(xu-xv))/(xvn-xv);
									tx+=yv;
									if (tx>yl)
									{
										tx=((xvn-xv)*(yl-yv))/(yvn-yv);
										tx+=xv;
										cairo_line_to(cr, tx, yl);
									}
									else cairo_line_to(cr, xu, tx);
									cairo_stroke(cr);
								}
								xt=(GTK_PLOT_LINEAR_BORDERS_RT|GTK_PLOT_LINEAR_BORDERS_DN);
							}
							else if (yvn<yu)
							{
								if (xt==0)
								{
									tx=((yvn-yv)*(xu-xv))/(xvn-xv);
									tx+=yv;
									if (tx<yu)
									{
										tx=((xvn-xv)*(yu-yv))/(yvn-yv);
										tx+=xv;
										cairo_line_to(cr, tx, yu);
									}
									else cairo_line_to(cr, xu, tx);
									cairo_stroke(cr);
								}
								xt=(GTK_PLOT_LINEAR_BORDERS_RT|GTK_PLOT_LINEAR_BORDERS_UP);
							}
							if (xt==0)
							{
								tx=((yvn-yv)*(xu-xv))/(xvn-xv);
								tx+=yv;
								cairo_line_to(cr, xu, tx);
								cairo_stroke(cr);
								xt=GTK_PLOT_LINEAR_BORDERS_RT;
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_DN)
							{
								tx=((yvn-yv)*(xu-xv))/(xvn-xv);
								tx+=yv;
								if (tx>yl)
								{
									cairo_move_to(cr, xu, tx);
									tx=((xvn-xv)*(yl-yv))/(yvn-yv);
									tx+=xv;
									cairo_line_to(cr, tx, yl);
									cairo_stroke(cr);
								}
								xt=GTK_PLOT_LINEAR_BORDERS_RT;
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_UP)
							{
								tx=((yvn-yv)*(xu-xv))/(xvn-xv);
								tx+=yv;
								if (tx>yu)
								{
									cairo_move_to(cr, xu, tx);
									tx=((xvn-xv)*(yu-yv))/(yvn-yv);
									tx+=xv;
									cairo_line_to(cr, tx, yu);
									cairo_stroke(cr);
								}
								xt=GTK_PLOT_LINEAR_BORDERS_RT;
							}
							else xt=GTK_PLOT_LINEAR_BORDERS_RT;
						}
						else if (yvn>yl)
						{
							if (xt==0)
							{
								tx=((xvn-xv)*(yl-yvn))/(yvn-yv);
								tx+=xvn;
								cairo_line_to(cr, tx, yl);
								cairo_stroke(cr);
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_LT)
							{
								tx=((xvn-xv)*(yl-yvn))/(yvn-yv);
								tx+=xvn;
								if (tx<xl)
								{
									cairo_move_to(cr, tx, yl);
									tx=((yvn-yv)*(xl-xv))/(xvn-xv);
									tx+=yv;
									cairo_line_to(cr, xl, tx);
									cairo_stroke(cr);
								}
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_RT)
							{
								tx=((xvn-xv)*(yl-yvn))/(yvn-yv);
								tx+=xvn;
								if (tx>xu)
								{
									cairo_move_to(cr, tx, yl);
									tx=((yvn-yv)*(xu-xvn))/(xvn-xv);
									tx+=yvn;
									cairo_line_to(cr, xu, tx);
									cairo_stroke(cr);
								}
							}
							xt=GTK_PLOT_LINEAR_BORDERS_DN;
						}
						else if (yvn<yu)
						{
							if (xt==0)
							{
								tx=((xvn-xv)*(yu-yvn))/(yvn-yv);
								tx+=xvn;
								cairo_line_to(cr, tx, yu);	
								cairo_stroke(cr);
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_LT)
							{
								tx=((xvn-xv)*(yu-yvn))/(yvn-yv);
								tx+=xvn;
								if (tx<xl)
								{
									cairo_move_to(cr, tx, yu);
									tx=((yvn-yv)*(xl-xv))/(xvn-xv);
									tx+=yv;
									cairo_line_to(cr, xl, tx);
									cairo_stroke(cr);
								}
							}
							else if (xt==GTK_PLOT_LINEAR_BORDERS_RT)
							{
								tx=((xvn-xv)*(yu-yvn))/(yvn-yv);
								tx+=xvn;
								if (tx>xu)
								{
									cairo_move_to(cr, tx, yu);
									tx=((yvn-yv)*(xu-xvn))/(xvn-xv);
									tx+=yvn;
									cairo_line_to(cr, xu, tx);
									cairo_stroke(cr);
								}
							}
							xt=GTK_PLOT_LINEAR_BORDERS_UP;
						}
						else /* within range */
						{
							if ((xt&GTK_PLOT_LINEAR_BORDERS_LT)!=0)
							{
								if ((xt&GTK_PLOT_LINEAR_BORDERS_DN)!=0)
								{
									tx=((yvn-yv)*(xl-xv))/(xvn-xv);
									tx+=yv;
									if (tx>yl)
									{
										tx=((xvn-xv)*(yl-yv))/(yvn-yv);
										tx+=xv;
										cairo_move_to(cr, tx, yl);
									}
									else cairo_move_to(cr, xl, tx);
								}
								else if ((xt&GTK_PLOT_LINEAR_BORDERS_UP)!=0)
								{
									tx=((yvn-yv)*(xl-xv))/(xvn-xv);
									tx+=yv;
									if (tx<yu)
									{
										tx=((xvn-xv)*(yu-yv))/(yvn-yv);
										tx+=xv;
										cairo_move_to(cr, tx, yu);
									}
									else cairo_move_to(cr, xl, tx);
								}
								else
								{
									tx=((yvn-yv)*(xl-xv))/(xvn-xv);
									tx+=yv;
									cairo_move_to(cr, xl, tx);
								}
							}
							else if ((xt&GTK_PLOT_LINEAR_BORDERS_RT)!=0)
							{
								if ((xt&GTK_PLOT_LINEAR_BORDERS_DN)!=0)
								{
									tx=((yvn-yv)*(xu-xvn))/(xvn-xv);
									tx+=yvn;
									if (tx>yl)
									{
										tx=((xvn-xv)*(yl-yvn))/(yvn-yv);
										tx+=xvn;
										cairo_move_to(cr, tx, yl);
									}
									else cairo_move_to(cr, xl, tx);
								}
								else if ((xt&GTK_PLOT_LINEAR_BORDERS_UP)!=0)
								{
									tx=((yvn-yv)*(xu-xvn))/(xvn-xv);
									tx+=yvn;
									if (tx<yu)
									{
										tx=((xvn-xv)*(yu-yvn))/(yvn-yv);
										tx+=xvn;
										cairo_move_to(cr, tx, yu);
									}
									else cairo_move_to(cr, xl, tx);
								}
								else
								{
									tx=((yvn-yv)*(xu-xvn))/(xvn-xv);
									tx+=yvn;
									cairo_move_to(cr, xu, tx);
								}
							}
							else if ((xt&GTK_PLOT_LINEAR_BORDERS_DN)!=0)
							{
								tx=((xvn-xv)*(yl-yv))/(yvn-yv);
								tx+=xv;
								cairo_move_to(cr, tx, yl);
							}
							else if ((xt&GTK_PLOT_LINEAR_BORDERS_UP)!=0)
							{
								tx=((xvn-xv)*(yu-yv))/(yvn-yv);
								tx+=xv;
								cairo_move_to(cr, tx, yu);
							}
							cairo_line_to(cr, xvn, yvn);
							xt=0;
						}
						xv=xvn;
						yv=yvn;
					}
					cairo_stroke(cr);
				}
			}
		}
		else if (((plot->flagd)&GTK_PLOT_LINEAR_DISP_PTS)!=0) /* points only */
		{
			for (k=0;k<(plt->ind->len);k++)
			{
				ft=fmod(k,(plt->cl->len));
				vv=g_array_index((plt->cl), GdkRGBA, ft);
				cairo_set_source_rgba(cr, (vv.red), (vv.green), (vv.blue), (vv.alpha));
				ft=g_array_index((plt->ind), gint, k);
				if (ft>=(plot->ydata->len)) break;
				st=g_array_index((plt->stride), gint, k);
				lt=(g_array_index((plt->sizes), gint, k)*st)+ft;
				if (lt>(plot->ydata->len)) lt=(plot->ydata->len);
				xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, ft)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
				yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, ft)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
				if ((yv<=yl)&&(yv>=yu)&&(xv>=xl)&&(xv<=xu))
				{
					cairo_arc(cr, xv, yv, (plot->ptsize), 0, MY_2PI);
					cairo_fill(cr);
				}
				for (j=st+ft; j<lt; j+=st)
				{
					xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
					yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
					if ((yv<=yl)&&(yv>=yu)&&(xv>=xl)&&(xv<=xu))
					{
						cairo_arc(cr, xv, yv, (plot->ptsize), 0, MY_2PI);
						cairo_fill(cr);
					}
				}
			}
		}
	}
}

static void gtk_plot_linear_redraw(GtkWidget *widget)
{
	cairo_region_t *region;
	GdkWindow *wdw;

	wdw=gtk_widget_get_window(widget);
	if (!wdw) return;
	region=gdk_window_get_clip_region(wdw);
	gdk_window_invalidate_region(wdw, region, TRUE);
	gdk_window_process_updates(wdw, TRUE);
	cairo_region_destroy(region);
}

gboolean gtk_plot_linear_update_scale(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx)
{
	GtkPlotLinearPrivate *priv;

	priv=GTK_PLOT_LINEAR_GET_PRIVATE(widget);
	(priv->bounds.xmin)=xn;
	(priv->bounds.xmax)=xx;
	(priv->bounds.ymin)=yn;
	(priv->bounds.ymax)=yx;
	gtk_plot_linear_redraw(widget);
	return FALSE;
}

gboolean gtk_plot_linear_update_scale_pretty(GtkWidget *widget, gdouble xl, gdouble xu, gdouble yl, gdouble yu)
{
	GtkPlotLinearPrivate *priv;
	GtkPlotLinear *plot;
	gdouble num, num3, xn, xx, yn, yx;
	gint num2, lt, ut, tk;

	if (xl>xu) {xn=xu; xx=xl;}
	else {xn=xl; xx=xu;}
	if (yl>yu) {yn=yu; yx=yl;}
	else {yn=yl; yx=yu;}
	plot=GTK_PLOT_LINEAR(widget);
	priv=GTK_PLOT_LINEAR_GET_PRIVATE(widget);
	num3=(xx-xn)/6;
	num=log10(num3);
	if (num>=0)
	{
		num2=(gint)num;
		num=fmod(num,1);
	}
	else
	{
		num2=(gint)floor(num);
		num=fmod(num,1);
		num++;
	}
	if (num==0)
	{
		lt=(gint)floor(xn/num3);
		ut=(gint)ceil(xx/num3);
		tk=(ut-lt);
		if (tk>6)
		{
			num3*=2;
			lt=(gint)floor(xn/num3);
			ut=(gint)ceil(xx/num3);
			tk=(ut-lt);
		}
		(priv->bounds.xmin)=(num3*(gdouble)lt);
		(priv->bounds.xmax)=(num3*(gdouble)ut);
		if (tk<=0) (priv->ticks.xj)=1;
		else (priv->ticks.xj)=tk;
	}
	else if (num<0.301029997)
	{
		num=exp(G_LN10*num2);
		num3=2*num;
		lt=(gint)floor(xn/num3);
		ut=(gint)ceil(xx/num3);
		tk=(ut-lt);
		if (tk>6)
		{
			num3=5*num;
			lt=(gint)floor(xn/num3);
			ut=(gint)ceil(xx/num3);
			tk=(ut-lt);
			if (tk>6)
			{
				num3*=2;
				lt=(gint)floor(xn/num3);
				ut=(gint)ceil(xx/num3);
				tk=(ut-lt);
			}
		}
		(priv->bounds.xmin)=(num3*(gdouble)lt);
		(priv->bounds.xmax)=(num3*(gdouble)ut);
		if (tk<=0) (priv->ticks.xj)=1;
		else (priv->ticks.xj)=tk;
	}
	else if (num<0.698970005)
	{
		num=exp(G_LN10*num2);
		num3=5*num;
		lt=(gint)floor(xn/num3);
		ut=(gint)ceil(xx/num3);
		tk=(ut-lt);
		if (tk>6)
		{
			num3*=2;
			lt=(gint)floor(xn/num3);
			ut=(gint)ceil(xx/num3);
			tk=(ut-lt);
			if (tk>6)
			{
				num3*=2;
				lt=(gint)floor(xn/num3);
				ut=(gint)ceil(xx/num3);
				tk=(ut-lt);
			}
		}
		(priv->bounds.xmin)=(num3*(gdouble)lt);
		(priv->bounds.xmax)=(num3*(gdouble)ut);
		if (tk<=0) (priv->ticks.xj)=1;
		else (priv->ticks.xj)=tk;
	}
	else
	{
		num=G_LN10*(++num2);
		num3=exp(num);
		lt=(gint)floor(xn/num3);
		ut=(gint)ceil(xx/num3);
		tk=(ut-lt);
		if (tk>6)
		{
			num3*=2;
			lt=(gint)floor(xn/num3);
			ut=(gint)ceil(xx/num3);
			tk=(ut-lt);
		}
		(priv->bounds.xmin)=(num3*(gdouble)lt);
		(priv->bounds.xmax)=(num3*(gdouble)ut);
		if (tk<=0) (priv->ticks.xj)=1;
		else (priv->ticks.xj)=tk;
	}
	if (num3<1)
	{
		num=-log10(num3);
		if (fmod(num,1)<FAC) (plot->xdp)=(guint)num;
		else (plot->xdp)=(guint)ceil(num);
	}
	else (plot->xdp)=0;
	num3=(yx-yn)/6;
	num=log10(num3);
	num2=(gint)floor(num);
	num=fmod(num,1);
	if (num==0)
	{
		lt=(gint)floor(yn/num3);
		ut=(gint)ceil(yx/num3);
		tk=(ut-lt);
		if (tk>6)
		{
			num3*=2;
			lt=(gint)floor(yn/num3);
			ut=(gint)ceil(yx/num3);
			tk=(ut-lt);
		}
		(priv->bounds.ymin)=(num3*(gdouble)lt);
		(priv->bounds.ymax)=(num3*(gdouble)ut);
		if (tk<=0) (priv->ticks.yj)=1;
		else (priv->ticks.yj)=tk;
	}
	else if (num<0.301029997)
	{
		num=exp(G_LN10*num2);
		num3=2*num;
		lt=(gint)floor(yn/num3);
		ut=(gint)ceil(yx/num3);
		tk=(ut-lt);
		if (tk>6)
		{
			num3=5*num;
			lt=(gint)floor(yn/num3);
			ut=(gint)ceil(yx/num3);
			tk=(ut-lt);
			if (tk>6)
			{
				num3*=2;
				lt=(gint)floor(yn/num3);
				ut=(gint)ceil(yx/num3);
				tk=(ut-lt);
			}
		}
		(priv->bounds.ymin)=(num3*(gdouble)lt);
		(priv->bounds.ymax)=(num3*(gdouble)ut);
		if (tk<=0) (priv->ticks.yj)=1;
		else (priv->ticks.yj)=tk;
	}
	else if (num<0.698970005)
	{
		num=exp(G_LN10*num2);
		num3=5*num;
		lt=(gint)floor(yn/num3);
		ut=(gint)ceil(yx/num3);
		tk=(ut-lt);
		if (tk>6)
		{
			num3*=2;
			lt=(gint)floor(yn/num3);
			ut=(gint)ceil(yx/num3);
			tk=(ut-lt);
			if (tk>6)
			{
				num3*=2;
				lt=(gint)floor(yn/num3);
				ut=(gint)ceil(yx/num3);
				tk=(ut-lt);
			}
		}
		(priv->bounds.ymin)=(num3*(gdouble)lt);
		(priv->bounds.ymax)=(num3*(gdouble)ut);
		if (tk<=0) (priv->ticks.yj)=1;
		else (priv->ticks.yj)=tk;
	}
	else
	{
		num=G_LN10*(++num2);
		num3=exp(num);
		lt=(gint)floor(yn/num3);
		ut=(gint)ceil(yx/num3);
		tk=(ut-lt);
		if (tk>6)
		{
			num3*=2;
			lt=(gint)floor(yn/num3);
			ut=(gint)ceil(yx/num3);
			tk=(ut-lt);
		}
		(priv->bounds.ymin)=(num3*(gdouble)lt);
		(priv->bounds.ymax)=(num3*(gdouble)ut);
		if (tk<=0) (priv->ticks.yj)=1;
		else (priv->ticks.yj)=tk;
	}
	if (num3<1)
	{
		num=-log10(num3);
		if (fmod(num,1)<FAC) (plot->ydp)=(guint)num;
		else (plot->ydp)=(guint)ceil(num);
	}
	else (plot->ydp)=0;
	gtk_plot_linear_redraw(widget);
	return FALSE;
}

gboolean gtk_plot_linear_print(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer data)
{
	cairo_t* cr=gtk_print_context_get_cairo_context(context);
	draw(GTK_WIDGET(data), cr);
	return FALSE;
}

gboolean gtk_plot_linear_print_eps(GtkWidget *plot, gchar* fout)
{
	cairo_t *cr;
	cairo_surface_t *surface;

	surface=cairo_ps_surface_create(fout, (gdouble) gtk_widget_get_allocated_width(plot), (gdouble) gtk_widget_get_allocated_height(plot));
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

gboolean gtk_plot_linear_print_png(GtkWidget *plot, gchar* fout)
{
	cairo_t *cr;
	cairo_surface_t *surface;

	surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (gdouble) gtk_widget_get_allocated_width(plot), (gdouble) gtk_widget_get_allocated_height(plot));
	cr=cairo_create(surface);
	draw(plot, cr);
	cairo_surface_write_to_png(surface, fout);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	return FALSE;
}

gboolean gtk_plot_linear_print_svg(GtkWidget *plot, gchar* fout)
{
	cairo_t *cr;
	cairo_surface_t *surface;

	surface=cairo_svg_surface_create(fout, (gdouble) gtk_widget_get_allocated_width(plot), (gdouble) gtk_widget_get_allocated_height(plot));
	cr=cairo_create(surface);
	draw(plot, cr);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	return FALSE;
}

static gboolean gtk_plot_linear_button_press(GtkWidget *widget, GdkEventButton *event)
{
	GtkPlotLinearPrivate *priv;
	GtkPlotLinear *plot;
	gint d;

	priv=GTK_PLOT_LINEAR_GET_PRIVATE(widget);
	plot=GTK_PLOT_LINEAR(widget);
	if ((priv->flagr)==0)
	{
		(priv->rescale.xmin)=(priv->bounds.xmin);
		(priv->rescale.ymin)=(priv->bounds.ymin);
		if (((plot->zmode)&(GTK_PLOT_LINEAR_ZOOM_SGL|GTK_PLOT_LINEAR_ZOOM_HZT))!=0)
		{
			if ((event->x)>=(priv->range.xn)) (priv->rescale.xmin)=(priv->bounds.xmax);
			else
			{
				d=(event->x)-(priv->range.xj);
				if (d>0) {(priv->rescale.xmin)+=(((priv->bounds.xmax)-(priv->bounds.xmin))*d)/((priv->range.xn)-(priv->range.xj)); (priv->flagr)=1;}
			}
		}
		if (((plot->zmode)&(GTK_PLOT_LINEAR_ZOOM_SGL|GTK_PLOT_LINEAR_ZOOM_VRT))!=0)
		{
			if ((event->y)<=(priv->range.yj)) (priv->rescale.ymin)=(priv->bounds.ymax);
			else
			{
				d=(priv->range.yn)-(event->y);
				if (d>0) {(priv->rescale.ymin)+=(((priv->bounds.ymax)-(priv->bounds.ymin))*d)/((priv->range.yn)-(priv->range.yj)); (priv->flagr)=1;}
			}
		}
	}
	return FALSE;
}

static gboolean gtk_plot_linear_motion_notify(GtkWidget *widget, GdkEventMotion *event)
{
	GtkPlotLinearPrivate *priv;
	GtkPlotLinear *plot;
	gdouble dx, dy;

	priv=GTK_PLOT_LINEAR_GET_PRIVATE(widget);
	plot=GTK_PLOT_LINEAR(widget);
	dx=((event->x)-(priv->range.xj))/((priv->range.xn)-(priv->range.xj));
	dy=((priv->range.yn)-(event->y))/((priv->range.yn)-(priv->range.yj));
	if ((dx>=0)&&(dy>=0)&&(dx<=1)&&(dy<=1))
	{
		(plot->xps)=((priv->bounds.xmax)*dx)+((priv->bounds.xmin)*(1-dx));
		(plot->yps)=((priv->bounds.ymax)*dy)+((priv->bounds.ymin)*(1-dy));
		g_signal_emit(plot, gtk_plot_linear_signals[MOVED], 0);
	}
	return FALSE;
}

static gboolean gtk_plot_linear_button_release(GtkWidget *widget, GdkEventButton *event)
{
	GtkPlotLinearPrivate *priv;
	GtkPlotLinear *plot;
	gint d, xw;
	gdouble xn, xx, yn, yx, s;

	priv=GTK_PLOT_LINEAR_GET_PRIVATE(widget);
	plot=GTK_PLOT_LINEAR(widget);
	if ((priv->flagr)==1)
	{
		if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_SGL)==0)
		{
			(priv->rescale.xmax)=(priv->bounds.xmax);
			(priv->rescale.ymax)=(priv->bounds.ymax);
			if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_HZT)!=0)
			{
				if ((event->x)<=(priv->range.xj)) (priv->rescale.xmax)=(priv->bounds.xmin);
				else
				{
					d=(priv->range.xn)-(event->x);
					if (d>0) (priv->rescale.xmax)-=(((priv->bounds.xmax)-(priv->bounds.xmin))*d)/((priv->range.xn)-(priv->range.xj));
				}
			}
			if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_VRT)!=0)
			{
				if ((event->y)>=(priv->range.yn)) (priv->rescale.ymax)=(priv->bounds.ymin);
				else
				{
					d=(event->y)-(priv->range.yj);
					if (d>0) (priv->rescale.ymax)-=(((priv->bounds.ymax)-(priv->bounds.ymin))*d)/((priv->range.yn)-(priv->range.yj));
				}
			}
			xn=(priv->rescale.xmax)-(priv->rescale.xmin);
			yn=(priv->rescale.ymax)-(priv->rescale.ymin);
			if (((xn>DZE)||(xn<NZE))&&((yn>DZE)||(yn<NZE)))
			{
				if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_DRG)!=0) gtk_plot_linear_update_scale_pretty(widget, (priv->bounds.xmin)-xn, (priv->bounds.xmax)-xn, (priv->bounds.ymin)-yn, (priv->bounds.ymax)-yn);
				else if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_OUT)==0) gtk_plot_linear_update_scale_pretty(widget, (priv->rescale.xmin), (priv->rescale.xmax), (priv->rescale.ymin), (priv->rescale.ymax));
				else
				{
					s=((priv->bounds.xmax)-(priv->bounds.xmin))/xn;
					if (s>0)
					{
						xn=((priv->bounds.xmin)-(priv->rescale.xmin))*s;
						xn+=(priv->bounds.xmin);
						xx=((priv->bounds.xmax)-(priv->rescale.xmax))*s;
						xx+=(priv->bounds.xmax);
						s=((priv->bounds.ymax)-(priv->bounds.ymin))/yn;
						if (s>0)
						{
							yn=((priv->bounds.ymin)-(priv->rescale.ymin))*s;
							yn+=(priv->bounds.ymin);
							yx=((priv->bounds.ymax)-(priv->rescale.ymax))*s;
							yx+=(priv->bounds.ymax);
							gtk_plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
						}
						else if (s<0)
						{
							yn=((priv->rescale.ymax)-(priv->bounds.ymin))*s;
							yn+=(priv->bounds.ymin);
							yx=((priv->rescale.ymin)-(priv->bounds.ymax))*s;
							yx+=(priv->bounds.ymax);
							gtk_plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
						}
					}
					else if (s<0)
					{
						xn=((priv->rescale.xmax)-(priv->bounds.xmin))*s;
						xn+=(priv->bounds.xmin);
						xx=((priv->rescale.xmin)-(priv->bounds.xmax))*s;
						xx+=(priv->bounds.xmax);
						s=((priv->bounds.ymax)-(priv->bounds.ymin))/yn;
						if (s>0)
						{
							yn=((priv->bounds.ymin)-(priv->rescale.ymin))*s;
							yn+=(priv->bounds.ymin);
							yx=((priv->bounds.ymax)-(priv->rescale.ymax))*s;
							yx+=(priv->bounds.ymax);
							gtk_plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
						}
						else if (s<0)
						{
							yn=((priv->rescale.ymax)-(priv->bounds.ymin))*s;
							yn+=(priv->bounds.ymin);
							yx=((priv->rescale.ymin)-(priv->bounds.ymax))*s;
							yx+=(priv->bounds.ymax);
							gtk_plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
						}
					}
				}
			}
		}
		else if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_OUT)==0)
		{
			xn=ZS*(priv->rescale.xmin);
			xx=xn;
			xn+=ZSC*(priv->bounds.xmin);
			xx+=ZSC*(priv->bounds.xmax);
			yn=ZS*(priv->rescale.ymin);
			yx=yn;
			yn+=ZSC*(priv->bounds.ymin);
			yx+=ZSC*(priv->bounds.ymax);
			gtk_plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
		}
		else
		{
			xn=-UZC*(priv->rescale.xmin);
			xx=xn;
			xn+=UZ*(priv->bounds.xmin);
			xx+=UZ*(priv->bounds.xmax);
			yn=-UZC*(priv->rescale.ymin);
			yx=yn;
			yn+=UZ*(priv->bounds.ymin);
			yx+=UZ*(priv->bounds.ymax);
			gtk_plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
		}
		(priv->flagr)=0;
	}
	else if ((event->y)<=11)
	{
		xw=gtk_widget_get_allocated_width(widget);
		if ((event->x)>=xw-22)
		{
			if ((event->x)>=xw-11)
			{
				if (((plot->zmode)&(GTK_PLOT_LINEAR_ZOOM_DRG|GTK_PLOT_LINEAR_ZOOM_OUT))==0)
				{
					if (((plot->zmode)&(GTK_PLOT_LINEAR_ZOOM_SGL))!=0) (plot->zmode)>>=1;
					(plot->zmode)|=GTK_PLOT_LINEAR_ZOOM_DRG;
				}
				else (plot->zmode)--;
			}
			else
			{
				(plot->zmode)-=GTK_PLOT_LINEAR_ZOOM_HZT;
				if (((plot->zmode)&(GTK_PLOT_LINEAR_ZOOM_SGL|GTK_PLOT_LINEAR_ZOOM_HZT|GTK_PLOT_LINEAR_ZOOM_VRT))==0)
				{
					if (((plot->zmode)&GTK_PLOT_LINEAR_ZOOM_DRG)!=0) (plot->zmode)|=(GTK_PLOT_LINEAR_ZOOM_VRT|GTK_PLOT_LINEAR_ZOOM_HZT);
					else (plot->zmode)|=GTK_PLOT_LINEAR_ZOOM_SGL;
				}
			}
			gtk_widget_queue_draw_area(widget, xw-22, 0, 22, 11);
		}
	}
	return FALSE;
}

void gtk_plot_linear_set_label(GtkPlotLinear *plot, gchar *xl, gchar *yl)
{
	if (plot->xlab) g_free(plot->xlab);
	if (plot->ylab) g_free(plot->ylab);
	{(plot->xlab)=g_strdup(xl); (plot->ylab)=g_strdup(yl);}
}

void gtk_plot_linear_set_data(GtkPlotLinear *plot, GArray *xd, GArray *yd, GArray *nd, GArray *sz, GArray *st)
{
	if (plot->xdata) g_array_unref(plot->xdata);
	if (plot->ydata) g_array_unref(plot->ydata);
	{(plot->xdata)=g_array_ref(xd); (plot->ydata)=g_array_ref(yd);}
	gtk_plot_set_indices(GTK_PLOT(plot), nd, sz, st);
}

static void gtk_plot_linear_finalise(GtkPlotLinear *plot)
{
	if (plot->xlab) {
		g_free(plot->xlab);
		plot->xlab=NULL;
	}
	if (plot->ylab) {
		g_free(plot->ylab);
		plot->ylab=NULL;
	}
	if (plot->xdata) {
		g_array_unref(plot->xdata);
		plot->xdata=NULL;
	}
	if (plot->ydata) {
		g_array_unref(plot->ydata);
		plot->ydata=NULL;
	}
}

static void gtk_plot_linear_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch (prop_id)
	{
		case PROP_BXN: GTK_PLOT_LINEAR_GET_PRIVATE(object)->bounds.xmin=g_value_get_double(value);
		break;
		case PROP_BXX: GTK_PLOT_LINEAR_GET_PRIVATE(object)->bounds.xmax=g_value_get_double(value);
		break;
		case PROP_BYN: GTK_PLOT_LINEAR_GET_PRIVATE(object)->bounds.ymin=g_value_get_double(value);
		break;
		case PROP_BYX: GTK_PLOT_LINEAR_GET_PRIVATE(object)->bounds.ymax=g_value_get_double(value);
		break;
		case PROP_XTJ: GTK_PLOT_LINEAR_GET_PRIVATE(object)->ticks.xj=g_value_get_uint(value);
		break;
		case PROP_YTJ: GTK_PLOT_LINEAR_GET_PRIVATE(object)->ticks.yj=g_value_get_uint(value);
		break;
		case PROP_XTN: GTK_PLOT_LINEAR_GET_PRIVATE(object)->ticks.xn=g_value_get_uint(value);
		break;
		case PROP_YTN: GTK_PLOT_LINEAR_GET_PRIVATE(object)->ticks.yn=g_value_get_uint(value);
		break;
		case PROP_FA: GTK_PLOT_LINEAR_GET_PRIVATE(object)->flaga=g_value_get_uint(value);
		break;
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gtk_plot_linear_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch (prop_id)
	{
		case PROP_BXN: g_value_set_double(value, GTK_PLOT_LINEAR_GET_PRIVATE(object)->bounds.xmin);
		break;
		case PROP_BXX: g_value_set_double(value, GTK_PLOT_LINEAR_GET_PRIVATE(object)->bounds.xmax);
		break;
		case PROP_BYN: g_value_set_double(value, GTK_PLOT_LINEAR_GET_PRIVATE(object)->bounds.ymin);
		break;
		case PROP_BYX: g_value_set_double(value, GTK_PLOT_LINEAR_GET_PRIVATE(object)->bounds.ymax);
		break;
		case PROP_XTJ: g_value_set_uint(value, GTK_PLOT_LINEAR_GET_PRIVATE(object)->ticks.xj);
		break;
		case PROP_YTJ: g_value_set_uint(value, GTK_PLOT_LINEAR_GET_PRIVATE(object)->ticks.yj);
		break;
		case PROP_XTN: g_value_set_uint(value, GTK_PLOT_LINEAR_GET_PRIVATE(object)->ticks.xn);
		break;
		case PROP_YTN: g_value_set_uint(value, GTK_PLOT_LINEAR_GET_PRIVATE(object)->ticks.yn);
		break;
		case PROP_FA: g_value_set_uint(value, GTK_PLOT_LINEAR_GET_PRIVATE(object)->flaga);
		break;
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static gboolean gtk_plot_linear_draw(GtkWidget *widget, cairo_t *cr)
{
	draw(widget, cr);
	drawz(widget, cr);
	return FALSE;
}

static void gtk_plot_linear_class_init(GtkPlotLinearClass *klass)
{
	GObjectClass *obj_klass;
	GtkWidgetClass *widget_klass;

	obj_klass=G_OBJECT_CLASS(klass);
	g_type_class_add_private(obj_klass, sizeof(GtkPlotLinearPrivate));
	(obj_klass->finalize)=(GObjectFinalizeFunc) gtk_plot_linear_finalise;
	(obj_klass->set_property)=gtk_plot_linear_set_property;
	(obj_klass->get_property)=gtk_plot_linear_get_property;
	g_object_class_install_property(obj_klass, PROP_BXN, g_param_spec_double("xmin", "Minimum x value", "Minimum value for the horizontal scale", -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_BXX, g_param_spec_double("xmax", "Maximum x value", "Maximum value for the horizontal scale", -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_BYN, g_param_spec_double("ymin", "Minimum y value", "Minimum value for the vertical scale", -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_BYX, g_param_spec_double("ymax", "Maximum y value", "Maximum value for the vertical scale", -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_XTJ, g_param_spec_uint("xbigticks", "Major x ticks-1", "Number of gaps between major ticks for the horizontal scale-1", 1, G_MAXINT, 4, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_YTJ, g_param_spec_uint("ybigticks", "Major y ticks-1", "Number of gaps between major ticks for the vertical scale-1", 1, G_MAXINT, 4, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_XTN, g_param_spec_uint("xsmallticks", "Minor x ticks+1", "Number of unlabelled tick divisions between major ticks for the horizontal scale+1", 1, G_MAXINT, 5, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_YTN, g_param_spec_uint("ysmallticks", "Minor y ticks+1", "Number of unlabelled ticks divisions between major ticks for the vertical scale+1", 1, G_MAXINT, 5, G_PARAM_READWRITE));
	g_object_class_install_property(obj_klass, PROP_FA, g_param_spec_flags("aflag", "Axis Flags", "Flags for axes behaviour: 32 = Labels right, 16 = Labels above, 8 = Wiggle on top, 4 = Wiggle underneath, 2 = Wiggle on Right, 1 = Wiggle on left", G_TYPE_FLAGS, 0, G_PARAM_READWRITE));
	widget_klass=GTK_WIDGET_CLASS(klass);
	(widget_klass->button_press_event)=gtk_plot_linear_button_press;
	(widget_klass->motion_notify_event)=gtk_plot_linear_motion_notify;
	(widget_klass->button_release_event)=gtk_plot_linear_button_release;
	(widget_klass->draw)=gtk_plot_linear_draw;
	gtk_widget_class_set_accessible_type(widget_klass, GTK_TYPE_PLOT_LINEAR_ACCESSIBLE);
	gtk_plot_linear_signals[MOVED]=g_signal_new("moved", G_OBJECT_CLASS_TYPE(obj_klass), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GtkPlotLinearClass, moved), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void gtk_plot_linear_init(GtkPlotLinear *plot)
{
	GtkPlotLinearPrivate *priv;

	gtk_widget_add_events(GTK_WIDGET(plot), GDK_BUTTON_PRESS_MASK|GDK_POINTER_MOTION_MASK|GDK_BUTTON_RELEASE_MASK);
	priv=GTK_PLOT_LINEAR_GET_PRIVATE(plot);
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

GtkWidget *gtk_plot_linear_new(void) {return g_object_new(GTK_PLOT_TYPE_LINEAR, NULL);}
