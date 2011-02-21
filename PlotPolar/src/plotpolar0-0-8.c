/***************************************************************************
 *            plotpolar.c
 *
 *  A GTK+ widget that plots data on polar axes
 *  version 0.1.0
 *  Features:
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
 * but WITHOUT ANY WARRANTY; without even the implied warranty ofpriv->ticks.zin
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

/* TO DO:
 * radial/degree case for labelling
 * draw data
 * ensure new wrapping handling implemented in draw
 * change zoom box images
 * flat azimuthal tick labels: need to change fitting routines for x labels.
 * Don't display labels for z2m or border radial
 */

#include <gtk/gtk.h>
#include <math.h>
#include <cairo-ps.h>
#include "plotpolar0-1-0.h"

#define PLOT_POLAR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), PLOT_TYPE_POLAR, PlotPolarPrivate))
#define ARP 0.05 /* Proportion of the graph occupied by arrows */
#define IRTR 0.577350269 /* 1/square root 3 */
#define TPI 6.28318530718 /* 2pi */
#define WGP 0.08 /* Proportion of the graph the centre hole occupies */
#define WFP 0.01 /* Proportion of wiggle that is flat to axis */
#define WMP 0.045 /* the mean of these */
#define WHP 0.020207259 /* wiggle height proportion */
#define DZE 0.00001 /* divide by zero threshold */
#define NZE -0.00001 /* negative of this */
#define JT 5 /* major tick length */
#define JTI 6 /* this incremented */
#define ZS 0.5 /* zoom scale */
#define ZSC 0.5 /* 1 minus this */
#define UZ 2 /* inverse of this */
#define UZC 1 /* this minus 1 */
#define WGS 15.0 /* default zc value */
#define MY_PI_6  0.5235987755982988730771072305465838140328615665625 /* pi/6 etc. */
#define MY_PI_12 0.2617993877991494365385536152732919070164307832813
#define MY_PI_18 0.1745329251994329576923690768488612713442871888542
#define MY_PI_36 0.0872664625997164788461845384244306356721435944271
#define MY_PI_72 0.0436332312998582394230922692122153178360717972135
#define MY_PI_90  0.0349065850398865915384738153697722542688574377708
#define MY_PI_180 0.0174532925199432957692369076848861271344287188854
#define MY_PI_360 0.0087266462599716478846184538424430635672143594427
#define 2_MY_PI 6.2831853071795864769252867665590057683943387987502
#define 3_MY_PI_4 2.3561944901923449288469825374596271631478770495313
#define N2_MY_PI -6.2831853071795864769252867665590057683943387987502
#define NMY_PI -3.1415926535897932384626433832795028841971693993751
#define NMY_PI_2 -1.5707963267948961192313216916397514420985846996876
#define NMY_PI_4 -0.78539816339744830961566084581987572104929234984378
#define N3_MY_PI_4 2.3561944901923449288469825374596271631478770495313
#define I180_MY_PI 57.295779513082320876798154814105170332405472466564
G_DEFINE_TYPE (PlotPolar, plot_polar, GTK_TYPE_DRAWING_AREA);
enum
  {
  PROP_0,
  PROP_BRN,
  PROP_BRX,
  PROP_BTN,
  PROP_BTX,
  PROP_CR,
  PROP_CT,
  PROP_RTJ,
  PROP_TTJ,
  PROP_RTN,
  PROP_TTN,
  PROP_RC,
  PROP_TC
  };

enum {MOVED, LAST_SIGNAL};
static guint plot_polar_signals[LAST_SIGNAL]={0};
struct xs {gdouble rmin, thmin, rmax, thmax;};
struct tk {guint r, zin, z2m; gdouble zc;};
struct pt {gdouble r, th;};
typedef struct _PlotPolarPrivate PlotPolarPrivate;
struct _PlotPolarPrivate
  {
  struct xs bounds, rescale;
  struct pt centre;
  struct tk ticks;
  gint x0, y0;
  gdouble s;
  guint rcs, thcs, flaga, flagr; /* flaga: 0/1xb=bottom/top, x0/1b=ccw/cw */
  };

static void plot_polar_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
  {
  PlotPolarPrivate *priv;

  priv=PLOT_POLAR_GET_PRIVATE(object);
  switch (prop_id)
    {
    case PROP_BRN:
      (priv->bounds.rmin)=g_value_get_double(value);
      break;
    case PROP_BRX:
      (priv->bounds.rmax)=g_value_get_double(value);
      break;
    case PROP_BTN:
      (priv->bounds.thmin)=g_value_get_double(value);
      break;
    case PROP_BTX:
      (priv->bounds.thmax)=g_value_get_double(value);
      break;
    case PROP_CR:
      (priv->centre.r)=g_value_get_double(value);
      break;
    case PROP_CT:
      (priv->centre.th)=g_value_get_double(value);
      break;
    case PROP_RT:
      (priv->ticks.r)=g_value_get_uint(value);
      break;
    case PROP_ZIT:
      (priv->ticks.zin)=g_value_get_uint(value);
      break;
    case PROP_ZTM:
      (priv->ticks.z2m)=g_value_get_uint(value);
      break;
    case PROP_ZC:
      (priv->ticks.zc)=g_value_get_uint(value);
      break;
    case PROP_RC:
      (priv->rcs)=g_value_get_uint(value);
      break;
    case PROP_TC:
      (priv->thcs)=g_value_get_uint(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
    }
  }

static void plot_polar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
  {
  PlotPolarPrivate *priv;

  priv=PLOT_POLAR_GET_PRIVATE(object);
  switch (prop_id)
    {
    case PROP_BRN:
      g_value_set_double(value, (priv->bounds.rmin));
      break;
    case PROP_BRX:
      g_value_set_double(value, (priv->bounds.rmax));
      break;
    case PROP_BTN:
      g_value_set_double(value, (priv->bounds.thmin));
      break;
    case PROP_BTX:
      g_value_set_double(value, (priv->bounds.thmax));
      break;
    case PROP_CR:
      g_value_set_double(value, (priv->centre.r));
      break;
    case PROP_CT:
      g_value_set_double(value, (priv->centre.th));
      break;
    case PROP_RT:
      g_value_set_uint(value, (priv->ticks.r));
      break;
    case PROP_ZIT:
      g_value_set_uint(value, (priv->ticks.zin));
      break;
    case PROP_ZTM:
      g_value_set_uint(value, (priv->ticks.z2m));
      break;
    case PROP_ZC:
      g_value_set_uint(value, (priv->ticks.zc));
      break;
    case PROP_RC:
      g_value_set_uint(value, (priv->rcs));
      break;
    case PROP_TC:
      g_value_set_uint(value, (priv->thcs));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
    }
  }

static void plot_polar_finalise(PlotPolar *plot)
  {
  PlotPolarPrivate *priv;

  priv=PLOT_POLAR_GET_PRIVATE(plot);
  if (plot->rlab) g_free(plot->rlab);
  if (plot->thlab) g_free(plot->thlab);
  if (plot->rdata) g_free(plot->rdata);
  if (plot->thdata) g_free(plot->thdata);
  }

static void draw(GtkWidget *widget, cairo_t *cr)
  {
  PlotPolarPrivate *priv;
  PlotPolar *plot;
  gint j, k, xw, yw, dtt, kx;
  gdouble thx, thn, dt, wr, sx, dr1, drs, drc, dz, rt, dwr, rl, dl;
  gchar lbl[10];
  cairo_text_extents_t extents;
  cairo_matrix_t mtr, mtr2;

  xw=widget->allocation.width;
  yw=widget->allocation.height;
  plot=PLOT_POLAR(widget);
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
  priv=PLOT_POLAR_GET_PRIVATE(plot);
  if ((priv->bounds.rmax)<0) {(priv->bounds.rmax)=-(priv->bounds.rmax); (priv->bounds.rmin)=-(priv->bounds.rmin);}
  if ((priv->bounds.rmax)<(priv->bounds.rmin))
    {
    dt=(priv->bounds.rmax);
    (priv->bounds.rmax)=(priv->bounds.rmin);
    (priv->bounds.rmin)=dt;
    }
  if ((priv->bounds.rmin)<=0) {(priv->bounds.rmin)=0; wr=0;}
  else wr=WGP*sqrt(xw*yw);
  if ((priv->centre.r)<=0) (priv->centre.r)=0;
  else if ((priv->centre.r)<(priv->bounds.rmax)) (priv->centre.r)=(priv->bounds.rmax);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_line_width(cr, 2);
  cairo_select_font_face(cr, "sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, plot->afsize);
  dtt=(plot->afsize)+(plot->lfsize)+JTI;/* determine scale and fit for graph */
  mtr.xx=1;
  mtr.xy=0;
  mtr.yx=0;
  mtr.yy=1;
  dr1=(priv->bounds.rmax)-(priv->bounds.rmin);
  drs=((priv->centre.r)-(priv->bounds.rmin))*sin(priv->centre.th);
  drc=((priv->centre.r)-(priv->bounds.rmin))*cos(priv->centre.th);
  thx=(priv->bounds.thmax);
  while (thx>G_PI) thx-=2_MY_PI;
  while (thx<=-G_PI) thx+=2_MY_PI;
  thn=(priv->bounds.thmin);
  while (thn>G_PI) thn-=2_MY_PI;
  while (thn<=-G_PI) thn+=2_MY_PI;
  dt=thx-thn;
  if ((dt!=0)&&(dt!=2*G_PI)&&(dt!=-2*G_PI))
    {
    if (dt>0)
      {
      if (((priv->centre.th)<=(priv->bounds.thmax))&&((priv->centre.th)>=(priv->bounds.thmin)))
        {
        if (dt>=G_PI)
          {
          (priv->s)=dr1-drc;
          if ((priv->s)<DZE) (priv->s)=inf;
          else (priv->s)=((xw/2)-(wr*(1-cos(priv->centre.th))))/(priv->s); /* check x0 to xw radial to get maximum scale */
          if ((thx+thn)>=0)
            { /* radial label ccw top to axis azimuthal label bottom to axis */
            (priv->flaga)=2;
            sx=dr1-drs;
            if (sx>DZE)
              { /* check y0 to 0 radial to get maximum scale */
              sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            sx=(dr1*cos(thx))-drc;
            if ((sx>DZE)||(sx<NZE))
              { /* check x1'||x1"=0 to get maximum scale */
              if (thx>=3_MY_PI_4) sx=((wr*(cos(priv->centre.th)))-((dtt+wr)*cos(thx))-(xw/2))/sx;
              else sx=((wr*(cos(priv->centre.th)-cos(thx)))+(dtt*sin(thx))-(xw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if (thn<=NMY_PI_2)
              {
              sx=dr1+drs;
              if (sx>DZE)
                { /* check y0 to yw radial to get maximum scale */
                sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            else
              { /* check y2',y3'=yw to get maximum scale */
              sx=drs-(dr1*sin(thn));
              if ((sx>DZE)||(sx<NZE))
                {
                sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))+(yw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              if ((drs>DZE)||(drs<NZE))
                {
                sx=((wr*(sin(thx)-sin(priv->centre.th)))+(dtt*cos(thx))+(yw/2))/drs;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            }
          else
            { /* radial label cw bottom to axis azimuthal label top to axis */
            (priv->flaga)=1;
            sx=dr1+drs;
            if (sx>DZE)
              { /* check y0 to yw radial to get maximum scale */
              sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            sx=(dr1*cos(thn))-drc;
            if ((sx>DZE)||(sx<NZE))
              { /* check x2'|x2"=0 to get maximum scale */
              if (thn<=N3_MY_PI_4) sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thn))-(xw/2))/sx;
              else sx=((wr*(cos(priv->centre.th)-cos(thn)))-(dtt*sin(thn))-(xw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if (thx>=G_PI_2)
              { /* check y0 to 0 radial to get maximum scale */
              sx=dr1-drs;
              if (sx>DZE)
                {
                sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            else
              { /* check y1',y4'=0 to get maximum scale */
              sx=drs-(dr1*sin(thx));
              if ((sx>DZE)||(sx<NZE))
                {
                sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))-(yw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              if ((drs>DZE)||(drs<NZE))
                {
                sx=((wr*(sin(thn)-sin(priv->centre.th)))-(dtt*cos(thn))-(yw/2))/drs;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            }
          }
        else if ((thx+thn)>=G_PI)
          { /* radial label ccw top to axis azimuthal label bottom to axis */
          (priv->flaga)=2;
          (priv->s)=drs-(dr1*sin(thn)); /* check y3'=yw to get maximum scale */
          if ((s>DZE)||(s<NZE)) (priv->s)=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))+(yw/2))/(priv->s);
          else (priv->s)=inf;
          sx=(dr1*cos(thx))-drc;
          if ((sx>DZE)||(sx<NZE))
            {/* check x1'||x1"=0 to get maximum scale */
            if (thx>=3_MY_PI_4) sx=((wr*(cos(priv->centre.th)))-((wr+dtt)*cos(thx))-(xw/2))/sx;
            else sx=((wr*(cos(priv->centre.th)-cos(thx)))+(dtt*sin(thx))-(xw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if (thn>=G_PI_2)
            {
            sx=drs-(dr1*sin(thn));
            if ((sx>DZE)||(sx<NZE))
              {/* check y2'=0 to get maximum scale */
              sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))-(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if ((drc>DZE)||(drc<NZE))
              {/* check x4=xw to get maximum scale */
              sx=((wr*((cos(thn))-(cos(priv->centre.th))))-(xw/2))/drc;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          else
            {
            sx=dr1-drs;
            if (sx>DZE)
              {/* check y0 to 0 radial to get maximum scale */
              sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            sx=(dr1*cos(thn))-drc;
            if ((sx>DZE)||(sx<NZE))
              {/* check x2'=xw to get maximum scale */
              sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thn))+(xw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          }
        else if ((thx+thn)>=0)
          { /* radial label cw top to axis azimuthal label bottom to axis */
          (priv->flaga)=3;
          if (thn>=0)
            {/* check y4'=yw to get maximum scale */
            if ((drs>DZE)||(drs<NZE)) (priv->s)=((wr*((sin(thn))-(sin(priv->centre.th))))-(dtt*cos(thn))+(yw/2))/drs;
            else (priv->s)=inf;
            sx=(dr1*cos(thn))-drc;
            if ((sx>DZE)||(sx<NZE))
              {/* check x2"||x2'=xw to get maximum scale */
              if (thn>=G_PI_4) sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thn))+(xw/2))/sx;
              else sx=((wr*(cos(priv->centre.th)-cos(thn)))-(dtt*sin(thn))+(xw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          else
            {/* check x0 to xw radial to get maximum scale */
            (priv->s)=dr1-drc;
            if (sx>DZE) (priv->s)=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/(priv->s);
            else (priv->s)=inf;
            sx=drs-(dr1*sin(thn));
            if ((sx>DZE)||(sx<NZE))
              {/* check y2'||y2"=yw to get maximum scale */
              if (thn<=NMY_PI_4) sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))+(yw/2))/sx;
              else sx=((wr*(sin(thn)-sin(priv->centre.th)))-(dtt*cos(thn))+(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          if (thx>=G_PI_2)
            {
            sx=dr1-drs;
            if (sx>DZE)
              {/* check y0 to 0 radial to get maximum scale */
              sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            sx=(dr1*cos(thx))-drc;
            if ((sx>DZE)||(sx<NZE))
              {/* check x1'=0 to get maximum scale */
              sx=((wr*(cos(priv->centre.th)))-((dtt+wr)*cos(thx))-(xw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          else
            {
            sx=drs-(dr1*sin(thx));;
            if ((sx>DZE)||(sx<NZE))
              {/* check y1'=0 to get maximum scale */
              sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))-(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if ((drc>DZE)||(drc<NZE))
              {/* check x3=0 to get maximum scale */
              sx=((wr*((cos(thx))-(cos(priv->centre.th))))+(xw/2))/drc;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          }
        else if ((thx+thn)>=NMY_PI)
          { /* radial label ccw bottom to axis azimuthal label top to axis */
          (priv->flaga)=0;
          if (thx>=0)
            {/* check x0 to xw radial to get maximum scale */
            (priv->s)=dr1-drc;
            if (sx>DZE) (priv->s)=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/(priv->s);
            else (priv->s)=inf;
            sx=drs-(dr1*sin(thx));;
            if ((sx>DZE)||(sx<NZE))
              {/* check y1"||y1'=0 to get maximum scale */
              if (thx>=G_PI_4) sx=((wr*((sin(thx))-(sin(priv->centre.th)))-(dtt*cos(thx))-(yw/2))/sx;
              else sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))-(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          else
            {/* check y3'=0 to get maximum scale */
            if ((drs>DZE)||(drs<NZE)) (priv->s)=((wr*(sin(thx)-sin(priv->centre.th)))+(dtt*cos(thx))-(yw/2))/drs;
            else (priv->s)=inf;
            sx=sx=(dr1*cos(thx))-drc;
            if ((sx>DZE)||(sx<NZE))
              {/* check x1"||x1'=xw to get maximum scale */
              if (thx<=NMY_PI_4) sx=((wr*(cos(priv->centre.th)-cos(thx)))+(dtt*sin(thx))-(xw/2))/sx;
              else sx=((wr*(cos(priv->centre.th)))-((wr+dtt)*cos(thx))+(xw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          if (thn<=NMY_PI_2)
            {
            sx=dr1+drs;
            if (sx>DZE)
              {/* check y0 to yw radial to get maximum scale */
              sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            sx=(dr1*cos(thn))-drc;
            if ((sx>DZE)||(sx<NZE))
              {/* check x2'=0 to get maximum scale */
              sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thn))-(xw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          else
            {
            sx=drs-(dr1*sin(thn));
            if ((sx>DZE)||(sx<NZE))
              {/* check y2'=yw to get maximum scale */
              sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))+(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if ((drc>DZE)||(drc<NZE))
              {/* check x4=0 to get maximum scale */
              sx=((wr*((cos(thn))-(cos(priv->centre.th))))+(xw/2))/drc;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          }
        else
          { /* radial label cw bottom to axis azimuthal label top to axis */
          (priv->flaga)=1;
          if ((drs>DZE)||(drs<NZE)) (priv->s)=((wr*((sin(thn))-(sin(priv->centre.th))))-(dtt*cos(thn))-(yw/2))/drs; /* check y4'=0 to get maximum scale */
          else (priv->s)=inf;
          sx=(dr1*cos(thn))-drc;
          if ((sx>DZE)||(sx<NZE))
            {/* check x2'||x2"=0 to get maximum scale */
            if (thn<=N3MY_PI_4) sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thn))-(xw/2))/sx;
            else sx=((wr*(cos(priv->centre.th)-cos(thn)))-(dtt*sin(thn))-(xw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if (thx<=NMY_PI_2)
            {
            sx=drs-(dr1*sin(thx));
            if ((sx>DZE)||(sx<NZE))
              {/* check y1'=yw to get maximum scale */
              sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))+(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if ((drc>DZE)||(drc<NZE))
              {/* check x3=xw to get maximum scale */
              sx=((wr*((cos(thx))-(cos(priv->centre.th))))-(xw/2))/drc;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          else
            {
            sx=dr1+drs;
            if (sx>DZE)
              {/* check y0 to yw radial to get maximum scale */
              sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            sx=(dr1*cos(thx))-drc;
            if ((sx>DZE)||(sx<NZE))
              {/* check x1'=xw to get maximum scale */
              sx=((wr*(cos(priv->centre.th)))-((wr+dtt)*cos(thx))-(xw/2))/sx;;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          }
        }
      else /* centre radial line not between limits. Need to swap limits */
        {/* check radial x0 to 0 */
        (priv->s)=dr1+drc;
        if ((priv->s)>DZE) (priv->s)=((xw/2)-dtt-(wr*(1+cos(priv->centre.th))))/(priv->s);
        else (priv->s)=inf;
        if (dt<=G_PI)
          {
          if ((thx+thn)>=G_PI)
            { /* radial label cw bottom to axis azimuthal label top to axis */
            (priv->flaga)=1;
            sx=dr1+drs;
            if (sx>DZE)
              {/* check radial y0 to yw */
              sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            sx=dr1-drc;
            if (sx>DZE)
              {/* check radial x0 to xw */
              sx=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if (thn>=G_PI_2)
              {
              sx=dr1-drs;
              if (sx>DZE)
                {/* check radial y0 to 0 */
                sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            else
             {
              sx=drs-(dr1*sin(thn));
              if ((sx>DZE)||(sx<NZE))
                {/* check y1'=0 */
                sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))-(yw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              if (thx>=3MY_PI_4)
                {
                sx=drs-(dr1*sin(thx));
                if ((sx>DZE)||(sx<NZE))
                  {/* check y2"=0 to get maximum scale */
                  sx=((wr*(sin(thx)-sin(priv->centre.th)))-(dtt*cos(thx))-(yw/2))/sx;
                  if (sx<(priv->s)) (priv->s)=sx;
                  }
                }
              }
            }
          else if ((thx+thn)>=0)
            { /* radial label ccw bottom to axis azimuthal label top to axis */
            (priv->flaga)=0;
            sx=dr1+drs;
            if (sx>DZE)
              {/* check radial y0 to yw */
              sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if (thx>=G_PI_2)
              {
              sx=drs-(dr1*sin(thx));
              if ((sx>DZE)||(sx<NZE))
                {/* check y2'=0 */
                sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))-(yw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              if (thn>=0)
                {
                sx=dr1-drc;
                if (sx>DZE)
                  {/* check radial x0 to xw */
                  sx=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/sx;
                  if (sx<(priv->s)) (priv->s)=sx;
                  }
                if (thn<=G_PI_4)
                  {
                  sx=drs-(dr1*sin(thn));
                  if ((sx>DZE)||(sx<NZE))
                    {/* check y1"=0 */
                    sx=((wr*((sin(thn))-(sin(priv->centre.th)))-(dtt*cos(thn))-(yw/2))/sx;
                    if (sx<(priv->s)) (priv->s)=sx;
                    }
                  }
                }
              else
                {
                sx=(dr1*cos(thn))-drc;
                if ((sx>DZE)||(sx<NZE))
                  {/* check x1'||x1"=xw to get maximum scale */
                  if (thn>=NMY_PI_4) sx=((wr*(cos(priv->centre.th)))-((dtt+wr)*cos(thn))+(xw/2))/sx;
                  else sx=((wr*(cos(priv->centre.th)-cos(thn)))+(dtt*sin(thn))+(xw/2))/sx;
                  if (sx<(priv->s)) (priv->s)=sx;
                  }
                }
              }
            else
              {
              sx=dr1-drs;
              if (sx>DZE)
                {/* check radial y0 to 0 */
                sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              if (thn>=0)
                {
                sx=dr1-drc;
                if (sx>DZE)
                  {/* check radial x0 to xw */
                  sx=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/sx;
                  if (sx<(priv->s)) (priv->s)=sx;
                  }
                }
              else
                {
                sx=(dr1*cos(thn))-drc;
                if ((sx>DZE)||(sx<NZE))
                  {/* check x1'||x1"=xw to get maximum scale */
                  if (thn>=NMY_PI_4) sx=((wr*(cos(priv->centre.th)))-((dtt+wr)*cos(thn))+(xw/2))/sx;
                  else sx=((wr*(cos(priv->centre.th)-cos(thn)))+(dtt*sin(thn))+(xw/2))/sx;
                  if (sx<(priv->s)) (priv->s)=sx;
                  }
                }
              }
            }
          else if ((thx+thn)>=NMY_PI)
            { /* radial label cw top to axis azimuthal label bottom to axis */
            (priv->flaga)=3;
            sx=dr1-drs;
            if (sx>DZE)
              {/* check radial y0 to 0 */
              sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if (thn<=NMY_PI_2)
              {
              sx=drs-(dr1*sin(thn));
              if ((sx>DZE)||(sx<NZE))
                {/* check y1'=yw */
                sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))+(yw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              if (thx<=0)
                {
                sx=dr1-drc;
                if (sx>DZE)
                  {/* check radial x0 to xw */
                  sx=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/sx;
                  if (sx<(priv->s)) (priv->s)=sx;
                  }
                if (thx>=NMY_PI_4)
                  {
                  sx=drs-(dr1*sin(thx));
                  if ((sx>DZE)||(sx<NZE))
                    {/* check y2"=yw to get maximum scale */
                    sx=((wr*(sin(thx)-sin(priv->centre.th)))-(dtt*cos(thx))+(yw/2))/sx;
                    if (sx<(priv->s)) (priv->s)=sx;
                    }
                  }
                }
              else
                {
                sx=(dr1*cos(thx))-drc;
                if ((sx>DZE)||(sx<NZE))
                  {/* check x2'||x2"=xw to get maximum scale */
                  if (thx<=G_PI_4) sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thx))+(xw/2))/sx;
                  else sx=((wr*(cos(priv->centre.th)-cos(thx)))-(dtt*sin(thx))+(xw/2))/sx;
                  if (sx<(priv->s)) (priv->s)=sx;
                  }
                }
              }
            else
              {
              sx=dr1+drs;
              if (sx>DZE)
                {/* check radial y0 to yw */
                sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              if (thx<=0)
                {
                sx=dr1-drc;
                if (sx>DZE)
                  {/* check radial x0 to xw */
                  sx=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/sx;
                  if (sx<(priv->s)) (priv->s)=sx;
                  }
                }
              else
                {
                sx=(dr1*cos(thx))-drc;
                if ((sx>DZE)||(sx<NZE))
                  {/* check x2'||x2"=xw to get maximum scale */
                  if (thx<=G_PI_4) sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thx))+(xw/2))/sx;
                  else sx=((wr*(cos(priv->centre.th)-cos(thx)))-(dtt*sin(thx))+(xw/2))/sx;
                  if (sx<(priv->s)) (priv->s)=sx;
                  }
                }
              }
            }
          else
            { /* radial label ccw top to axis azimuthal label bottom to axis */
            (priv->flaga)=2;
            sx=dr1-drs;
            if (sx>DZE)
              {/* check radial y0 to 0 */
              sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            sx=dr1-drc;
            if (sx>DZE)
              {/* check radial x0 to xw */
              sx=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if (thx>=NMY_PI_2)
              {
              sx=drs-(dr1*sin(thx));
              if ((sx>DZE)||(sx<NZE))
                {/* check y2'=yw */
                sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))+(yw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              if (thn<=N3MY_PI_4)
                {
                sx=drs-(dr1*sin(thn));
                if ((sx>DZE)||(sx<NZE))
                  {/* check y1"=yw */
                  sx=((wr*((sin(thn))-(sin(priv->centre.th)))-(dtt*cos(thn))+(yw/2))/sx;
                  if (sx<(priv->s)) (priv->s)=sx;
                  }
                }
              }
            else
              {
              sx=dr1+drs;
              if (sx>DZE)
                {/* check radial y0 to yw */
                sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            }
          }
        else if ((thx+thn)>=0)
          { /* radial label cw bottom to axis azimuthal label top to axis */
          (priv->flaga)=1;
          if (thn<=NMY_PI_2)
            {
            sx=drs-(dr1*sin(thn));
            if ((sx>DZE)||(sx<NZE))
              {/* check y1'=yw */
              sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))+(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if ((drc>DZE)||(drc<NZE))
              {/* check x3=xw to get maximum scale */
              sx=((wr*((cos(thn))-(cos(priv->centre.th))))-(xw/2))/drc;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if (thx>=3MY_PI_4)
              {
              sx=drs-(dr1*sin(thx));
              if ((sx>DZE)||(sx<NZE))
                {/* check y2"=0 to get maximum scale */
                sx=((wr*(sin(thx)-sin(priv->centre.th)))-(dtt*cos(thx))-(yw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            else
              {
              sx=drs-(dr1*sin(thx));
              if ((sx>DZE)||(sx<NZE))
                {/* check y2'=0 */
              sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))-(yw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              if ((drc>DZE)||(drc<NZE))
                {/* check x4'=xw to get maximum scale */
                sx=((wr*((cos(thx))-(cos(priv->centre.th))))+(dtt*sin(thx))-(xw/2))/drc;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            }
          else
            {
            sx=dr1+drs;
            if (sx>DZE)
              {/* check radial y0 to yw */
              sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            sx=(dr1*cos(thn))-drc;
            if ((sx>DZE)||(sx<NZE))
              {/* check x1'=xw to get maximum scale */
              sx=((wr*(cos(priv->centre.th)))-((dtt+wr)*cos(thn))+(xw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            sx=drs-(dr1*sin(thx));
            if ((sx>DZE)||(sx<NZE))
              {/* check y2"||y2'=0 */
              if (thx>=3MY_PI_4) sx=((wr*(sin(thx)-sin(priv->centre.th)))-(dtt*cos(thx))-(yw/2))/sx;
              else sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))-(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          }
        else
          { /* radial label ccw top to axis azimuthal label bottom to axis */
          (priv->flaga)=2;
          if (thx>=G_PI_2)
            {
            sx=drs-(dr1*sin(thx));
            if ((sx>DZE)||(sx<NZE))
              {/* check y2'=0 */
              sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))-(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if ((drc>DZE)||(drc<NZE))
              {/* check x4=xw to get maximum scale */
              sx=((wr*((cos(thx))-(cos(priv->centre.th))))-(xw/2))/drc;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if (thn<=N3MY_PI_4)
              {
              sx=drs-(dr1*sin(thn));
              if ((sx>DZE)||(sx<NZE))
                {/* check y1"=yw */
                sx=((wr*((sin(thn))-(sin(priv->centre.th)))-(dtt*cos(thn))+(yw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            else
              {
              sx=drs-(dr1*sin(thn));
              if ((sx>DZE)||(sx<NZE))
                {/* check y1'=yw */
                sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))+(yw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              if ((drc>DZE)||(drc<NZE))
                {/* check x3'=xw to get maximum scale */
                sx=((wr*((cos(thn))-(cos(priv->centre.th))))-(dtt*sin(thn))-(xw/2))/drc;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            }
          else
            {
            sx=dr1-drs;
            if (sx>DZE)
              {/* check radial y0 to 0 */
              sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            sx=(dr1*cos(thx))-drc;
            if ((sx>DZE)||(sx<NZE))
              {/* check x2'=xw to get maximum scale */
              sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thx))+(xw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            sx=drs-(dr1*sin(thn));
            if ((sx>DZE)||(sx<NZE))
              {/* check y1"||y1'=yw */
              if (thn<=N3MY_PI_4) sx=((wr*((sin(thn))-(sin(priv->centre.th)))-(dtt*cos(thn))+(yw/2))/sx;
              else sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))+(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
             }
            }
          }
        dt=thx;
        thx=thn;
        thn=dt;
        }
      }
    else if (((priv->centre.th)>=(priv->bounds.thmax))&&((priv->centre.th)<=(priv->bounds.thmin)))
      {/* centre radial line not between limits. Need to swap limits */
      if (dt<=NMY_PI)
        {
        (priv->s)=dr1-drc;
        if ((priv->s)<DZE) (priv->s)=inf;
        else (priv->s)=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/(priv->s); /* check x0 to xw radial to get maximum scale */
        if ((thx+thn)>=0)
          { /* radial label ccw top to axis azimuthal label bottom to axis */
          (priv->flaga)=2;
          sx=dr1-drs;
          if (sx>DZE)
            { /* check y0 to 0 radial to get maximum scale */
            sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx
            if (sx<(priv->s)) (priv->s)=sx;
            }
          sx=(dr1*cos(thn))-drc;
          if ((sx>DZE)||(sx<NZE))
            { /* check x1'||x1"=0 to get maximum scale */
            if (thn>=3MY_PI_4) sx=((wr*(cos(priv->centre.th)-cos(thn)))-(dtt*cos(thn))-(xw/2))/sx;
            else sx=((wr*(cos(priv->centre.th)-cos(thn)))+(dtt*sin(thn))-(xw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if (thx<=NMY_PI_2)
            {
            sx=dr1+drs;
            if (sx>DZE)
              { /* check y0 to yw radial to get maximum scale */
              sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          else
            { /* check y2',y3'=yw to get maximum scale */
            sx=drs-(dr1*sin(thx));
            if ((sx>DZE)||(sx<NZE))
              {
              sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))+(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if ((drs>DZE)||(drs<NZE))
              {
              sx=((wr*(sin(thn)-sin(priv->centre.th)))+(dtt*cos(thn))+(yw/2))/drs;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          }
        else
          { /* radial label cw bottom to axis azimuthal label top to axis */
          (priv->flaga)=1;
          sx=dr1+drs;
          if (sx>DZE)
            { /* check y0 to yw radial to get maximum scale */
            sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          sx=(dr1*cos(thx))-drc;
          if ((sx>DZE)||(sx<NZE))
            { /* check x2'|x2"=0 to get maximum scale */
            if (thx<=N3MY_PI_4) sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thx))-(xw/2))/sx;
            else sx=((wr*(cos(priv->centre.th)-cos(thx)))-(dtt*sin(thx))-(xw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if (thn>=G_PI_2)
            { /* check y0 to 0 radial to get maximum scale */
            sx=dr1-drs;
            if (sx>DZE)
              {
              sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          else
            { /* check y1',y4'=0 to get maximum scale */
            sx=drs-(dr1*sin(thn));
            if ((sx>DZE)||(sx<NZE))
              {
              sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))-(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if ((drs>DZE)||(drs<NZE))
              {
              sx=((wr*(sin(thx)-sin(priv->centre.th)))-(dtt*cos(thx))-(yw/2))/drs;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          }
        }
      else if ((thx+thn)>=G_PI)
        { /* radial label ccw top to axis azimuthal label bottom to axis */
        (priv->flaga)=2;
        (priv->s)=drs-(dr1*sin(thx)); /* check y3'=yw to get maximum scale */
        if (((priv->s)>DZE)||((priv->s)<NZE)) (priv->s)=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))+(yw/2))/(priv->s);
        else (priv->s)=inf;
        sx=(dr1*cos(thn))-drc;
        if ((sx>DZE)||(sx<NZE))
          {/* check x1'||x1"=0 to get maximum scale */
          if (thn>=3MY_PI_4) sx=((wr*(cos(priv->centre.th)))-((wr+dtt)*cos(thn))-(xw/2))/sx;
          else sx=((wr*(cos(priv->centre.th)-cos(thn)))+(dtt*sin(thn))-(xw/2))/sx;
          if (sx<(priv->s)) (priv->s)=sx;
          }
        if (thx>=G_PI_2)
          {
          sx=drs-(dr1*sin(thx));
          if ((sx>DZE)||(sx<NZE))
            {/* check y2'=0 to get maximum scale */
            sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))-(yw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if ((drc>DZE)||(drc<NZE))
            {/* check x4=xw to get maximum scale */
            sx=((wr*((cos(thx))-(cos(priv->centre.th))))-(xw/2))/drc;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          }
        else
          {
          sx=dr1-drs;
          if (sx>DZE)
            {/* check y0 to 0 radial to get maximum scale */
            sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          sx=(dr1*cos(thx))-drc;
          if ((sx>DZE)||(sx<NZE))
            {/* check x2'=xw to get maximum scale */
            sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thx))+(xw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          }
        }
      else if ((thx+thn)>=0)
        { /* radial label cw top to axis azimuthal label bottom to axis */
        (priv->flaga)=3;
        if (thx>=0)
          {/* check y4'=yw to get maximum scale */
          if ((drs>DZE)||(drs<NZE)) (priv->s)=((wr*((sin(thx))-(sin(priv->centre.th))))-(dtt*cos(thx))+(yw/2))/drs;
          else (priv->s)=inf;
          sx=(dr1*cos(thx))-drc;
          if ((sx>DZE)||(sx<NZE))
            {/* check x2"||x2'=xw to get maximum scale */
            if (thx>=G_PI_4) sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thx))+(xw/2))/sx;
            else sx=((wr*(cos(priv->centre.th)-cos(thx)))-(dtt*sin(thx))+(xw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          }
        else
          {/* check x0 to xw radial to get maximum scale */
          s=dr1-drc;
          if ((priv->s)>DZE) (priv->s)=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/(priv->s);
          else (priv->s)=inf;
          sx=drs-(dr1*sin(thx));
          if ((sx>DZE)||(sx<NZE))
            {/* check y2'||y2"=yw to get maximum scale */
            if (thx<=NMY_PI_4) sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))+(yw/2))/sx;
            else sx=((wr*(sin(thx)-sin(priv->centre.th)))-(dtt*cos(thx))+(yw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          }
        if (thn>=G_PI_2)
          {
          sx=dr1-drs;
          if (sx>DZE)
            {/* check y0 to 0 radial to get maximum scale */
            sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          sx=(dr1*cos(thn))-drc;
          if ((sx>DZE)||(sx<NZE))
            {/* check x1'=0 to get maximum scale */
            sx=((wr*(cos(priv->centre.th)))-((dtt+wr)*cos(thn))-(xw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          }
        else
          {
          sx=drs-(dr1*sin(thn));;
          if ((sx>DZE)||(sx<NZE))
            {/* check y1'=0 to get maximum scale */
            sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))-(yw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if ((drc>DZE)||(drc<NZE))
            {/* check x3=0 to get maximum scale */
            sx=((wr*((cos(thn))-(cos(priv->centre.th))))+(xw/2))/drc;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          }
        }
      else if ((thx+thn)>=NMY_PI)
        { /* radial label ccw bottom to axis azimuthal label top to axis */
        (priv->flaga)=0;
        if (thn>=0)
          {/* check x0 to xw radial to get maximum scale */
          (priv->s)=dr1-drc;
          if ((priv->s)>DZE) (priv->s)=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/(priv->s);
          else (priv->s)=inf;
          sx=drs-(dr1*sin(thn));;
          if ((sx>DZE)||(sx<NZE))
            {/* check y1"||y1'=0 to get maximum scale */
            if (thn>=G_PI_4) sx=((wr*((sin(thn))-(sin(priv->centre.th)))-(dtt*cos(thn))-(yw/2))/sx;
            else sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))-(yw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          }
        else
          {/* check y3'=0 to get maximum scale */
          if ((drs>DZE)||(drs<NZE)) (priv->s)=((wr*(sin(thn)-sin(priv->centre.th)))+(dtt*cos(thn))-(yw/2))/drs;
          else (priv->s)=inf;
          sx=sx=(dr1*cos(thn))-drc;
          if ((sx>DZE)||(sx<NZE))
            {/* check x1"||x1'=xw to get maximum scale */
            if (thn<=NMY_PI_4) sx=((wr*(cos(priv->centre.th)-cos(thn)))+(dtt*sin(thn))-(xw/2))/sx;
            else sx=((wr*(cos(priv->centre.th)))-((wr+dtt)*cos(thn))+(xw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          }
        if (thx<=NMY_PI_2)
          {
          sx=dr1+drs;
          if (sx>DZE)
            {/* check y0 to yw radial to get maximum scale */
            sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          sx=(dr1*cos(thx))-drc;
          if ((sx>DZE)||(sx<NZE))
            {/* check x2'=0 to get maximum scale */
            sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thx))-(xw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          }
        else
          {
          sx=drs-(dr1*sin(thx));
          if ((sx>DZE)||(sx<NZE))
            {/* check y2'=yw to get maximum scale */
            sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))+(yw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if ((drc>DZE)||(drc<NZE))
            {/* check x4=0 to get maximum scale */
            sx=((wr*((cos(thx))-(cos(priv->centre.th))))+(xw/2))/drc;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          }
        }
      else
        { /* radial label cw bottom to axis azimuthal label top to axis */
        (priv->flaga)=1;
        /* check y4'=0 to get maximum scale */
        if ((drs>DZE)||(drs<NZE)) (priv->s)=((wr*((sin(thx))-(sin(priv->centre.th))))-(dtt*cos(thx))-(yw/2))/drs;
        else (priv->s)=inf;
        sx=(dr1*cos(thx))-drc;
        if ((sx>DZE)||(sx<NZE))
          {/* check x2'||x2"=0 to get maximum scale */
          if (thx<=N3MY_PI_4) sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thx))-(xw/2))/sx;
          else sx=((wr*(cos(priv->centre.th)-cos(thx)))-(dtt*sin(thx))-(xw/2))/sx;
          if (sx<(priv->s)) (priv->s)=sx;
          }
        if (thn<=NMY_PI_2)
          {
          sx=drs-(dr1*sin(thn));
          if ((sx>DZE)||(sx<NZE))
            {/* check y1'=yw to get maximum scale */
            sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))+(yw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if ((drc>DZE)||(drc<NZE))
            {/* check x3=xw to get maximum scale */
            sx=((wr*((cos(thn))-(cos(priv->centre.th))))-(xw/2))/drc;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          }
        else
          {
          sx=dr1+drs;
          if (sx>DZE)
            {/* check y0 to yw radial to get maximum scale */
            sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          sx=(dr1*cos(thn))-drc;
          if ((sx>DZE)||(sx<NZE))
            {/* check x1'=xw to get maximum scale */
            sx=((wr*(cos(priv->centre.th)))-((wr+dtt)*cos(thn))-(xw/2))/sx;;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          }
        }
      dt=thx;
      thx=thn;
      thn=dt;
      }
    else
      {/* check radial x0 to 0 */
      (priv->s)=dr1+drc;
      if ((priv->s)>DZE) (priv->s)=((xw/2)-dtt-(wr*(1+cos(priv->centre.th))))/(priv->s);
      else (priv->s)=inf;
      if (dt>=NMY_PI)
        {
        if ((thx+thn)>=G_PI)
          { /* radial label cw bottom to axis azimuthal label top to axis */
          (priv->flaga)=1;
          sx=dr1+drs;
          if (sx>DZE)
            {/* check radial y0 to yw */
            sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          sx=dr1-drc;
          if (sx>DZE)
            {/* check radial x0 to xw */
            sx=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if (thx>=G_PI_2)
            {
            sx=dr1-drs;
            if (sx>DZE)
              {/* check radial y0 to 0 */
              sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          else
            {
            sx=drs-(dr1*sin(thx));
            if ((sx>DZE)||(sx<NZE))
              {/* check y1'=0 */
              sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))-(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if (thn>=3MY_PI_4)
              {
              sx=drs-(dr1*sin(thn));
              if ((sx>DZE)||(sx<NZE))
                {/* check y2"=0 to get maximum scale */
                sx=((wr*(sin(thn)-sin(priv->centre.th)))-(dtt*cos(thn))-(yw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            }
          }
        else if ((thx+thn)>=0)
          { /* radial label ccw bottom to axis azimuthal label top to axis */
          (priv->flaga)=0;
          sx=dr1+drs;
          if (sx>DZE)
            {/* check radial y0 to yw */
            sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if (thn>=G_PI_2)
            {
            sx=drs-(dr1*sin(thn));
            if ((sx>DZE)||(sx<NZE))
              {/* check y2'=0 */
              sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))-(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if (thx>=0)
              {
              sx=dr1-drc;
              if (sx>DZE)
                {/* check radial x0 to xw */
                sx=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              if (thx<=G_PI_4)
                {
                sx=drs-(dr1*sin(thx));
                if ((sx>DZE)||(sx<NZE))
                  {/* check y1"=0 */
                  sx=((wr*((sin(thx))-(sin(priv->centre.th)))-(dtt*cos(thx))-(yw/2))/sx;
                  if (sx<(priv->s)) (priv->s)=sx;
                  }
                }
              }
            else
              {
              sx=(dr1*cos(thx))-drc;
              if ((sx>DZE)||(sx<NZE))
                {/* check x1'||x1"=xw to get maximum scale */
                if (thx>=NMY_PI_4) sx=((wr*(cos(priv->centre.th)))-((dtt+wr)*cos(thx))+(xw/2))/sx;
                else sx=((wr*(cos(priv->centre.th)-cos(thx)))+(dtt*sin(thx))+(xw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            }
          else
            {
            sx=dr1-drs;
            if (sx>DZE)
              {/* check radial y0 to 0 */
              sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if (thx>=0)
              {
              sx=dr1-drc;
              if (sx>DZE)
                {/* check radial x0 to xw */
                sx=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            else
              {
              sx=(dr1*cos(thx))-drc;
              if ((sx>DZE)||(sx<NZE))
                {/* check x1'||x1"=xw to get maximum scale */
                if (thx>=NMY_PI_4) sx=((wr*(cos(priv->centre.th)))-((dtt+wr)*cos(thx))+(xw/2))/sx;
                else sx=((wr*(cos(priv->centre.th)-cos(thx)))+(dtt*sin(thx))+(xw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            }
          }
        else if ((thx+thn)>=NMY_PI)
          { /* radial label cw top to axis azimuthal label bottom to axis */
          (priv->flaga)=3;
          sx=dr1-drs;
          if (sx>DZE)
            {/* check radial y0 to 0 */
            sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if (thx<=NMY_PI_2)
            {
            sx=drs-(dr1*sin(thx));
            if ((sx>DZE)||(sx<NZE))
              {/* check y1'=yw */
              sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))+(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if ((thn)<=0)
              {
              sx=dr1-drc;
              if (sx>DZE)
                {/* check radial x0 to xw */
                sx=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              if (thn>=NMY_PI_4)
                {
                sx=drs-(dr1*sin(thn));
                if ((sx>DZE)||(sx<NZE))
                  {/* check y2"=yw to get maximum scale */
                  sx=((wr*(sin(thn)-sin(priv->centre.th)))-(dtt*cos(thn))+(yw/2))/sx;
                  if (sx<(priv->s)) (priv->s)=sx;
                  }
                }
              }
            else
              {
              sx=(dr1*cos(thn))-drc;
              if ((sx>DZE)||(sx<NZE))
                {/* check x2'||x2"=xw to get maximum scale */
                if (thn<=G_PI_4) sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thn))+(xw/2))/sx;
                else sx=((wr*(cos(priv->centre.th)-cos(thn)))-(dtt*sin(thn))+(xw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            }
          else
            {
            sx=dr1+drs;
            if (sx>DZE)
              {/* check radial y0 to yw */
              sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if (thn<=0)
              {
              sx=dr1-drc;
              if (sx>DZE)
                {/* check radial x0 to xw */
                sx=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            else
              {
              sx=(dr1*cos(thn))-drc;
              if ((sx>DZE)||(sx<NZE))
                {/* check x2'||x2"=xw to get maximum scale */
                if (thn<=G_PI_4) sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thn))+(xw/2))/sx;
                else sx=((wr*(cos(priv->centre.th)-cos(thn)))-(dtt*sin(thn))+(xw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            }
          }
        else
          { /* radial label ccw top to axis azimuthal label bottom to axis */
          (priv->flaga)=2;
          sx=dr1-drs;
          if (sx>DZE)
            {/* check radial y0 to 0 */
            sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          sx=dr1-drc;
          if (sx>DZE)
            {/* check radial x0 to xw */
            sx=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if (thn>=NMY_PI_2)
            {
            sx=drs-(dr1*sin(thn));
            if ((sx>DZE)||(sx<NZE))
              {/* check y2'=yw */
              sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))+(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if (thx<=N3MY_PI_4)
              {
              sx=drs-(dr1*sin(thx));
              if ((sx>DZE)||(sx<NZE))
                {/* check y1"=yw */
                sx=((wr*((sin(thx))-(sin(priv->centre.th)))-(dtt*cos(thx))+(yw/2))/sx;
                if (sx<(priv->s)) (priv->s)=sx;
                }
              }
            }
          else
            {
            sx=dr1+drs;
            if (sx>DZE)
              {/* check radial y0 to yw */
              sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          }
        }
      else if ((thx+thn)>=0)
        { /* radial label cw bottom to axis azimuthal label top to axis */
        (priv->flaga)=1;
        if (thx<=NMY_PI_2)
          {
          sx=drs-(dr1*sin(thx));
           f ((sx>DZE)||(sx<NZE))
            {/* check y1'=yw */
            sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))+(yw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if ((drc>DZE)||(drc<NZE))
            {/* check x3=xw to get maximum scale */
            sx=((wr*((cos(thx))-(cos(priv->centre.th))))-(xw/2))/drc;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if (thn>=3MY_PI_4)
            {
            sx=drs-(dr1*sin(thn));
            if ((sx>DZE)||(sx<NZE))
              {/* check y2"=0 to get maximum scale */
              sx=((wr*(sin(thn)-sin(priv->centre.th)))-(dtt*cos(thn))-(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          else
            {
            sx=drs-(dr1*sin(thn));
            if ((sx>DZE)||(sx<NZE))
              {/* check y2'=0 */
              sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))-(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if ((drc>DZE)||(drc<NZE))
              {/* check x4'=xw to get maximum scale */
              sx=((wr*((cos(thn))-(cos(priv->centre.th))))+(dtt*sin(thn))-(xw/2))/drc;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          }
        else
          {
          sx=dr1+drs;
          if (sx>DZE)
            {/* check radial y0 to yw */
            sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          sx=(dr1*cos(thx))-drc;
          if ((sx>DZE)||(sx<NZE))
            {/* check x1'=xw to get maximum scale */
            sx=((wr*(cos(priv->centre.th)))-((dtt+wr)*cos(thx))+(xw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          sx=drs-(dr1*sin(thn));
          if ((sx>DZE)||(sx<NZE))
            {/* check y2"||y2'=0 */
            if (thn>=3MY_PI_4) sx=((wr*(sin(thn)-sin(priv->centre.th)))-(dtt*cos(thn))-(yw/2))/sx;
            else sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))-(yw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          }
        }
      else
        { /* radial label ccw top to axis azimuthal label bottom to axis */
        (priv->flaga)=2;
        if (thn>=G_PI_2)
          {
          sx=drs-(dr1*sin(thn));
          if ((sx>DZE)||(sx<NZE))
            {/* check y2'=0 */
            sx=(((wr+dtt)*sin(thn))-(wr*sin(priv->centre.th))-(yw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if ((drc>DZE)||(drc<NZE))
            {/* check x4=xw to get maximum scale */
            sx=((wr*((cos(thn))-(cos(priv->centre.th))))-(xw/2))/drc;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          if (thx<=N3MY_PI_4)
            {
            sx=drs-(dr1*sin(thx));
            if ((sx>DZE)||(sx<NZE))
              {/* check y1"=yw */
              sx=((wr*((sin(thx))-(sin(priv->centre.th)))-(dtt*cos(thx))+(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          else
            {
            sx=drs-(dr1*sin(thx));
            if ((sx>DZE)||(sx<NZE))
              {/* check y1'=yw */
              sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))+(yw/2))/sx;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            if ((drc>DZE)||(drc<NZE))
              {/* check x3'=xw to get maximum scale */
              sx=((wr*((cos(thx))-(cos(priv->centre.th))))-(dtt*sin(thx))-(xw/2))/drc;
              if (sx<(priv->s)) (priv->s)=sx;
              }
            }
          }
        else
          {
          sx=dr1-drs;
          if (sx>DZE)
            {/* check radial y0 to 0 */
            sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          sx=(dr1*cos(thn))-drc;
          if ((sx>DZE)||(sx<NZE))
            {/* check x2'=xw to get maximum scale */
            sx=((wr*cos(priv->centre.th))-((wr+dtt)*cos(thn))+(xw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          sx=drs-(dr1*sin(thx));
          if ((sx>DZE)||(sx<NZE))
            {/* check y1"||y1'=yw */
            if (thx<=N3MY_PI_4) sx=((wr*((sin(thx))-(sin(priv->centre.th)))-(dtt*cos(thx))+(yw/2))/sx;
            else sx=(((wr+dtt)*sin(thx))-(wr*sin(priv->centre.th))+(yw/2))/sx;
            if (sx<(priv->s)) (priv->s)=sx;
            }
          }
        }
      }
    (priv->x0)=(xw/2)-(drc*s)-(wr*cos(priv->centre.th));
    (priv->y0)=(yw/2)+(drs*s)+(wr*sin(priv->centre.th));
    sx=thn;
    kx=1<<(priv->ticks.z2m);
    dz=dt/(kx*(priv->ticks.zin));
    cairo_move_to(cr, (priv->x0)+(wr*cos(sx)), (priv->y0)-(wr*sin(sx)));
    cairo_line_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
    if ((priv->flaga)==0)
      {
      mtr2.xx=cos(thx);
      mtr2.xy=sin(thx);
      mtr2.yx=-mtr2.xy;
      mtr2.yy=mtr2.xx;
      if ((plot->flags)&1==0)
        {
        for (j=1; j<=(priv_>ticks.zin); j++)
          {
          for (k=1; k<kx; k++)
            {
            sx+=dz;/* additional azimuthal ticks */
            cairo_move_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
            cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
            cairo_stroke(cr);
            g_snprintf(lbl, (priv->thcs), "%f", sx*I180_MY_PI);
            cairo_text_extents(cr, lbl, &extents);
            dl=(extents.width)*tan(sx);/* inf error? */
            rl=wr+JTI+((priv->s)*dr1);
            if (abs(dl)>=(extents.height))
              {
              cairo_move_to(cr, (priv->x0)+((rl+((extents.height)/(2*sin(sx))))*cos(sx))-((extents.width)/2)-(extents.x_bearing), (priv->y0)-(rl*sin(sx))-(extents.height)-(extents.y_bearing));/* need to remove height from y if sx -ve */
              }
            else
              {
              cairo_move_to(cr, (priv->x0)+((rl+?)*cos(sx))-((extents.width)/2)-(extents.x_bearing), (priv->y0)+((wr+JT+?+((priv->s)*dr1))*sin(sx))-((extents.height)/2)-(extents.y_bearing));/* fix question mark */
              }
            cairo_show_text(cr, lbl);
            /* draw zaimuthal tick labels */
            }
          sx+=dz/* inner radial lines to tick */;
          cairo_move_to(cr, (priv->x0)+(wr*cos(sx)), (priv->y0)-(wr*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
          }
        }
      else
        {
        for (j=1; j<=(priv_>ticks.zin); j++)
          {
          for (k=1; k<kx; k++)
            {
            sx+=dz;/* additional azimuthal ticks */
            cairo_move_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
            cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
            g_snprintf(lbl, (priv->thcs), "%f", sx);
            /* draw zaimuthal tick labels */
            }
          sx+=dz/* inner radial lines to tick */;
          cairo_move_to(cr, (priv->x0)+(wr*cos(sx)), (priv->y0)-(wr*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
          }
        }
      cairo_stroke(cr);
      cairo_new_sub_path(cr);
      cairo_arc(cr, (priv->x0), (priv->y0), wr, -thx, -thn);
      rt=wr;
      dwr=dr*(priv->s)/(priv->ticks.r);
      kx=1;
      for (j=1; j<(priv->ticks.r); j++)
        {
        rt+=dwr;
        cairo_new_sub_path(cr);/* draw arc grids */
        cairo_arc(cr, (priv->x0), (priv->y0), rt, -(priv->bounds.thmax)-(JT/rt), -(priv->bounds.thmin));
        if ((rt*dt)>=((priv->ticks.zc)*kx*(priv->ticks.zin)))
          {/* add additional radials */
          dz=((priv->bounds.thmax)-(priv->bounds.thmin))/(kx*(priv->ticks.zin));
          sx=(priv->bounds.thmin)+(dz/2);
          cairo_move_to(cr, (priv->x0)+(rt*cos(sx)), (priv->y0)-(rt*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
          for (j=1; j<(kx*(priv->ticks.zin));j++)
            {
            sx+=dz;
            cairo_move_to(cr, (priv->x0)+(rt*cos(sx)), (priv->y0)-(rt*sin(sx)));
            cairo_line_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
            }
          kx*=2;
          }
        cairo_stroke(cr);
        g_snprintf(lbl, (priv->rcs), "%f", (priv->bounds.rmin)+((rt-wr)/(priv->s)));
        cairo_text_extents(cr, lbl, &extents);
        sx=rt-((extents.width)/2)-(extents.x_bearing);
        dz=-JTI-(extents.height)-(extents.y_bearing);
        cairo_move_to(cr, (priv->x0)+(sx*cos(thx))+(dz*sin(thx)), (priv->y0)+(dz*cos(thx))-(sx*sin(thx)));
        cairo_set_matrix(cr, &mtr2);
        cairo_show_text(cr, lbl);/* draw radial tick labels*/
        cairo_set_matrix(cr, &mtr);
        }
      rt+=dwr;
      cairo_new_sub_path(cr);
      cairo_arc(cr, (priv->x0), (priv->y0), rt, -thx, -thn);
      cairo_set_font_size(cr, plot->lfsize);
      cairo_text_extents(cr, (plot->rlab), &extents);
      sx=(dr1*(priv->s))-((extents.width)/2)-(extents.x_bearing);
      dz=-dtt-(extents.y_bearing);
      cairo_move_to(cr, (priv->x0)+(sx*cos(thx))+(dz*sin(thx)), (priv->y0)+(dz*cos(thx))-(sx*sin(thx)));
      cairo_set_matrix(cr, &mtr2);
      cairo_show_text(cr, lbl);/* draw radial label*/
      cairo_set_matrix(cr, &mtr);
      /* draw azimuthal label */
      }
    else if ((priv->flaga)==2)
      {
      mtr2.xx=-cos(thx);
      mtr2.xy=-sin(thx);
      mtr2.yx=-mtr2.xy;
      mtr2.yy=mtr2.xx;
      if ((plot->flags)&1==0)
        {
        for (j=1; j<=(priv_>ticks.zin); j++)
          {
          for (k=1; k<kx; k++)
            {
            sx+=dz;/* additional azimuthal ticks */
            cairo_move_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
            cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
            cairo_stroke(cr);
            g_snprintf(lbl, (priv->thcs), "%f", sx*90/G_PI_2);
            /* draw zaimuthal tick labels */
            }
          sx+=dz/* inner radial lines to tick */;
          cairo_move_to(cr, (priv->x0)+(wr*cos(sx)), (priv->y0)-(wr*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
          }
        }
      else
        {
        for (j=1; j<=(priv_>ticks.zin); j++)
          {
          for (k=1; k<kx; k++)
            {
            sx+=dz;/* additional azimuthal ticks */
            cairo_move_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
            cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
            g_snprintf(lbl, (priv->thcs), "%f", sx);
            /* draw zaimuthal tick labels */
            }
          sx+=dz/* inner radial lines to tick */;
          cairo_move_to(cr, (priv->x0)+(wr*cos(sx)), (priv->y0)-(wr*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
          }
        }
      cairo_stroke(cr);
      cairo_new_sub_path(cr);
      cairo_arc(cr, (priv->x0), (priv->y0), wr, -thx, -thn);
      rt=wr;
      dwr=dr*(priv->s)/(priv->ticks.r);
      kx=1;
      for (j=1; j<(priv->ticks.r); j++)
        {
        rt+=dwr;
        cairo_new_sub_path(cr);/* draw arc grids */
        cairo_arc(cr, (priv->x0), (priv->y0), rt, -thx-(JT/rt), -thn);
        if ((rt*dt)>=((priv->ticks.zc)*kx*(priv->ticks.zin)))
          {/* add additional radials */
          dz=(thx-thn)/(kx*(priv->ticks.zin));
          sx=thn+(dz/2);
          cairo_move_to(cr, (priv->x0)+(rt*cos(sx)), (priv->y0)-(rt*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
          for (j=1; j<(kx*(priv->ticks.zin));j++)
            {
            sx+=dz;
            cairo_move_to(cr, (priv->x0)+(rt*cos(sx)), (priv->y0)-(rt*sin(sx)));
            cairo_line_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
            }
          kx*=2;
          }
        cairo_stroke(cr);
        g_snprintf(lbl, (priv->rcs), "%f", (priv->bounds.rmin)+((rt-wr)/(priv->s)));
        cairo_text_extents(cr, lbl, &extents);
        sx=rt+((extents.width)/2)+(extents.x_bearing);
        dz=-JTI+(extents.y_bearing);
        cairo_move_to(cr, (priv->x0)+(sx*cos(thx))+(dz*sin(thx)), (priv->y0)+(dz*cos(thx))-(sx*sin(thx)));
        cairo_set_matrix(cr, &mtr2);
        cairo_show_text(cr, lbl);/* draw radial tick labels*/
        cairo_set_matrix(cr, &mtr);
        }
      rt+=dwr;
      cairo_new_sub_path(cr);
      cairo_arc(cr, (priv->x0), (priv->y0), rt, -thx, -thn);
      cairo_set_font_size(cr, plot->lfsize);
      cairo_text_extents(cr, (plot->rlab), &extents);
      sx=(dr1*(priv->s))+((extents.width)/2)+(extents.x_bearing);
      dz=-dtt+(extents.height)+(extents.y_bearing);
      cairo_move_to(cr, (priv->x0)+(sx*cos(thx))+(dz*sin(thx)), (priv->y0)+(dz*cos(thx))-(sx*sin(thx)));
      cairo_set_matrix(cr, &mtr2);
      cairo_show_text(cr, lbl);/* draw radial label*/
      cairo_set_matrix(cr, &mtr);
      /* draw azimuthal label */
      }
    else if ((priv->flaga)==3)
      {
      mtr2.xx=cos(thn);
      mtr2.xy=sin(thn);
      mtr2.yx=-mtr2.xy;
      mtr2.yy=mtr2.xx;
      if ((plot->flags)&1==0)
        {
        for (j=1; j<=(priv_>ticks.zin); j++)
          {
          for (k=1; k<kx; k++)
            {
            sx+=dz;/* additional azimuthal ticks */
            cairo_move_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
            cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
            cairo_stroke(cr);
            g_snprintf(lbl, (priv->thcs), "%f", sx*I180_MY_PI);
            /* draw zaimuthal tick labels */
            }
          sx+=dz/* inner radial lines to tick */;
          cairo_move_to(cr, (priv->x0)+(wr*cos(sx)), (priv->y0)-(wr*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
          }
        }
      else
        {
        for (j=1; j<=(priv_>ticks.zin); j++)
          {
          for (k=1; k<kx; k++)
            {
            sx+=dz;/* additional azimuthal ticks */
            cairo_move_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
            cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
            g_snprintf(lbl, (priv->thcs), "%f", sx);
            /* draw zaimuthal tick labels */
            }
          sx+=dz/* inner radial lines to tick */;
          cairo_move_to(cr, (priv->x0)+(wr*cos(sx)), (priv->y0)-(wr*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
          }
        }
      cairo_stroke(cr);
      cairo_new_sub_path(cr);
      cairo_arc(cr, (priv->x0), (priv->y0), wr, -thx, -thn);
      rt=wr;
      dwr=dr*(priv->s)/(priv->ticks.r);
      kx=1;
      for (j=1; j<(priv->ticks.r); j++)
        {
        rt+=dwr;
        cairo_new_sub_path(cr);/* draw arc grids */
        cairo_arc(cr, (priv->x0), (priv->y0), rt, -thx, -thn+(JT/rt));
        if ((rt*dt)>=((priv->ticks.zc)*kx*(priv->ticks.zin)))
          {/* add additional radials */
          dz=(thx-thn)/(kx*(priv->ticks.zin));
          sx=thn+(dz/2);
          cairo_move_to(cr, (priv->x0)+(rt*cos(sx)), (priv->y0)-(rt*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
          for (j=1; j<(kx*(priv->ticks.zin));j++)
            {
            sx+=dz;
            cairo_move_to(cr, (priv->x0)+(rt*cos(sx)), (priv->y0)-(rt*sin(sx)));
            cairo_line_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
            }
          kx*=2;
          }
        cairo_stroke(cr);
        g_snprintf(lbl, (priv->rcs), "%f", (priv->bounds.rmin)+((rt-wr)/(priv->s)));
        cairo_text_extents(cr, lbl, &extents);
        sx=rt-((extents.width)/2)-(extents.x_bearing);
        dz=JTI-(extents.y_bearing);
        cairo_move_to(cr, (priv->x0)+(sx*cos(thx))+(dz*sin(thx)), (priv->y0)+(dz*cos(thx))-(sx*sin(thx)));
        cairo_set_matrix(cr, &mtr2);
        cairo_show_text(cr, lbl);/* draw radial tick labels*/
        cairo_set_matrix(cr, &mtr);
        }
      rt+=dwr;
      cairo_new_sub_path(cr);
      cairo_arc(cr, (priv->x0), (priv->y0), rt, -thx, -thn);
      cairo_set_font_size(cr, plot->lfsize);
      cairo_text_extents(cr, (plot->rlab), &extents);
      sx=(dr1*(priv->s))-((extents.width)/2)-(extents.x_bearing);
      dz=dtt-(extents.height)-(extents.y_bearing);
      cairo_move_to(cr, (priv->x0)+(sx*cos(thx))+(dz*sin(pthx)), (priv->y0)+(dz*cos(thx))-(sx*sin(thx)));
      cairo_set_matrix(cr, &mtr2);
      cairo_show_text(cr, lbl);/* draw radial label*/
      cairo_set_matrix(cr, &mtr);
      /* draw azimuthal label */
      }
    else
      {
      mtr2.xx=-cos(thn);
      mtr2.xy=-sin(thn);
      mtr2.yx=-mtr2.xy;
      mtr2.yy=mtr2.xx;
      if ((plot->flags)&1==0)
        {
        for (j=1; j<=(priv_>ticks.zin); j++)
          {
          for (k=1; k<kx; k++)
            {
            sx+=dz;/* additional azimuthal ticks */
            cairo_move_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
            cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
            cairo_stroke(cr);
            g_snprintf(lbl, (priv->thcs), "%f", sx*90/G_PI_2);
            /* draw zaimuthal tick labels */
            }
          sx+=dz/* inner radial lines to tick */;
          cairo_move_to(cr, (priv->x0)+(wr*cos(sx)), (priv->y0)-(wr*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
          }
        }
      else
        {
        for (j=1; j<=(priv_>ticks.zin); j++)
          {
          for (k=1; k<kx; k++)
            {
            sx+=dz;/* additional azimuthal ticks */
            cairo_move_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
            cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
            g_snprintf(lbl, (priv->thcs), "%f", sx);
            /* draw zaimuthal tick labels */
            }
          sx+=dz/* inner radial lines to tick */;
          cairo_move_to(cr, (priv->x0)+(wr*cos(sx)), (priv->y0)-(wr*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
          }
        }
      cairo_stroke(cr);
      cairo_new_sub_path(cr);
      cairo_arc(cr, (priv->x0), (priv->y0), wr, -thx, -thn);
      rt=wr;
      dwr=dr*(priv->s)/(priv->ticks.r);
      kx=1;
      for (j=1; j<(priv->ticks.r); j++)
        {
        rt+=dwr;
        cairo_new_sub_path(cr);/* draw arc grids */
        cairo_arc(cr, (priv->x0), (priv->y0), rt, -thx, -thn+(JT/rt));
        if ((rt*dt)>=((priv->ticks.zc)*kx*(priv->ticks.zin)))
          {/* add additional radials */
          dz=(thx-thn)/(kx*(priv->ticks.zin));
          sx=thn+(dz/2);
          cairo_move_to(cr, (priv->x0)+(rt*cos(sx)), (priv->y0)-(rt*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
          for (j=1; j<(kx*(priv->ticks.zin));j++)
            {
            sx+=dz;
            cairo_move_to(cr, (priv->x0)+(rt*cos(sx)), (priv->y0)-(rt*sin(sx)));
            cairo_line_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
            }
          kx*=2;
          }
        cairo_stroke(cr);
        g_snprintf(lbl, (priv->rcs), "%f", (priv->bounds.rmin)+((rt-wr)/(priv->s)));
        cairo_text_extents(cr, lbl, &extents);
        sx=rt+((extents.width)/2)+(extents.x_bearing);
        dz=JTI+(extents.height)+(extents.y_bearing);
        cairo_move_to(cr, (priv->x0)+(sx*cos(thx))+(dz*sin(thx)), (priv->y0)+(dz*cos(thx))-(sx*sin(thx)));
        cairo_set_matrix(cr, &mtr2);
        cairo_show_text(cr, lbl);/* draw radial tick labels*/
        cairo_set_matrix(cr, &mtr);
        }
      rt+=dwr;
      cairo_new_sub_path(cr);
      cairo_arc(cr, (priv->x0), (priv->y0), rt, -thx, -thn);
      cairo_set_font_size(cr, plot->lfsize);
      cairo_text_extents(cr, (plot->rlab), &extents);
      sx=(dr1*(priv->s))+((extents.width)/2)+(extents.x_bearing);
      dz=dtt+(extents.y_bearing);
      cairo_move_to(cr, (priv->x0)+(sx*cos(thx))+(dz*sin(thx)), (priv->y0)+(dz*cos(thx))-(sx*sin(thx)));
      cairo_set_matrix(cr, &mtr2);
      cairo_show_text(cr, lbl);/* draw radial label*/
      cairo_set_matrix(cr, &mtr);
      /* draw azimuthal label */
      }
    }
  else if ((priv->bounds.rmin)==0)
    {
    (priv->s)=dr1-drc;/* check radial x0 to xw */
    if (sx>DZE) (priv->s)=((xw/2)-dtt)/(priv->s);
    else (priv->s)=inf;
    sx=dr1-drs;
    if (sx>DZE)
      {/* check radial y0 to 0 */
      sx=((yw/2)-dtt)/sx;
      if (sx<(priv->s)) (priv->s)=sx;
      }
    sx=dr1+drc;
    if (sx>DZE)
      {/* check radial x0 to 0 */
      sx=((xw/2)-dtt-(wr*(1+cos(priv->centre.th))))/sx;
      if (sx<(priv->s)) (priv->s)=sx;
      }
    sx=dr1+drs;
    if (sx>DZE)
      {/* check radial y0 to yw */
      sx=((yw/2)-dtt)/sx;
      if (sx<(priv->s)) (priv->s)=sx;
      }
    (priv->flaga)=4;
    (priv->x0)=(xw/2)-(drc*(priv->s));
    (priv->y0)=(yw/2)+(drs*(priv->s));
    sx=0;
    kx=1<<(priv->ticks.z2m);
    dz=2_MY_PI/(kx*(priv->ticks.zin));
    cairo_move_to(cr, (priv->x0), (priv->y0));
    cairo_line_to(cr, (priv->x0)+((priv->s)*dr1), (priv->y0));
    if ((priv->flags)&1==0)
      {
      for (j=1; j<(priv_>ticks.zin); j++)
        {
        for (k=1; k<kx; k++)
          {
          sx+=dz;/* additional azimuthal ticks */
          cairo_move_to(cr, (priv->x0)+(((priv->s)*dr1)*cos(sx)), (priv->y0)-(((priv->s)*dr1)*sin(sx));
          cairo_line_to(cr, (priv->x0)+((JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((JT+((priv->s)*dr1))*sin(sx));
          g_snprintf(lbl, (priv->thcs), "%f", sx*I180_MY_PI);
          /* draw zaimuthal tick labels */
          }
        sx+=dz/* inner radial lines to tick */;
        cairo_move_to(cr, (priv->x0), (priv->y0));
        cairo_line_to(cr, (priv->x0)+((JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((JT+((priv->s)*dr1))*sin(sx)));
        }
      }
    else
      {
      for (j=1; j<(priv_>ticks.zin); j++)
        {
        for (k=1; k<kx; k++)
          {
          sx+=dz;/* additional azimuthal ticks */
          cairo_move_to(cr, (priv->x0)+(((priv->s)*dr1)*cos(sx)), (priv->y0)-(((priv->s)*dr1)*sin(sx));
          cairo_line_to(cr, (priv->x0)+((JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((JT+((priv->s)*dr1))*sin(sx));
          g_snprintf(lbl, (priv->thcs), "%f", sx);
          /* draw zaimuthal tick labels */
          }
        sx+=dz/* inner radial lines to tick */;
        cairo_move_to(cr, (priv->x0), (priv->y0));
        cairo_line_to(cr, (priv->x0)+((JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((JT+((priv->s)*dr1))*sin(sx)));
        }
      }
    for (k=1; k<kx; k++)
      {
      sx+=dz;/* additional azimuthal ticks */
      cairo_move_to(cr, (priv->x0)+(((priv->s)*dr1)*cos(sx)), (priv->y0)-(((priv->s)*dr1)*sin(sx)));
      cairo_line_to(cr, (priv->x0)+((JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((JT+((priv->s)*dr1))*sin(sx)));
      }
    cairo_stroke(cr);
    rt=0;
    dwr=dr1*(priv->s)/(priv->ticks.r);
    kx=1;
    for (j=1; j<(priv->ticks.r); j++)
      {
      rt+=dwr;
      cairo_new_sub_path(cr);
      cairo_arc(cr, (priv->x0), (priv->y0), rt, NMY_PI, G_PI);
      if ((rt*2_MY_PI)>=((priv->ticks.zc)*kx*(priv->ticks.zin)))
        {/* add additional radials */
        dz=2*G_PI/(kx*(priv->ticks.zin));
        sx=(dz/2);
        cairo_move_to(cr, (priv->x0)+(rt*cos(sx)), (priv->y0)-(rt*sin(sx)));
        cairo_line_to(cr, (priv->x0)+(((priv->s)*dr1)*cos(sx)), (priv->y0)-(((priv->s)*dr1)*sin(sx)));
        for (j=1; j<(kx*(priv->ticks.zin));j++)
          {
          sx+=dz;
          cairo_move_to(cr, (priv->x0)+(rt*cos(sx)), (priv->y0)-(rt*sin(sx)));
          cairo_line_to(cr, (priv->x0)+(((priv->s)*dr1)*cos(sx)), (priv->y0)-(((priv->s)*dr1)*sin(sx)));
          }
        kx*=2;
        }
      cairo_stroke(cr);
      g_snprintf(lbl, (priv->rcs), "%f", (rt/s));
      cairo_text_extents(cr, lbl, &extents);
      cairo_move_to(cr, (priv->x0)+rt-((extents.width)/2)-(extents.x_bearing), (priv->y0)+JTI-(extents.y_bearing));
      cairo_show_text(cr, lbl);/* draw radial tick labels*/
      }
    rt+=dwr;
    cairo_new_sub_path(cr);
    cairo_arc(cr, (priv->x0), (priv->y0), rt, NMY_PI, G_PI);
    cairo_stroke(cr);
    /* draw azimuthal tick labels */
    cairo_set_font_size(cr, plot->lfsize); /* draw radial label */
    cairo_text_extents(cr, (plot->rlab), &extents);
    cairo_move_to(cr, (priv->x0)+(((dr1*(priv->s))-(extents.width))/2)-(extents.x_bearing), (priv->y0)+dtt-(extents.height)-(extents.y_bearing));
    cairo_show_text(cr, (plot->rlab));
    /* draw azimuthal label */
    }
  else
    {
    (priv->s)=dr1-drc;/* check radial x0 to xw */
    if (sx>DZE) (priv->s)=((xw/2)-dtt-(wr*(1-cos(priv->centre.th))))/(priv->s);
    else (priv->s)=inf;
    sx=dr1-drs;
    if (sx>DZE)
      {/* check radial y0 to 0 */
      sx=((yw/2)-dtt-(wr*(1-sin(priv->centre.th))))/sx;
      if (sx<(priv->s)) (priv->s)=sx;
      }
    sx=dr1+drc;
    if (sx>DZE)
      {/* check radial x0 to 0 */
      sx=((xw/2)-dtt-(wr*(1+cos(priv->centre.th))))/sx;
      if (sx<(priv->s)) (priv->s)=sx;
      }
    sx=dr1+drs;
    if (sx>DZE)
      {/* check radial y0 to yw */
      sx=((yw/2)-dtt-(wr*(1+sin(priv->centre.th))))/sx;
      if (sx<(priv->s)) (priv->s)=sx;
      }
    (priv->flaga)=4;
    (priv->x0)=(xw/2)-(drc*(priv->s))-(wr*cos(priv->centre.th));
    (priv->y0)=(yw/2)+(drs*(priv->s))+(wr*sin(priv->centre.th));
    sx=0;
    kx=1<<(priv->ticks.z2m);
    dz=2*G_PI/(kx*(priv->ticks.zin));
    dt=4;
    cairo_save(cr);
    cairo_set_dash(cr, &dt, 1, 0);
    cairo_move_to(cr, (priv->x0)+(wr/3), (priv->y0));
    cairo_line_to(cr, (priv->x0)+wr, (priv->y0));
    cairo_stroke(cr);
    cairo_restore(cr);
    cairo_line_to(cr, (priv->x0)+wr+((priv->s)*dr1), (priv->y0));
    if ((priv->flags)&1==0)
      {
      for (j=1; j<(priv_>ticks.zin); j++)
        {
        for (k=1; k<kx; k++)
          {
          sx+=dz;/* additional azimuthal ticks */
          cairo_move_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
          g_snprintf(lbl, (priv->thcs), "%f", sx*I180_MY_PI);
          /* draw zaimuthal tick labels */
          }
        sx+=dz/* inner radial lines to tick */;
        cairo_stroke(cr);
        cairo_save(cr);
        cairo_set_dash(cr, &dt, 1, 0);
        cairo_move_to(cr, (priv->x0)+((wr/3)*cos(sx)), (priv->y0)-((wr/3)*sin(sx)));
        cairo_line_to(cr, (priv->x0)+(wr*cos(sx)), (priv->y0)-(wr*sin(sx)));
        cairo_stroke(cr);
        cairo_restore(cr);
        cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
        }
      }
    else
      {
      for (j=1; j<(priv_>ticks.zin); j++)
        {
        for (k=1; k<kx; k++)
          {
          sx+=dz;/* additional azimuthal ticks */
          cairo_move_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
          g_snprintf(lbl, (priv->thcs), "%f", sx);
          /* draw zaimuthal tick labels */
          }
        sx+=dz/* inner radial lines to tick */;
        cairo_stroke(cr);
        cairo_save(cr);
        cairo_set_dash(cr, &dt, 1, 0);
        cairo_move_to(cr, (priv->x0)+((wr/3)*cos(sx)), (priv->y0)-((wr/3)*sin(sx)));
        cairo_line_to(cr, (priv->x0)+(wr*cos(sx)), (priv->y0)-(wr*sin(sx)));
        cairo_stroke(cr);
        cairo_restore(cr);
        cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
        }
      }
    for (k=1; k<kx; k++)
      {
      sx+=dz;/* additional azimuthal ticks */
      cairo_move_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
      cairo_line_to(cr, (priv->x0)+((wr+JT+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+JT+((priv->s)*dr1))*sin(sx)));
      }
    cairo_stroke(cr);
    cairo_new_sub_path(cr);
    cairo_arc(cr, (priv->x0), (priv->y0), wr, NMY_PI, G_PI);
    rt=wr;
    dwr=dr1*(priv->s)/(priv->ticks.r);
    kx=1;
    for (j=1; j<(priv->ticks.r); j++)
      {
      rt+=dwr;
      cairo_new_sub_path(cr);
      cairo_arc(cr, (priv->x0), (priv->y0), rt, NMY_PI, G_PI);
      if ((rt*2_MY_PI)>=((priv->ticks.zc)*kx*(priv->ticks.zin)))
        {/* add additional radials */
        dz=2_MY_PI/(kx*(priv->ticks.zin));
        sx=(dz/2);
        cairo_move_to(cr, (priv->x0)+(rt*cos(sx)), (priv->y0)-(rt*sin(sx)));
        cairo_line_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
        for (j=1; j<(kx*(priv->ticks.zin));j++)
          {
          sx+=dz;
          cairo_move_to(cr, (priv->x0)+(rt*cos(sx)), (priv->y0)-(rt*sin(sx)));
          cairo_line_to(cr, (priv->x0)+((wr+((priv->s)*dr1))*cos(sx)), (priv->y0)-((wr+((priv->s)*dr1))*sin(sx)));
          }
        kx*=2;
        }
      cairo_stroke(cr);
      g_snprintf(lbl, (priv->rcs), "%f", (priv->bounds.rmin)+((rt-wr)/(priv->s)));
      cairo_text_extents(cr, lbl, &extents);
      cairo_move_to(cr, (priv->x0)+rt-((extents.width)/2)-(extents.x_bearing), (priv->y0)+JTI-(extents.y_bearing));
      cairo_show_text(cr, lbl);/* draw radial tick labels*/
      }
    rt+=dwr;
    cairo_new_sub_path(cr);
    cairo_arc(cr, (priv->x0), (priv->y0), rt, NMY_PI, G_PI);
    cairo_stroke(cr);
    /* draw azimuthal tick labels */
    cairo_set_font_size(cr, plot->lfsize);
    cairo_text_extents(cr, (plot->rlab), &extents);
    cairo_move_to(cr, (priv->x0)+wr+(((dr1*(priv->s))-(extents.width))/2)-(extents.x_bearing), (priv->y0)+dtt-(extents.height)-(extents.y_bearing));
    cairo_show_text(cr, (plot->rlab)); /* draw radial label */
    /* draw azimuthal label */
    }
  if (plot->rdata && plot->thdata) /* plot data */
    {
    xv=xl+((xu-xl)*(g_array_index(plot->xdata, gdouble, 0)-(priv->bounds.xmin))/((priv->bounds.xmax)-(priv->bounds.xmin)));
    yv=yl+((yu-yl)*(g_array_index(plot->ydata, gdouble, 0)-(priv->bounds.ymin))/((priv->bounds.ymax)-(priv->bounds.ymin)));
    if (((plot->flags)&2)!=0)
      {
      cairo_set_line_width(cr, (plot->linew));
      if (((plot->flags)&4)!=0) /* lines and points */
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
                  tx=((xvn-xv)*(yu-yv))/(yvn-yv); /* divide by zero handling? */
                  tx+=xv;
                  if ((tx<=xu)&&(yvn>yv)) cairo_move_to(cr, tx, yu);
                  else
                    {
                    tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                    tx+=yv;
                    cairo_move_to(cr, xu, tx);
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
                tx=((xvn-xv)*(yu-yv))/(yvn-yv);
                tx+=xv;
                if ((tx<=xu)&&(yvn<yv)) cairo_line_to(cr, tx, yu);
                else
                  {
                  tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_line_to(cr, xu, tx);
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
                  tx=((xvn-xv)*(yv-yl))/(yv-yvn); /* divide by zero handling? */
                  tx+=xv;
                  if ((tx<=xu)&&(yvn<yv)) cairo_move_to(cr, tx, yl);
                  else
                    {
                    tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                    tx+=yv;
                    cairo_move_to(cr, xu, tx);
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
                tx=((xvn-xv)*(yl-yv))/(yvn-yv);
                tx+=xv;
                if ((tx<=xu)&&(yvn>yv)) cairo_line_to(cr, tx, yl);
                else
                  {
                  tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_line_to(cr, xu, tx);
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
                  tx=((xvn-xv)*(yu-yv))/(yvn-yv); /* divide by zero handling? */
                  tx+=xv;
                  if ((tx>=xl)&&(yvn>yv)) cairo_move_to(cr, tx, yu);
                  else
                    {
                    tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                    tx+=yv;
                    cairo_move_to(cr, xl, tx);
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
                tx=((xvn-xv)*(yu-yv))/(yvn-yv);
                tx+=xv;
                if ((tx>=xl)&&(yvn<yv)) cairo_line_to(cr, tx, yu);
                else
                  {
                  tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_line_to(cr, xl, tx);
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
                  tx=((xvn-xv)*(yv-yl))/(yv-yvn); /* divide by zero handling? */
                  tx+=xv;
                  if ((tx>=xl)&&(yvn<yv)) cairo_move_to(cr, tx, yl);
                  else
                    {
                    tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                    tx+=yv;
                    cairo_move_to(cr, xl, tx);
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
                tx=((xvn-xv)*(yl-yv))/(yvn-yv);
                tx+=xv;
                if ((tx>=xl)&&(yvn>yv)) cairo_line_to(cr, tx, yl);
                else
                  {
                  tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_line_to(cr, xl, tx);
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
                tx=((xvn-xv)*(yu-yv))/(yvn-yv); /* divide by zero handling? */
                tx+=xv;
                if ((tx<=xu)&&(yvn>yv)) cairo_move_to(cr, tx, yu);
                else
                  {
                  tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_move_to(cr, xu, tx);
                  }
                }
              cairo_line_to(cr, xvn, yvn);
              xs=0;
              }
            else if (xs==0)
              {
              tx=((xvn-xv)*(yu-yv))/(yvn-yv);
              tx+=xv;
              if ((tx<=xu)&&(yvn<yv)) cairo_line_to(cr, tx, yu);
              else
                {
                tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                tx+=yv;
                cairo_line_to(cr, xu, tx);
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
                tx=((xvn-xv)*(yv-yl))/(yv-yvn); /* divide by zero handling? */
                tx+=xv;
                if ((tx<=xu)&&(yvn<yv)) cairo_move_to(cr, tx, yl);
                else
                  {
                  tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_move_to(cr, xu, tx);
                  }
                }
              cairo_line_to(cr, xvn, yvn);
              xs=0;
              }
            else if (xs==0)
              {
              tx=((xvn-xv)*(yl-yv))/(yvn-yv);
              tx+=xv;
              if ((tx<=xu)&&(yvn>yv)) cairo_line_to(cr, tx, yl);
              else
                {
                tx=((yvn-yv)*(xu-xv))/(xvn-xv);
                tx+=yv;
                cairo_line_to(cr, xu, tx);
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
                tx=((xvn-xv)*(yu-yv))/(yvn-yv); /* divide by zero handling? */
                tx+=xv;
                if ((tx>=xl)&&(yvn>yv)) cairo_move_to(cr, tx, yu);
                else
                  {
                  tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_move_to(cr, xl, tx);
                  }
                }
              cairo_line_to(cr, xvn, yvn);
              xs=0;
              }
            else if (xs==0)
              {
              tx=((xvn-xv)*(yu-yv))/(yvn-yv);
              tx+=xv;
              if ((tx>=xl)&&(yvn<yv)) cairo_line_to(cr, tx, yu);
              else
                {
                tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                tx+=yv;
                cairo_line_to(cr, xl, tx);
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
                tx=((xvn-xv)*(yv-yl))/(yv-yvn); /* divide by zero handling? */
                tx+=xv;
                if ((tx>=xl)&&(yvn<yv)) cairo_move_to(cr, tx, yl);
                else
                  {
                  tx=((yvn-yv)*(xl-xv))/(xvn-xv);
                  tx+=yv;
                  cairo_move_to(cr, xl, tx);
                  }
                }
              cairo_line_to(cr, xvn, yvn);
              xs=0;
              }
            else if (xs==0)
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
    else if (((plot->flags)&4)!=0) /* points only */
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
          }num
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

static void plot_polar_redraw(GtkWidget *widget)
  {
  GdkRegion *region;

  if (!widget->window) return;
  region=gdk_drawable_get_clip_region(widget->window);
  gdk_window_invalidate_region(widget->window, region, TRUE);
  gdk_window_process_updates(widget->window, TRUE);
  gdk_region_destroy(region);
  }

gboolean plot_polar_update_scale(GtkWidget *widget, gdouble rn, gdouble rx, gdouble thn, gdouble thx, gdouble rcn, gdouble thc)
  {
  PlotPolarPrivate *priv;

  priv=PLOT_POLAR_GET_PRIVATE(widget);
  (priv->bounds.rmin)=rn;
  (priv->bounds.rmax)=rx;
  if ((plot->flags)&1==0) {(priv->bounds.thmin)=thn*MY_PI_180; (priv->centre.th)=thc*MY_PI_180; (priv->bounds.thmax)=thx*MY_PI_180;}
  else {(priv->bounds.thmin)=thn; (priv->centre.th)=thc; (priv->bounds.thmax)=thx;}
  (priv->centre.r)=rcn;
  plot_polar_redraw(widget);
  return FALSE;
  }

gboolean plot_polar_update_scale_pretty(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx)
  {
  PlotPolar *plot;
  PlotPolarPrivate *priv;
  gdouble num, num3, thn. thx;
  gint num2, lt, ut, tk, dtt;

  priv=PLOT_POLAR_GET_PRIVATE(widget);
  plot=PLOT_POLAR(widget);
  (priv->ticks.zc)=WGS;
  if (xx<0) {xx=-xx; xn=-xn;}
  if (xx<xn) {num=xx; xx=xn; xn=num;}
  (priv->rcs)=2; /* determine number of characters needed for radial labels and pretty max/min values */
  if (xn<=0)
    {
    (priv->bounds.rmin)=0;
    num=log10(xx);
    if (num>=1)
      {
      num2=(gint)num;
      num=fmod(num,1);
      (priv->rcs)+=num2;
      }
    else if (num>=0)
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
    num2--;
    if (num==0) {(priv->ticks.r)=5; (priv->bounds.rmax)=xx; num2--;}
    else if (num<0.39794)
      {
      (priv->ticks.r)=ceil(num*20);
      num2--;
      (priv->bounds.rmax)=5*(priv->ticks.r)*exp(G_LN10*num2);
      }
    else if (num<0.69897)
      {
      (priv->ticks.r)=ceil(num*10);
      (priv->bounds.rmax)=(priv->ticks.r)*exp(G_LN10*num2);
      }
    else
      {
      (priv->ticks.r)=ceil(num*5);
      (priv->bounds.rmax)=2*(priv->ticks.r)*exp(G_LN10*num2);
      }
    if (num2<0) (priv->rcs)+=1-num2;
    }
  else
    {
    num3=(xx-xn)/5;
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
      if (tk>5)
        {
        num3*=2;
        lt=floor(xn/num3);
        ut=ceil(xx/num3);
        tk=(ut-lt);
        }
      }
    else if (num<0.39794)
      {
      num=G_LN10*num2;
      num=exp(num);
      num3=2*num;
      lt=floor(xn/num3);
      ut=ceil(xx/num3);
      tk=(ut-lt);
      if (tk>5)
        {
        num3=5*num;
        lt=floor(xn/num3);
        ut=ceil(xx/num3);
        tk=(ut-lt);
        if (tk>5)
          {
          num3*=2;
          lt=floor(xn/num3);
          ut=ceil(xx/num3);
          tk=(ut-lt);
          }
        }
      num=fmod(num,1);
      }
    else if (num<0.69897)
      {
      num=G_LN10*num2;
      num=exp(num);
      num3=5*num;
      lt=floor(xn/num3);
      ut=ceil(xx/num3);
      tk=(ut-lt);
      if (tk>5)
        {
        num3*=2;
        lt=floor(xn/num3);
        ut=ceil(xx/num3);
        tk=(ut-lt);
        if (tk>5)
          {
          num3*=2;
          lt=floor(xn/num3);
          ut=ceil(xx/num3);
          tk=(ut-lt);
          }
        }
      }
    else
      {
      num2++;
      num=G_LN10*num2;
      num3=exp(num);
      lt=floor(xn/num3);
      ut=ceil(xx/num3);
      tk=(ut-lt);
      if (tk>5)
        {
        num3*=2;
        lt=floor(xn/num3);
        ut=ceil(xx/num3);
        tk=(ut-lt);
        }
      }
    (priv->bounds.rmin)=(num3*(gdouble)lt);
    (priv->bounds.rmax)=(num3*(gdouble)ut);
    (priv->ticks.r)=tk;
    if ((priv->bounds.rmax)>=10) (priv->rcs)+=floor(log10(xx));
    if (num3<1) (priv->rcs)+=1-floor(log10(num3));
    }
  if ((priv->rcs)>8) (priv->rcs)=8;
  dtt=(plot->afsize)+(plot->lfsize)+JTI;
  if ((plot->flags)&1==0)
    {
    num=(yx-yn);
    if (num<0)
      {
      num+=360;
      if ((yx+yn)<0) {thx=yx+360; thn=yn;}
      else {thn=yn-360; thx=yx;}
      }
    else {thx=yx; thn=yn;}
    if (num>=360)
      {
      if (yx>=360) {(priv->bounds.thmax)=2_MY_PI; (priv->bounds.thmin)=0; (priv->thcs)=4;}
      else if (yn<=-360) {(priv->bounds.thmax)=0; (priv->bounds.thmin)=N2_MY_PI; (priv->thcs)=5;}
      else {(priv->bounds.thmax)=G_PI; (priv->bounds.thmin)=NMY_PI; (priv->thcs)=5;}
      (priv->ticks.zin)=12;(priv->th(plot->flags)&1==0cs)=8;
      (priv->ticks.z2m)=2;
      (priv->centre.r)=0;
      (priv->centre.th)=0;
      }
    else if (num>150)
      {
      ut=ceil(thx/30);
      lt=floor(thy/30);
      (priv->ticks.zin)=ut-lt;
      (priv->bounds.thmax)=ut*MY_PI_6;
      (priv->bounds.thmin)=lt*MY_PI_6;
      (priv->ticks.z2m)=2;
      if (thn<-100) (priv->thcs)=5;
      else (priv->thcs)=4;
      (priv->centre.r)=(((priv->bounds.rmax)+dtt+(priv->bounds.rmin))*(1+cos(num*MY_PI_360)))/2;
      (priv->centre.th)=(ut+lt)*MY_PI_12;
      }
    else if (num>60)
      {
      ut=ceil(thx/10);
      lt=floor(thy/10);
      (priv->ticks.zin)=ut-lt;
      (priv->bounds.thmax)=ut*MY_PI_18;
      (priv->bounds.thmin)=lt*MY_PI_18;
      (priv->ticks.z2m)=1;
      if (thn<-100) (priv->thcs)=5;
      else if ((thn>=-10) && (thx<=100)) (priv->thcs)=3;
      else (priv->thcs)=4;
      (priv->centre.r)=((priv->bounds.rmax)+dtt+(priv->bounds.rmin))/2;
      (priv->centre.th)=(ut+lt)*MY_PI_36;
      }
    else if (num>30)
      {
      ut=ceil(thx/5);
      lt=floor(thy/5);
      (priv->ticks.zin)=ut-lt;
      (priv->bounds.thmax)=ut*MY_PI_36;
      (priv->bounds.thmin)=lt*MY_PI_36;
      (priv->ticks.z2m)=0;
      if (thn<-100) (priv->thcs)=5;
      else if ((thn>=-10) && (thx<=100)) (priv->thcs)=3;
      else (priv->thcs)=4;
      (priv->centre.r)=((priv->bounds.rmax)+dtt+(priv->bounds.rmin))/2;
      (priv->centre.th)=(ut+lt)*MY_PI_72;
      }
    else if (num>12)
      {
      ut=ceil(thx/2);
      lt=floor(thy/2);
      (priv->ticks.zin)=ut-lt;
      (priv->bounds.thmax)=ut*MY_PI_90;
      (priv->bounds.thmin)=lt*MY_PI_90;
      (priv->ticks.z2m)=0;
      if (thn<-100) (priv->thcs)=5;
      else if ((thn>=-10) && (thx<=100)) (priv->thcs)=3;
      else (priv->thcs)=4;
      (priv->centre.r)=((priv->bounds.rmax)+dtt+(priv->bounds.rmin))/2;
      (priv->centre.th)=(ut+lt)*MY_(plot->flags)&1==0PI_180;
      }
    else
      {
      ut=ceil(thx);
      lt=floor(thy);
      (priv->bounds.thmax)=ut*MY_PI_180;
      (priv->bounds.thmin)=lt*MY_PI_180;
      (priv->ticks.zin)=ut-lt;
      (priv->ticks.z2m)=0;
      if (thn<-100) (priv->thcs)=5;
      else if (thn>=-10)
        {
        if (thx<=100)
          {
          if ((thn>=0) && (thx<=10)) (priv->thcs)=2;
          else (priv->thcs)=3;
        else (priv->thcs)=4;
        }
      else (priv->thcs)=4;
      (priv->centre.r)=((priv->bounds.rmax)+dtt+(priv->bounds.rmin))/2;
      (priv->centre.th)=(ut+lt)*MY_PI_360;
      }
    }
  else
    {
    thn=yn/G_PI;
    thx=yx/G_PI;
    num=(thx-thn);
    if (num<0)
      {
      num+=2;
      if ((thn+thx)<0) thx+=2;
      else thn-=2;
      }
    if (num>=2)
      {
      if (thx>=2) {(priv->bounds.thmax)=2_MY_PI; (priv->bounds.thmin)=0; (priv->thcs)=6;}
      else if (yn<=-2) {(priv->bounds.thmax)=0; (priv->bounds.thmin)=N2_MY_PI; (priv->thcs)=7;}
      else {(priv->bounds.thmax)=G_PI; (priv->bounds.thmin)=NMY_PI; (priv->thcs)=6;}
      (priv->ticks.zin)=12;
      (priv->ticks.z2m)=2;
      (priv->centre.r)=0;
      (priv->centre.th)=0;
      }
    else if (6*num>5)
      {
      ut=ceil(thx*6);
      lt=floor(thn*6);
      (priv->ticks.zin)=ut-lt;
      (priv->bounds.thmax)=MY_PI_6*ut;
      (priv->bounds.thmin)=MY_PI_6*lt;
      (priv->ticks.z2m)=2;
      if (lt<-10) (priv->thcs)=7;
      else if ((lt>=-1)&&(ut<=10)) (priv->thcs)=5;
      else (priv->thcs)=6;
      (priv->centre.r)=(((priv->bounds.rmax)+dtt+(priv->bounds.rmin))*(1+cos(num*G_PI_2)))/2;
      (priv->centre.th)=(ut+lt)*MY_PI_12;
      }
    else if (3*num>1)
      {
      ut=ceil(thx*18);
      lt=floor(thn*18);
      (priv->ticks.zin)=ut-lt;
      (priv->bounds.thmax)=MY_PI_18*ut;
      (priv->bounds.thmin)=MY_PI_18*lt;
      (priv->ticks.z2m)=1;
      if (lt<-10) (priv->thcs)=8;
      else if ((lt>=-1)&&(ut<=10)) (priv->thcs)=6;
      else (priv->thcs)=7;
      (priv->centre.r)=((priv->bounds.rmax)+dtt+(priv->bounds.rmin))/2;
      (priv->centre.th)=(ut+lt)*MY_PI_36;
      }
    else if (6*num>1)
      {
      ut=ceil(thx*36);
      lt=floor(thn*36);
      (priv->ticks.zin)=ut-lt;
      (priv->bounds.thmax)=MY_PI_36*ut;
      (priv->bounds.thmin)=MY_PI_36*lt;
      (priv->ticks.z2m)=0;
      if (lt<-10) (priv->thcs)=8;
      else if ((lt>=-1)&&(ut<=10)) (priv->thcs)=6;
      else (priv->thcs)=7;
      (priv->centre.r)=((priv->bounds.rmax)+dtt+(priv->bounds.rmin))/2;
      (priv->centre.th)=(ut+lt)*MY_PI_72;
      }
    else if (15*num>1)
      {(plot->flags)&1==0
      ut=ceil(thx*90);
      lt=floor(thn*90);
      (priv->ticks.zin)=ut-lt;
      (priv->bounds.thmax)=MY_PI_90*ut;
      (priv->bounds.thmin)=MY_PI_90*lt;
      (priv->ticks.z2m)=0;
      if (lt<-100) (priv->thcs)=9;
      else if ((lt<-10)||(ut>100)) (priv->thcs)=8;
      else if ((lt>=-1)&&(ut<=10)) (priv->thcs)=6;
      else (priv->thcs)=7;
      (priv->centre.r)=((priv->bounds.rmax)+dtt+(priv->bounds.rmin))/2;
      (priv->centre.th)=(ut+lt)*MY_PI_180;
      }
    else
      {
      ut=ceil(thx*180);
      lt=floor(thn*180);
      (priv->ticks.zin)=ut-lt;
      (priv->bounds.thmax)=MY_PI_180*ut;
      (priv->bounds.thmin)=MY_PI_180*lt;
      (priv->ticks.z2m)=0;
      if (lt<-100) (priv->thcs)=10;
      else if ((lt<-10)||(ut>100)) (priv->thcs)=9;
      else if ((lt>=-1)&&(ut<=10)) (priv->thcs)=7;
      else (priv->thcs)=8;
      (priv->centre.r)=((priv->bounds.rmax)+dtt+(priv->bounds.rmin))/2;
      (priv->centre.th)=(ut+lt)*MY_PI_360;
      }
    }
  plot_linear_redraw(widget);
  return FALSE;
  }

gboolean plot_polar_print_eps(GtkWidget *plot, gchar* fout)
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

static gboolean plot_polar_button_press(GtkWidget *widget, GdkEventButton *event)
  {
  PlotPolarPrivate *priv;
  PlotPolar *plot;
  gdouble dy, dx, dt;

  priv=PLOT_POLAR_GET_PRIVATE(widget);
  plot=PLOT_POLAR(widget);
  if ((priv->flagr)==0)
    {
    (priv->rescale.rmax)=(priv->bounds.rmax);
    (priv->rescale.thmax)=(priv->bounds.thmax);
    dy=(priv->y0)-(event->y);
    dx=(event->x)-(priv->x0);
    if (((plot->zmode)&12)!=0)
      {
      dt=atan2(dy, dx); /* divide by zero handling */
      if ((dt<=(priv->bounds.thmax))&&(dt>=(priv->bounds.thmin))) {(priv->rescale.thmax)=dt; (priv->flagr)=1;}
      else if (((dt+2_MY_PI)<=(priv->bounds.thmax))&&((dt+2_MY_PI)<=(priv->bounds.thmin))) {(priv->rescale.thmax)=dt+2_MY_PI; (priv->flagr)=1;}
      else if (((dt-2_MY_PI)<=(priv->bounds.thmax))&&((dt-2_MY_PI)<=(priv->bounds.thmin))) {(priv->rescale.thmax)=dt-2_MY_PI; (priv->flagr)=1;}
      }
    if (((plot->zmode)&10)!=0)
      {
      dx*=dx;
      dy*=dy;
      dy+=dx;
      dy=sqrt(dy)/(priv->s);
      else if (((dt+2_MY_PI)<=(priv->bounds.thmax))&&((dt+2_MY_PI)<=(priv->bounds.thmin))) {(priv->rescale.thmax)=dt+2_MY_PI; (priv->flagr)=1;}
      else if (((dt-2_MY_PI)<=(priv->bounds.thmax))&&((dt-2_MY_PI)<=(priv->bounds.thmin))) {(priv->rescale.thmax)=dt-2_MY_PI; (priv->flagr)=1;}
      if ((dy<=(priv->bounds.rmax))&&(dy>=(priv->bounds.rmin))) {(priv->rescale.rmax)=dy; (priv->flagr)=1;}
      }
    }
  return FALSE;
  }

static gboolean plot_polar_motion_notify(GtkWidget *widget, GdkEventMotion *event)
  {
  PlotPolarPrivate *priv;
  PlotPolar *plot;
  gdouble dx, dy, dt;

  priv=PLOT_POLAR_GET_PRIVATE(widget);
  plot=PLOT_POLAR(widget);
  dy=(priv->y0)-(event->y);
  dx=(event->x)-(priv->x0);
  dt=atan2(dy, dx); /* divide by zero handling */
  if ((dt<=(priv->bounds.thmax))&&(dt>=(priv->bounds.thmin)))
    {
    dy*=dy;
    dx*=dx;
    dy+=dy;
    dy=sqrt(dy)/(priv->s);
    if ((dy>=(priv->bounds.rmin))&&(dy<=(priv->bounds.rmax)))
      {
      (plot->rps)=dy;
      if ((plot->flags)&1==0) (plot->thps)=dt*I180_MY_PI;
      else (plot->thps)=dt;
      g_signal_emit(plot, plot_polar_signals[MOVED], 0);
      }
    }
  else if (((dt+2_MY_PI)<=(priv->bounds.thmax))&&((dt+2_MY_PI)>=(priv->bounds.thmin)))
    {
    dy*=dy;
    dx*=dx;
    dy+=dy;
    dy=sqrt(dy)/(priv->s);
    if ((dy>=(priv->bounds.rmin))&&(dy<=(priv->bounds.rmax)))
      {
      (plot->rps)=dy;
      if ((plot->flags)&1==0) (plot->thps)=(dt*I180_MY_PI)+360;
      else (plot->thps)=dt+2_MY_PI;
      g_signal_emit(plot, plot_polar_signals[MOVED], 0);
      }
    }
  else if (((dt-2_MY_PI)<=(priv->bounds.thmax))&&((dt-2_MY_PI)>=(priv->bounds.thmin)))
    {
    dy*=dy;
    dx*=dx;
    dy+=dy;
    dy=sqrt(dy)/(priv->s);
    if ((dy>=(priv->bounds.rmin))&&(dy<=(priv->bounds.rmax)))
      {
      (plot->rps)=dy;
      if ((plot->flags)&1==0) (plot->thps)=(dt*I180_MY_PI)-360;
      else (plot->thps)=dt-2_MY_PI;
      g_signal_emit(plot, plot_polar_signals[MOVED], 0);
      }
    }
  return FALSE;
  }

static gboolean plot_polar_button_release(GtkWidget *widget, GdkEventButton *event)
  {
  PlotPolarPrivate *priv;
  PlotPolar *plot;
  gint d, xw;
  gdouble xn, xx, yn, yx, s;

  priv=PLOT_POLAR_GET_PRIVATE(widget);
  plot=PLOT_POLAR(widget);
  if ((priv->flagr)==1)
    {
    if (((plot->zmode)&8)==0)
      {
      (priv->rescale.rmin)=(priv->bounds.rmin);
      (priv->rescale.thmin)=(priv->bounds.thmin);
      dy=(priv->y0)-(event->y);
      dx=(event->x)-(priv->x0);
      if (((plot->zmode)&4)!=0)
        {
        dt=atan2(dy, dx); /* divide by zero handling */
        if ((dt<=(priv->bounds.thmax))&&(dt>=(priv->bounds.thmin))) (priv->rescale.thmin)=dt;
        else if (((dt+2_MY_PI)<=(priv->bounds.thmax))&&((dt+2_MY_PI)<=(priv->bounds.thmin))) (priv->rescale.thmin)=dt+2_MY_PI;
        else if (((dt-2_MY_PI)<=(priv->bounds.thmax))&&((dt-2_MY_PI)<=(priv->bounds.thmin))) (priv->rescale.thmin)=dt-2_MY_PI;
        }
      if (((plot->zmode)&2)!=0)
        {
        dx*=dx;
        dy*=dy;
        dy+=dx;
        dy=sqrt(dy)/(priv->s);
        if ((dy<=(priv->bounds.rmax))&&(dt>=(priv->bounds.rmin))) (priv->rescale.rmin)=dy;
        }
      if (((plot->zmode)&1)==0)
        {
        if ((priv->rescale.thmax)!=(priv->rescale.thmin))
          {
          if ((priv->rescale.rmax)>(priv->rescale.rmin))
            {
            xn=(priv->rescale.rmin);
            xx=(priv->rescale.rmax);
            yn=(priv->rescale.thmin);
            yx=(priv->rescale.thmax);
            if ((plot->flags)&1==0) {yn*=I180_MY_PI; yx*=I180_MY_PI;}
            plot_polar_update_scale_pretty(widget, xn, xx, yn, yx);
            }
          else if ((priv->rescale.rmax)<(priv->rescale.rmin))
            {
            xn=(priv->rescale.rmax);
            xx=(priv->rescale.rmin);
            yn=(priv->rescale.thmin);
            yx=(priv->rescale.thmax);
            if ((plot->flags)&1==0) {yn*=I180_MY_PI; yx*=I180_MY_PI;}
            plot_polar_update_scale_pretty(widget, xn, xx, yn, yx);
            }
          }
        }
      else if (((priv->rescale.rmax)!=(priv->rescale.rmin))&&((priv->rescale.thmax)!=(priv->rescale.thmin)))
        {
        s=((priv->bounds.rmax)-(priv->bounds.rmin))/((priv->rescale.rmax)-(priv->rescale.rmin));
        if (s>0) {xn=((priv->bounds.rmin)-(priv->rescale.rmin))*s; xx=((priv->bounds.rmax)-(priv->rescale.rmax))*s;}
        else {xn=((priv->rescale.rmax)-(priv->bounds.rmin))*s; xx=((priv->rescale.rmin)-(priv->bounds.rmax))*s;}
        xn+=(priv->bounds.rmin);
        xx+=(priv->bounds.rmax);
        s=((priv->bounds.thmax)-(priv->bounds.thmin))/((priv->rescale.thmax)-(priv->rescale.thmin));
        if (s>0) {yn=((priv->bounds.thmin)-(priv->rescale.thmin))*s; yx=((priv->bounds.thmax)-(priv->rescale.thmax))*s;}
        else if (s<0) {yn=((priv->rescale.thmax)-(priv->bounds.thmin))*s; yx=((priv->rescale.thmin)-(priv->bounds.thmax))*s;}
        yn+=(priv->bounds.thmin);
        yx+=(priv->bounds.thmax);
        if ((plot->flags)&1==0) {yn*=I180_MY_PI; yx*=I180_MY_PI;}
        plot_polar_update_scale_pretty(widget, xn, xx, yn, yx);
        }
      }
    else if (((plot->zmode)&1)==0)
      {
      xn=((priv->rescale.rmax)*ZS)+(ZSC*(priv->bounds.rmin));
      xx=((priv->rescale.rmax)*ZS)+(ZSC*(priv->bounds.rmax));
      yn=((priv->rescale.thmax)*ZS)+(ZSC*(priv->bounds.thmin));
      yx=((priv->rescale.thmax)*ZS)+(ZSC*(priv->bounds.thmax));
      if ((plot->flags)&1==0) {yn*=I180_MY_PI; yx*=I180_MY_PI;}
      plot_polar_update_scale_pretty(widget, xn, xx, yn, yx);
      }
    else
      {
      xn=((priv->bounds.rmin)*UZ)-(UZC*(priv->rescale.rmax));
      xx=((priv->bounds.rmax)*UZ)-(UZC*(priv->rescale.rmax));
      yn=((priv->bounds.thmin)*UZ)-(UZC*(priv->rescale.thmax));
      yx=((priv->bounds.thmax)*UZ)-(UZC*(priv->rescale.thmax));
      if ((plot->flags)&1==0) {yn*=I180_MY_PI; yx*=I180_MY_PI;}
      plot_polar_update_scale_pretty(widget, xn, xx, yn, yx);
      }
    (priv->flagr)=0;
    }
  else if ((event->y)<=10)
    {
    xw=widget->allocation.width;
    if ((event->x)>=xw-20)
      {
      if ((event->x)>xw-10) {(plot->zmode)^=1; plot_polar_redraw(widget);}
      else if (((plot->zmode)&8)!=0) {(plot->zmode)&=1; plot_polar_redraw(widget);}
      else {(plot->zmode)++; (plot->zmode)++; plot_polar_redraw(widget);}
      }
    }
  return FALSE;
  }

static gboolean plot_polar_expose(GtkWidget *plot, GdkEventExpose *event)
  {
  cairo_t *cr;

  cr=gdk_cairo_create(plot->window);
  cairo_rectangle(cr, (event->area.x), (event->area.y), (event->area.width), (event->area.height));
  cairo_clip(cr);
  draw(plot, cr);
  cairo_destroy(cr);
  return FALSE;
  }

static void plot_polar_class_init(PlotPolarClass *klass)
  {
  GObjectClass *obj_klass;
  GtkWidgetClass *widget_klass;

  obj_klass=G_OBJECT_CLASS(klass);
  g_type_class_add_private(obj_klass, sizeof(PlotPolarPrivate));
  obj_klass->finalize=(GObjectFinalizeFunc) plot_polar_finalise;
  obj_klass->set_property=plot_polar_set_property;
  obj_klass->get_property=plot_polar_get_property;
  g_object_class_install_property(obj_klass, PROP_BRN, g_param_spec_double("rmin", "Minimum r value", "Minimum value for the radial scale", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_BRX, g_param_spec_double("rmax", "Maximum r value", "Maximum value for the radial scale", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_BTN, g_param_spec_double("thmin", "Minimum theta value", "Minimum value for the azimuthal scale", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_BTX, g_param_spec_double("thmax", "Maximum theta value", "Maximum value for the azimuthal scale", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_CR, g_param_spec_double("rcnt", "Centre r value", "Radial value at the centre of the plot", G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_CT, g_param_spec_double("thcnt", "Centre theta value", "Azimuthal value at the centre of the plot", -G_PI, G_PI, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_RT, g_param_spec_uint("rticks", "Radial ticks-1", "Number of grid arcs drawn-1", 1, G_MAXINT, 4, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_ZIT, g_param_spec_uint("thticksin", "Inner radial lines-1", "Number of radial lines at the centre of the circle-1", 1, G_MAXINT, 4, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_ZTM, g_param_spec_uint("thtickmul", "log2((ticks-1)/thticksin)+1", "Number of times rticks must be doubled to equal the number of ticks on the outer arc -1", 1, G_MAXINT, 5, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_ZC, g_param_spec_double("mulcrit", "Critical length where a new radial line is formed", "distance an arc segment between two neighbouring radial lines needs to be before a new radial line forks off", 1, G_MAXINT, 5, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_RC, g_param_spec_uint("rchar", "radial label characters", "Number of characters to store radial label strings", 1, 10, 5, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_TC, g_param_spec_uint("thchar", "theta label characters", "Number of characters to store azimuthal label strings", 1, 10, 5, G_PARAM_READWRITE));
  widget_klass=GTK_WIDGET_CLASS(klass);
  widget_klass->button_press_event=plot_polar_button_press;
  widget_klass->motion_notify_event=plot_polar_motion_notify;
  widget_klass->button_release_event=plot_polar_button_release;
  widget_klass->expose_event=plot_polar_expose;
  plot_polar_signals[MOVED]=g_signal_new("moved", G_OBJECT_CLASS_TYPE(obj_klass), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (PlotPolarClass, moved), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
  }

static void plot_polar_init(PlotPolar *plot)
  {
  PlotPolarPrivate *priv;

  gtk_widget_add_events(GTK_WIDGET(plot), GDK_BUTTON_PRESS_MASK|GDK_POINTER_MOTION_MASK|GDK_BUTTON_RELEASE_MASK);
  priv=PLOT_POLAR_GET_PRIVATE(plot);
  (priv->bounds.rmin)=0;
  (priv->bounds.rmax)=1;
  (priv->bounds.thmin)=0;
  (priv->bounds.thmax)=1;
  (priv->centre.r)=0;
  (priv->centre.th)=0;
  (priv->ticks.r)=4;
  (priv->ticks.zin)=12;
  (priv->ticks.z2m)=2;
  (priv->ticks.zc)=15.0;
  (priv->rcs)=5;
  (priv->thcs)=5;
  (priv->flagr)=0;
  (plot->flaga)=0;
  (plot->rdata)=NULL;
  (plot->thdata)=NULL;
  (plot->size)=0;
  (plot->rlab)=g_strdup("Amplitude");
  (plot->thlab)=g_strdup("Azimuth");
  (plot->afsize)=12;
  (plot->lfsize)=12;
  (plot->ptsize)=5;
  (plot->linew)=2;
  (plot->zmode)=6;
  (plot->rps)=0;
  (plot->thps)=0;
  (plot->flag)=2; /* change in header accordingly */
  }

GtkWidget *plot_polar_new(void)
  {
  return g_object_new(PLOT_TYPE_POLAR, NULL);
  }
