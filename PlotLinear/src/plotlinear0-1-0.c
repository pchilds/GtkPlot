/***************************************************************************
 *            plotlinear.c
 *
 *  A GTK+ widget that plots data
 *  version 0.1.0
 *  Features:
 *            automatic wiggle insertion
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

#include <gtk/gtk.h>
#include <math.h>
#include<cairo-ps.h>
#include "plotlinear0-1-0.h"

#define PLOT_LINEAR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), PLOT_TYPE_LINEAR, PlotLinearPrivate))
#define ARP 0.05 /* Proportion of the graph occupied by arrows */
#define IRTR 0.577350269 /* 1/square root 3 */
#define TPI 6.28318530718 /* 2pi */
#define WGP 0.08 /* Proportion of the graph the wiggles occupy */
#define WFP 0.01 /* Proportion of wiggle that is flat to axis */
#define WMP 0.045 /* the mean of these */
#define WHP 0.020207259 /* wiggle height proportion */
#define DZE 0.00001 /* divide by zero threshold */
#define NZE -0.00001 /* negative of this */
#define JT 5 /* major tick length */
#define JTI 6 /* this incremented */
#define NT 3 /* minor tick length */
#define ZS 0.5 /* zoom scale */
#define ZSC 0.5 /* 1 minus this */
#define UZ 2 /* inverse of this */
#define UZC 1 /* this minus 1 */
G_DEFINE_TYPE (PlotLinear, plot_linear, GTK_TYPE_DRAWING_AREA);
enum
  {
  PROP_0,
  PROP_BXN,
  PROP_BXX,
  PROP_BYN,
  PROP_BYX,
  PROP_XTJ,
  PROP_YTJ,
  PROP_XTN,
  PROP_YTN,
  PROP_XC,
  PROP_YC,
  PROP_FA
  };

enum
  {
  MOVED,
  LAST_SIGNAL
  };

static guint plot_linear_signals[LAST_SIGNAL]={0};
typedef struct _PlotLinearPrivate PlotLinearPrivate;

struct xs
  {
  gdouble xmin, ymin, xmax, ymax;
  };

struct tk
  {
  guint xj, yj, xn, yn;
  };

struct _PlotLinearPrivate
  {
  struct xs bounds;
  struct tk ticks;
  guint xcs;
  guint ycs;
  guint flaga;
  guint flagr;
  struct tk range;
  struct xs rescale;
  };

static void plot_linear_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
  {
  PlotLinearPrivate *priv;

  priv=PLOT_LINEAR_GET_PRIVATE(object);
  switch (prop_id)
    {
    case PROP_BXN:
      priv->bounds.xmin=g_value_get_double(value);
      break;
    case PROP_BXX:
      priv->bounds.xmax=g_value_get_double(value);
      break;
    case PROP_BYN:
      priv->bounds.ymin=g_value_get_double(value);
      break;
    case PROP_BYX:
      priv->bounds.ymax=g_value_get_double(value);
      break;
    case PROP_XTJ:
      priv->ticks.xj=g_value_get_uint(value);
      break;
    case PROP_YTJ:
      priv->ticks.yj=g_value_get_uint(value);
      break;
    case PROP_XTN:
      priv->ticks.xn=g_value_get_uint(value);
      break;
    case PROP_YTN:
      priv->ticks.yn=g_value_get_uint(value);
      break;
    case PROP_XC:
      priv->xcs=g_value_get_uint(value);
      break;
    case PROP_YC:
      priv->ycs=g_value_get_uint(value);
      break;
    case PROP_FA:
      priv->flaga=g_value_get_uint(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
    }
  }

static void plot_linear_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
  {
  PlotLinearPrivate *priv;

  priv=PLOT_LINEAR_GET_PRIVATE(object);
  switch (prop_id)
    {
    case PROP_BXN:
      g_value_set_double(value, priv->bounds.xmin);
      break;
    case PROP_BXX:
      g_value_set_double(value, priv->bounds.xmax);
      break;
    case PROP_BYN:
      g_value_set_double(value, priv->bounds.ymin);
      break;
    case PROP_BYX:
      g_value_set_double(value, priv->bounds.ymax);
      break;
    case PROP_XTJ:
      g_value_set_uint(value, priv->ticks.xj);
      break;
    case PROP_YTJ:
      g_value_set_uint(value, priv->ticks.yj);
      break;
    case PROP_XTN:
      g_value_set_uint(value, priv->ticks.xn);
      break;
    case PROP_YTN:
      g_value_set_uint(value, priv->ticks.yn);
      break;
    case PROP_XC:
      g_value_set_uint(value, priv->xcs);
      break;
    case PROP_YC:
      g_value_set_uint(value, priv->ycs);
      break;
    case PROP_FA:
      g_value_set_uint(value, priv->flaga);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
    }
  }

static void plot_linear_finalise(PlotLinear *plot)
  {
  PlotLinearPrivate *priv;

  priv=PLOT_LINEAR_GET_PRIVATE(plot);
  if (plot->xlab) g_free(plot->xlab);
  if (plot->ylab) g_free(plot->ylab);
  if (plot->xdata) g_free(plot->xdata);
  if (plot->ydata) g_free(plot->ydata);
  }

static void draw(GtkWidget *widget, cairo_t *cr)
  {
  PlotLinearPrivate *priv;
  PlotLinear *plot;
  guint xs;
  gint j, k, xw, yw, xr, xr2, yr, yr2, xa, ya, xl, yl, xu, yu, tf, tz, to, tn, tnn, xv, yv, xvn, yvn, dtt, tx;
  gdouble dt;
  gchar lbl[10];
  cairo_text_extents_t extents;
  cairo_matrix_t mtr, mtr2, mtr3;

  mtr.xx=1; /* initialise */
  mtr.xy=0;
  mtr.yx=0;
  mtr.yy=1;
  mtr2.xx=0;
  mtr2.xy=1;
  mtr2.yx=-1;
  mtr2.yy=0;
  mtr3.xx=0;
  mtr3.xy=-1;
  mtr3.yx=1;
  mtr3.yy=0;
  plot=PLOT_LINEAR(widget);
  xw=widget->allocation.width;
  yw=widget->allocation.height;
  priv=PLOT_LINEAR_GET_PRIVATE(plot);
  priv->flaga&=48;
  dtt=(plot->afsize)+(plot->lfsize);
  xr=MIN(xw*ARP,dtt);
  xr2=(xr-2)*IRTR;
  yr=MIN(yw*ARP,dtt);
  yr2=(yr-2)*IRTR;
  dtt+=JTI;
  cairo_set_line_width(cr, 1); /* draw zoom boxes */
  cairo_rectangle(cr, xw-20, 0, 10, 10);
  cairo_rectangle(cr, xw-10, 0, 10, 10);
  cairo_move_to(cr, xw-8, 5);
  cairo_line_to(cr, xw-2, 5);
  cairo_move_to(cr, xw-5, 2);
  cairo_line_to(cr, xw-5, 8);
  if (((plot->zmode)&1)==0)
    {
    cairo_move_to(cr, xw-6, 2);
    cairo_line_to(cr, xw-5, 1);
    cairo_line_to(cr, xw-4, 2);
    cairo_move_to(cr, xw-2, 4);
    cairo_line_to(cr, xw-1, 5);
    cairo_line_to(cr, xw-2, 6);
    cairo_move_to(cr, xw-6, 8);
    cairo_line_to(cr, xw-5, 9);
    cairo_line_to(cr, xw-4, 8);
    cairo_move_to(cr, xw-8, 4);
    cairo_line_to(cr, xw-9, 5);
    cairo_line_to(cr, xw-8, 6);
    }
  else
    {
    cairo_move_to(cr, xw-7, 3);
    cairo_line_to(cr, xw-3, 7);
    cairo_move_to(cr, xw-7, 7);
    cairo_line_to(cr, xw-3, 3);
    }
  cairo_stroke(cr);
  if (((plot->zmode)&8)!=0)
    {
    cairo_move_to(cr, xw-19, 1);
    cairo_line_to(cr, xw-11, 9);
    cairo_move_to(cr, xw-19, 9);
    cairo_line_to(cr, xw-11, 1);
    cairo_stroke(cr);
    }
  else
    {
    cairo_save(cr);
    dt=1;
    cairo_set_dash(cr, &dt, 1, 0);
    if (((plot->zmode)&4)!=0)
      {
      cairo_move_to(cr, xw-18, 3);
      cairo_line_to(cr, xw-12, 3);
      cairo_move_to(cr, xw-18, 7);
      cairo_line_to(cr, xw-12, 7);
      cairo_stroke(cr);
      }
    if (((plot->zmode)&2)!=0)
      {
      cairo_move_to(cr, xw-17, 2);
      cairo_line_to(cr, xw-17, 8);
      cairo_move_to(cr, xw-13, 2);
      cairo_line_to(cr, xw-13, 8);
      cairo_stroke(cr);
      }
    cairo_restore(cr);
    }
  cairo_select_font_face(cr, "sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, plot->afsize);
  if (priv->bounds.ymax<=DZE) /* determine positions of axes */
    {
    priv->flaga|=32;
    g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymin));
    cairo_text_extents(cr, lbl, &extents);
    yl=yw-((extents.width)/2)-1; /* allow space for lower label */
    ya=dtt;
    yu=(yl-ya)*((priv->bounds.ymax)/(priv->bounds.ymin));
    if (yu>(yw*WGP))
      {
      priv->flaga|=4;
      yu=yw*WGP;
      }
    yu+=ya;
    }
  else if (priv->bounds.ymin>=NZE)
    {
    yu=yr;
    ya=yw-dtt;
    yl=(ya-yu)*((priv->bounds.ymin)/(priv->bounds.ymax));
    if (yl>yw*WGP)
      {
      priv->flaga|=8;
      yl=(yw*WGP);
      }
    yl=-yl;
    yl+=ya;
    }
  else if (((priv->flaga)&32)!=0)
    {
    if ((priv->bounds.ymin)+(priv->bounds.ymax)<=0)
      {
      g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymin));
      cairo_text_extents(cr, lbl, &extents);
      yl=yw-((extents.width)/2)-1; /* allow space for lower label */
      yu=(yl-dtt)*((priv->bounds.ymax)/(priv->bounds.ymin));
      yu+=dtt;
      if (yu<yr)
        {
        yu=yr;
        ya=((yl*(priv->bounds.ymax))-(yu*(priv->bounds.ymin)))/((priv->bounds.ymax)-(priv->bounds.ymin));
        }
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
        g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymin));
        cairo_text_extents(cr, lbl, &extents);
        yl-=-((extents.width)/2);
        yl--; /* allow space for lower label */
        }
      ya=((yl*(priv->bounds.ymax))-(yu*(priv->bounds.ymin)))/((priv->bounds.ymax)-(priv->bounds.ymin));
      }
    }
  if (priv->bounds.xmax<=DZE)
    {
    priv->flaga|=16;
    g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin));
    cairo_text_extents(cr, lbl, &extents);
    xl=(extents.width)/2; /* allow space for left label */
    xa=xw-dtt;
    xu=(xa-xl)*((priv->bounds.xmax)/(priv->bounds.xmin));
    if (xu>(xw*WGP))
      {
      priv->flaga|=1;
      xu=xw*WGP;
      }
    xu=-xu;
    xu+=xa;
    }
  else if (priv->bounds.xmin>=NZE)
    {
    xu=xw-xr;
    xa=dtt;
    xl=(xu-xa)*((priv->bounds.xmin)/(priv->bounds.xmax));
    if (xl>(xw*WGP))
      {
      priv->flaga|=2;
      xl=(xw*WGP);
      }
    xl+=xa;
    }
  else if (((priv->flaga)&16)!=0)
    {
    if ((priv->bounds.xmin)+(priv->bounds.xmax)<=0)
      {
      g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin));
      cairo_text_extents(cr, lbl, &extents);
      xl=(extents.width)/2; /* allow space for left label */
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
        g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin));
        cairo_text_extents(cr, lbl, &extents);
        xl=(extents.width)/2; /* allow space for left label */
        xa=((xl*(priv->bounds.xmax))-(xu*(priv->bounds.xmin)))/((priv->bounds.xmax)-(priv->bounds.xmin));
        }
      else
        {
        xl=0;
        xa=-xu*((priv->bounds.xmin)/((priv->bounds.xmax)-(priv->bounds.xmin)));
        }
      }
    }
  (priv->range.xj)=xl;
  (priv->range.yj)=yu;
  (priv->range.xn)=xu;
  (priv->range.yn)=yl;
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_line_width(cr, 2);
  cairo_move_to(cr, 0, ya);
  if (((priv->flaga)&1)!=0) /* draw x wiggles */
    {
    cairo_line_to(cr, xa-(WGP*xw), ya);
    cairo_curve_to(cr, xa-(WMP*xw), ya-(WHP*xw), xa-(WMP*xw), ya+(WHP*xw), xa-(WFP*xw), ya);
    }
  else if (((priv->flaga)&2)!=0)
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
  if (((priv->flaga)&4)!=0) /* draw y wiggles */
    {
    cairo_line_to(cr, xa, ya+(WGP*yw));
    cairo_curve_to(cr, xa-(WHP*yw), ya+(WMP*yw), xa+(WHP*yw), ya+(WMP*yw), xa, ya+(WFP*yw));
    }
  else if (((priv->flaga)&8)!=0)
    {
    cairo_line_to(cr, xa, ya-(WFP*yw));
    cairo_curve_to(cr, xa-(WHP*yw), ya-(WMP*yw), xa+(WHP*yw), ya-(WMP*yw), xa, ya-(WGP*yw));
    }
  yr--;
  yr--;
  cairo_line_to(cr, xa, 0); /* draw y axis */
  cairo_line_to(cr, xa-yr2, yr); /* draw y arrow */
  cairo_move_to(cr, xa, 0);
  cairo_line_to(cr, xa+yr2, yr);
  cairo_stroke(cr);
  cairo_set_font_size(cr, plot->lfsize); /* draw ticks, grid and labels */
  cairo_text_extents(cr, plot->xlab, &extents);
  dt=5;
  if (((priv->flaga)&1)!=0)
    {
    to=xl;
    if (((priv->flaga)&32)!=0)
      {
      cairo_move_to(cr, ((xu+xl-(extents.width))/2)-(extents.x_bearing), ya-dtt-(extents.y_bearing));
      cairo_show_text(cr, (plot->xlab));
      cairo_set_font_size(cr, (plot->afsize));
      cairo_move_to(cr, to, ya);
      cairo_line_to(cr, to, ya-JT);
      cairo_stroke(cr);
      g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin));
      cairo_text_extents(cr, lbl, &extents);
      cairo_move_to(cr, (to-((extents.width)/2))-(extents.x_bearing), ya-JTI-(extents.height)-(extents.y_bearing));
      cairo_show_text(cr, lbl);
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
        g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin)+((j*(priv->bounds.xmax-priv->bounds.xmin))/(priv->ticks.xj)));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, (tn-((extents.width)/2))-(extents.x_bearing), ya-JTI-(extents.height)-(extents.y_bearing));
        cairo_show_text(cr, lbl);
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
      cairo_move_to(cr, ((xu+xl-(extents.width))/2)-(extents.x_bearing), ya+dtt-(extents.height)-(extents.y_bearing));
      cairo_show_text(cr, plot->xlab);
      cairo_set_font_size(cr, plot->afsize);
      cairo_move_to(cr, to, ya);
      cairo_line_to(cr, to, ya+JT);
      cairo_stroke(cr);
      g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin));
      cairo_text_extents(cr, lbl, &extents);
      cairo_move_to(cr, (to-((extents.width)/2))-(extents.x_bearing), ya+JTI-(extents.y_bearing));
      cairo_show_text(cr, lbl);
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
        g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin)+((j*(priv->bounds.xmax-priv->bounds.xmin))/(priv->ticks.xj)));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, (tn-((extents.width)/2))-(extents.x_bearing), ya+JTI-(extents.y_bearing));
        cairo_show_text(cr, lbl);
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
  else if (((priv->flaga)&2)!=0)
    {
    to=xl;
    if (((priv->flaga)&32)!=0)
      {
      cairo_move_to(cr, ((xu+xl-(extents.width))/2)-(extents.x_bearing), ya-dtt-(extents.y_bearing));
      cairo_show_text(cr, plot->xlab);
      cairo_set_font_size(cr, plot->afsize);
      cairo_move_to(cr, to, ya);
      cairo_line_to(cr, to, ya-JT);
      cairo_stroke(cr);
      g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin));
      cairo_text_extents(cr, lbl, &extents);
      cairo_move_to(cr, (to-((extents.width)/2))-(extents.x_bearing), ya-JTI-(extents.height)-(extents.y_bearing));
      cairo_show_text(cr, lbl);
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
        g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin)+((j*(priv->bounds.xmax-priv->bounds.xmin))/(priv->ticks.xj)));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, (tn-((extents.width)/2))-(extents.x_bearing), ya-JTI-(extents.height)-(extents.y_bearing));
        cairo_show_text(cr, lbl);
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
      cairo_move_to(cr, ((xu+xl-(extents.width))/2)-(extents.x_bearing), ya+dtt-(extents.height)-(extents.y_bearing));
      cairo_show_text(cr, plot->xlab);
      cairo_set_font_size(cr, plot->afsize);
      cairo_move_to(cr, to, ya);
      cairo_line_to(cr, to, ya+JT);
      cairo_stroke(cr);
      g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin));
      cairo_text_extents(cr, lbl, &extents);
      cairo_move_to(cr, (to-((extents.width)/2))-(extents.x_bearing), ya+JTI-(extents.y_bearing));
      cairo_show_text(cr, lbl);
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
        g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin)+((j*(priv->bounds.xmax-priv->bounds.xmin))/(priv->ticks.xj)));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, (tn-((extents.width)/2))-(extents.x_bearing), ya+JTI-(extents.y_bearing));
        cairo_show_text(cr, lbl);
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
    tz=((xu+1-xa)*(priv->ticks.xj))/(xu-xl);
    tf=((xu-xa)*(priv->ticks.xj))/tz;
    tf=xu-tf;
    if (((priv->flaga)&32)!=0)
      {
      cairo_move_to(cr, ((xu+xa-(extents.width))/2)-(extents.x_bearing), ya-dtt-(extents.y_bearing));
      cairo_show_text(cr, plot->xlab);
      cairo_set_font_size(cr, plot->afsize);
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
        g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmax)-((j*(priv->bounds.xmax-priv->bounds.xmin)*(xu-tf))/((xu-xl)*(priv->ticks.xj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, (tn-((extents.width)/2))-(extents.x_bearing), ya-JTI-(extents.height)-(extents.y_bearing));
        cairo_show_text(cr, lbl);
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
        g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmax)-((j*(priv->bounds.xmax-priv->bounds.xmin)*(xu-tf))/((xu-xl)*(priv->ticks.xj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, (tn-((extents.width)/2))-(extents.x_bearing), ya-JTI-(extents.height)-(extents.y_bearing));
        cairo_show_text(cr, lbl);
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
      cairo_move_to(cr, ((xu+xa-(extents.width))/2)-(extents.x_bearing), ya+dtt-(extents.height)-(extents.y_bearing));
      cairo_show_text(cr, plot->xlab);
      cairo_set_font_size(cr, plot->afsize);
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
        g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmax)-((j*(priv->bounds.xmax-priv->bounds.xmin)*(xu-tf))/((xu-xl)*(priv->ticks.xj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, (tn-((extents.width)/2))-(extents.x_bearing), ya+JTI-(extents.y_bearing));
        cairo_show_text(cr, lbl);
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
        g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmax)-((j*(priv->bounds.xmax-priv->bounds.xmin)*(xu-tf))/((xu-xl)*(priv->ticks.xj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, (tn-((extents.width)/2))-(extents.x_bearing), ya+JTI-(extents.y_bearing));
        cairo_show_text(cr, lbl);
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
    tf=((xa-xl)*(priv->ticks.xj))/tz;
    tf+=xl;
    if (((priv->flaga)&32)!=0)
      {
      cairo_move_to(cr, ((xa+xl-(extents.width))/2)-(extents.x_bearing), ya-dtt-(extents.y_bearing));
      cairo_show_text(cr, plot->xlab);
      cairo_set_font_size(cr, plot->afsize);
      cairo_move_to(cr, to, ya);
      cairo_line_to(cr, to, ya-JT);
      cairo_stroke(cr);
      g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin));
      cairo_text_extents(cr, lbl, &extents);
      cairo_move_to(cr, (to-((extents.width)/2))-(extents.x_bearing), ya-JTI-(extents.height)-(extents.y_bearing));
      cairo_show_text(cr, lbl);
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
        g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin)+((j*(priv->bounds.xmax-priv->bounds.xmin)*(tf-xl))/((xu-xl)*(priv->ticks.xj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, (tn-((extents.width)/2))-(extents.x_bearing), ya-JTI-(extents.height)-(extents.y_bearing));
        cairo_show_text(cr, lbl);
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
        g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin)+((j*(priv->bounds.xmax-priv->bounds.xmin)*(tf-xl))/((xu-xl)*(priv->ticks.xj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, (tn-((extents.width)/2))-(extents.x_bearing), ya-JTI-(extents.height)-(extents.y_bearing));
        cairo_show_text(cr, lbl);
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
      cairo_move_to(cr, ((xa+xl-(extents.width))/2)-(extents.x_bearing), ya+dtt-(extents.height)-(extents.y_bearing));
      cairo_show_text(cr, plot->xlab);
      cairo_set_font_size(cr, plot->afsize);
      cairo_move_to(cr, to, ya);
      cairo_line_to(cr, to, ya+JT);
      cairo_stroke(cr);
      g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin));
      cairo_text_extents(cr, lbl, &extents);
      cairo_move_to(cr, (to-((extents.width)/2))-(extents.x_bearing), ya+JTI-(extents.y_bearing));
      cairo_show_text(cr, lbl);
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
        g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin)+((j*(priv->bounds.xmax-priv->bounds.xmin)*(tf-xl))/((xu-xl)*(priv->ticks.xj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, (tn-((extents.width)/2))-(extents.x_bearing), ya+JTI-(extents.y_bearing));
        cairo_show_text(cr, lbl);
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
        g_snprintf(lbl, (priv->xcs), "%f", (priv->bounds.xmin)+((j*(priv->bounds.xmax-priv->bounds.xmin)*(tf-xl))/((xu-xl)*(priv->ticks.xj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, (tn-((extents.width)/2))-(extents.x_bearing), ya+JTI-(extents.y_bearing));
        cairo_show_text(cr, lbl);
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
  cairo_set_font_size(cr, plot->lfsize);
  cairo_text_extents(cr, plot->ylab, &extents);
  if (((priv->flaga)&4)!=0)
    {
    to=yu;
    if (((priv->flaga)&16)!=0)
      {
      cairo_move_to(cr, xa+dtt+(extents.y_bearing), ((yl+yu-(extents.width))/2)+(extents.x_bearing));
      cairo_set_matrix(cr, &mtr3);
      cairo_show_text(cr, plot->ylab);
      cairo_set_matrix(cr, &mtr);
      cairo_set_font_size(cr, plot->afsize);
      g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymax));
      cairo_text_extents(cr, lbl, &extents);
      cairo_move_to(cr, xa+JTI+(extents.y_bearing)+(extents.height), to-((extents.width)/2)+(extents.x_bearing));
      cairo_set_matrix(cr, &mtr3);
      cairo_show_text(cr, lbl);
      cairo_set_matrix(cr, &mtr);
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
        g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymax)-((j*(priv->bounds.ymax-priv->bounds.ymin))/(priv->ticks.yj)));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, xa+JTI+(extents.y_bearing)+(extents.height), tn-((extents.width)/2)+(extents.x_bearing));
        cairo_set_matrix(cr, &mtr3);
        cairo_show_text(cr, lbl);
        cairo_set_matrix(cr, &mtr);
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
      cairo_move_to(cr, xa-dtt-(extents.y_bearing), ((yl+yu+(extents.width))/2)-(extents.x_bearing));
      cairo_set_matrix(cr, &mtr2);
      cairo_show_text(cr, plot->ylab);
      cairo_set_matrix(cr, &mtr);
      cairo_set_font_size(cr, plot->afsize);
      g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymax));
      cairo_text_extents(cr, lbl, &extents);
      cairo_move_to(cr, xa-JTI-(extents.y_bearing)-(extents.height), to+((extents.width)/2)-(extents.x_bearing));
      cairo_set_matrix(cr, &mtr2);
      cairo_show_text(cr, lbl);
      cairo_set_matrix(cr, &mtr);
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
        g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymax)-((j*(priv->bounds.ymax-priv->bounds.ymin))/(priv->ticks.yj)));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, xa-JTI-(extents.y_bearing)-(extents.height), tn+((extents.width)/2)-(extents.x_bearing));
        cairo_set_matrix(cr, &mtr2);
        cairo_show_text(cr, lbl);
        cairo_set_matrix(cr, &mtr);
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
  else if (((priv->flaga)&8)!=0)
    {
    to=yu;
    if (((priv->flaga)&16)!=0)
      {
      cairo_move_to(cr, xa+dtt+(extents.y_bearing), ((yl+yu-(extents.width))/2)+(extents.x_bearing));
      cairo_set_matrix(cr, &mtr3);
      cairo_show_text(cr, plot->ylab);
      cairo_set_matrix(cr, &mtr);
      cairo_set_font_size(cr, plot->afsize);
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
        g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymax)-((j*(priv->bounds.ymax-priv->bounds.ymin))/(priv->ticks.yj)));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, xa+JTI+(extents.y_bearing)+(extents.height), tn-((extents.width)/2)+(extents.x_bearing));
        cairo_set_matrix(cr, &mtr3);
        cairo_show_text(cr, lbl);
        cairo_set_matrix(cr, &mtr);
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
      cairo_move_to(cr, xa-dtt-(extents.y_bearing), ((yl+yu+(extents.width))/2)-(extents.x_bearing));
      cairo_set_matrix(cr, &mtr2);
      cairo_show_text(cr, plot->ylab);
      cairo_set_matrix(cr, &mtr);
      cairo_set_font_size(cr, plot->afsize);
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
        g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymax)-((j*(priv->bounds.ymax-priv->bounds.ymin))/(priv->ticks.yj)));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, xa-JTI-(extents.y_bearing)-(extents.height), tn+((extents.width)/2)-(extents.x_bearing));
        cairo_set_matrix(cr, &mtr2);
        cairo_show_text(cr, lbl);
        cairo_set_matrix(cr, &mtr);
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
    tf=((ya-yu)*(priv->ticks.yj))/tz;
    tf+=yu;
    if (((priv->flaga)&16)!=0)
      {
      cairo_move_to(cr, xa+dtt+(extents.y_bearing), ((ya+yu-(extents.width))/2)+(extents.x_bearing));
      cairo_set_matrix(cr, &mtr3);
      cairo_show_text(cr, plot->ylab);
      cairo_set_matrix(cr, &mtr);
      cairo_set_font_size(cr, plot->afsize);
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
        g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymax)-((j*(priv->bounds.ymax-priv->bounds.ymin)*(tf-yu))/((yl-yu)*(priv->ticks.yj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, xa+JTI+(extents.y_bearing)+(extents.height), tn-((extents.width)/2)+(extents.x_bearing));
        cairo_set_matrix(cr, &mtr3);
        cairo_show_text(cr, lbl);
        cairo_set_matrix(cr, &mtr);
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
        g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymax)-((j*(priv->bounds.ymax-priv->bounds.ymin)*(tf-yu))/((yl-yu)*(priv->ticks.yj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, xa+JTI+(extents.y_bearing)+(extents.height), tn-((extents.width)/2)+(extents.x_bearing));
        cairo_set_matrix(cr, &mtr3);
        cairo_show_text(cr, lbl);
        cairo_set_matrix(cr, &mtr);
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
      cairo_move_to(cr, xa-dtt-(extents.y_bearing), ((ya+yu+(extents.width))/2)-(extents.x_bearing));
      cairo_set_matrix(cr, &mtr2);
      cairo_show_text(cr, plot->ylab);
      cairo_set_matrix(cr, &mtr);
      cairo_set_font_size(cr, plot->afsize);
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
        g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymax)-((j*(priv->bounds.ymax-priv->bounds.ymin)*(tf-yu))/((yl-yu)*(priv->ticks.yj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, xa-JTI-(extents.y_bearing)-(extents.height), tn+((extents.width)/2)-(extents.x_bearing));
        cairo_set_matrix(cr, &mtr2);
        cairo_show_text(cr, lbl);
        cairo_set_matrix(cr, &mtr);
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
        g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymax)-((j*(priv->bounds.ymax-priv->bounds.ymin)*(tf-yu))/((yl-yu)*(priv->ticks.yj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, xa-JTI-(extents.y_bearing)-(extents.height), tn+((extents.width)/2)-(extents.x_bearing));
        cairo_set_matrix(cr, &mtr2);
        cairo_show_text(cr, lbl);
        cairo_set_matrix(cr, &mtr);
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
    tf=((yl-ya)*(priv->ticks.yj))/tz;
    tf=yl-tf;
    if (((priv->flaga)&16)!=0)
      {
      cairo_move_to(cr, xa+dtt+(extents.y_bearing), ((yl+ya-(extents.width))/2)+(extents.x_bearing));
      cairo_set_matrix(cr, &mtr3);
      cairo_show_text(cr, plot->ylab);
      cairo_set_matrix(cr, &mtr);
      cairo_set_font_size(cr, plot->afsize);
      g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymin));
      cairo_text_extents(cr, lbl, &extents);
      cairo_move_to(cr, xa+JTI+(extents.y_bearing)+(extents.height), to-((extents.width)/2)+(extents.x_bearing));
      cairo_set_matrix(cr, &mtr3);
      cairo_show_text(cr, lbl);
      cairo_set_matrix(cr, &mtr);
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
        g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymin)+((j*(priv->bounds.ymax-priv->bounds.ymin)*(yl-tf))/((yl-yu)*(priv->ticks.yj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, xa+JTI+(extents.y_bearing)+(extents.height), tn-((extents.width)/2)+(extents.x_bearing));
        cairo_set_matrix(cr, &mtr3);
        cairo_show_text(cr, lbl);
        cairo_set_matrix(cr, &mtr);
        cairo_set_line_width(cr, 1);
        cairo_save(cr);
        cairo_set_dash(cr, &dt,1, 0);
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
        g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymin)+((j*(priv->bounds.ymax-priv->bounds.ymin)*(yl-tf))/((yl-yu)*(priv->ticks.yj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, xa+JTI+(extents.y_bearing)+(extents.height), tn-((extents.width)/2)+(extents.x_bearing));
        cairo_set_matrix(cr, &mtr3);
        cairo_show_text(cr, lbl);
        cairo_set_matrix(cr, &mtr);
        cairo_set_line_width(cr, 1);
        cairo_save(cr);
        cairo_set_dash(cr, &dt,1, 0);
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
      cairo_move_to(cr, xa-dtt-(extents.y_bearing), ((yl+ya+(extents.width))/2)-(extents.x_bearing));
      cairo_set_matrix(cr, &mtr2);
      cairo_show_text(cr, plot->ylab);
      cairo_set_matrix(cr, &mtr);
      cairo_set_font_size(cr, plot->afsize);
      g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymin));
      cairo_text_extents(cr, lbl, &extents);
      cairo_move_to(cr, xa-JTI-(extents.y_bearing)-(extents.height), to+((extents.width)/2)-(extents.x_bearing));
      cairo_set_matrix(cr, &mtr2);
      cairo_show_text(cr, lbl);
      cairo_set_matrix(cr, &mtr);
      cairo_move_to(cr, xa, to);
      cairo_line_to(cr, xa-JT, to);
      cairo_stroke(cr);
      cairo_set_line_width(cr, 1);
      cairo_save(cr);
      cairo_set_dash(cr, &dt,1, 0);
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
        g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymin)+((j*(priv->bounds.ymax-priv->bounds.ymin)*(yl-tf))/((yl-yu)*(priv->ticks.yj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, xa-JTI-(extents.y_bearing)-(extents.height), tn+((extents.width)/2)-(extents.x_bearing));
        cairo_set_matrix(cr, &mtr2);
        cairo_show_text(cr, lbl);
        cairo_set_matrix(cr, &mtr);
        cairo_set_line_width(cr, 1);
        cairo_save(cr);
        cairo_set_dash(cr, &dt,1, 0);
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
        g_snprintf(lbl, (priv->ycs), "%f", (priv->bounds.ymin)+((j*(priv->bounds.ymax-priv->bounds.ymin)*(yl-tf))/((yl-yu)*(priv->ticks.yj))));
        cairo_text_extents(cr, lbl, &extents);
        cairo_move_to(cr, xa-JTI-(extents.y_bearing)-(extents.height), tn+((extents.width)/2)-(extents.x_bearing));
        cairo_set_matrix(cr, &mtr2);
        cairo_show_text(cr, lbl);
        cairo_set_matrix(cr, &mtr);
        cairo_set_line_width(cr, 1);
        cairo_save(cr);
        cairo_set_dash(cr, &dt,1, 0);
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
  if (plot->xdata && plot->ydata) /* plot data */
    {
    xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, 0)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
    yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, 0)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
    if (((plot->flagd)&1)!=0)
      {
      cairo_set_line_width(cr, (plot->linew));
      if (((plot->flagd)&2)!=0) /* lines and points */
        {
        if (((priv->flaga)&1)!=0)
          {
          if (((priv->flaga)&4)!=0)
            {
            if ((yv>=yu)&&(xv<=xu))
              {
              xs=0;
              cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI);
              cairo_fill(cr);
              cairo_move_to(cr, xv, yv);
              }
            else xs=1;
            for (j=1; j<(plot->size); j++)
              {
              xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
              yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
              if ((yvn>=yu)&&(xvn<=xu))
                {
                if (xs==1)
                  {
                  tx=yvn-yv;
                  if ((tx<DZE)&&(tx>NZE)) cairo_move_to(cr, xu, yv);
                  else
                    {
                    tx=((xvn-xv)*(yu-yv))/tx;
                    tx+=xv;
                    if ((tx<=xu)&&(yvn>yv)) cairo_move_to(cr, tx, yu);
                    else
                      {
                      tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                      tx+=yv;
                      cairo_move_to(cr, xu, tx);
                      }
                    }
                  }
                cairo_line_to(cr, xvn, yvn);
                cairo_stroke(cr);
                cairo_arc(cr, xvn, yvn, (plot->ptsize), 0, TPI);
                cairo_fill(cr);
                cairo_move_to(cr, xvn, yvn);
                xs=0;
                }
              else if (xs==0)
                {
                tx=yvn-yv;
                if ((tx<DZE)&&(tx>NZE)) cairo_line_to(cr, xu, yv);
                else
                  {
                  tx=((xvn-xv)*(yu-yv))/tx;
                  tx+=xv;
                  if ((tx<=xu)&&(yvn<yv)) cairo_line_to(cr, tx, yu);
                  else
                    {
                    tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                    tx+=yv;
                    cairo_line_to(cr, xu, tx);
                    }
                  }
                cairo_stroke(cr);
                xs=1;
                }
              else if ((((xvn>=xu)&&(xu>=xv))||((xvn<=xu)&&(xu<=xv)))&&(((yvn>=yu)&&(yu>=yv))||((yvn<=yu)&&(yu<=yv))))
                {
                tx=((xvn-xv)*(yu-yv))/(yvn-yv);
                tx+=xv;
                if (tx<=xu)
                  {
                  cairo_move_to(cr, tx, yu);
                  tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_line_to(cr, xu, tx);
                  }
                xs=1;
                }
              else xs=1;
              xv=xvn;
              yv=yvn;
              }
            }
          else if (((priv->flaga)&8)!=0)
            {
            if ((yv<=yl)&&(xv<=xu))
              {
              xs=0;
              cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI);
              cairo_fill(cr);
              cairo_move_to(cr, xv, yv);
              }
            else xs=1;
            for (j=1; j<(plot->size); j++)
              {
              xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
              yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
              if ((yvn<=yl)&&(xvn<=xu))
                {
                if (xs==1)
                  {
                  tx=yvn-yv;
                  if ((tx<DZE)&&(tx>NZE)) cairo_move_to(cr, xu, yv);
                  else
                    {
                    tx=((xvn-xv)*(yv-yl))/tx;
                    tx+=xv;
                    if ((tx<=xu)&&(yvn<yv)) cairo_move_to(cr, tx, yl);
                    else
                      {
                      tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                      tx+=yv;
                      cairo_move_to(cr, xu, tx);
                      }
                    }
                  }
                cairo_line_to(cr, xvn, yvn);
                cairo_stroke(cr);
                cairo_arc(cr, xvn, yvn, (plot->ptsize), 0, TPI);
                cairo_fill(cr);
                cairo_move_to(cr, xvn, yvn);
                xs=0;
                }
              else if (xs==0)
                {
                tx=yvn-yv;
                if ((tx<DZE)&&(tx>NZE)) cairo_line_to(cr, xu, yv);
                else
                  {
                  tx=((xvn-xv)*(yl-yv))/tx;
                  tx+=xv;
                  if ((tx<=xu)&&(yvn>yv)) cairo_line_to(cr, tx, yl);
                  else
                    {
                    tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                    tx+=yv;
                    cairo_line_to(cr, xu, tx);
                    }
                  }
                cairo_stroke(cr);
                xs=1;
                }
              else if ((((xvn>=xu)&&(xu>=xv))||((xvn<=xu)&&(xu<=xv)))&&(((yvn>=yl)&&(yl>=yv))||((yvn<=yl)&&(yl<=yv))))
                {
                tx=((xvn-xv)*(yl-yv))/(yvn-yv);
                tx+=xv;
                if (tx<=xu)
                  {
                  cairo_move_to(cr, tx, yl);
                  tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_line_to(cr, xu, tx);
                  }
                xs=1;
                }
              else xs=1;
              xv=xvn;
              yv=yvn;
              }
            }
          else
            {
            if (xv<=xu)
              {
              xs=0;
              cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI);
              cairo_fill(cr);
              cairo_move_to(cr, xv, yv);
              }
            else xs=1;
            for (j=1; j<(plot->size); j++)
              {
              xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
              yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
              if (xvn<=xu)
                {
                if (xs==1)
                  {
                  tx=((yvn-yv)*(xu-xv))/(xvn-xv); /* divide by zero handling? */
                  tx+=yv;
                  cairo_move_to(cr, xu, tx);
                  }
                cairo_line_to(cr, xvn, yvn);
                cairo_stroke(cr);
                cairo_arc(cr, xvn, yvn, (plot->ptsize), 0, TPI);
                cairo_fill(cr);
                cairo_move_to(cr, xvn, yvn);
                xs=0;
                }
              else if (xs==0)
                {
                tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                tx+=yv;
                cairo_line_to(cr, xu, tx);
                cairo_stroke(cr);
                xs=1;
                }
              else xs=1;
              xv=xvn;
              yv=yvn;
              }
            }
          }
        else if (((priv->flaga)&2)!=0)
          {
          if (((priv->flaga)&4)!=0)
            {
            if ((yv>=yu)&&(xv>=xl))
              {
              xs=0;
              cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI);
              cairo_fill(cr);
              cairo_move_to(cr, xv, yv);
              }
            else xs=1;
            for (j=1; j<(plot->size); j++)
              {
              xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
              yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
              if ((yvn>=yu)&&(xvn>=xl))
                {
                if (xs==1)
                  {
                  tx=yvn-yv;
                  if ((tx<DZE)&&(tx>NZE)) cairo_move_to(cr, xl, yv);
                  else
                    {
                    tx=((xvn-xv)*(yu-yv))/tx;
                    tx+=xv;
                    if ((tx>=xl)&&(yvn>yv)) cairo_move_to(cr, tx, yu);
                    else
                      {
                      tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                      tx+=yv;
                      cairo_move_to(cr, xl, tx);
                      }
                    }
                  }
                cairo_line_to(cr, xvn, yvn);
                cairo_stroke(cr);
                cairo_arc(cr, xvn, yvn, (plot->ptsize), 0, TPI);
                cairo_fill(cr);
                cairo_move_to(cr, xvn, yvn);
                xs=0;
                }
              else if (xs==0)
                {
                tx=yvn-yv;
                if ((tx<DZE)&&(tx>NZE)) cairo_line_to(cr, xl, yv);
                else
                  {
                  tx=((xvn-xv)*(yu-yv))/tx;
                  tx+=xv;
                  if ((tx>=xl)&&(yvn<yv)) cairo_line_to(cr, tx, yu);
                  else
                    {
                    tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                    tx+=yv;
                    cairo_line_to(cr, xl, tx);
                    }
                  }
                cairo_stroke(cr);
                xs=1;
                }
              else if ((((xvn>=xl)&&(xl>=xv))||((xvn<=xl)&&(xl<=xv)))&&(((yvn>=yu)&&(yu>=yv))||((yvn<=yu)&&(yu<=yv))))
                {
                tx=((xvn-xv)*(yu-yv))/(yvn-yv);
                tx+=xv;
                if (tx>=xl)
                  {
                  cairo_move_to(cr, tx, yu);
                  tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_line_to(cr, xl, tx);
                  }
                xs=1;
                }
              else xs=1;
              xv=xvn;
              yv=yvn;
              }
            }
          else if (((priv->flaga)&8)!=0)
            {
            if ((yv<=yl)&&(xv>=xl))
              {
              xs=0;
              cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI);
              cairo_fill(cr);
              cairo_move_to(cr, xv, yv);
              }
            else xs=1;
            for (j=1; j<(plot->size); j++)
              {
              xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
              yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
              if ((yvn<=yl)&&(xvn>=xl))
                {
                if (xs==1)
                  {
                  tx=yvn-yv;
                  if ((tx<DZE)&&(tx>NZE)) cairo_move_to(cr, xl, yv);
                  else
                    {
                    tx=((xvn-xv)*(yv-yl))/tx;
                    tx+=xv;
                    if ((tx>=xl)&&(yvn<yv)) cairo_move_to(cr, tx, yl);
                    else
                      {
                      tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                      tx+=yv;
                      cairo_move_to(cr, xl, tx);
                      }
                    }
                  }
                cairo_line_to(cr, xvn, yvn);
                cairo_stroke(cr);
                cairo_arc(cr, xvn, yvn, (plot->ptsize), 0, TPI);
                cairo_fill(cr);
                cairo_move_to(cr, xvn, yvn);
                xs=0;
                }
              else if (xs==0)
                {
                tx=yvn-yv;
                if ((tx<DZE)&&(tx>NZE)) cairo_line_to(cr, xl, yv);
                else
                  {
                  tx=((xvn-xv)*(yl-yv))/tx;
                  tx+=xv;
                  if ((tx>=xl)&&(yvn>yv)) cairo_line_to(cr, tx, yl);
                  else
                    {
                    tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                    tx+=yv;
                    cairo_line_to(cr, xl, tx);
                    }
                  }
                cairo_stroke(cr);
                xs=1;
                }
              else if ((((xvn>=xl)&&(xl>=xv))||((xvn<=xl)&&(xl<=xv)))&&(((yvn>=yl)&&(yl>=yv))||((yvn<=yl)&&(yl<=yv))))
                {
                tx=((xvn-xv)*(yl-yv))/(yvn-yv);
                tx+=xv;
                if (tx>=xl)
                  {
                  cairo_move_to(cr, tx, yl);
                  tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_line_to(cr, xu, tx);
                  }
                xs=1;
                }
              else xs=1;
              xv=xvn;
              yv=yvn;
              }
            }
          else
            {
            if (xv>=xl)
              {
              xs=0;
              cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI);
              cairo_fill(cr);
              cairo_move_to(cr, xv, yv);
              }
            else xs=1;
            for (j=1; j<(plot->size); j++)
              {
              xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
              yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
              if (xvn>=xl)
                {
                if (xs==1)
                  {
                  tx=((yvn-yv)*(xu-xv))/(xvn-xv); /* divide by zero handling? */
                  tx+=yv;
                  cairo_move_to(cr, xu, tx);
                  }
                cairo_line_to(cr, xvn, yvn);
                cairo_stroke(cr);
                cairo_arc(cr, xvn, yvn, (plot->ptsize), 0, TPI);
                cairo_fill(cr);
                cairo_move_to(cr, xvn, yvn);
                xs=0;
                }
              else if (xs==0)
                {
                tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                tx+=yv;
                cairo_line_to(cr, xu, tx);
                cairo_stroke(cr);
                xs=1;
                }
              else xs=1;
              xv=xvn;
              yv=yvn;
              }
            }
          }
        else if (((priv->flaga)&4)!=0)
          {
          if (yv>=yu)
            {
            xs=0;
            cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI);
            cairo_fill(cr);
            cairo_move_to(cr, xv, yv);
            }
          else xs=1;
          for (j=1; j<(plot->size); j++)
            {
            xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            if (yvn>=yu)
              {
              if (xs==1)
                {
                tx=((xvn-xv)*(yu-yv))/(yvn-yv); /* divide by zero handling? */
                tx+=xv;
                cairo_move_to(cr, tx, yu);
                }
              cairo_line_to(cr, xvn, yvn);
              cairo_stroke(cr);
              cairo_arc(cr, xvn, yvn, (plot->ptsize), 0, TPI);
              cairo_fill(cr);
              cairo_move_to(cr, xvn, yvn);
              xs=0;
              }
            else if (xs==0)
              {
              tx=((xvn-xv)*(yu-yv))/(yvn-yv);
              tx+=xv;
              cairo_line_to(cr, tx, yu);
              cairo_stroke(cr);
              xs=1;
              }
            else xs=1;
            xv=xvn;
            yv=yvn;
            }
          }
        else if (((priv->flaga)&8)!=0)
          {
          if (yv<=yl)
            {
            xs=0;
            cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI);
            cairo_fill(cr);
            cairo_move_to(cr, xv, yv);
            }
          else xs=1;
          for (j=1; j<(plot->size); j++)
            {
            xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            if (yvn<=yl)
              {
              if (xs==1)
                {
                tx=((xvn-xv)*(yv-yl))/(yv-yvn); /* divide by zero handling? */
                tx+=xv;
                cairo_move_to(cr, tx, yl);
                }
              cairo_line_to(cr, xvn, yvn);
              cairo_stroke(cr);
              cairo_arc(cr, xvn, yvn, (plot->ptsize), 0, TPI);
              cairo_fill(cr);
              cairo_move_to(cr, xvn, yvn);
              xs=0;
              }
            else if (xs==0)
              {
              tx=((xvn-xv)*(yl-yv))/(yvn-yv);
              tx+=xv;
              cairo_line_to(cr, tx, yl);
              cairo_stroke(cr);
              xs=1;
              }
            else xs=1;
            xv=xvn;
            yv=yvn;
            }
          }
        else
          {
          cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI);
          cairo_fill(cr);
          cairo_move_to(cr, xv, yv);
          for (j=1; j<(plot->size); j++)
            {
            xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            cairo_line_to(cr, xv, yv);
            cairo_stroke(cr);
            cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI);
            cairo_fill(cr);
            cairo_move_to(cr, xv, yv);
            }
          }
        } /* lines only */
      else if (((priv->flaga)&1)!=0)
        {
        if (((priv->flaga)&4)!=0)
          {
          if ((yv>=yu)&&(xv<=xu)) {xs=0; cairo_move_to(cr, xv, yv);}
          else xs=1;
          for (j=1; j<(plot->size); j++)
            {
            xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            if ((yvn>=yu)&&(xvn<=xu))
              {
              if (xs==1)
                {
                tx=yvn-yv;
                if ((tx<DZE)&&(tx>NZE)) cairo_move_to(cr, xu, yv);
                else
                  {
                  tx=((xvn-xv)*(yu-yv))/tx;
                  tx+=xv;
                  if ((tx<=xu)&&(yvn>yv)) cairo_move_to(cr, tx, yu);
                  else
                    {
                    tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                    tx+=yv;
                    cairo_move_to(cr, xu, tx);
                    }
                  }
                }
              cairo_line_to(cr, xvn, yvn);
              xs=0;
              }
            else if (xs==0)
              {
              tx=yvn-yv;
              if ((tx<DZE)&&(tx>NZE)) cairo_line_to(cr, xu, yv);
              else
                {
                tx=((xvn-xv)*(yu-yv))/tx;
                tx+=xv;
                if ((tx<=xu)&&(yvn<yv)) cairo_line_to(cr, tx, yu);
                else
                  {
                  tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_line_to(cr, xu, tx);
                  }
                }
              xs=1;
              }
            else if ((((xvn>=xu)&&(xu>=xv))||((xvn<=xu)&&(xu<=xv)))&&(((yvn>=yu)&&(yu>=yv))||((yvn<=yu)&&(yu<=yv))))
              {
              tx=((xvn-xv)*(yu-yv))/(yvn-yv);
              tx+=xv;
              if (tx<=xu)
                {
                cairo_move_to(cr, tx, yu);
                tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                tx+=yv;
                cairo_line_to(cr, xu, tx);
                }
              xs=1;
              }
            else xs=1;
            xv=xvn;
            yv=yvn;
            }
          cairo_stroke(cr);
          }
        else if (((priv->flaga)&8)!=0)
          {
          if ((yv<=yl)&&(xv<=xu)) {xs=0; cairo_move_to(cr, xv, yv);}
          else xs=1;
          for (j=1; j<(plot->size); j++)
            {
            xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            if ((yvn<=yl)&&(xvn<=xu))
              {
              if (xs==1)
                {
                tx=yvn-yv;
                if ((tx<DZE)&&(tx>NZE)) cairo_move_to(cr, xu, yv);
                else
                  {
                  tx=((xvn-xv)*(yv-yl))/tx;
                  tx+=xv;
                  if ((tx<=xu)&&(yvn<yv)) cairo_move_to(cr, tx, yl);
                  else
                    {
                    tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                    tx+=yv;
                    cairo_move_to(cr, xu, tx);
                    }
                  }
                }
              cairo_line_to(cr, xvn, yvn);
              xs=0;
              }
            else if (xs==0)
              {
              tx=yvn-yv;
              if ((tx<DZE)&&(tx>NZE)) cairo_line_to(cr, xu, yv);
              else
                {
                tx=((xvn-xv)*(yl-yv))/tx;
                tx+=xv;
                if ((tx<=xu)&&(yvn>yv)) cairo_line_to(cr, tx, yl);
                else
                  {
                  tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_line_to(cr, xu, tx);
                  }
                }
              xs=1;
              }
            else if ((((xvn>=xu)&&(xu>=xv))||((xvn<=xu)&&(xu<=xv)))&&(((yvn>=yl)&&(yl>=yv))||((yvn<=yl)&&(yl<=yv))))
              {
              tx=((xvn-xv)*(yl-yv))/(yvn-yv);
              tx+=xv;
              if (tx<=xu)
                {
                cairo_move_to(cr, tx, yl);
                tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                tx+=yv;
                cairo_line_to(cr, xu, tx);
                }
              xs=1;
              }
            else xs=1;
            xv=xvn;
            yv=yvn;
            }
          cairo_stroke(cr);
          }
        else
          {
          if (xv<=xu) {xs=0; cairo_move_to(cr, xv, yv);}
          else xs=1;
          for (j=1; j<(plot->size); j++)
            {
            xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            if (xvn<=xu)
              {
              if (xs==1)
                {
                tx=((yvn-yv)*(xu-xv))/(xvn-xv); /* divide by zero handling? */
                tx+=yv;
                cairo_move_to(cr, xu, tx);
                }
              cairo_line_to(cr, xvn, yvn);
              xs=0;
              }
            else if (xs==0)
              {
              tx=((yvn-yv)*(xu-xv))/(xvn-xv);
              tx+=yv;
              cairo_line_to(cr, xu, tx);
              xs=1;
              }
            else xs=1;
            xv=xvn;
            yv=yvn;
            }
          cairo_stroke(cr);
          }
        }
      else if (((priv->flaga)&2)!=0)
        {
        if (((priv->flaga)&4)!=0)
          {
          if ((yv>=yu)&&(xv>=xl)) {xs=0; cairo_move_to(cr, xv, yv);}
          else xs=1;
          for (j=1; j<(plot->size); j++)
            {
            xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            if ((yvn>=yu)&&(xvn>=xl))
              {
              if (xs==1)
                {
                tx=yvn-yv;
                if ((tx<DZE)&&(tx>NZE)) cairo_move_to(cr, xl, yv);
                else
                  {
                  tx=((xvn-xv)*(yu-yv))/tx;
                  tx+=xv;
                  if ((tx>=xl)&&(yvn>yv)) cairo_move_to(cr, tx, yu);
                  else
                    {
                    tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                    tx+=yv;
                    cairo_move_to(cr, xl, tx);
                    }
                  }
                }
              cairo_line_to(cr, xvn, yvn);
              xs=0;
              }
            else if (xs==0)
              {
              tx=yvn-yv;
              if ((tx<DZE)&&(tx>NZE)) cairo_line_to(cr, xl, yv);
              else
                {
                tx=((xvn-xv)*(yu-yv))/tx;
                tx+=xv;
                if ((tx>=xl)&&(yvn<yv)) cairo_line_to(cr, tx, yu);
                else
                  {
                  tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_line_to(cr, xl, tx);
                  }
                }
              xs=1;
              }
            else if ((((xvn>=xl)&&(xl>=xv))||((xvn<=xl)&&(xl<=xv)))&&(((yvn>=yu)&&(yu>=yv))||((yvn<=yu)&&(yu<=yv))))
              {
              tx=((xvn-xv)*(yu-yv))/(yvn-yv);
              tx+=xv;
              if (tx>=xl)
                {
                cairo_move_to(cr, tx, yu);
                tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                tx+=yv;
                cairo_line_to(cr, xl, tx);
                }
              xs=1;
              }
            else xs=1;
            xv=xvn;
            yv=yvn;
            }
          cairo_stroke(cr);
          }
        else if (((priv->flaga)&8)!=0)
          {
          if ((yv<=yl)&&(xv>=xl)) {xs=0; cairo_move_to(cr, xv, yv);}
          else xs=1;
          for (j=1; j<(plot->size); j++)
            {
            xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            if ((yvn<=yl)&&(xvn>=xl))
              {
              if (xs==1)
                {
                tx=yvn-yv;
                if ((tx<DZE)&&(tx>NZE)) cairo_move_to(cr, xl, yv);
                else
                  {
                  tx=((xvn-xv)*(yv-yl))/tx;
                  tx+=xv;
                  if ((tx>=xl)&&(yvn<yv)) cairo_move_to(cr, tx, yl);
                  else
                    {
                    tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                    tx+=yv;
                    cairo_move_to(cr, xl, tx);
                    }
                  }
                }
              cairo_line_to(cr, xvn, yvn);
              xs=0;
              }
            else if (xs==0)
              {
              tx=yvn-yv;
              if ((tx<DZE)&&(tx>NZE)) cairo_line_to(cr, xl, yv);
              else
                {
                tx=((xvn-xv)*(yl-yv))/(yvn-yv);
                tx+=xv;
                if ((tx>=xl)&&(yvn>yv)) cairo_line_to(cr, tx, yl);
                else
                  {
                  tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_line_to(cr, xl, tx);
                  }
                }
              xs=1;
              }
            else if ((((xvn>=xl)&&(xl>=xv))||((xvn<=xl)&&(xl<=xv)))&&(((yvn>=yl)&&(yl>=yv))||((yvn<=yl)&&(yl<=yv))))
              {
              tx=((xvn-xv)*(yl-yv))/(yvn-yv);
              tx+=xv;
              if (tx>=xl)
                {
                cairo_move_to(cr, tx, yl);
                tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                tx+=yv;
                cairo_line_to(cr, xu, tx);
                }
              xs=1;
              }
            else xs=1;
            xv=xvn;
            yv=yvn;
            }
          cairo_stroke(cr);
          }
        else
          {
          if (xv>=xl) {xs=0; cairo_move_to(cr, xv, yv);}
          else xs=1;
          for (j=1; j<(plot->size); j++)
            {
            xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            if (xvn>=xl)
              {
              if (xs==1)
                {
                tx=((yvn-yv)*(xu-xv))/(xvn-xv); /* divide by zero handling? */
                tx+=yv;
                cairo_move_to(cr, xu, tx);
                }
              cairo_line_to(cr, xvn, yvn);
              xs=0;
              }
            else if (xs==0)
              {
              tx=((yvn-yv)*(xu-xv))/(xvn-xv);
              tx+=yv;
              cairo_line_to(cr, xu, tx);
              xs=1;
              }
            else xs=1;
            xv=xvn;
            yv=yvn;
            }
          cairo_stroke(cr);
          }
        }
      else if (((priv->flaga)&4)!=0)
        {
        if (yv>=yu) {xs=0; cairo_move_to(cr, xv, yv);}
        else xs=1;
        for (j=1; j<(plot->size); j++)
          {
          xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
          yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
          if (yvn>=yu)
            {
            if (xs==1)
              {
              tx=((xvn-xv)*(yu-yv))/(yvn-yv); /* divide by zero handling? */
              tx+=xv;
              cairo_move_to(cr, tx, yu);
              }
            cairo_line_to(cr, xvn, yvn);
            xs=0;
            }
          else if (xs==0)
            {
            tx=((xvn-xv)*(yu-yv))/(yvn-yv);
            tx+=xv;
            cairo_line_to(cr, tx, yu);
            xs=1;
            }
          else xs=1;
          xv=xvn;
          yv=yvn;
          }
        cairo_stroke(cr);
        }
      else if (((priv->flaga)&8)!=0)
        {
        if (yv<=yl) {xs=0; cairo_move_to(cr, xv, yv);}
        else xs=1;
        for (j=1; j<(plot->size); j++)
          {
          xvn=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
          yvn=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
          if (yvn<=yl)
            {
            if (xs==1)
              {
              tx=((xvn-xv)*(yv-yl))/(yv-yvn); /* divide by zero handling? */
              tx+=xv;
              cairo_move_to(cr, tx, yl);
              }
            cairo_line_to(cr, xvn, yvn);
            xs=0;
            }
          else if (xs==0)
            {
            tx=((xvn-xv)*(yl-yv))/(yvn-yv);
            tx+=xv;
            cairo_line_to(cr, tx, yl);
            xs=1;
            }
          else xs=1;
          xv=xvn;
          yv=yvn;
          }
        cairo_stroke(cr);
        }
      else
        {
        cairo_move_to(cr, xv, yv);
        for (j=1; j<(plot->size); j++)
          {
          xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
          yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
          cairo_line_to(cr, xv, yv);
          }
        cairo_stroke(cr);
        }
      }
    else if (((plot->flagd)&2)!=0) /* points only */
      {
      if (((priv->flaga)&1)!=0)
        {
        if (((priv->flaga)&4)!=0)
          {
          if ((yv>=yu)&&(xv<=xu)) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
          for (j=1; j<(plot->size); j++)
            {
            xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            if ((yv>=yu)&&(xv<=xu)) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
            }
          }
        else if (((priv->flaga)&8)!=0)
          {
          if ((yv<=yl)&&(xv<=xu)) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
          for (j=1; j<(plot->size); j++)
            {
            xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            if ((yv<=yl)&&(xv<=xu)) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
            }
          }
        else
          {
          if (xv<=xu) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
          for (j=1; j<(plot->size); j++)
            {
            xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            if (xv<=xu) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
            }
          }
        }
      else if (((priv->flaga)&2)!=0)
        {
        if (((priv->flaga)&4)!=0)
          {
          if ((yv>=yu)&&(xv>=xl)) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
          for (j=1; j<(plot->size); j++)
            {
            xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            if ((yv>=yu)&&(xv>=xl)) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
            }
          }
        else if (((priv->flaga)&8)!=0)
          {
          if ((yv<=yl)&&(xv>=xl)) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
          for (j=1; j<(plot->size); j++)
            {
            xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            if ((yv<=yl)&&(xv>=xl)) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
            }
          }
        else
          {
          if (xv>=xl) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
          for (j=1; j<(plot->size); j++)
            {
            xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
            yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
            if (xv>=xl) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
            }
          }
        }
      else if (((priv->flaga)&4)!=0)
        {
        if (yv>=yu) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
        for (j=1; j<(plot->size); j++)
          {
          xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
          yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
          if (yv>=yu) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
          }
        }
      else if (((priv->flaga)&8)!=0)
        {
        if (yv<=yl) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
        for (j=1; j<(plot->size); j++)
          {
          xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
          yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
          if (yv<=yl) {cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI); cairo_fill(cr);}
          }
        }
      else
        {
        cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI);
        cairo_fill(cr);
        for (j=1; j<(plot->size); j++)
          {
          xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, j)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
          yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, j)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
          cairo_arc(cr, xv, yv, (plot->ptsize), 0, TPI);
          cairo_fill(cr);
          }
        }
      }
    }
  }

static void plot_linear_redraw(GtkWidget *widget)
  {
  GdkRegion *region;

  if (!widget->window) return;
  region=gdk_drawable_get_clip_region(widget->window);
  gdk_window_invalidate_region(widget->window, region, TRUE);
  gdk_window_process_updates(widget->window, TRUE);
  gdk_region_destroy(region);
  }

gboolean plot_linear_update_scale(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx)
  {
  PlotLinearPrivate *priv;

  priv=PLOT_LINEAR_GET_PRIVATE(widget);
  priv->bounds.xmin=xn;
  priv->bounds.xmax=xx;
  priv->bounds.ymin=yn;
  priv->bounds.ymax=yx;
  plot_linear_redraw(widget);
  return FALSE;
  }

gboolean plot_linear_update_scale_pretty(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx)
  {
  PlotLinearPrivate *priv;
  gdouble num, num3;
  gint num2, lt, ut, tk;

  priv=PLOT_LINEAR_GET_PRIVATE(widget);
  (priv->xcs)=2;
  if (xn>0)
    {
    if (xx>=10)
      {
      num=log10(xx);
      num2=(gint)num;
      (priv->xcs)+=num2;
      }
    }
  else if (xx<0)
    {
    (priv->xcs)++;
    if (xn<=-10)
      {
      num=log10(-xn);
      num2=(gint)num;
      (priv->xcs)+=num2;
      }
    }
  else if (xn<=-10)
    {
    num=log10(-xn);
    num2=(gint)num;
    num2++;
    if (xx>=1000)
      {
      num=log10(xx);
      lt=(gint)num;
      if (lt>num2) (priv->xcs)+=lt;
      else (priv->xcs)+=num2;
      }
    else (priv->xcs)+=num2;
    }
  else if (xx>=100)
    {
    num=log10(xx);
    num2=(gint)num;
    (priv->xcs)+=num2;
    }
  else (priv->xcs)++;
  (priv->ycs)=2;
  if (yn>0)
    {
    if (yx>=10)
      {
      num=log10(yx);
      num2=(gint)num;
      (priv->ycs)+=num2;
      }
    }
  else if (yx<0)
    {
    (priv->ycs)++;
    if (yn<=-10)
      {
      num=log10(-yn);
      num2=(gint)num;
      (priv->ycs)+=num2;
      }
    }
  else if (yn<=-10)
    {
    num=log10(-yn);
    num2=(gint)num;
    num2++;
    if (yx>=1000)
      {
      num=log10(yx);
      lt=(gint)num;
      if (lt>num2) (priv->ycs)+=lt;
      else (priv->ycs)+=num2;
      }
    else (priv->ycs)+=num2;
    }
  else if (yx>=100)
    {
    num=log10(yx);
    num2=(gint)num;
    (priv->ycs)+=num2;
    }
  else (priv->ycs)++;
  num3=(xx-xn)/6;
  num=log10(num3);
  if (num>=0)
    {
    num2=(gint)num;
    num=fmod(num,1);
    }
  else
    {
    num2=floor(num);
    num=fmod(num,1);
    num++;
    }
  if (num==0)
    {
    lt=floor(xn/num3);
    ut=ceil(xx/num3);
    tk=(ut-lt);
    if (tk>6)
      {
      num3*=2;
      lt=floor(xn/num3);
      ut=ceil(xx/num3);
      tk=(ut-lt);
      }
    (priv->bounds.xmin)=(num3*(gdouble)lt);
    (priv->bounds.xmax)=(num3*(gdouble)ut);
    (priv->ticks.xj)=tk;
    }
  else if (num<0.301029997)
    {
    num=G_LN10*num2;
    num=exp(num);
    num3=2*num;
    lt=floor(xn/num3);
    ut=ceil(xx/num3);
    tk=(ut-lt);
    if (tk>6)
      {
      num3=5*num;
      lt=floor(xn/num3);
      ut=ceil(xx/num3);
      tk=(ut-lt);
      if (tk>6)
        {
        num3*=2;
        lt=floor(xn/num3);
        ut=ceil(xx/num3);
        tk=(ut-lt);
        }
      }
    (priv->bounds.xmin)=(num3*(gdouble)lt);
    (priv->bounds.xmax)=(num3*(gdouble)ut);
    (priv->ticks.xj)=tk;
    }
  else if (num<0.698970005)
    {
    num=G_LN10*num2;
    num=exp(num);
    num3=5*num;
    lt=floor(xn/num3);
    ut=ceil(xx/num3);
    tk=(ut-lt);
    if (tk>6)
      {
      num3*=2;
      lt=floor(xn/num3);
      ut=ceil(xx/num3);
      tk=(ut-lt);
      if (tk>6)
        {
        num3*=2;
        lt=floor(xn/num3);
        ut=ceil(xx/num3);
        tk=(ut-lt);
        }
      }
    (priv->bounds.xmin)=(num3*(gdouble)lt);
    (priv->bounds.xmax)=(num3*(gdouble)ut);
    (priv->ticks.xj)=tk;
    }
  else
    {
    num2++;
    num=G_LN10*num2;
    num3=exp(num);
    lt=floor(xn/num3);
    ut=ceil(xx/num3);
    tk=(ut-lt);
    if (tk>6)
      {
      num3*=2;
      lt=floor(xn/num3);
      ut=ceil(xx/num3);
      tk=(ut-lt);
      }
    (priv->bounds.xmin)=(num3*(gdouble)lt);
    (priv->bounds.xmax)=(num3*(gdouble)ut);
    (priv->ticks.xj)=tk;
    }
  if (num3<1)
    {
    (priv->xcs)++;
    num=log10(num3);
    num=-num;
    num2=ceil(num);
    (priv->xcs)+=num2;
    }
  if ((priv->xcs)>10) (priv->xcs)=10;
  num3=(yx-yn)/6;
  num=log10(num3);
  num2=floor(num);
  num=fmod(num,1);
  if (num==0)
    {
    lt=floor(yn/num3);
    ut=ceil(yx/num3);
    tk=(ut-lt);
    if (tk>6)
      {
      num3*=2;
      lt=floor(yn/num3);
      ut=ceil(yx/num3);
      tk=(ut-lt);
      }
    (priv->bounds.ymin)=(num3*(gdouble)lt);
    (priv->bounds.ymax)=(num3*(gdouble)ut);
    (priv->ticks.yj)=tk;
    }
  else if (num<0.301029997)
    {
    num=G_LN10*num2;
    num=exp(num);
    num3=2*num;
    lt=floor(yn/num3);
    ut=ceil(yx/num3);
    tk=(ut-lt);
    if (tk>6)
      {
      num3=5*num;
      lt=floor(yn/num3);
      ut=ceil(yx/num3);
      tk=(ut-lt);
      if (tk>6)
        {
        num3*=2;
        lt=floor(yn/num3);
        ut=ceil(yx/num3);
        tk=(ut-lt);
        }
      }
    (priv->bounds.ymin)=(num3*(gdouble)lt);
    (priv->bounds.ymax)=(num3*(gdouble)ut);
    (priv->ticks.yj)=tk;
    }
  else if (num<0.698970005)
    {
    num=G_LN10*num2;
    num=exp(num);
    num3=5*num;
    lt=floor(yn/num3);
    ut=ceil(yx/num3);
    tk=(ut-lt);
    if (tk>6)
      {
      num3*=2;
      lt=floor(yn/num3);
      ut=ceil(yx/num3);
      tk=(ut-lt);
      if (tk>6)
        {
        num3*=2;
        lt=floor(yn/num3);
        ut=ceil(yx/num3);
        tk=(ut-lt);
        }
      }
    (priv->bounds.ymin)=(num3*(gdouble)lt);
    (priv->bounds.ymax)=(num3*(gdouble)ut);
    (priv->ticks.yj)=tk;
    }
  else
    {
    num2++;
    num=G_LN10*num2;
    num3=exp(num);
    lt=floor(yn/num3);
    ut=ceil(yx/num3);
    tk=(ut-lt);
    if (tk>6)
      {
      num3*=2;
      lt=floor(yn/num3);
      ut=ceil(yx/num3);
      tk=(ut-lt);
      }
    (priv->bounds.ymin)=(num3*(gdouble)lt);
    (priv->bounds.ymax)=(num3*(gdouble)ut);
    (priv->ticks.yj)=tk;
    }
  if (num3<1)
    {
    (priv->ycs)++;
    num=log10(num3);
    num=-num;
    num2=ceil(num);
    (priv->ycs)+=num2;
    }
  if ((priv->ycs)>10) (priv->ycs)=10;
  plot_linear_redraw(widget);
  return FALSE;
  }

gboolean plot_linear_print_eps(GtkWidget *plot, gchar* fout)
  {
  cairo_t *cr;
  cairo_surface_t *surface;

  surface=cairo_ps_surface_create(fout, (gdouble) plot->allocation.width, (gdouble) plot->allocation.height);
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

static gboolean plot_linear_button_press(GtkWidget *widget, GdkEventButton *event)
  {
  PlotLinearPrivate *priv;
  PlotLinear *plot;
  gint d;

  priv=PLOT_LINEAR_GET_PRIVATE(widget);
  plot=PLOT_LINEAR(widget);
  if ((priv->flagr)==0)
    {
    (priv->rescale.xmin)=(priv->bounds.xmin);
    (priv->rescale.ymin)=(priv->bounds.ymin);
    if (((plot->zmode)&10)!=0)
      {
      if ((event->x)>=(priv->range.xn)) (priv->rescale.xmin)=(priv->bounds.xmax);
      else
        {
        d=(event->x)-(priv->range.xj);
        if (d>0) {(priv->rescale.xmin)+=(((priv->bounds.xmax)-(priv->bounds.xmin))*d)/((priv->range.xn)-(priv->range.xj)); (priv->flagr)=1;}
        }
      }
    if (((plot->zmode)&12)!=0)
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

static gboolean plot_linear_motion_notify(GtkWidget *widget, GdkEventMotion *event)
  {
  PlotLinearPrivate *priv;
  PlotLinear *plot;
  gdouble dx, dy;

  priv=PLOT_LINEAR_GET_PRIVATE(widget);
  plot=PLOT_LINEAR(widget);
  dx=((event->x)-(priv->range.xj))/((priv->range.xn)-(priv->range.xj));
  dy=((priv->range.yn)-(event->y))/((priv->range.yn)-(priv->range.yj));
  if ((dx>=0)&&(dy>=0)&&(dx<=1)&&(dy<=1))
    {
    (plot->xps)=((priv->bounds.xmax)*dx)+((priv->bounds.xmin)*(1-dx));
    (plot->yps)=((priv->bounds.ymax)*dy)+((priv->bounds.ymin)*(1-dy));
    g_signal_emit(plot, plot_linear_signals[MOVED], 0);
    }
  return FALSE;
  }

static gboolean plot_linear_button_release(GtkWidget *widget, GdkEventButton *event)
  {
  PlotLinearPrivate *priv;
  PlotLinear *plot;
  gint d, xw;
  gdouble xn, xx, yn, yx, s;

  priv=PLOT_LINEAR_GET_PRIVATE(widget);
  plot=PLOT_LINEAR(widget);
  if ((priv->flagr)==1)
    {
    if (((plot->zmode)&8)==0)
      {
      (priv->rescale.xmax)=(priv->bounds.xmax);
      (priv->rescale.ymax)=(priv->bounds.ymax);
      if (((plot->zmode)&2)!=0)
        {
        if ((event->x)<=(priv->range.xj)) (priv->rescale.xmax)=(priv->bounds.xmin);
        else
          {
          d=(priv->range.xn)-(event->x);
          if (d>0) (priv->rescale.xmax)-=(((priv->bounds.xmax)-(priv->bounds.xmin))*d)/((priv->range.xn)-(priv->range.xj));
          }
        }
      if (((plot->zmode)&4)!=0)
        {
        if ((event->y)>=(priv->range.yn)) (priv->rescale.ymax)=(priv->bounds.ymin);
        else
          {
          d=(event->y)-(priv->range.yj);
          if (d>0) (priv->rescale.ymax)-=(((priv->bounds.ymax)-(priv->bounds.ymin))*d)/((priv->range.yn)-(priv->range.yj));
          }
        }
      if (((plot->zmode)&1)==0) /* move checks into updatepretty */
        {
        if ((priv->rescale.xmax)>(priv->rescale.xmin))
          {
          xn=(priv->rescale.xmin);
          xx=(priv->rescale.xmax);
          if ((priv->rescale.ymax)>(priv->rescale.ymin))
            {
            yn=(priv->rescale.ymin);
            yx=(priv->rescale.ymax);
            plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
            }
          else if ((priv->rescale.ymax)<(priv->rescale.ymin))
            {
            yn=(priv->rescale.ymax);
            yx=(priv->rescale.ymin);
            plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
            }
          }
        else if ((priv->rescale.xmax)<(priv->rescale.xmin))
          {
          xn=(priv->rescale.xmax);
          xx=(priv->rescale.xmin);
          if ((priv->rescale.ymax)>(priv->rescale.ymin))
            {
            yn=(priv->rescale.ymin);
            yx=(priv->rescale.ymax);
            plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
            }
          else if ((priv->rescale.ymax)<(priv->rescale.ymin))
            {
            yn=(priv->rescale.ymax);
            yx=(priv->rescale.ymin);
            plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
            }
          }
        }
      else
        {
        s=((priv->bounds.xmax)-(priv->bounds.xmin))/((priv->rescale.xmax)-(priv->rescale.xmin));
        if (s>0)
          {
          xn=((priv->bounds.xmin)-(priv->rescale.xmin))*s;
          xn+=(priv->bounds.xmin);
          xx=((priv->bounds.xmax)-(priv->rescale.xmax))*s;
          xx+=(priv->bounds.xmax);
          s=((priv->bounds.ymax)-(priv->bounds.ymin))/((priv->rescale.ymax)-(priv->rescale.ymin));
          if (s>0)
            {
            yn=((priv->bounds.ymin)-(priv->rescale.ymin))*s;
            yn+=(priv->bounds.ymin);
            yx=((priv->bounds.ymax)-(priv->rescale.ymax))*s;
            yx+=(priv->bounds.ymax);
            plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
            }
          else if (s<0)
            {
            yn=((priv->rescale.ymax)-(priv->bounds.ymin))*s;
            yn+=(priv->bounds.ymin);
            yx=((priv->rescale.ymin)-(priv->bounds.ymax))*s;
            yx+=(priv->bounds.ymax);
            plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
            }
          }
        else if (s<0)
          {
          xn=((priv->rescale.xmax)-(priv->bounds.xmin))*s;
          xn+=(priv->bounds.xmin);
          xx=((priv->rescale.xmin)-(priv->bounds.xmax))*s;
          xx+=(priv->bounds.xmax);
          s=((priv->bounds.ymax)-(priv->bounds.ymin))/((priv->rescale.ymax)-(priv->rescale.ymin));
          if (s>0)
            {
            yn=((priv->bounds.ymin)-(priv->rescale.ymin))*s;
            yn+=(priv->bounds.ymin);
            yx=((priv->bounds.ymax)-(priv->rescale.ymax))*s;
            yx+=(priv->bounds.ymax);
            plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
            }
          else if (s<0)
            {
            yn=((priv->rescale.ymax)-(priv->bounds.ymin))*s;
            yn+=(priv->bounds.ymin);
            yx=((priv->rescale.ymin)-(priv->bounds.ymax))*s;
            yx+=(priv->bounds.ymax);
            plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
            }
          }
        }
      }
    else if (((plot->zmode)&1)==0)
      {
      xn=((priv->rescale.xmin)*ZS)+(ZSC*(priv->bounds.xmin));
      xx=((priv->rescale.xmin)*ZS)+(ZSC*(priv->bounds.xmax));
      yn=((priv->rescale.ymin)*ZS)+(ZSC*(priv->bounds.ymin));
      yx=((priv->rescale.ymin)*ZS)+(ZSC*(priv->bounds.ymax));
      plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
      }
    else
      {
      xn=((priv->bounds.xmin)*UZ)-(UZC*(priv->rescale.xmin));
      xx=((priv->bounds.xmax)*UZ)-(UZC*(priv->rescale.xmin));
      yn=((priv->bounds.ymin)*UZ)-(UZC*(priv->rescale.ymin));
      yx=((priv->bounds.ymax)*UZ)-(UZC*(priv->rescale.ymin));
      plot_linear_update_scale_pretty(widget, xn, xx, yn, yx);
      }
    (priv->flagr)=0;
    }
  else if ((event->y)<=10)
    {
    xw=(widget->allocation.width);
    if ((event->x)>=xw-20)
      {
      if ((event->x)>xw-10) {(plot->zmode)^=1; plot_linear_redraw(widget);}
      else if (((plot->zmode)&8)!=0) {(plot->zmode)&=1; plot_linear_redraw(widget);}
      else {(plot->zmode)++; (plot->zmode)++; plot_linear_redraw(widget);}
      }
    }
  return FALSE;
  }

static gboolean plot_linear_expose(GtkWidget *plot, GdkEventExpose *event)
  {
  cairo_t *cr;

  cr=gdk_cairo_create(plot->window);
  cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
  cairo_clip(cr);
  draw(plot, cr);
  cairo_destroy(cr);
  return FALSE;
  }

static void plot_linear_class_init(PlotLinearClass *klass)
  {
  GObjectClass *obj_klass;
  GtkWidgetClass *widget_klass;

  obj_klass=G_OBJECT_CLASS(klass);
  g_type_class_add_private(obj_klass, sizeof(PlotLinearPrivate));
  obj_klass->finalize=(GObjectFinalizeFunc) plot_linear_finalise;
  obj_klass->set_property=plot_linear_set_property;
  obj_klass->get_property=plot_linear_get_property;
  g_object_class_install_property(obj_klass, PROP_BXN, g_param_spec_double("xmin", "Minimum x value", "Minimum value for the horizontal scale", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_BXX, g_param_spec_double("xmax", "Maximum x value", "Maximum value for the horizontal scale", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_BYN, g_param_spec_double("ymin", "Minimum y value", "Minimum value for the vertical scale", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_BYX, g_param_spec_double("ymax", "Maximum y value", "Maximum value for the vertical scale", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_XTJ, g_param_spec_uint("xbigticks", "Major x ticks-1", "Number of gaps between major ticks for the horizontal scale-1", 1, G_MAXINT, 4, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_YTJ, g_param_spec_uint("ybigticks", "Major y ticks-1", "Number of gaps between major ticks for the vertical scale-1", 1, G_MAXINT, 4, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_XTN, g_param_spec_uint("xsmallticks", "Minor x ticks+1", "Number of unlabelled tick divisions between major ticks for the horizontal scale+1", 1, G_MAXINT, 5, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_YTN, g_param_spec_uint("ysmallticks", "Minor y ticks+1", "Number of unlabelled ticks divisions between major ticks for the vertical scale+1", 1, G_MAXINT, 5, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_XC, g_param_spec_uint("xchar", "x label characters", "Number of characters to store x label strings", 1, 10, 5, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_YC, g_param_spec_uint("ychar", "y label characters", "Number of characters to store y label strings", 1, 10, 5, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_FA, g_param_spec_flags("aflag", "Axis Flags", "Flags for axes behaviour: 32 = Labels right, 16 = Labels above, 8 = Wiggle on top, 4 = Wiggle underneath, 2 = Wiggle on Right, 1 = Wiggle on left", G_TYPE_FLAGS, 0, G_PARAM_READWRITE));
  widget_klass=GTK_WIDGET_CLASS(klass);
  widget_klass->button_press_event=plot_linear_button_press;
  widget_klass->motion_notify_event=plot_linear_motion_notify;
  widget_klass->button_release_event=plot_linear_button_release;
  widget_klass->expose_event=plot_linear_expose;
  plot_linear_signals[MOVED]=g_signal_new("moved", G_OBJECT_CLASS_TYPE(obj_klass), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (PlotLinearClass, moved), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
  }

static void plot_linear_init(PlotLinear *plot)
  {
  PlotLinearPrivate *priv;

  gtk_widget_add_events(GTK_WIDGET(plot), GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);
  priv=PLOT_LINEAR_GET_PRIVATE(plot);
  priv->bounds.xmin=0;
  priv->bounds.xmax=1;
  priv->bounds.ymin=0;
  priv->bounds.ymax=1;
  priv->ticks.xj=4;
  priv->ticks.yj=4;
  priv->ticks.xn=5;
  priv->ticks.yn=5;
  priv->range.xj=0;
  priv->range.yj=0;
  priv->range.xn=1;
  priv->range.yn=1;
  priv->xcs=5;
  priv->ycs=5;
  priv->flaga=0;
  priv->flagr=0;
  plot->xdata=NULL;
  plot->ydata=NULL;
  plot->size=0;
  plot->xlab=g_strdup("Domain");
  plot->ylab=g_strdup("Amplitude");
  plot->afsize=12;
  plot->lfsize=12;
  plot->ptsize=5;
  plot->linew=2;
  plot->zmode=6;
  plot->xps=0;
  plot->yps=0;
  plot->flagd=1;
  }

GtkWidget *plot_linear_new(void)
  {
  return g_object_new(PLOT_TYPE_LINEAR, NULL);
  }
