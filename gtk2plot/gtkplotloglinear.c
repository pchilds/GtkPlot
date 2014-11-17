/***************************************************************************
 *            gtkplotloglinear.c
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
#include "gtkplotloglinear.h"

#define GTK_PLOT_LOG_LINEAR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_PLOT_TYPE_LOG_LINEAR, GtkPlotLogLinearPrivate))
#define ARP 0.05 /* Proportion of the graph occupied by arrows */
#define IRTR 0.577350269 /* 1/square root 3 */
#define MY_2PI 6.2831853071795864769252867665590057683943387987502
#define LTN /* log_10(9) */
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
typedef enum {
	GTK_PLOT_LOG_LINEAR_BORDERS_LT = 1 << 0,
	GTK_PLOT_LOG_LINEAR_BORDERS_RT = 1 << 1,
	GTK_PLOT_LOG_LINEAR_BORDERS_DN = 1 << 2,
	GTK_PLOT_LOG_LINEAR_BORDERS_UP = 1 << 3
} GtkPlotLogLinearBorders;
typedef enum {
	GTK_PLOT_LOG_LINEAR_AXES_LW = 1 << 0,
	GTK_PLOT_LOG_LINEAR_AXES_RW = 1 << 1,
	GTK_PLOT_LOG_LINEAR_AXES_DW = 1 << 2,
	GTK_PLOT_LOG_LINEAR_AXES_UW = 1 << 3,
	GTK_PLOT_LOG_LINEAR_AXES_LR = 1 << 4,
	GTK_PLOT_LOG_LINEAR_AXES_LT = 1 << 5
} GtkPlotLogLinearAxes;
G_DEFINE_TYPE (GtkPlotLogLinear, gtk_plot_log_linear, GTK_TYPE_PLOT);
enum {PROP_0, PROP_BXN, PROP_BXX, PROP_BYN, PROP_BYX, PROP_XTJ, PROP_YTJ, PROP_XTN, PROP_YTN, PROP_FA};
enum {MOVED, KEY_CHANGED, LAST_SIGNAL};
static guint gtk_plot_log_linear_signals[LAST_SIGNAL]={0};
typedef struct _GtkPlotLogLinearPrivate GtkPlotLogLinearPrivate;
struct xs {gdouble xmin, ymin, xmax, ymax;};
struct tk {guint xj, yj, xn, yn;};
struct _GtkPlotLogLinearPrivate {struct xs bounds, rescale; struct tk ticks, range; guint flaga, flagr;};
static const gdouble dsh1[1] = {3.0};
static const gdouble dsh2[2] = {4.0,2.0};
static const gdouble SZ[3]   = {5.0,7.5,10.0};

static void drawz(GtkWidget *widget, cairo_t *cr) {
  gdouble dt;
  gint j, xw;
  GtkPlotLogLinear *plot;
  static const gdouble AR[3]={0.5,1.0,1.5};

  plot=GTK_PLOT_LOG_LINEAR(widget);
  cairo_save(cr);
  cairo_set_source_rgba(cr, 0, 0, 0, 1);
  cairo_set_line_width(cr, 1);
  cairo_set_dash(cr, NULL, 0, 0.0);
  xw=widget->allocation.width;
  j=(xw<400)?0:(xw<1000)?1:2;
  cairo_rectangle(cr,   xw-4*SZ[j]-1.5, 0.5   , 2*SZ[j], 2*SZ[j]);
  cairo_rectangle(cr,   xw-2*SZ[j]-0.5, 0.5   , 2*SZ[j], 2*SZ[j]);
  cairo_move_to(cr,     xw-2*SZ[j]+1.0        ,   SZ[j]+0.5);
  cairo_line_to(cr,     xw        -2.0        ,   SZ[j]+0.5);
  if (((plot->zmode)&GTK_PLOT_LOG_LINEAR_ZOOM_DRG)!=0) {
    cairo_move_to(cr,   xw        -2.0  -AR[j],   SZ[j]+0.5-2*AR[j]);
    cairo_line_to(cr,   xw        -2.0        ,   SZ[j]+0.5);
    cairo_line_to(cr,   xw        -2.0  -AR[j],   SZ[j]+0.5+2*AR[j]);
  } else {
    cairo_move_to(cr,   xw  -SZ[j]-0.5        ,         2.0);
    cairo_line_to(cr,   xw  -SZ[j]-0.5        , 2*SZ[j]-1.0);
    if (((plot->zmode)&GTK_PLOT_LOG_LINEAR_ZOOM_OUT)==0) {
      cairo_move_to(cr, xw  -SZ[j]-0.5-2*AR[j],         2.0  +AR[j]);
      cairo_line_to(cr, xw  -SZ[j]-0.5        ,         2.0);
      cairo_line_to(cr, xw  -SZ[j]-0.5+2*AR[j],         2.0  +AR[j]);
      cairo_move_to(cr, xw        -2.0  -AR[j],   SZ[j]+0.5-2*AR[j]);
      cairo_line_to(cr, xw        -2.0        ,   SZ[j]+0.5);
      cairo_line_to(cr, xw        -2.0  -AR[j],   SZ[j]+0.5+2*AR[j]);
      cairo_move_to(cr, xw  -SZ[j]-0.5-2*AR[j], 2*SZ[j]-1.0  -AR[j]);
      cairo_line_to(cr, xw  -SZ[j]-0.5        , 2*SZ[j]-1.0);
      cairo_line_to(cr, xw  -SZ[j]-0.5+2*AR[j], 2*SZ[j]-1.0  -AR[j]);
      cairo_move_to(cr, xw-2*SZ[j]+1.0  +AR[j],   SZ[j]+0.5-2*AR[j]);
      cairo_line_to(cr, xw-2*SZ[j]+1.0        ,   SZ[j]+0.5);
      cairo_line_to(cr, xw-2*SZ[j]+1.0  +AR[j],   SZ[j]+0.5+2*AR[j]);
    } else {
      cairo_move_to(cr, xw-  SZ[j]-0.5-4*AR[j],   SZ[j]+0.5-4*AR[j]);
      cairo_line_to(cr, xw-  SZ[j]-0.5+4*AR[j],   SZ[j]+0.5+4*AR[j]);
      cairo_move_to(cr, xw-  SZ[j]-0.5-4*AR[j],   SZ[j]+0.5+4*AR[j]);
      cairo_line_to(cr, xw-  SZ[j]-0.5+4*AR[j],   SZ[j]+0.5-4*AR[j]);
    }
  }
  cairo_stroke(cr);
  if (((plot->zmode)&GTK_PLOT_LOG_LINEAR_ZOOM_SGL)==0) {
    dt=1;
    cairo_set_dash(cr, &dt, 1, 0);
    if (((plot->zmode)&GTK_PLOT_LOG_LINEAR_ZOOM_VRT)!=0) {
      cairo_move_to(cr, xw-4*SZ[j]    ,         2.5);
      cairo_line_to(cr, xw-2*SZ[j]-3.0,         2.5);
      cairo_move_to(cr, xw-4*SZ[j]    , 2*SZ[j]-1.5);
      cairo_line_to(cr, xw-2*SZ[j]-3.0, 2*SZ[j]-1.5);
      cairo_stroke(cr);
    }
    if (((plot->zmode)&GTK_PLOT_LOG_LINEAR_ZOOM_HZT)!=0) {
      cairo_move_to(cr, xw-4*SZ[j]+0.5,         2.0);
      cairo_line_to(cr, xw-4*SZ[j]+0.5, 2*SZ[j]-1.0);
      cairo_move_to(cr, xw-2*SZ[j]-3.5,         2.0);
      cairo_line_to(cr, xw-2*SZ[j]-3.5, 2*SZ[j]-1.0);
      cairo_stroke(cr);
    }
  }
  cairo_restore(cr);
}

static void drawk(GtkWidget *widget, cairo_t *cr) {
  gdouble hc, vv, wv, xv, yv;
  gint ft, hg, j, k=0, wd, xw;
  GtkPlot *plt;
  GtkPlotLogLinear *plot;
  guint fg;
  PangoLayout *lyt;

  plt=GTK_PLOT(widget);
  if (!plt->ky || !*(plt->ky)) return;
  cairo_save(cr);
  xw=widget->allocation.width;
  j=(xw<400)?0:(xw<1000)?1:2;
  hc=2*SZ[j]+3.0;
  plot=GTK_PLOT_LOG_LINEAR(widget);
  cairo_set_line_width(cr, plot->linew);
  do {
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
	lyt=pango_cairo_create_layout(cr);
	pango_layout_set_font_description(lyt, plt->lfont);
	pango_layout_set_text(lyt, (plt->ky)[k], -1);
	pango_layout_get_pixel_size(lyt, &wd, &hg);
    cairo_move_to(cr, xw-wd-2*SZ[j]-1.0, hc);
    pango_cairo_show_layout(cr, lyt);
    g_object_unref(lyt);
	hc+=hg/2+1.0;
    cairo_set_dash(cr, NULL, 0, 0);
    ft=fmod(k,plt->rd->len);
    vv=g_array_index(plt->rd, gdouble, ft);
    wv=g_array_index(plt->gr, gdouble, ft);
    xv=g_array_index(plt->bl, gdouble, ft);
    yv=g_array_index(plt->al, gdouble, ft);
    if (plot->kdata && k<(plot->kdata->len)) {
      fg=g_array_index(plot->kdata, guint, k);
      if (fg&GTK_PLOT_LOG_LINEAR_KEY_CLR) yv=0.0;
      switch ((fg&GTK_PLOT_LOG_LINEAR_KEY_DSH)>>1) {
        case 2:
          cairo_set_dash(cr, dsh2, sizeof(dsh2)/sizeof(dsh2[0]), 0);
          break;
        case 1:
          cairo_set_dash(cr, dsh1, sizeof(dsh1)/sizeof(dsh1[0]), 0);
          break;
        default:
          break;
      }
    }
    cairo_set_source_rgba(cr, vv, wv, xv, yv);
    cairo_move_to(cr, xw-2*SZ[j], hc);
    cairo_line_to(cr, xw-0.5, hc);
    cairo_stroke(cr);
	hc+=hg/2+1.0;
  } while ((plt->ky)[++k]);
  cairo_restore(cr);
}

static void draw(GtkWidget *widget, cairo_t *cr) {
  cairo_matrix_t mtr2, mtr3;
  gchar *str1=NULL;
  gchar lbl[BFL];
  gdouble av, dely, dt, lr1, lr2, vv, wv, zv;
  gint dtt, ft, hg, j, k, lt, st, tf, to, tn, tnn, tx, tz, wd, xa, xl, xr, xr2, xt, xu, xv, xvn, xw, ya, yl, yr, yr2, yu, yv, yvn, yw;
  GtkPlot *plt;
  GtkPlotLogLinear *plot;
  GtkPlotLogLinearPrivate *priv;
  guint fg, lr3;
  PangoLayout *lyt;

	{mtr2.xx=0; mtr2.xy=1; mtr2.yx=-1; mtr2.yy=0; mtr2.x0=0; mtr2.y0=0;}/* initialise */
	{mtr3.xx=0; mtr3.xy=-1; mtr3.yx=1; mtr3.yy=0; mtr3.x0=0; mtr3.y0=0;}
  plot=GTK_PLOT_LOG_LINEAR(widget);
  plt=GTK_PLOT(widget);
  xw=widget->allocation.width;
  yw=widget->allocation.height;
  priv=GTK_PLOT_LOG_LINEAR_GET_PRIVATE(plot);
  priv->flaga&=GTK_PLOT_LOG_LINEAR_AXES_LT|GTK_PLOT_LOG_LINEAR_AXES_LR;
  dely=(priv->bounds.ymax-priv->bounds.ymin)/priv->ticks.yj;
  lyt=pango_cairo_create_layout(cr);
  pango_layout_set_font_description(lyt, plt->lfont);
  str1=g_strconcat(plot->xlab, plot->ylab, NULL);
  pango_layout_set_text(lyt, str1, -1);
  pango_layout_get_pixel_size(lyt, &wd, &dtt);
  g_free(str1);
  g_object_unref(lyt);
  lyt=pango_cairo_create_layout(cr);
  pango_layout_set_font_description(lyt, plt->afont);
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
  if (priv->bounds.ymax<=DZE) { /* determine positions of axes */
    priv->flaga|=GTK_PLOT_LOG_LINEAR_AXES_LT;
    lr3=3;
    if (priv->bounds.ymin<-1.0) lr3+=floor(log10(-(priv->bounds.ymin)));
    if ((plot->ydp)!=0) lr3+=plot->ydp+1;
    if (lr3<BFL) g_snprintf(lbl, lr3, "%f", priv->bounds.ymin);
    else g_snprintf(lbl, BFL, "%f", priv->bounds.ymin);
    pango_layout_set_text(lyt, lbl, -1);
    pango_layout_get_pixel_size(lyt, &wd, &hg);
    yl=yw-(wd/2)-1; /* allow space for lower label */
    ya=dtt;
    yu=(yl-ya)*priv->bounds.ymax/priv->bounds.ymin;
    if (yu>yw*WGP) {
      priv->flaga|=GTK_PLOT_LOG_LINEAR_AXES_DW;
      yu=yw*WGP;
    }
    yu+=ya;
  } else if (priv->bounds.ymin>=NZE) {
    yu=yr;
    ya=yw-dtt;
    yl=(ya-yu)*priv->bounds.ymin/priv->bounds.ymax;
    if (yl>yw*WGP) {
      priv->flaga|=GTK_PLOT_LOG_LINEAR_AXES_UW;
      yl=yw*WGP;
    }
    yl=ya-yl;
  } else if (((priv->flaga)&GTK_PLOT_LOG_LINEAR_AXES_LT)!=0) {
    if (priv->bounds.ymin+priv->bounds.ymax<=0) {
      lr3=3;
      if (priv->bounds.ymin<-1) lr3+=floor(log10(-(priv->bounds.ymin)));
      if ((plot->ydp)!=0) lr3+=plot->ydp+1;
      if (lr3<BFL) g_snprintf(lbl, lr3, "%f", priv->bounds.ymin);
      else g_snprintf(lbl, BFL, "%f", (priv->bounds.ymin));
      pango_layout_set_text(lyt, lbl, -1);
      pango_layout_get_pixel_size(lyt, &wd, &hg);
      yl=yw-(wd/2)-1; /* allow space for lower label */
      yu=dtt+(yl-dtt)*priv->bounds.ymax/priv->bounds.ymin;
      if (yu<yr) {
        yu=yr;
        ya=(yl*priv->bounds.ymax-yr*priv->bounds.ymin)/(priv->bounds.ymax-priv->bounds.ymin);
      } else ya=dtt;
    } else {
      yl=yw;
      yu=yr;
      ya=(yl*priv->bounds.ymax-yu*priv->bounds.ymin)/(priv->bounds.ymax-priv->bounds.ymin);
      if (ya>(yw-xr2)) {
        yl*=(yw-xr2)/ya;
        ya=yw-xr2;
      }
    }
  } else {
    yu=yr;
    ya=yw-dtt;
    yl=ya+(yu-ya)*priv->bounds.ymin/priv->bounds.ymax;
    if (yl>yw) {
      yl=yw;
      if (priv->bounds.ymin+priv->bounds.ymax<=0) {
        lr3=3;
        if (priv->bounds.ymin<-1.0) lr3+=floor(log10(-(priv->bounds.ymin)));
        if ((plot->ydp)!=0) lr3+=plot->ydp+1;
        if (lr3<BFL) g_snprintf(lbl, lr3, "%f", priv->bounds.ymin);
        else g_snprintf(lbl, BFL, "%f", priv->bounds.ymin);
        pango_layout_set_text(lyt, lbl, -1);
        pango_layout_get_pixel_size(lyt, &wd, &hg);
        yl+=wd/2;
        yl--; /* allow space for lower label */
      }
      ya=(yl*priv->bounds.ymax-yu*priv->bounds.ymin)/(priv->bounds.ymax-priv->bounds.ymin);
    }
  }
  g_object_unref(lyt);
  lyt=pango_cairo_create_layout(cr);
  pango_layout_set_font_description(lyt, plt->afont);
  xu=xw-xr;
  xa=dtt;
  priv->flaga|=GTK_PLOT_LOG_LINEAR_AXES_RW;
  xl=dtt+xw*WGP;
  g_object_unref(lyt);
  priv->range.xj=xl;
  priv->range.yj=yu;
  priv->range.xn=xu;
  priv->range.yn=yl;
/* draw x wiggles */
  cairo_set_source_rgba(cr, 0, 0, 0, 1);
  cairo_set_line_width(cr, 2);
  cairo_move_to(cr, 0, ya);
  cairo_line_to(cr, dtt+WFP*xw, ya);
  cairo_curve_to(cr, dtt+WMP*xw, ya-WHP*xw, dtt+WMP*xw, ya+WHP*xw, xl, ya);
  xr-=2;
  cairo_line_to(cr, xw, ya); /* draw x axis */
  cairo_line_to(cr, xw-xr, ya-xr2); /* draw x arrow */
  cairo_move_to(cr, xw, ya);
  cairo_line_to(cr, xw-xr, ya+xr2);
  cairo_move_to(cr, xa, yw);
/* draw y wiggles */
  if (((priv->flaga)&GTK_PLOT_LOG_LINEAR_AXES_DW)!=0) { 
		cairo_line_to(cr, xa, ya+(WGP*yw));
		cairo_curve_to(cr, xa-(WHP*yw), ya+(WMP*yw), xa+(WHP*yw), ya+(WMP*yw), xa, ya+(WFP*yw));
  } else if (((priv->flaga)&GTK_PLOT_LOG_LINEAR_AXES_UW)!=0) {
		cairo_line_to(cr, xa, ya-(WFP*yw));
		cairo_curve_to(cr, xa-(WHP*yw), ya-(WMP*yw), xa+(WHP*yw), ya-(WMP*yw), xa, ya-(WGP*yw));
  }
  yr-=2;
  cairo_line_to(cr, xa, 0); /* draw y axis */
  cairo_line_to(cr, xa-yr2, yr); /* draw y arrow */
  cairo_move_to(cr, xa, 0);
  cairo_line_to(cr, xa+yr2, yr);
  cairo_stroke(cr);
/* draw minor x grid bits */
  dt=5;
  cairo_save(cr);
  cairo_set_source_rgba(cr, 0, 0, 0, 0.4);
  cairo_set_line_width(cr, 1);
  cairo_set_dash(cr, &dt, 1, 0);
  if ((tn=priv->bounds.xmin-floor(priv->bounds.xmin))>DZE) for (k=ceil(exp(G_LN10*tn)); k<priv->ticks.xn+2; k++) {
    tnn=xl+(log10(k)-tn)*(xu-xl)/(priv->bounds.xmax-priv->bounds.xmin);
    cairo_move_to(cr, tnn, yu);
    cairo_line_to(cr, tnn, yl);
  }
  for (j=ceil(priv->bounds.xmin); j<=floor(priv->bounds.xmax); j++) for (k=2; k<priv->ticks.xn+2; k++) {
    tnn=xl+(j+log10(k)-priv->bounds.xmin)*(xu-xl)/(priv->bounds.xmax-priv->bounds.xmin);
    cairo_move_to(cr, tnn, yu);
    cairo_line_to(cr, tnn, yl);
  }
  if ((tn=priv->bounds.xmax-floor(priv->bounds.xmax))>DZE) {
    k=2;
    while ((k<priv->ticks.xn+2)&&(k<ceil(exp(G_LN10*tn)))) {
      tnn=xu+(log10(k)-tn)*(xu-xl)/(priv->bounds.xmax-priv->bounds.xmin);
      cairo_move_to(cr, tnn, yu);
      cairo_line_to(cr, tnn, yl);
      k++;
    }
  }
  cairo_stroke(cr);
  cairo_restore(cr);
  lyt=pango_cairo_create_layout(cr); /* draw ticks, grid and labels */
  pango_layout_set_font_description(lyt, plt->lfont);
  pango_layout_set_text(lyt, plot->xlab, -1);
  pango_layout_get_pixel_size(lyt, &wd, &hg);
  if (((priv->flaga)&GTK_PLOT_LOG_LINEAR_AXES_LT)!=0) {
    cairo_move_to(cr, (xu+xl-wd)/2, ya-dtt);
    pango_cairo_show_layout(cr, lyt);
    g_object_unref(lyt);
    for (j=ceil(priv->bounds.xmin); j<=floor(priv->bounds.xmax); j++) {
      tn=xl+(j-priv->bounds.xmin)*(xu-xl)/(priv->bounds.xmax-priv->bounds.xmin);
      cairo_save(cr);
      cairo_set_line_width(cr, 1);
      cairo_set_dash(cr, &dt, 1, 0);
      cairo_move_to(cr, tn, yu);
      cairo_line_to(cr, tn, yl);
      cairo_stroke(cr);
      cairo_restore(cr);
      cairo_move_to(cr, tn, ya);
      cairo_line_to(cr, tn, ya-JT);
      cairo_stroke(cr);
      lyt=pango_cairo_create_layout(cr);
      pango_layout_set_font_description(lyt, plt->afont);
      if (j>=0) g_snprintf(lbl, BFL, "%d", (gint) round(exp(G_LN10*j)));
      else g_snprintf(lbl, BFL, "%.*f", -j, 1.01*exp(G_LN10*j));
      pango_layout_set_text(lyt, lbl, -1);
      pango_layout_get_pixel_size(lyt, &wd, &hg);
      cairo_move_to(cr, tn-wd/2, ya-JTI-hg);
      pango_cairo_show_layout(cr, lyt);
      g_object_unref(lyt);
    }
  } else {
    cairo_move_to(cr, (xu+xl-wd)/2, ya+dtt-hg);
    pango_cairo_show_layout(cr, lyt);
    g_object_unref(lyt);
    for (j=ceil(priv->bounds.xmin); j<=floor(priv->bounds.xmax); j++) {
      tn=xl+(j-priv->bounds.xmin)*(xu-xl)/(priv->bounds.xmax-priv->bounds.xmin);
      cairo_save(cr);
      cairo_set_line_width(cr, 1);
      cairo_set_dash(cr, &dt, 1, 0);
      cairo_move_to(cr, tn, yu);
      cairo_line_to(cr, tn, yl);
      cairo_stroke(cr);
      cairo_restore(cr);
      cairo_move_to(cr, tn, ya);
      cairo_line_to(cr, tn, ya+JT);
      cairo_stroke(cr);
      lyt=pango_cairo_create_layout(cr);
      pango_layout_set_font_description(lyt, plt->afont);
      if (j>=0) g_snprintf(lbl, BFL, "%d", (gint) round(exp(G_LN10*j)));
      else g_snprintf(lbl, BFL, "%.*f", -j, 1.01*exp(G_LN10*j));
      pango_layout_set_text(lyt, lbl, -1);
      pango_layout_get_pixel_size(lyt, &wd, &hg);
      cairo_move_to(cr, tn-wd/2, ya+JTI);
      pango_cairo_show_layout(cr, lyt);
      g_object_unref(lyt);
    }
  }
  lyt=pango_cairo_create_layout(cr);
  pango_layout_set_font_description(lyt, plt->lfont);
  pango_layout_set_text(lyt, plot->ylab, -1);
  pango_layout_get_pixel_size(lyt, &wd, &hg);
  if (((priv->flaga)&GTK_PLOT_LOG_LINEAR_AXES_DW)!=0) {
    to=yu;
    if (((priv->flaga)&GTK_PLOT_LOG_LINEAR_AXES_LR)!=0) {
      cairo_move_to(cr, xa+dtt, (yl+yu-wd)/2);
      cairo_set_matrix(cr, &mtr3);
      pango_cairo_update_layout(cr, lyt);
      pango_cairo_show_layout(cr, lyt);
      g_object_unref(lyt);
      cairo_identity_matrix(cr);
      lyt=pango_cairo_create_layout(cr);
      pango_layout_set_font_description(lyt, plt->afont);
      lr1=round(priv->bounds.ymax*exp(G_LN10*plot->ydp));
      if (priv->bounds.ymax>DZE){
        lr1+=0.1;
        lr1*=exp(-G_LN10*plot->ydp);
        if (lr1>=1) {
          lr2=log10(lr1);
          lr3=2;
          if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
          else lr3+=(guint) ceil(lr2);
          if ((plot->ydp)!=0) lr3+=plot->ydp+1;
          if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
          else g_snprintf(lbl, BFL, "%f", lr1);
        } else if (plot->ydp<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
        else g_snprintf(lbl, BFL, "%f", lr1);
      } else if (priv->bounds.ymax<NZE) {
        lr1-=0.1;
        lr1*=exp(-G_LN10*plot->ydp);
        if (lr1<=-1) {
          lr2=log10(-lr1);
          lr3=3;
          if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
          else lr3+=(guint) ceil(lr2);
          if ((plot->ydp)!=0) lr3+=plot->ydp+1;
          if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
          else g_snprintf(lbl, BFL, "%f", lr1);
        } else if (plot->ydp<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
        else g_snprintf(lbl, BFL, "%f", lr1);
      } else g_snprintf(lbl, 2, "%f", 0.0);
      pango_layout_set_text(lyt, lbl, -1);
      pango_layout_get_pixel_size(lyt, &wd, &hg);
      cairo_move_to(cr, xa+JTI+hg, to-wd/2);
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
      for (j=1; j<=priv->ticks.yj; j++) {
        tn=yu+(yl-yu)*j/priv->ticks.yj;
        for (k=1; k<priv->ticks.yn; k++) {
          tnn=to+(tn-to)*k/priv->ticks.yn;
          cairo_move_to(cr, xa, tnn);
          cairo_line_to(cr, xa+NT, tnn);
        }
        cairo_stroke(cr);
        cairo_set_line_width(cr, 2);
        cairo_move_to(cr, xa, tn);
        cairo_line_to(cr, xa+JT, tn);
        cairo_stroke(cr);
        lyt=pango_cairo_create_layout(cr);
        pango_layout_set_font_description(lyt, plt->afont);
        lr2=priv->bounds.ymax-j*dely;
        lr1=round(lr2*exp(G_LN10*plot->ydp));
        if (lr2>DZE) {
          lr1+=0.1;
          lr1*=exp(-G_LN10*plot->ydp);
          if (lr1>=1) {
            lr2=log10(lr1);
            lr3=2;
            if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
            else lr3+=(guint) ceil(lr2);
            if ((plot->ydp)!=0) lr3+=plot->ydp+1;
            if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
            else g_snprintf(lbl, BFL, "%f", lr1);
          } else if (plot->ydp<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
          else g_snprintf(lbl, BFL, "%f", lr1);
        } else if (lr2<NZE) {
          lr1-=0.1;
          lr1*=exp(-G_LN10*plot->ydp);
          if (lr1<=-1) {
            lr2=log10(-lr1);
            lr3=3;
            if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
            else lr3+=(guint) ceil(lr2);
            if ((plot->ydp)!=0) lr3+=plot->ydp+1;
            if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
            else g_snprintf(lbl, BFL, "%f", lr1);
          } else if ((plot->ydp)<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
          else g_snprintf(lbl, BFL, "%f", lr1);
        } else g_snprintf(lbl, 2, "%f", 0.0);
        pango_layout_set_text(lyt, lbl, -1);
        pango_layout_get_pixel_size(lyt, &wd, &hg);
        cairo_move_to(cr, xa+JTI+hg, tn-wd/2);
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
    } else {
      cairo_move_to(cr, xa-dtt, (yl+yu+wd)/2);
      cairo_set_matrix(cr, &mtr2);
      pango_cairo_update_layout(cr, lyt);
      pango_cairo_show_layout(cr, lyt);
      g_object_unref(lyt);
      cairo_identity_matrix(cr);
      lyt=pango_cairo_create_layout(cr);
      pango_layout_set_font_description(lyt, plt->afont);
      lr1=round(priv->bounds.ymax*exp(G_LN10*plot->ydp));
      if (priv->bounds.ymax>DZE) {
        lr1+=0.1;
        lr1*=exp(-G_LN10*plot->ydp);
        if (lr1>=1) {
          lr2=log10(lr1);
          lr3=2;
          if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
          else lr3+=(guint) ceil(lr2);
          if ((plot->ydp)!=0) lr3+=plot->ydp+1;
          if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
          else g_snprintf(lbl, BFL, "%f", lr1);
        } else if ((plot->ydp)<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
        else g_snprintf(lbl, BFL, "%f", lr1);
      } else if (priv->bounds.ymax<NZE) {
        lr1-=0.1;
        lr1*=exp(-G_LN10*(plot->ydp));
        if (lr1<=-1) {
          lr2=log10(-lr1);
          lr3=3;
          if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
          else lr3+=(guint) ceil(lr2);
          if ((plot->ydp)!=0) lr3+=plot->ydp+1;
          if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
          else g_snprintf(lbl, BFL, "%f", lr1);
        } else if ((plot->ydp)<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
        else g_snprintf(lbl, BFL, "%f", lr1);
      } else g_snprintf(lbl, 2, "%f", 0.0);
      pango_layout_set_text(lyt, lbl, -1);
      pango_layout_get_pixel_size(lyt, &wd, &hg);
      cairo_move_to(cr, xa-JTI-hg, to+wd/2);
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
      for (j=1; j<=priv->ticks.yj; j++) {
        tn=yu+(yl-yu)*j/priv->ticks.yj;
        for (k=1; k<priv->ticks.yn; k++) {
          tnn=to+(tn-to)*k/priv->ticks.yn;
          cairo_move_to(cr, xa, tnn);
          cairo_line_to(cr, xa-NT, tnn);
        }
        cairo_stroke(cr);
        cairo_set_line_width(cr, 2);
        cairo_move_to(cr, xa, tn);
        cairo_line_to(cr, xa-JT, tn);
        cairo_stroke(cr);
        lyt=pango_cairo_create_layout(cr);
        pango_layout_set_font_description(lyt, plt->afont);
        lr2=priv->bounds.ymax-j*dely;
        lr1=round(lr2*exp(G_LN10*plot->ydp));
        if (lr2>DZE) {
          lr1+=0.1;
          lr1*=exp(-G_LN10*plot->ydp);
          if (lr1>=1) {
            lr2=log10(lr1);
            lr3=2;
            if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
            else lr3+=(guint) ceil(lr2);
            if ((plot->ydp)!=0) lr3+=plot->ydp+1;
            if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
            else g_snprintf(lbl, BFL, "%f", lr1);
          } else if (plot->ydp<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
          else g_snprintf(lbl, BFL, "%f", lr1);
        } else if (lr2<NZE) {
          lr1-=0.1;
          lr1*=exp(-G_LN10*plot->ydp);
          if (lr1<=-1) {
            lr2=log10(-lr1);
            lr3=3;
            if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
            else lr3+=(guint) ceil(lr2);
            if ((plot->ydp)!=0) lr3+=plot->ydp+1;
            if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
            else g_snprintf(lbl, BFL, "%f", lr1);
          } else if (plot->ydp<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
          else g_snprintf(lbl, BFL, "%f", lr1);
        } else g_snprintf(lbl, 2, "%f", 0.0);
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
  } else if (((priv->flaga)&GTK_PLOT_LOG_LINEAR_AXES_UW)!=0) {
    to=yu;
    if (((priv->flaga)&GTK_PLOT_LOG_LINEAR_AXES_LR)!=0) {
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
			for (j=1; j<=priv->ticks.yj; j++) {
				tn=yu+(yl-yu)*j/priv->ticks.yj;
				for (k=1; k<priv->ticks.yn; k++) {
					tnn=to+(tn-to)*k/priv->ticks.yn;
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa+NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa+JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, plt->afont);
				lr2=priv->bounds.ymax-j*dely;
				lr1=round(lr2*exp(G_LN10*plot->ydp));
				if (lr2>DZE) {
					lr1+=0.1;
					lr1*=exp(-G_LN10*plot->ydp);
					if (lr1>=1) {
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
						else lr3+=(guint) ceil(lr2);
						if ((plot->ydp)!=0) lr3+=plot->ydp+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					} else if ((plot->ydp)<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else if (lr2<NZE) {
					lr1-=0.1;
					lr1*=exp(-G_LN10*plot->ydp);
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
						else lr3+=(guint) ceil(lr2);
						if ((plot->ydp)!=0) lr3+=plot->ydp+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if (plot->ydp<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				}
				else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa+JTI+hg, tn-wd/2);
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
    } else {
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
			for (j=1; j<=priv->ticks.yj; j++) {
				tn=yu+(yl-yu)*j/priv->ticks.yj;
				for (k=1; k<priv->ticks.yn; k++) {
					tnn=to+(tn-to)*k/priv->ticks.yn;
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa-NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa-JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, plt->afont);
				lr2=priv->bounds.ymax-j*dely;
				lr1=round(lr2*exp(G_LN10*plot->ydp));
				if (lr2>DZE) {
					lr1+=0.1;
					lr1*=exp(-G_LN10*plot->ydp);
					if (lr1>=1) {
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
						else lr3+=(guint) ceil(lr2);
						if ((plot->ydp)!=0) lr3+=plot->ydp+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					}
					else if (plot->ydp<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else if (lr2<NZE) {
					lr1-=0.1;
					lr1*=exp(-G_LN10*plot->ydp);
					if (lr1<=-1)
					{
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
						else lr3+=(guint) ceil(lr2);
						if ((plot->ydp)!=0) lr3+=plot->ydp+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					} else if (plot->ydp<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else g_snprintf(lbl, 2, "%f", 0.0);
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
  } else if ((yu+yl)<=(2*ya)) {
    to=yu;
    tz=(ya+1-yu)*priv->ticks.yj/(yl-yu);
    if (tz==0) tf=yu;
    else tf=yu+(ya-yu)*priv->ticks.yj/tz;
    if (((priv->flaga)&GTK_PLOT_LOG_LINEAR_AXES_LR)!=0) {
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
			for (j=1; j<tz; j++) {
				tn=yu+(yl-yu)*j/priv->ticks.yj;
				for (k=1; k<priv->ticks.yn; k++) {
					tnn=to+(tn-to)*k/priv->ticks.yn;
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa+NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa+JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, plt->afont);
				lr2=priv->bounds.ymax-j*dely*(tf-yu)/(yl-yu);
				lr1=round(lr2*exp(G_LN10*plot->ydp));
				if (lr2>DZE) {
					lr1+=0.1;
					lr1*=exp(-G_LN10*plot->ydp);
					if (lr1>=1) {
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
						else lr3+=(guint) ceil(lr2);
						if ((plot->ydp)!=0) lr3+=plot->ydp+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					} else if (plot->ydp<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else if (lr2<NZE) {
					lr1-=0.1;
					lr1*=exp(-G_LN10*plot->ydp);
					if (lr1<=-1) {
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
						else lr3+=(guint) ceil(lr2);
						if ((plot->ydp)!=0) lr3+=plot->ydp+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					} else if (plot->ydp<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa+JTI+hg, tn-wd/2);
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
			tn=yu+(yl-yu)*tz/priv->ticks.yj;
			for (k=1; k<priv->ticks.yn; k++) {
				tnn=to+(tn-to)*k/priv->ticks.yn;
				cairo_move_to(cr, xa, tnn);
				cairo_line_to(cr, xa+NT, tnn);
			}
			cairo_stroke(cr);
			to=tn;
			for (j=tz+1; j<priv->ticks.yj; j++) {
				tn=yu+(yl-yu)*j/priv->ticks.yj;
				for (k=1; k<priv->ticks.yn; k++) {
					tnn=to+(tn-to)*k/priv->ticks.yn;
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa+NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa+JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, plt->afont);
				lr2=priv->bounds.ymax-j*dely*(tf-yu)/(yl-yu);
				lr1=round(lr2*exp(G_LN10*plot->ydp));
				if (lr2>DZE) {
					lr1+=0.1;
					lr1*=exp(-G_LN10*plot->ydp);
					if (lr1>=1) {
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
						else lr3+=(guint) ceil(lr2);
						if ((plot->ydp)!=0) lr3+=plot->ydp+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					} else if (plot->ydp<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else if (lr2<NZE) {
					lr1-=0.1;
					lr1*=exp(-G_LN10*plot->ydp);
					if (lr1<=-1) {
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
						else lr3+=(guint) ceil(lr2);
						if ((plot->ydp)!=0) lr3+=plot->ydp+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					} else if (plot->ydp<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa+JTI+hg, tn-wd/2);
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
      tnn=to+(tf-to)*k/priv->ticks.yn;
      for (k=1; (tnn<=yl)&&(k<=priv->ticks.yn); k++) {
        cairo_move_to(cr, xa, tnn);
        cairo_line_to(cr, xa+NT, tnn);
        tnn=to+(tf-to)*k/priv->ticks.yn;
      }
    } else {
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
			for (k=1; k<priv->ticks.yn; k++) {
				tnn=to+(((tn-to)*k)/(priv->ticks.yn));
				cairo_move_to(cr, xa, tnn);
				cairo_line_to(cr, xa-NT, tnn);
			}
			cairo_stroke(cr);
			to=tn;
			for (j=(tz+1); j<priv->ticks.yj; j++) {
				tn=yu+(yl-yu)*j/priv->ticks.yj;
				for (k=1; k<priv->ticks.yn; k++) {
					tnn=to+(tn-to)*k/priv->ticks.yn;
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa-NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa-JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, plt->afont);
				lr2=priv->bounds.ymax-j*dely*(tf-yu)/(yl-yu);
				lr1=round(lr2*exp(G_LN10*plot->ydp));
				if (lr2>DZE) {
					lr1+=0.1;
					lr1*=exp(-G_LN10*plot->ydp);
					if (lr1>=1) {
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
						else lr3+=(guint) ceil(lr2);
						if ((plot->ydp)!=0) lr3+=plot->ydp+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					} else if ((plot->ydp)<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else if (lr2<NZE) {
					lr1-=0.1;
					lr1*=exp(-G_LN10*plot->ydp);
					if (lr1<=-1) {
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
						else lr3+=(guint) ceil(lr2);
						if ((plot->ydp)!=0) lr3+=plot->ydp+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					} else if (plot->ydp<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa-JTI-hg, tn+wd/2);
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
      for (k=1; (tnn<=yl)&&(k<=priv->ticks.yn); k++) {
        cairo_move_to(cr, xa, tnn);
        cairo_line_to(cr, xa-NT, tnn);
        tnn=to+(tf-to)*k/priv->ticks.yn;
      }
    }
  } else {
    to=yl;
    tz=(yl+1-ya)*priv->ticks.yj/(yl-yu);
    if (tz==0) tf=yl;
    else tf=yl-(yl-ya)*priv->ticks.yj/tz;
    if (((priv->flaga)&GTK_PLOT_LOG_LINEAR_AXES_LR)!=0) {
			cairo_move_to(cr, xa+dtt, (yl+ya-wd)/2);
			cairo_set_matrix(cr, &mtr3);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			lyt=pango_cairo_create_layout(cr);
			pango_layout_set_font_description(lyt, plt->afont);
			lr1=round(priv->bounds.ymin*exp(G_LN10*plot->ydp));
			if ((priv->bounds.ymin)>DZE) {
				lr1+=0.1;
				lr1*=exp(-G_LN10*plot->ydp);
				if (lr1>=1) {
					{lr2=log10(lr1); lr3=2;}
					if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
					else lr3+=(guint) ceil(lr2);
					if ((plot->ydp)!=0) lr3+=plot->ydp+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else if (plot->ydp<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
				else g_snprintf(lbl, BFL, "%f", lr1);
			} else if (priv->bounds.ymin<NZE) {
				lr1-=0.1;
				lr1*=exp(-G_LN10*plot->ydp);
				if (lr1<=-1) {
					{lr2=log10(-lr1); lr3=3;}
					if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
					else lr3+=(guint) ceil(lr2);
					if ((plot->ydp)!=0) lr3+=plot->ydp+1;
					if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else if (plot->ydp<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
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
			for (j=1; j<tz; j++) {
				tn=yl-(yl-yu)*j/priv->ticks.yj;
				for (k=1; k<priv->ticks.yn; k++) {
					tnn=to-(to-tn)*k/priv->ticks.yn;
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa+NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa+JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, plt->afont);
				lr2=priv->bounds.ymin+j*dely*(yl-tf)/(yl-yu);
				lr1=round(lr2*exp(G_LN10*plot->ydp));
				if (lr2>DZE) {
					lr1+=0.1;
					lr1*=exp(-G_LN10*(plot->ydp));
					if (lr1>=1)
					{
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
						else lr3+=(guint) ceil(lr2);
						if ((plot->ydp)!=0) lr3+=plot->ydp+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					} else if (plot->ydp<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else if (lr2<NZE) {
					lr1-=0.1;
					lr1*=exp(-G_LN10*plot->ydp);
					if (lr1<=-1) {
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
						else lr3+=(guint) ceil(lr2);
						if ((plot->ydp)!=0) lr3+=plot->ydp+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					} else if (plot->ydp<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa+JTI+hg, tn-wd/2);
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
			tn=yl-(yl-yu)*tz/priv->ticks.yj;
			for (k=1; k<priv->ticks.yn; k++) {
				tnn=to-(to-tn)*k/priv->ticks.yn;
				cairo_move_to(cr, xa, tnn);
				cairo_line_to(cr, xa+NT, tnn);
			}
			cairo_stroke(cr);
			to=tn;
			for (j=tz+1; j<priv->ticks.yj; j++) {
				tn=yl-(yl-yu)*j/priv->ticks.yj;
				for (k=1; k<priv->ticks.yn; k++) {
					tnn=to-(to-tn)*k/priv->ticks.yn;
					cairo_move_to(cr, xa, tnn);
					cairo_line_to(cr, xa+NT, tnn);
				}
				cairo_stroke(cr);
				cairo_set_line_width(cr, 2);
				cairo_move_to(cr, xa, tn);
				cairo_line_to(cr, xa+JT, tn);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, plt->afont);
				lr2=priv->bounds.ymin+j*dely*(yl-tf)/(yl-yu);
				lr1=round(lr2*exp(G_LN10*plot->ydp));
				if (lr2>DZE) {
					lr1+=0.1;
					lr1*=exp(-G_LN10*plot->ydp);
					if (lr1>=1) {
						{lr2=log10(lr1); lr3=2;}
						if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
						else lr3+=(guint) ceil(lr2);
						if ((plot->ydp)!=0) lr3+=plot->ydp+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					} else if (plot->ydp<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else if (lr2<NZE) {
					lr1-=0.1;
					lr1*=exp(-G_LN10*plot->ydp);
					if (lr1<=-1) {
						{lr2=log10(-lr1); lr3=3;}
						if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
						else lr3+=(guint) ceil(lr2);
						if ((plot->ydp)!=0) lr3+=plot->ydp+1;
						if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
						else g_snprintf(lbl, BFL, "%f", lr1);
					} else if (plot->ydp<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
					else g_snprintf(lbl, BFL, "%f", lr1);
				} else g_snprintf(lbl, 2, "%f", 0.0);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				cairo_move_to(cr, xa+JTI+hg, tn-wd/2);
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
      tnn=to-(to-tf)*k/priv->ticks.yn;
      for (k=1; (tnn>=yu)&&(k<=priv->ticks.yn); k++) {
        cairo_move_to(cr, xa, tnn);
        cairo_line_to(cr, xa+NT, tnn);
        tnn=to-(to-tf)*k/priv->ticks.yn;
      }
    } else {
      cairo_move_to(cr, xa-dtt, (yl+ya+wd)/2);
      cairo_set_matrix(cr, &mtr2);
      pango_cairo_update_layout(cr, lyt);
      pango_cairo_show_layout(cr, lyt);
      g_object_unref(lyt);
      cairo_identity_matrix(cr);
      lyt=pango_cairo_create_layout(cr);
      pango_layout_set_font_description(lyt, plt->afont);
      lr1=round(priv->bounds.ymin*exp(G_LN10*plot->ydp));
      if (priv->bounds.ymin>DZE) {
        lr1+=0.1;
        lr1*=exp(-G_LN10*plot->ydp);
        if (lr1>=1) {
          lr2=log10(lr1);
          lr3=2;
          if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
          else lr3+=(guint) ceil(lr2);
          if ((plot->ydp)!=0) lr3+=plot->ydp+1;
          if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
          else g_snprintf(lbl, BFL, "%f", lr1);
        } else if (plot->ydp<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
        else g_snprintf(lbl, BFL, "%f", lr1);
      } else if (priv->bounds.ymin<NZE) {
        lr1-=0.1;
        lr1*=exp(-G_LN10*plot->ydp);
        if (lr1<=-1) {
          lr2=log10(-lr1);
          lr3=3;
          if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
          else lr3+=(guint) ceil(lr2);
          if ((plot->ydp)!=0) lr3+=plot->ydp+1;
          if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
          else g_snprintf(lbl, BFL, "%f", lr1);
        } else if (plot->ydp<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
        else g_snprintf(lbl, BFL, "%f", lr1);
      } else g_snprintf(lbl, 2, "%f", 0.0);
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
      for (j=1; j<tz; j++) {
        tn=yl-(yl-yu)*j/priv->ticks.yj;
        for (k=1; k<priv->ticks.yn; k++) {
          tnn=to-(to-tn)*k/priv->ticks.yn;
          cairo_move_to(cr, xa, tnn);
          cairo_line_to(cr, xa-NT, tnn);
        }
        cairo_stroke(cr);
        cairo_set_line_width(cr, 2);
        cairo_move_to(cr, xa, tn);
        cairo_line_to(cr, xa-JT, tn);
        cairo_stroke(cr);
        lyt=pango_cairo_create_layout(cr);
        pango_layout_set_font_description(lyt, plt->afont);
        lr2=priv->bounds.ymin+j*dely*(yl-tf)/(yl-yu);
        lr1=round(lr2*exp(G_LN10*plot->ydp));
        if (lr2>DZE) {
          lr1+=0.1;
          lr1*=exp(-G_LN10*plot->ydp);
          if (lr1>=1) {
            lr2=log10(lr1);
            lr3=2;
            if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
            else lr3+=(guint) ceil(lr2);
            if ((plot->ydp)!=0) lr3+=plot->ydp+1;
            if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
            else g_snprintf(lbl, BFL, "%f", lr1);
          } else if (plot->ydp<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
          else g_snprintf(lbl, BFL, "%f", lr1);
        } else if (lr2<NZE) {
          lr1-=0.1;
          lr1*=exp(-G_LN10*plot->ydp);
          if (lr1<=-1) {
            lr2=log10(-lr1);
            lr3=3;
            if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
            else lr3+=(guint) ceil(lr2);
            if ((plot->ydp)!=0) lr3+=plot->ydp+1;
            if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
            else g_snprintf(lbl, BFL, "%f", lr1);
          } else if (plot->ydp<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
          else g_snprintf(lbl, BFL, "%f", lr1);
        } else g_snprintf(lbl, 2, "%f", 0.0);
        pango_layout_set_text(lyt, lbl, -1);
        pango_layout_get_pixel_size(lyt, &wd, &hg);
        cairo_move_to(cr, xa-JTI-hg, tn-wd/2);
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
      tn=yl-(yl-yu)*tz/priv->ticks.yj;
      for (k=1; k<priv->ticks.yn; k++) {
        tnn=to-(to-tn)*k/priv->ticks.yn;
        cairo_move_to(cr, xa, tnn);
        cairo_line_to(cr, xa-NT, tnn);
      }
      cairo_stroke(cr);
      to=tn;
      for (j=tz+1; j<priv->ticks.yj; j++) {
        tn=yl-(yl-yu)*j/priv->ticks.yj;
        for (k=1; k<priv->ticks.yn; k++) {
          tnn=to-(to-tn)*k/priv->ticks.yn;
          cairo_move_to(cr, xa, tnn);
          cairo_line_to(cr, xa-NT, tnn);
        }
        cairo_stroke(cr);
        cairo_set_line_width(cr, 2);
        cairo_move_to(cr, xa, tn);
        cairo_line_to(cr, xa-JT, tn);
        cairo_stroke(cr);
        lyt=pango_cairo_create_layout(cr);
        pango_layout_set_font_description(lyt, plt->afont);
        lr2=priv->bounds.ymin+j*dely*(yl-tf)/(yl-yu);
        lr1=round(lr2*exp(G_LN10*plot->ydp));
        if (lr2>DZE) {
          lr1+=0.1;
          lr1*=exp(-G_LN10*plot->ydp);
          if (lr1>=1) {
            lr2=log10(lr1);
            lr3=2;
            if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
            else lr3+=(guint) ceil(lr2);
            if ((plot->ydp)!=0) lr3+=plot->ydp+1;
            if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
            else g_snprintf(lbl, BFL, "%f", lr1);
          } else if (plot->ydp<BF3) g_snprintf(lbl, plot->ydp+3, "%f", lr1);
          else g_snprintf(lbl, BFL, "%f", lr1);
        } else if (lr2<NZE) {
          lr1-=0.1;
          lr1*=exp(-G_LN10*plot->ydp);
          if (lr1<=-1) {
            lr2=log10(-lr1);
            lr3=3;
            if (fmod(lr2,1)<NAC) lr3+=(guint) lr2;
            else lr3+=(guint) ceil(lr2);
            if ((plot->ydp)!=0) lr3+=plot->ydp+1;
            if (lr3<BFL) g_snprintf(lbl, lr3, "%f", lr1);
            else g_snprintf(lbl, BFL, "%f", lr1);
          } else if (plot->ydp<BF4) g_snprintf(lbl, plot->ydp+4, "%f", lr1);
          else g_snprintf(lbl, BFL, "%f", lr1);
        } else g_snprintf(lbl, 2, "%f", 0.0);
        pango_layout_set_text(lyt, lbl, -1);
        pango_layout_get_pixel_size(lyt, &wd, &hg);
        cairo_move_to(cr, xa-JTI-hg, tn-wd/2);
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
      tnn=to-(to-tf)*k/priv->ticks.yn;
      for (k=1; (tnn>=yu)&&(k<=priv->ticks.yn); k++) {
        cairo_move_to(cr, xa, tnn);
        cairo_line_to(cr, xa-NT, tnn);
        tnn=to-(to-tf)*k/priv->ticks.yn;
      }
    }
  }
  cairo_stroke(cr);
  if ((plot->xdata)&&(plot->ydata)) { /* plot data */
    if (((plot->flagd)&GTK_PLOT_LOG_LINEAR_DISP_LIN)!=0) {
      cairo_set_line_width(cr, (plot->linew));
      if (((plot->flagd)&GTK_PLOT_LOG_LINEAR_DISP_PTS)!=0) { /* lines and points */
        for (k=0; k<plt->ind->len; k++) {
          cairo_set_dash(cr, NULL, 0, 0);
          if (plot->kdata && k<(plot->kdata->len)) {
            fg=g_array_index(plot->kdata, guint, k);
            if (fg&GTK_PLOT_LOG_LINEAR_KEY_CLR) continue;
            switch ((fg&GTK_PLOT_LOG_LINEAR_KEY_DSH)>>1) {
              case 2:
                cairo_set_dash(cr, dsh2, sizeof(dsh2)/sizeof(dsh2[0]), 0);
                break;
              case 1:
                cairo_set_dash(cr, dsh1, sizeof(dsh1)/sizeof(dsh1[0]), 0);
                break;
              default:
                break;
            }
          }
          dtt=fmod(k,plt->rd->len);
          vv=g_array_index(plt->rd, gdouble, dtt);
          wv=g_array_index(plt->gr, gdouble, dtt);
          zv=g_array_index(plt->bl, gdouble, dtt);
          av=g_array_index(plt->al, gdouble, dtt);
          cairo_set_source_rgba(cr, vv, wv, zv, av);
          ft=g_array_index(plt->ind, gint, k);
          if (ft>=(plot->ydata->len)) break;
          st=g_array_index(plt->stride, gint, k);
          lt=g_array_index(plt->sizes, gint, k)*st+ft;
          if (lt>(plot->ydata->len)) lt=plot->ydata->len;
          if (g_array_index(plot->xdata, gdouble, ft)<DZE) xv=xl-1.0;
          else xv=xl+(xu-xl)*(log10(g_array_index(plot->xdata, gdouble, ft))-priv->bounds.xmin)/(priv->bounds.xmax-priv->bounds.xmin);
          yv=yl+(yu-yl)*(g_array_index(plot->ydata, gdouble, ft)-priv->bounds.ymin)/(priv->bounds.ymax-priv->bounds.ymin);
          if (xv<xl) {
            if (yv>yl) xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT|GTK_PLOT_LOG_LINEAR_BORDERS_DN;
            else if (yv<yu) xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT|GTK_PLOT_LOG_LINEAR_BORDERS_UP;
            else xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT;
          } else if (xv>xu) {
            if (yv>yl) xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT|GTK_PLOT_LOG_LINEAR_BORDERS_DN;
            else if (yv<yu) xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT|GTK_PLOT_LOG_LINEAR_BORDERS_DN;
            else xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT;
          } else if (yv>yl) xt=GTK_PLOT_LOG_LINEAR_BORDERS_DN;
          else if (yv<yu) xt=GTK_PLOT_LOG_LINEAR_BORDERS_UP;
          else {
            xt=0;
            cairo_arc(cr, xv, yv, plot->ptsize, 0, MY_2PI);
            cairo_fill(cr);
            cairo_move_to(cr, xv, yv);
          }
          for (j=st+ft; j<lt; j+=st) {
            if (g_array_index(plot->xdata, gdouble, j)<DZE) xvn=xl-1.0; 
            else xvn=xl+(xu-xl)*(log10(g_array_index(plot->xdata, gdouble, j))-priv->bounds.xmin)/(priv->bounds.xmax-priv->bounds.xmin);
            yvn=yl+(yu-yl)*(g_array_index(plot->ydata, gdouble, j)-priv->bounds.ymin)/(priv->bounds.ymax-priv->bounds.ymin);
            if (xvn<xl) {
              if (yvn>yl) {
                if (xt==0) {
                  tx=yvn+(yvn-yv)*(xl-xvn)/(xvn-xv);
                  if (tx>yl) cairo_line_to(cr, xvn+(xvn-xv)*(yl-yvn)/(yvn-yv), yl);
                  else cairo_line_to(cr, xl, tx);
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT|GTK_PLOT_LOG_LINEAR_BORDERS_DN;
              } else if (yvn<yu) {
                if (xt==0) {
                  tx=yvn+(yvn-yv)*(xl-xvn)/(xvn-xv);
                  if (tx<yu) cairo_line_to(cr, xvn+(xvn-xv)*(yu-yvn)/(yvn-yv), yu);
                  else cairo_line_to(cr, xl, tx);
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT|GTK_PLOT_LOG_LINEAR_BORDERS_UP;
              } else if (xt==0) {
                cairo_line_to(cr, xl, yvn+(yvn-yv)*(xl-xvn)/(xvn-xv));
                cairo_stroke(cr);
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT;
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_DN) {
                tx=yvn+(yvn-yv)*(xl-xvn)/(xvn-xv);
                if (tx>yl) {
                  cairo_move_to(cr, xl, tx);
                  cairo_line_to(cr, xv+(xvn-xv)*(yl-yv)/(yvn-yv), yl);
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT;
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_UP) {
                tx=yvn+(yvn-yv)*(xl-xvn)/(xvn-xv);
                if (tx<yu) {
                  cairo_move_to(cr, xl, tx);
                  cairo_line_to(cr, xv+(xvn-xv)*(yu-yv)/(yvn-yv), yu);
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT;
              }
              else xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT;
            } else if (xvn>xu) {
              if (yvn>yl) {
                if (xt==0) {
                  tx=yv+(yvn-yv)*(xu-xv)/(xvn-xv);
                  if (tx>yl) cairo_line_to(cr, xv+(xvn-xv)*(yl-yv)/(yvn-yv), yl);
                  else cairo_line_to(cr, xu, tx);
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT|GTK_PLOT_LOG_LINEAR_BORDERS_DN;
              } else if (yvn<yu) {
                if (xt==0) {
                  tx=yv+(yvn-yv)*(xu-xv)/(xvn-xv);
                  if (tx<yu) cairo_line_to(cr, xv+(xvn-xv)*(yu-yv)/(yvn-yv), yu);
                  else cairo_line_to(cr, xu, tx);
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT|GTK_PLOT_LOG_LINEAR_BORDERS_UP;
              }
              if (xt==0) {
                cairo_line_to(cr, xu, yv+(yvn-yv)*(xu-xv)/(xvn-xv));
                cairo_stroke(cr);
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT;
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_DN) {
                tx=yv+(yvn-yv)*(xu-xv)/(xvn-xv);
                if (tx>yl) {
                  cairo_move_to(cr, xu, tx);
                  cairo_line_to(cr, xv+(xvn-xv)*(yl-yv)/(yvn-yv), yl);
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT;
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_UP) {
                tx=yv+(yvn-yv)*(xu-xv)/(xvn-xv);
                if (tx>yu) {
                  cairo_move_to(cr, xu, tx);
                  cairo_line_to(cr, xv+(xvn-xv)*(yu-yv)/(yvn-yv), yu);
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT;
              } else xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT;
            } else if (yvn>yl) {
              if (xt==0) {
                cairo_line_to(cr, xvn+(xvn-xv)*(yl-yvn)/(yvn-yv), yl);
                cairo_stroke(cr);
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_LT) {
                tx=xvn+(xvn-xv)*(yl-yvn)/(yvn-yv);
                if (tx<xl) {
                  cairo_move_to(cr, tx, yl);
                  cairo_line_to(cr, xl, yv+(yvn-yv)*(xl-xv)/(xvn-xv));
                  cairo_stroke(cr);
                }
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_RT) {
                tx=xvn+(xvn-xv)*(yl-yvn)/(yvn-yv);/* SIGFPE here. change? */
                if (tx>xu) {
                  cairo_move_to(cr, tx, yl);
                  cairo_line_to(cr, xu, yvn+(yvn-yv)*(xu-xvn)/(xvn-xv));
                  cairo_stroke(cr);
                }
              }
              xt=GTK_PLOT_LOG_LINEAR_BORDERS_DN;
            } else if (yvn<yu) {
              if (xt==0) {
                cairo_line_to(cr, xvn+(xvn-xv)*(yu-yvn)/(yvn-yv), yu);	
                cairo_stroke(cr);
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_LT) {
                tx=xvn+(xvn-xv)*(yu-yvn)/(yvn-yv);
                if (tx<xl) {
                  cairo_move_to(cr, tx, yu);
                  cairo_line_to(cr, xl, yv+(yvn-yv)*(xl-xv)/(xvn-xv));
                  cairo_stroke(cr);
                }
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_RT) {
                tx=xvn+(xvn-xv)*(yu-yvn)/(yvn-yv);
                if (tx>xu) {
                  cairo_move_to(cr, tx, yu);
                  cairo_line_to(cr, xu, yvn+(yvn-yv)*(xu-xvn)/(xvn-xv));
                  cairo_stroke(cr);
                }
              }
              xt=GTK_PLOT_LOG_LINEAR_BORDERS_UP;
            } else { /* within range */
              if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_LT)!=0) {
                if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_DN)!=0) {
                  tx=yv+(yvn-yv)*(xl-xv)/(xvn-xv);
                  if (tx>yl) cairo_move_to(cr, xv+(xvn-xv)*(yl-yv)/(yvn-yv), yl);
                  else cairo_move_to(cr, xl, tx);
                } else if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_UP)!=0) {
                  tx=yv+(yvn-yv)*(xl-xv)/(xvn-xv);
                  if (tx<yu) cairo_move_to(cr, xv+(xvn-xv)*(yu-yv)/(yvn-yv), yu);
                  else cairo_move_to(cr, xl, tx);
                } else cairo_move_to(cr, xl, yv+(yvn-yv)*(xl-xv)/(xvn-xv));
              } else if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_RT)!=0) {
                if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_DN)!=0) {
                  tx=yvn+(yvn-yv)*(xu-xvn)/(xvn-xv);
                  if (tx>yl) cairo_move_to(cr, xvn+(xvn-xv)*(yl-yvn)/(yvn-yv), yl);
                  else cairo_move_to(cr, xl, tx);
                } else if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_DN)!=0) {
                  tx=yvn+(yvn-yv)*(xu-xvn)/(xvn-xv);
                  if (tx<yu) cairo_move_to(cr, xvn+(xvn-xv)*(yu-yvn)/(yvn-yv), yu);
                  else cairo_move_to(cr, xl, tx);
                } else cairo_move_to(cr, xu, yvn+(yvn-yv)*(xu-xvn)/(xvn-xv));
              } else if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_DN)!=0) cairo_move_to(cr, xv+(xvn-xv)*(yl-yv)/(yvn-yv), yl);
              else if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_UP)!=0) cairo_move_to(cr, xv+(xvn-xv)*(yu-yv)/(yvn-yv), yu);
              cairo_line_to(cr, xvn, yvn);
              cairo_stroke(cr);
              cairo_arc(cr, xvn, yvn, plot->ptsize, 0, MY_2PI);
              cairo_fill(cr);
              cairo_move_to(cr, xvn, yvn);
              xt=0;
            }
            xv=xvn;
            yv=yvn;
          }
        }
      } else { /* lines only */
        for (k=0;k<(plt->ind->len);k++) {
          cairo_set_dash(cr, NULL, 0, 0);
          if (plot->kdata && k<(plot->kdata->len)) {
            fg=g_array_index(plot->kdata, guint, k);
            if (fg&GTK_PLOT_LOG_LINEAR_KEY_CLR) continue;
            switch ((fg&GTK_PLOT_LOG_LINEAR_KEY_DSH)>>1) {
              case 2:
                cairo_set_dash(cr, dsh2, sizeof(dsh2)/sizeof(dsh2[0]), 0);
                break;
              case 1:
                cairo_set_dash(cr, dsh1, sizeof(dsh1)/sizeof(dsh1[0]), 0);
                break;
              default:
                break;
            }
          }
          dtt=fmod(k,plt->rd->len);
          vv=g_array_index(plt->rd, gdouble, dtt);
          wv=g_array_index(plt->gr, gdouble, dtt);
          zv=g_array_index(plt->bl, gdouble, dtt);
          av=g_array_index(plt->al, gdouble, dtt);
          cairo_set_source_rgba(cr, vv, wv, zv, av);
          ft=g_array_index(plt->ind, gint, k);
          if (ft>=plot->ydata->len) break;
          st=g_array_index(plt->stride, gint, k);
          lt=g_array_index(plt->sizes, gint, k)*st+ft;
          if (lt>plot->ydata->len) lt=plot->ydata->len;
          if (g_array_index(plot->xdata, gdouble, ft)<DZE) xv=xl-1.0;
          else xv=xl+(xu-xl)*(log10(g_array_index(plot->xdata, gdouble, ft))-priv->bounds.xmin)/(priv->bounds.xmax-priv->bounds.xmin);/*segfault here when adding a plot*/
          yv=yl+(yu-yl)*(g_array_index(plot->ydata, gdouble, ft)-priv->bounds.ymin)/(priv->bounds.ymax-priv->bounds.ymin);
          if (xv<xl) {
            if (yv>yl) xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT|GTK_PLOT_LOG_LINEAR_BORDERS_DN;
            else if (yv<yu) xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT|GTK_PLOT_LOG_LINEAR_BORDERS_UP;
            else xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT;
          } else if (xv>xu) {
            if (yv>yl) xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT|GTK_PLOT_LOG_LINEAR_BORDERS_DN;
            else if (yv<yu) xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT|GTK_PLOT_LOG_LINEAR_BORDERS_UP;
            else xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT;
          } else if (yv>yl)xt=GTK_PLOT_LOG_LINEAR_BORDERS_DN;
          else if (yv<yu) xt=GTK_PLOT_LOG_LINEAR_BORDERS_UP;
          else {
            xt=0;
            cairo_move_to(cr, xv, yv);
          }
          for (j=st+ft; j<lt; j+=st) {
            if (g_array_index(plot->xdata, gdouble, j)<DZE) xvn=xl-1.0;
            else xvn=xl+(xu-xl)*(log10(g_array_index(plot->xdata, gdouble, j))-priv->bounds.xmin)/(priv->bounds.xmax-priv->bounds.xmin);
            yvn=yl+(yu-yl)*(g_array_index(plot->ydata, gdouble, j)-priv->bounds.ymin)/(priv->bounds.ymax-priv->bounds.ymin);
            if (xvn<xl) {
              if (yvn>yl) {
                if (xt==0) {
                  tx=xvn-xv;
                  if ((tx<DZE)&&(tx>NZE)) cairo_line_to(cr, xvn, yl);
                  else {
                    tx=yvn+(yvn-yv)*(xl-xvn)/tx;
                    if (tx>yl) cairo_line_to(cr, xvn+(xvn-xv)*(yl-yvn)/(yvn-yv), yl);
                    else cairo_line_to(cr, xl, tx);
                  }
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT|GTK_PLOT_LOG_LINEAR_BORDERS_DN;
              } else if (yvn<yu) {
                if (xt==0) {
                  tx=xvn-xv;
                  if ((tx<DZE)&&(tx>NZE)) cairo_line_to(cr, xvn, yu);
                  else {
                    tx=yvn+(yvn-yv)*(xl-xvn)/tx;
                    if (tx<yu) cairo_line_to(cr, xvn+(xvn-xv)*(yu-yvn)/(yvn-yv), yu);
                    else cairo_line_to(cr, xl, tx);
                  }
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT|GTK_PLOT_LOG_LINEAR_BORDERS_UP;
              } else if (xt==0) {
                tx=xvn-xv;
                if ((tx<DZE)&&(tx>NZE)) cairo_line_to(cr, xl, yvn);
                else cairo_line_to(cr, xl, yvn+(yvn-yv)*(xl-xvn)/tx);
                cairo_stroke(cr);
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT;
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_DN) {
                tx=yvn+(yvn-yv)*(xl-xvn)/(xvn-xv);
                if (tx>yl) {
                  cairo_move_to(cr, xl, tx);
                  cairo_line_to(cr, xv+(xvn-xv)*(yl-yv)/(yvn-yv), yl);
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT;
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_UP) {
                tx=yvn+(yvn-yv)*(xl-xvn)/(xvn-xv);
                if (tx<yu) {
                  cairo_move_to(cr, xl, tx);
                  cairo_line_to(cr, xv+(xvn-xv)*(yu-yv)/(yvn-yv), yu);
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT;
              } else xt=GTK_PLOT_LOG_LINEAR_BORDERS_LT;
            } else if (xvn>xu) {
              if (yvn>yl) {
                if (xt==0) {
                  tx=yv+(yvn-yv)*(xu-xv)/(xvn-xv);
                  if (tx>yl) cairo_line_to(cr, xv+(xvn-xv)*(yl-yv)/(yvn-yv), yl);
                  else cairo_line_to(cr, xu, tx);
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT|GTK_PLOT_LOG_LINEAR_BORDERS_DN;
              } else if (yvn<yu) {
                if (xt==0) {
                  tx=yv+(yvn-yv)*(xu-xv)/(xvn-xv);
                  if (tx<yu) cairo_line_to(cr, xv+(xvn-xv)*(yu-yv)/(yvn-yv), yu);
                  else cairo_line_to(cr, xu, tx);
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT|GTK_PLOT_LOG_LINEAR_BORDERS_UP;
              }
              if (xt==0) {
                cairo_line_to(cr, xu, yv+(yvn-yv)*(xu-xv)/(xvn-xv));
                cairo_stroke(cr);
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT;
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_DN) {
                tx=yv+(yvn-yv)*(xu-xv)/(xvn-xv);
                if (tx>yl) {
                  cairo_move_to(cr, xu, tx);
                  cairo_line_to(cr, xv+(xvn-xv)*(yl-yv)/(yvn-yv), yl);
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT;
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_UP) {
                tx=yv+(yvn-yv)*(xu-xv)/(xvn-xv);
                if (tx>yu) {
                  cairo_move_to(cr, xu, tx);
                  cairo_line_to(cr, xv+(xvn-xv)*(yu-yv)/(yvn-yv), yu);
                  cairo_stroke(cr);
                }
                xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT;
              } else xt=GTK_PLOT_LOG_LINEAR_BORDERS_RT;
            } else if (yvn>yl) {
              if (xt==0) {
                cairo_line_to(cr, xvn+(xvn-xv)*(yl-yvn)/(yvn-yv), yl);
                cairo_stroke(cr);
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_LT) {
                tx=xvn+(xvn-xv)*(yl-yvn)/(yvn-yv);
                if (tx<xl) {
                  cairo_move_to(cr, tx, yl);
                  cairo_line_to(cr, xl, yv+(yvn-yv)*(xl-xv)/(xvn-xv));
                  cairo_stroke(cr);
                }
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_RT) {
                tx=xvn+(xvn-xv)*(yl-yvn)/(yvn-yv);
                if (tx>xu) {
                  cairo_move_to(cr, tx, yl);
                  cairo_line_to(cr, xu, yvn+(yvn-yv)*(xu-xvn)/(xvn-xv));
                  cairo_stroke(cr);
                }
              }
              xt=GTK_PLOT_LOG_LINEAR_BORDERS_DN;
            } else if (yvn<yu) {
              if (xt==0) {
                cairo_line_to(cr, xvn+(xvn-xv)*(yu-yvn)/(yvn-yv), yu);	
                cairo_stroke(cr);
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_LT) {
                tx=xvn+(xvn-xv)*(yu-yvn)/(yvn-yv);
                if (tx<xl) {
                  cairo_move_to(cr, tx, yu);
                  cairo_line_to(cr, xl, yv+(yvn-yv)*(xl-xv)/(xvn-xv));
                  cairo_stroke(cr);
                }
              } else if (xt==GTK_PLOT_LOG_LINEAR_BORDERS_RT) {
                tx=xvn+(xvn-xv)*(yu-yvn)/(yvn-yv);
                if (tx>xu) {
                  cairo_move_to(cr, tx, yu);
                  cairo_line_to(cr, xu, yvn+(yvn-yv)*(xu-xvn)/(xvn-xv));
                  cairo_stroke(cr);
                }
              }
              xt=GTK_PLOT_LOG_LINEAR_BORDERS_UP;
            } else { /* within range */
              if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_LT)!=0) {
                if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_DN)!=0) {
                  tx=yv+(yvn-yv)*(xl-xv)/(xvn-xv);
                  if (tx>yl) cairo_move_to(cr, xv+(xvn-xv)*(yl-yv)/(yvn-yv), yl);
                  else cairo_move_to(cr, xl, tx);
                } else if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_UP)!=0) {
                  tx=yv+(yvn-yv)*(xl-xv)/(xvn-xv);
                  if (tx<yu) cairo_move_to(cr, xv+(xvn-xv)*(yu-yv)/(yvn-yv), yu);
                  else cairo_move_to(cr, xl, tx);
                } else cairo_move_to(cr, xl, yv+(yvn-yv)*(xl-xv)/(xvn-xv));
              } else if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_RT)!=0) {
                if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_DN)!=0) {
                  tx=yvn+(yvn-yv)*(xu-xvn)/(xvn-xv);
                  if (tx>yl) cairo_move_to(cr, xvn+(xvn-xv)*(yl-yvn)/(yvn-yv), yl);
                  else cairo_move_to(cr, xl, tx);
                } else if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_UP)!=0) {
                  tx=yvn+(yvn-yv)*(xu-xvn)/(xvn-xv);
                  if (tx<yu) cairo_move_to(cr, xvn+(xvn-xv)*(yu-yvn)/(yvn-yv), yu);
                  else cairo_move_to(cr, xl, tx);
                } else cairo_move_to(cr, xu, yvn+(yvn-yv)*(xu-xvn)/(xvn-xv));
              } else if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_DN)!=0) cairo_move_to(cr, xv+(xvn-xv)*(yl-yv)/(yvn-yv), yl);
              else if ((xt&GTK_PLOT_LOG_LINEAR_BORDERS_UP)!=0) cairo_move_to(cr, xv+(xvn-xv)*(yu-yv)/(yvn-yv), yu);
              cairo_line_to(cr, xvn, yvn);
              xt=0;
            }
            xv=xvn;
            yv=yvn;
          }
          cairo_stroke(cr);
        }
      }
    } else if (((plot->flagd)&GTK_PLOT_LOG_LINEAR_DISP_PTS)!=0) { /* points only */
      for (k=0;k<(plt->ind->len);k++) {
        if (plot->kdata && k<(plot->kdata->len) && (g_array_index(plot->kdata, guint, k)&GTK_PLOT_LOG_LINEAR_KEY_CLR)) continue;
        dtt=fmod(k,plt->rd->len);
        vv=g_array_index(plt->rd, gdouble, dtt);
        wv=g_array_index(plt->gr, gdouble, dtt);
        zv=g_array_index(plt->bl, gdouble, dtt);
        av=g_array_index(plt->al, gdouble, dtt);
        cairo_set_source_rgba(cr, vv, wv, zv, av);
        ft=g_array_index((plt->ind), gint, k);
        if (ft>=(plot->ydata->len)) break;
        st=g_array_index(plt->stride, gint, k);
        lt=g_array_index(plt->sizes, gint, k)*st+ft;
        if (lt>(plot->ydata->len)) lt=plot->ydata->len;
        for (j=ft; j<lt; j+=st) {
          if (g_array_index(plot->xdata, gdouble, j)<DZE) continue;
          xv=xl+(xu-xl)*(log10(g_array_index(plot->xdata, gdouble, j))-priv->bounds.xmin)/(priv->bounds.xmax-priv->bounds.xmin);
          yv=yl+(yu-yl)*(g_array_index(plot->ydata, gdouble, j)-priv->bounds.ymin)/(priv->bounds.ymax-priv->bounds.ymin);
          if ((yv<=yl)&&(yv>=yu)&&(xv>=xl)&&(xv<=xu)) {
            cairo_arc(cr, xv, yv, plot->ptsize, 0, MY_2PI);
            cairo_fill(cr);
          }
        }
      }
    }
  }
}

static void gtk_plot_log_linear_redraw(GtkWidget *widget) {
  GdkRegion *region;
  GdkWindow *wdw;

  wdw=gtk_widget_get_window(widget);
  if (!wdw) return;
  region=gdk_drawable_get_clip_region(wdw);
  gdk_window_invalidate_region(wdw, region, TRUE);
  gdk_window_process_updates(wdw, TRUE);
  gdk_region_destroy(region);
}

gboolean gtk_plot_log_linear_update_scale(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx) {
  GtkPlotLogLinearPrivate *priv;

  priv=GTK_PLOT_LOG_LINEAR_GET_PRIVATE(widget);
  priv->bounds.xmin=xn;
  priv->bounds.xmax=xx;
  priv->bounds.ymin=yn;
  priv->bounds.ymax=yx;
  gtk_plot_log_linear_redraw(widget);
  return FALSE;
}

gboolean gtk_plot_log_linear_update_scale_pretty(GtkWidget *widget, gdouble xl, gdouble xu, gdouble yl, gdouble yu) {
  GtkPlotLogLinearPrivate *priv;
  GtkPlotLogLinear *plot;
  gdouble num, num3, xn, xx, yn, yx;
  gint num2, lt, ut, tk;

  if (xl>xu) {
    xn=xu;
    xx=xl;
  } else {
    xn=xl;
    xx=xu;
  }
  if (xx<DZE) return TRUE;
  plot=GTK_PLOT_LOG_LINEAR(widget);
  priv=GTK_PLOT_LOG_LINEAR_GET_PRIVATE(widget);
  ut=(gint) ceil(log10(xx));
  if (xn<=DZE) {
    lt=ut-6;
    priv->ticks.xj=6;
  } else {
    lt=(gint) floor(log10(xn));
    tk=ut-lt;
    if (tk<=0) priv->ticks.xj=1;
    else if (tk>6) {
      priv->ticks.xj=6;
      lt=ut-6;
    } else priv->ticks.xj=tk;
  }
  if (priv->ticks.xj<4) priv->ticks.xn=8;
  else priv->ticks.xn=4;
  priv->bounds.xmin=(gdouble) lt;
  priv->bounds.xmax=(gdouble) ut;
  if (yl>yu) {yn=yu; yx=yl;}
  else {yn=yl; yx=yu;}
  num3=(yx-yn)/6;
  num=log10(num3);
  num2=(gint) floor(num);
  num=fmod(num,1);
  if (num==0) {
    lt=(gint) floor(yn/num3);
    ut=(gint) ceil(yx/num3);
    tk=ut-lt;
    if (tk>6) {
      num3*=2;
      lt=(gint) floor(yn/num3);
      ut=(gint) ceil(yx/num3);
      tk=(ut-lt);
    }
    priv->bounds.ymin=num3*(gdouble) lt;
    priv->bounds.ymax=num3*(gdouble) ut;
    if (tk<=0) priv->ticks.yj=1;
    else priv->ticks.yj=tk;
  } else if (num<0.301029997) {
    num=exp(G_LN10*num2);
    num3=2*num;
    lt=(gint) floor(yn/num3);
    ut=(gint) ceil(yx/num3);
    tk=ut-lt;
    if (tk>6) {
      num3=5*num;
      lt=(gint) floor(yn/num3);
      ut=(gint) ceil(yx/num3);
      tk=ut-lt;
      if (tk>6) {
        num3*=2;
        lt=(gint) floor(yn/num3);
        ut=(gint) ceil(yx/num3);
        tk=ut-lt;
      }
    }
    priv->bounds.ymin=num3*(gdouble) lt;
    priv->bounds.ymax=num3*(gdouble) ut;
    if (tk<=0) priv->ticks.yj=1;
    else priv->ticks.yj=tk;
  } else if (num<0.698970005) {
    num=exp(G_LN10*num2);
    num3=5*num;
    lt=(gint) floor(yn/num3);
    ut=(gint) ceil(yx/num3);
    tk=ut-lt;
    if (tk>6) {
      num3*=2;
      lt=(gint) floor(yn/num3);
      ut=(gint) ceil(yx/num3);
      tk=ut-lt;
      if (tk>6) {
        num3*=2;
        lt=(gint) floor(yn/num3);
        ut=(gint) ceil(yx/num3);
        tk=ut-lt;
      }
    }
    priv->bounds.ymin=num3*(gdouble) lt;
    priv->bounds.ymax=num3*(gdouble) ut;
    if (tk<=0) priv->ticks.yj=1;
    else priv->ticks.yj=tk;
  } else {
    num=G_LN10*(++num2);
    num3=exp(num);
    lt=(gint) floor(yn/num3);
    ut=(gint) ceil(yx/num3);
    tk=(ut-lt);
    if (tk>6) {
      num3*=2;
      lt=(gint) floor(yn/num3);
      ut=(gint) ceil(yx/num3);
      tk=(ut-lt);
    }
    priv->bounds.ymin=num3*(gdouble) lt;
    priv->bounds.ymax=num3*(gdouble) ut;
    if (tk<=0) priv->ticks.yj=1;
    else priv->ticks.yj=tk;
  }
  if (num3<1) {
    num=-log10(num3);
    if (fmod(num,1)<FAC) plot->ydp=(guint) num;
    else plot->ydp=(guint) ceil(num);
  } else plot->ydp=0;
  gtk_plot_log_linear_redraw(widget);
  return FALSE;
}

gboolean gtk_plot_log_linear_print(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer data) {
  cairo_t* cr;

  cr=gtk_print_context_get_cairo_context(context);
  drawk(GTK_WIDGET(data), cr);
  draw(GTK_WIDGET(data), cr);
  return FALSE;
}

gboolean gtk_plot_log_linear_print_eps(GtkWidget *plot, gchar* fout) {
  cairo_t *cr;
  cairo_surface_t *surface;

  surface=cairo_ps_surface_create(fout, (gdouble) plot->allocation.width, (gdouble) plot->allocation.height);
  cairo_ps_surface_set_eps(surface, TRUE);
  cairo_ps_surface_restrict_to_level(surface, CAIRO_PS_LEVEL_2);
  cr=cairo_create(surface);
  drawk(plot, cr);
  draw(plot, cr);
  cairo_surface_show_page(surface);
  cairo_destroy(cr);
  cairo_surface_finish(surface);
  cairo_surface_destroy(surface);
  return FALSE;
}

gboolean gtk_plot_log_linear_print_png(GtkWidget *plot, gchar* fout) {
  cairo_t *cr;
  cairo_surface_t *surface;

  surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (gdouble) plot->allocation.width, (gdouble) plot->allocation.height);
  cr=cairo_create(surface);
  drawk(plot, cr);
  draw(plot, cr);
  cairo_surface_write_to_png(surface, fout);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  return FALSE;
}

gboolean gtk_plot_log_linear_print_svg(GtkWidget *plot, gchar* fout) {
  cairo_t *cr;
  cairo_surface_t *surface;

  surface=cairo_svg_surface_create(fout, (gdouble) plot->allocation.width, (gdouble) plot->allocation.height);
  cr=cairo_create(surface);
  drawk(plot, cr);
  draw(plot, cr);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  return FALSE;
}

static gboolean gtk_plot_log_linear_button_press(GtkWidget *widget, GdkEventButton *event) {
  gdouble s;
  gint d, hg, j, wd, xw;
  GtkPlot *plt;
  GtkPlotLogLinear *plot;
  GtkPlotLogLinearPrivate *priv;
  PangoLayout *lyt;

  priv=GTK_PLOT_LOG_LINEAR_GET_PRIVATE(widget);
  if ((priv->flagr)==0) {
    xw=widget->allocation.width;
    j=(xw<400)?0:(xw<1000)?1:2;
    plt=GTK_PLOT(widget);
    plot=GTK_PLOT_LOG_LINEAR(widget);
    if ((event->y)<=2*SZ[j]+1.0) {
      if ((event->x)>=xw-2*SZ[j]-1.0) {
        priv->flagr=2;
	    return FALSE;
      }
      if ((event->x)>=xw-4*SZ[j]-2.0) {
        priv->flagr=3;
	    return FALSE;
      }
    } else if ((plot->kdata)&&(plot->kdata->len>0)&&(plt->ky)&&(*(plt->ky))&&((event->x)>=xw-2*SZ[j]-1.0)) {
      d=0;
      s=2*SZ[j]+3.0;
      do {
	    lyt=pango_layout_new(gtk_widget_get_pango_context(widget));
	    pango_layout_set_font_description(lyt, plt->lfont);
	    pango_layout_set_text(lyt, (plt->ky)[d], -1);
	    pango_layout_get_pixel_size(lyt, &wd, &hg);
	    g_object_unref(lyt);
	    s+=hg+2.0;
	    if (s>(event->y)) {
	      priv->flagr=d+4;
	      return FALSE;
	    }
      } while (((++d)<plot->kdata->len)&&((plt->ky)[d]));
    }
    priv->rescale.xmin=priv->bounds.xmin;
    if (((plot->zmode)&(GTK_PLOT_LOG_LINEAR_ZOOM_SGL|GTK_PLOT_LOG_LINEAR_ZOOM_HZT))!=0) {
      if (event->x>=priv->range.xn) priv->rescale.xmin=priv->bounds.xmax;
      else {
        d=event->x-priv->range.xj;
        if (d>0) {
          priv->rescale.xmin+=(priv->bounds.xmax-priv->bounds.xmin)*d/(priv->range.xn-priv->range.xj);
          priv->flagr=1;
        }
      }
    }
    priv->rescale.ymin=priv->bounds.ymin;
    if (((plot->zmode)&(GTK_PLOT_LOG_LINEAR_ZOOM_SGL|GTK_PLOT_LOG_LINEAR_ZOOM_VRT))!=0) {
      if (event->y<=priv->range.yj) priv->rescale.ymin=priv->bounds.ymax;
      else {
        d=priv->range.yn-event->y;
        if (d>0) {
          priv->rescale.ymin+=(priv->bounds.ymax-priv->bounds.ymin)*d/(priv->range.yn-priv->range.yj);
          priv->flagr=1;
        }
      }
    }
  }
  return FALSE;
}

static gboolean gtk_plot_log_linear_motion_notify(GtkWidget *widget, GdkEventMotion *event) {
  gdouble dx, dy;
  GtkPlotLogLinear *plot;
  GtkPlotLogLinearPrivate *priv;

  priv=GTK_PLOT_LOG_LINEAR_GET_PRIVATE(widget);
  dx=(event->x-priv->range.xj)/(priv->range.xn-priv->range.xj);
  dy=(priv->range.yn-event->y)/(priv->range.yn-priv->range.yj);
  if ((dx>=0)&&(dy>=0)&&(dx<=1)&&(dy<=1)) {
    plot=GTK_PLOT_LOG_LINEAR(widget);
    plot->xps=exp(G_LN10*(priv->bounds.xmax*dx+priv->bounds.xmin*(1-dx)));
    plot->yps=priv->bounds.ymax*dy+priv->bounds.ymin*(1-dy);
    g_signal_emit(plot, gtk_plot_log_linear_signals[MOVED], 0);
  }
  return FALSE;
}

static gboolean gtk_plot_log_linear_button_release(GtkWidget *widget, GdkEventButton *event) {
  gdouble xn, xx, yn, yx, s;
  gint d, hg, j, wd, xw;
  GtkPlot *plt;
  GtkPlotLogLinear *plot;
  GtkPlotLogLinearPrivate *priv;
  guint *ptr;
  PangoLayout *lyt;

  priv=GTK_PLOT_LOG_LINEAR_GET_PRIVATE(widget);
  plot=GTK_PLOT_LOG_LINEAR(widget);
  xw=widget->allocation.width;
  j=(xw<400)?0:(xw<1000)?1:2;
  switch (priv->flagr) {
    case 1:
      if (((plot->zmode)&GTK_PLOT_LOG_LINEAR_ZOOM_SGL)==0) {
        priv->rescale.xmax=priv->bounds.xmax;
        if (((plot->zmode)&GTK_PLOT_LOG_LINEAR_ZOOM_HZT)!=0) {
          if (event->x<=priv->range.xj) (priv->rescale.xmax)=(priv->bounds.xmin);
          else {
            d=priv->range.xn-event->x;
            if (d>0) priv->rescale.xmax-=(priv->bounds.xmax-priv->bounds.xmin)*d/(priv->range.xn-priv->range.xj);
          }
        }
        priv->rescale.ymax=priv->bounds.ymax;
        if (((plot->zmode)&GTK_PLOT_LOG_LINEAR_ZOOM_VRT)!=0) {
          if (event->y>=priv->range.yn) priv->rescale.ymax=priv->bounds.ymin;
          else {
            d=event->y-priv->range.yj;
            if (d>0) priv->rescale.ymax-=(priv->bounds.ymax-priv->bounds.ymin)*d/(priv->range.yn-priv->range.yj);
          }
        }
        xn=priv->rescale.xmax-priv->rescale.xmin;
        yn=priv->rescale.ymax-priv->rescale.ymin;
        if (((xn>DZE)||(xn<NZE))&&((yn>DZE)||(yn<NZE))) {
          if (((plot->zmode)&GTK_PLOT_LOG_LINEAR_ZOOM_DRG)!=0) gtk_plot_log_linear_update_scale_pretty(widget, exp(G_LN10*(priv->bounds.xmin-xn)), exp(G_LN10*(priv->bounds.xmax-xn)), priv->bounds.ymin-yn, priv->bounds.ymax-yn);
          else if (((plot->zmode)&GTK_PLOT_LOG_LINEAR_ZOOM_OUT)==0) gtk_plot_log_linear_update_scale_pretty(widget, exp(G_LN10*priv->rescale.xmin), exp(G_LN10*priv->rescale.xmax), priv->rescale.ymin, priv->rescale.ymax);
          else {
            s=(priv->bounds.xmax-priv->bounds.xmin)/xn;
            if (s>0) {
              xn=(priv->bounds.xmin-priv->rescale.xmin)*s;
              xn+=priv->bounds.xmin;
              xx=(priv->bounds.xmax-priv->rescale.xmax)*s;
              xx+=priv->bounds.xmax;
              s=(priv->bounds.ymax-priv->bounds.ymin)/yn;
              if (s>0) {
                yn=(priv->bounds.ymin-priv->rescale.ymin)*s;
                yn+=priv->bounds.ymin;
                yx=(priv->bounds.ymax-priv->rescale.ymax)*s;
                yx+=priv->bounds.ymax;
                gtk_plot_log_linear_update_scale_pretty(widget, exp(G_LN10*xn), exp(G_LN10*xx), yn, yx);
              } else if (s<0) {
                yn=(priv->rescale.ymax-priv->bounds.ymin)*s;
                yn+=priv->bounds.ymin;
                yx=(priv->rescale.ymin-priv->bounds.ymax)*s;
                yx+=priv->bounds.ymax;
                gtk_plot_log_linear_update_scale_pretty(widget, exp(G_LN10*xn), exp(G_LN10*xx), yn, yx);
              }
            } else if (s<0) {
              xn=(priv->rescale.xmax-priv->bounds.xmin)*s;
              xn+=priv->bounds.xmin;
              xx=(priv->rescale.xmin-priv->bounds.xmax)*s;
              xx+=priv->bounds.xmax;
              s=(priv->bounds.ymax-priv->bounds.ymin)/yn;
              if (s>0) {
                yn=(priv->bounds.ymin-priv->rescale.ymin)*s;
                yn+=priv->bounds.ymin;
                yx=(priv->bounds.ymax-priv->rescale.ymax)*s;
                yx+=priv->bounds.ymax;
                gtk_plot_log_linear_update_scale_pretty(widget, exp(G_LN10*xn), exp(G_LN10*xx), yn, yx);
              } else if (s<0) {
                yn=(priv->rescale.ymax-priv->bounds.ymin)*s;
                yn+=priv->bounds.ymin;
                yx=(priv->rescale.ymin-priv->bounds.ymax)*s;
                yx+=priv->bounds.ymax;
                gtk_plot_log_linear_update_scale_pretty(widget, exp(G_LN10*xn), exp(G_LN10*xx), yn, yx);
              }
            }
          }
        }
      } else if (((plot->zmode)&GTK_PLOT_LOG_LINEAR_ZOOM_OUT)==0) {
        xn=ZS*priv->rescale.xmin;
        xx=xn;
        xn+=ZSC*priv->bounds.xmin;
        xx+=ZSC*priv->bounds.xmax;
        yn=ZS*priv->rescale.ymin;
        yx=yn;
        yn+=ZSC*priv->bounds.ymin;
        yx+=ZSC*priv->bounds.ymax;
        gtk_plot_log_linear_update_scale_pretty(widget, exp(G_LN10*xn), exp(G_LN10*xx), yn, yx);
      } else {
        xn=-UZC*priv->rescale.xmin;
        xx=xn;
        xn+=UZ*priv->bounds.xmin;
        xx+=UZ*priv->bounds.xmax;
        yn=-UZC*priv->rescale.ymin;
        yx=yn;
        yn+=UZ*priv->bounds.ymin;
        yx+=UZ*priv->bounds.ymax;
        gtk_plot_log_linear_update_scale_pretty(widget, exp(G_LN10*xn), exp(G_LN10*xx), yn, yx);
      }
      break;
    case 2:
      if (((event->y)<=2*SZ[j]+1.0)&&((event->x)>=xw-2*SZ[j]-1.0)) {
        if (((plot->zmode)&(GTK_PLOT_LOG_LINEAR_ZOOM_DRG|GTK_PLOT_LOG_LINEAR_ZOOM_OUT))==0) {
          if (((plot->zmode)&(GTK_PLOT_LOG_LINEAR_ZOOM_SGL))!=0) plot->zmode>>=1;
          plot->zmode|=GTK_PLOT_LOG_LINEAR_ZOOM_DRG;
        } else (plot->zmode)--;
        gtk_widget_queue_draw_area(widget, xw-2*SZ[j]-1.0, 0, 2*SZ[j]+1.0, 2*SZ[j]+1.0);
      }
      break;
    case 3:
      if (((event->y)<=2*SZ[j]+1.0)&&((event->x)>=xw-4*SZ[j]-2.0)&&((event->x)<=xw-2*SZ[j]-1.0)) {
        plot->zmode-=GTK_PLOT_LOG_LINEAR_ZOOM_HZT;
        if (((plot->zmode)&(GTK_PLOT_LOG_LINEAR_ZOOM_SGL|GTK_PLOT_LOG_LINEAR_ZOOM_HZT|GTK_PLOT_LOG_LINEAR_ZOOM_VRT))==0) {
          if (((plot->zmode)&GTK_PLOT_LOG_LINEAR_ZOOM_DRG)!=0) plot->zmode|=GTK_PLOT_LOG_LINEAR_ZOOM_VRT|GTK_PLOT_LOG_LINEAR_ZOOM_HZT;
          else plot->zmode|=GTK_PLOT_LOG_LINEAR_ZOOM_SGL;
        }
        gtk_widget_queue_draw_area(widget, xw-4*SZ[j]-2.0, 0, 2*SZ[j]+1.0, 2*SZ[j]+1.0);
      }
      break;
    case 0:
      break;
    default:
      plt=GTK_PLOT(widget);
      if ((plt->ky)&&(*(plt->ky))&&((event->x)>=xw-2*SZ[j]-1.0)&&((event->y)>=2*SZ[j]+1.0)) {
        d=0;
        s=2*SZ[j]+3.0;
        do {
	      lyt=pango_layout_new(gtk_widget_get_pango_context(widget));
	      pango_layout_set_font_description(lyt, plt->lfont);
	      pango_layout_set_text(lyt, (plt->ky)[d], -1);
	      pango_layout_get_pixel_size(lyt, &wd, &hg);
	      g_object_unref(lyt);
	      s+=hg+2.0;
	      if (s>(event->y)) {
	        if (d+4==priv->flagr) {
	          ptr=&g_array_index(plot->kdata, guint, d);
	          (*ptr)^=GTK_PLOT_LOG_LINEAR_KEY_CLR;
              g_signal_emit(plot, gtk_plot_log_linear_signals[KEY_CHANGED], 0);
              gtk_plot_log_linear_redraw(widget);
	        }
	        break;
	      }
        } while ((plt->ky)[d++]);
      }
      break;
  }
  priv->flagr=0;
  return FALSE;
}

void gtk_plot_log_linear_set_label(GtkPlotLogLinear *plot, gchar *xl, gchar *yl) {
  if (plot->xlab) g_free(plot->xlab);
  if (plot->ylab) g_free(plot->ylab);
  plot->xlab = g_strdup(xl);
  plot->ylab = g_strdup(yl);
}

void gtk_plot_log_linear_set_data(GtkPlotLogLinear *plot, GArray *xd, GArray *yd, GArray *nd, GArray *sz, GArray *st, GArray *kd) {
  if (plot->xdata) g_array_unref(plot->xdata);
  plot->xdata = g_array_ref(xd);
  if (plot->ydata) g_array_unref(plot->ydata);
  plot->ydata = g_array_ref(yd);
  if (plot->kdata) g_array_unref(plot->kdata);
  plot->kdata = kd?g_array_ref(kd):NULL;
  gtk_plot_set_indices(GTK_PLOT(plot), nd, sz, st);
}

static void gtk_plot_log_linear_finalise(GtkPlotLogLinear *plot) {
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
  if (plot->kdata) {
    g_array_unref(plot->kdata);
    plot->kdata=NULL;
  }
}

static void gtk_plot_log_linear_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
  switch (prop_id) {
    case PROP_BXN:
      if (g_value_get_double(value)>DZE) GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->bounds.xmin=log10(g_value_get_double(value));
      break;
    case PROP_BXX:
      if (g_value_get_double(value)>DZE) GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->bounds.xmax=log10(g_value_get_double(value));
      break;
    case PROP_BYN:
      GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->bounds.ymin=g_value_get_double(value);
      break;
    case PROP_BYX:
      GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->bounds.ymax=g_value_get_double(value);
      break;
    case PROP_XTJ:
      GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->ticks.xj=g_value_get_uint(value);
      break;
    case PROP_YTJ:
      GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->ticks.yj=g_value_get_uint(value);
      break;
    case PROP_XTN:
      GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->ticks.xn=g_value_get_uint(value);
      break;
    case PROP_YTN:
      GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->ticks.yn=g_value_get_uint(value);
      break;
    case PROP_FA:
      GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->flaga=g_value_get_uint(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void gtk_plot_log_linear_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
  switch (prop_id) {
    case PROP_BXN:
      g_value_set_double(value, exp(G_LN10*GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->bounds.xmin));
      break;
    case PROP_BXX:
      g_value_set_double(value, exp(G_LN10*GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->bounds.xmax));
      break;
    case PROP_BYN:
      g_value_set_double(value, GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->bounds.ymin);
      break;
    case PROP_BYX:
      g_value_set_double(value, GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->bounds.ymax);
      break;
    case PROP_XTJ:
      g_value_set_uint(value, GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->ticks.xj);
      break;
    case PROP_YTJ:
      g_value_set_uint(value, GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->ticks.yj);
      break;
    case PROP_XTN:
      g_value_set_uint(value, GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->ticks.xn);
      break;
    case PROP_YTN:
      g_value_set_uint(value, GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->ticks.yn);
      break;
    case PROP_FA:
      g_value_set_uint(value, GTK_PLOT_LOG_LINEAR_GET_PRIVATE(object)->flaga);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static gboolean gtk_plot_log_linear_expose(GtkWidget *widget, GdkEventExpose *event) {
  cairo_t *cr;

  cr=gdk_cairo_create(gtk_widget_get_window(widget));
  cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
  cairo_clip(cr);
  drawk(widget, cr);
  draw(widget, cr);
  drawz(widget, cr);
  cairo_destroy(cr);
  return FALSE;
}

static void gtk_plot_log_linear_class_init(GtkPlotLogLinearClass *klass) {
  GObjectClass *obj_klass;
  GtkWidgetClass *widget_klass;

  obj_klass=G_OBJECT_CLASS(klass);
  g_type_class_add_private(obj_klass, sizeof(GtkPlotLogLinearPrivate));
  obj_klass->finalize     = (GObjectFinalizeFunc) gtk_plot_log_linear_finalise;
  obj_klass->set_property = gtk_plot_log_linear_set_property;
  obj_klass->get_property = gtk_plot_log_linear_get_property;
  g_object_class_install_property(obj_klass, PROP_BXN, g_param_spec_double("xmin", "Minimum x value", "Minimum value for the horizontal scale", DZE, G_MAXDOUBLE, 0.01, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_BXX, g_param_spec_double("xmax", "Maximum x value", "Maximum value for the horizontal scale", DZE, G_MAXDOUBLE, 100.0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_BYN, g_param_spec_double("ymin", "Minimum y value", "Minimum value for the vertical scale", -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_BYX, g_param_spec_double("ymax", "Maximum y value", "Maximum value for the vertical scale", -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_XTJ, g_param_spec_uint(  "xbtk", "Major x ticks-1", "Number of gaps between major ticks for the horizontal scale-1", 1, G_MAXINT, 4, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_YTJ, g_param_spec_uint(  "ybtk", "Major y ticks-1", "Number of gaps between major ticks for the vertical scale-1", 1, G_MAXINT, 4, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_XTN, g_param_spec_uint(  "xstk", "Minor x ticks+1", "Number of unlabelled tick divisions between major ticks for the horizontal scale+1", 1, G_MAXINT, 5, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_YTN, g_param_spec_uint(  "ystk", "Minor y ticks+1", "Number of unlabelled ticks divisions between major ticks for the vertical scale+1", 1, G_MAXINT, 5, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_FA, g_param_spec_flags(  "aflg", "Axis Flags",      "Flags for axes behaviour: 32 = Labels right, 16 = Labels above, 8 = Wiggle on top, 4 = Wiggle underneath, 2 = Wiggle on Right, 1 = Wiggle on left", G_TYPE_FLAGS, 0, G_PARAM_READWRITE));
  widget_klass=GTK_WIDGET_CLASS(klass);
  widget_klass->button_press_event   = gtk_plot_log_linear_button_press;
  widget_klass->motion_notify_event  = gtk_plot_log_linear_motion_notify;
  widget_klass->button_release_event = gtk_plot_log_linear_button_release;
  widget_klass->expose_event         = gtk_plot_log_linear_expose;
  gtk_plot_log_linear_signals[MOVED]       = g_signal_new("moved",       G_OBJECT_CLASS_TYPE(obj_klass), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(GtkPlotLogLinearClass, moved),       NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
  gtk_plot_log_linear_signals[KEY_CHANGED] = g_signal_new("key_changed", G_OBJECT_CLASS_TYPE(obj_klass), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(GtkPlotLogLinearClass, key_changed), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void gtk_plot_log_linear_init(GtkPlotLogLinear *plot) {
  GtkPlotLogLinearPrivate *priv;

  gtk_widget_add_events(GTK_WIDGET(plot), GDK_BUTTON_PRESS_MASK|GDK_POINTER_MOTION_MASK|GDK_BUTTON_RELEASE_MASK);
  plot->xdata=plot->ydata=plot->kdata=NULL;
  plot->xlab=g_strdup("Domain");
  plot->ylab=g_strdup("Amplitude");
  plot->flagd=GTK_PLOT_LOG_LINEAR_DISP_LIN;
  plot->ptsize=5;
  plot->linew=2;
  plot->zmode=GTK_PLOT_LOG_LINEAR_ZOOM_VRT|GTK_PLOT_LOG_LINEAR_ZOOM_HZT;
  priv=GTK_PLOT_LOG_LINEAR_GET_PRIVATE(plot);
  priv->bounds.xmin=-2.0;
  priv->bounds.xmax=2.0;
  plot->xps=1.0;
  priv->ticks.xj=4;
  priv->ticks.xn=8;
  plot->ydp=2;
  plot->yps=0;
  priv->bounds.ymin=0.0;
  priv->bounds.ymax=1.0;
  priv->ticks.yj=4;
  priv->ticks.yn=5;
  priv->range.xj=priv->range.yj=0;
  priv->range.xn=priv->range.yn=1;
  priv->flaga=priv->flagr=0;
}

GtkWidget *gtk_plot_log_linear_new(void) {return g_object_new(GTK_PLOT_TYPE_LOG_LINEAR, NULL);}
