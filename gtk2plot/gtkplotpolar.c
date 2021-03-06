/***************************************************************************
 *            gtkplotpolar.c
 *
 *  A GTK+ widget that plots data on polar axes
 *  version 0.1.0
 *  Features:
 *            optimal visualisation of axes
 *            zoomable ranges
 *            signal emission for mouse movement
 *
 *  Sat Dec  4 17:18:14 2010
 *  Copyright  2011  Paul Childs
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

/* TO DO:
 * curved azimuthal axis label
 * splines in drawn lines
 * ensure new wrapping handling implemented consistently throughout
 * BUGS:
 * positioning not centred well (bad centre values?)
 * single click zoom in no radius (e.g. -4pi/6 to 3pi/6)
 */

#include <gtk/gtk.h>
#include <math.h>
#include <cairo-ps.h>
#include <cairo-svg.h>
#include "gtkplotpolar.h"

#define GTK_PLOT_POLAR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_PLOT_TYPE_POLAR, GtkPlotPolarPrivate))
#define ARP 0.05 /* Proportion of the graph occupied by arrows */
#define IRTR 0.577350269 /* 1/square root 3 */
#define WGP 0.08 /* Proportion of the graph the centre hole occupies */
#define WFP 0.01 /* Proportion of wiggle that is flat to axis */
#define WMP 0.045 /* the mean of these */
#define WHP 0.020207259 /* wiggle height proportion */
#define DZE 0.00001 /* divide by zero threshold */
#define NZE -0.00001 /* negative of this */
#define FAC 0.25 /* floating point accuracy check for logarithms etc */
#define FA2 0.1 /* floating point accuracy check for logarithms etc */
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
#define MY_2PI 6.2831853071795864769252867665590057683943387987502
#define MY_3PI_4 2.3561944901923449288469825374596271631478770495313
#define N2_MY_PI -6.2831853071795864769252867665590057683943387987502
#define NMY_PI -3.1415926535897932384626433832795028841971693993751
#define NMY_PI_2 -1.5707963267948961192313216916397514420985846996876
#define NMY_PI_4 -0.78539816339744830961566084581987572104929234984378
#define N3_MY_PI_4 2.3561944901923449288469825374596271631478770495313
#define I180_MY_PI 57.295779513082320876798154814105170332405472466564
#define I_MY_PI 0.3183098861837906715377675267450287240689192914809
#define I_MY_2_PI 0.1591549430918953357688837633725143620344596457405
#define L10_2PT5 0.3979400086720376095725222105510139464636202370758
#define L10_5 0.6989700043360188047862611052755069732318101185379
#define BFL 10 /*buffer length for axes*/
typedef enum
{
	GTK_PLOT_POLAR_BORDERS_IN = 1 << 0,
	GTK_PLOT_POLAR_BORDERS_OUT = 1 << 1,
	GTK_PLOT_POLAR_BORDERS_CW = 1 << 2,
	GTK_PLOT_POLAR_BORDERS_CCW = 1 << 3
} GtkPlotPolarBorders;
typedef enum
{
	GTK_PLOT_POLAR_AXES_CW = 1 << 0,
	GTK_PLOT_POLAR_AXES_DN = 1 << 1,
	GTK_PLOT_POLAR_AXES_NL = 1 << 2
} GtkPlotPolarAxes;
G_DEFINE_TYPE (GtkPlotPolar, gtk_plot_polar, GTK_TYPE_PLOT);
enum {PROP_0, PROP_BRN, PROP_BRX, PROP_BTN, PROP_BTX, PROP_CR, PROP_CT, PROP_RT, PROP_ZIT, PROP_ZTM, PROP_ZC, PROP_RC, PROP_TC, PROP_HV};
enum {MOVED, KEY_CHANGED, LAST_SIGNAL};
static guint gtk_plot_polar_signals[LAST_SIGNAL]={0};
struct xs {gdouble rmin, thmin, rmax, thmax;};
struct tk {guint r, zin, z2m; gdouble zc;};
struct pt {gdouble r, th;};
typedef struct _GtkPlotPolarPrivate GtkPlotPolarPrivate;
struct _GtkPlotPolarPrivate {struct xs bounds, rescale; struct pt centre; struct tk ticks; gint x0, y0; gdouble s, wr; guint rcs, thcs, flaga, flagr; gboolean hv;};
static const gdouble dsh1[1] = {3.0};
static const gdouble dsh2[2] = {4.0,2.0};
static const gdouble SZ[3]   = {5.0,7.5,10.0};

static void drawz(GtkWidget *widget, cairo_t *cr) {
  GtkPlotPolar *plot;
  gint j, xw;
  gdouble dt;
  static const gdouble AR[3]={0.5,1.0,1.5};

  plot=GTK_PLOT_POLAR(widget);
  cairo_save(cr);
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
  cairo_set_line_width(cr, 1.0);
  cairo_set_dash(cr, NULL, 0, 0.0);
  xw=widget->allocation.width;
  j=(xw<400)?0:(xw<1000)?1:2;
  cairo_rectangle(cr, xw-4*SZ[j]-1.5, 0.5   , 2*SZ[j], 2*SZ[j]);
  cairo_rectangle(cr, xw-2*SZ[j]-0.5, 0.5   , 2*SZ[j], 2*SZ[j]);
  cairo_move_to(cr,   xw-2*SZ[j]+1.0        ,   SZ[j]+0.5);
  cairo_line_to(cr,   xw        -2.0        ,   SZ[j]+0.5);
  cairo_move_to(cr,   xw  -SZ[j]-0.5        ,         2.0);
  cairo_line_to(cr,   xw  -SZ[j]-0.5        , 2*SZ[j]-1.0);
  if (((plot->zmode)&GTK_PLOT_POLAR_ZOOM_OUT)==0) {
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
  cairo_stroke(cr);
  if (((plot->zmode)&GTK_PLOT_POLAR_ZOOM_SGL)==0) {
    dt=1;
    cairo_set_dash(cr, &dt, 1, 0.0);
    if (((plot->zmode)&GTK_PLOT_POLAR_ZOOM_AZM)!=0) {
      cairo_move_to(cr, xw- 4*SZ[j]    + 0.5    ,         3.0);
      cairo_line_to(cr, xw-10*SZ[j]/3.0-13.0/6.0, 2*SZ[j]-1.0);
      cairo_move_to(cr, xw- 2*SZ[j]    - 3.5    ,         3.0);
      cairo_line_to(cr, xw- 8*SZ[j]/3.0- 5.0/6.0, 2*SZ[j]-1.0);
      cairo_stroke(cr);
    }
    if (((plot->zmode)&GTK_PLOT_POLAR_ZOOM_RDL)!=0) {
      cairo_move_to(cr, xw- 4*SZ[j]    + 0.5    , 3.0);
      cairo_curve_to(cr, xw-3*SZ[j]-1.5,         2.0, xw-3*SZ[j]-1.5,         2.0, xw-2*SZ[j]    -3.5    ,         3.0);
      cairo_move_to(cr, xw-10*SZ[j]/3.0-13.0/6.0, 2*SZ[j]-1.0);
      cairo_curve_to(cr, xw-3*SZ[j]-1.5, 2*SZ[j]-2.5, xw-3*SZ[j]-1.5, 2*SZ[j]-2.5, xw-8*SZ[j]/3.0-5.0/6.0, 2*SZ[j]-1.0);
      cairo_stroke(cr);
    }
  }
  cairo_restore(cr);
}

static void drawk(GtkWidget *widget, cairo_t *cr) {
  gdouble hc, vv, wv, xv, yv;
  gint ft, hg, j, k=0, wd, xw;
  GtkPlot *plt;
  GtkPlotPolar *plot;
  guint fg;
  PangoLayout *lyt;

  plt=GTK_PLOT(widget);
  if (!plt->ky || !*(plt->ky)) return;
  xw=widget->allocation.width;
  j=(xw<400)?0:(xw<1000)?1:2;
  hc=2*SZ[j]+3.0;
  plot=GTK_PLOT_POLAR(widget);
  cairo_save(cr);
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
    cairo_set_dash(cr, NULL, 0, 0.0);
    ft=fmod(k,plt->rd->len);
    vv=g_array_index(plt->rd, gdouble, ft);
    wv=g_array_index(plt->gr, gdouble, ft);
    xv=g_array_index(plt->bl, gdouble, ft);
    yv=g_array_index(plt->al, gdouble, ft);
    if (plot->kdata && k<(plot->kdata->len)) {
      fg=g_array_index(plot->kdata, guint, k);
      if (fg&GTK_PLOT_POLAR_KEY_CLR) yv=0.0;
      switch ((fg&GTK_PLOT_POLAR_KEY_DSH)>>GTK_PLOT_POLAR_KEY_DSO) {
        case 2:
          cairo_set_dash(cr, dsh2, sizeof(dsh2)/sizeof(dsh2[0]), 0.0);
          break;
        case 1:
          cairo_set_dash(cr, dsh1, sizeof(dsh1)/sizeof(dsh1[0]), 0.0);
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
  cairo_matrix_t mtr;
  gchar lbl[BFL];
  gchar *str1=NULL, *str2=NULL, *str3;
  gdouble dtr, dtt, tt, thx, thn, dt, sx, csx, ssx, dr1, drs, drc, dz, rt, dwr, rl, ctx, ctn, stx, stn, r, th, rn, tn, x, y, vv, wv, xv, yv;
  gint ft, hg, j, j0, jl, k, kx, lt, st, wd, xt, xw, yw;
  GtkPlot *plt;
  GtkPlotPolar *plot;
  GtkPlotPolarPrivate *priv;
  guint fg;
  PangoLayout *lyt;

	xw=widget->allocation.width;
	yw=widget->allocation.height;
	plot=GTK_PLOT_POLAR(widget);
	plt=GTK_PLOT(widget);
	priv=GTK_PLOT_POLAR_GET_PRIVATE(plot);/* determine scale and fit for graph */
	
/*
	if ((priv->bounds.rmax)<0.0) {(priv->bounds.rmax)=-(priv->bounds.rmax); (priv->bounds.rmin)=-(priv->bounds.rmin);}
	if ((priv->bounds.rmax)<(priv->bounds.rmin))
	{
		dt=(priv->bounds.rmax);
		(priv->bounds.rmax)=(priv->bounds.rmin);
		(priv->bounds.rmin)=dt;
	}
	if ((priv->bounds.rmin)<=0.0) {(priv->bounds.rmin)=0.0; (priv->wr)=0.0;}
	else (priv->wr)=WGP*sqrt(xw*yw);
	if ((priv->centre.r)<=0.0) (priv->centre.r)=0.0;
	else if ((priv->centre.r)>(priv->bounds.rmax)) (priv->centre.r)=(priv->bounds.rmax);
*/
	if ((priv->bounds.rmin<DZE)&&(priv->bounds.rmin>NZE)) {
		priv->bounds.rmin=0.0;
		priv->wr=0.0;
	} else priv->wr=WGP*sqrt(xw*yw);
	dr1=priv->bounds.rmax-priv->bounds.rmin;
	if (((dr1>=0)&&(priv->centre.r>priv->bounds.rmax))||((dr1<=0)&&(priv->centre.r<priv->bounds.rmax))) priv->centre.r=priv->bounds.rmax;
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
	cairo_set_line_width(cr, 2.0);
	lyt=pango_cairo_create_layout(cr);
	pango_layout_set_font_description(lyt, plt->afont);
	str1=g_strdup("8");
	pango_layout_set_text(lyt, str1, -1);
	g_free(str1);
	pango_layout_get_pixel_size(lyt, &wd, &hg);
	g_object_unref(lyt);
	ctx=wd*(priv->thcs-1);
	ctx*=ctx;
	tt=sqrt(hg*hg+ctx);
	lyt=pango_cairo_create_layout(cr);
	pango_layout_set_font_description(lyt, plt->lfont);
	str1=g_strdup("8");
	pango_layout_set_text(lyt, str1, -1);
	g_free(str1);
	pango_layout_get_pixel_size(lyt, &wd, &hg);
	g_object_unref(lyt);
	dtr=hg+JTI;
	dtt=tt+dtr;
	dtr+=hg;
	tt=(tt*0.5)+JTI;
/* drs and drc are the vertical and horizontal distances between rn,tc and rc,tc */
	drc=priv->centre.r-priv->bounds.rmin;
	if (((dr1>=0)&&(drc<0.0))||((dr1<=0)&&(drc>0.0))) {
		drs=0.0;
		drc=0.0;
	} else {
		drs=drc*sin(priv->centre.th);
		drc*=cos(priv->centre.th);
	}
	thx=priv->bounds.thmax;
	while (thx>G_PI) thx-=MY_2PI;
	while (thx<NMY_PI) thx+=MY_2PI;
	thn=priv->bounds.thmin;
	while (thn>G_PI) thn-=MY_2PI;
	while (thn<NMY_PI) thn+=MY_2PI;
	dt=thx-thn;
	if (((dt>DZE)||(dt<NZE))&&((dt+MY_2PI>DZE)||(dt+MY_2PI<NZE))&&((dt-MY_2PI>DZE)||(dt-MY_2PI<NZE))) {
		if ((priv->centre.th>priv->bounds.thmax)||(priv->centre.th<priv->bounds.thmin)) {
			ctx=thx;
			thx=thn;
			thn=ctx;
			dt=-dt;
		}
		ctx=cos(thx);
		ctn=cos(thn);
		stx=sin(thx);
		stn=sin(thn);
		if (dt>=G_PI) {
			priv->s=dr1-drc;
			if ((dr1>=0.0)&&(priv->s<DZE)) priv->s=G_MAXDOUBLE;
			else if ((dr1<=0.0)&&(priv->s>NZE)) priv->s=-G_MAXDOUBLE;
			else priv->s=(xw/2-priv->wr*(1.0-cos(priv->centre.th)))/priv->s; /* check x0 to xw radial to get maximum scale */
			if (thx+thn>=0.0) { /* radial label ccw top to axis azimuthal label bottom to axis */
				priv->flaga=GTK_PLOT_POLAR_AXES_DN;
				sx=dr1-drs;
				if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) { /* check y0 to 0 radial to get maximum scale */
					sx=(yw/2-dtt-priv->wr*(1.0-sin(priv->centre.th)))/sx;
					if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
				}
				sx=dr1*ctx-drc;
				if ((sx>DZE)||(sx<NZE)) { /* check x1'||x1"=0 to get maximum scale */
					if (thx>=MY_3PI_4) sx=(priv->wr*cos(priv->centre.th)-ctx*(dtt+priv->wr)-xw/2)/sx;
					else sx=(priv->wr*(cos(priv->centre.th)-ctx)+dtt*stx-xw/2)/sx;
					if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
				}
				if (thn<=NMY_PI_2) {
					sx=dr1+drs;
					if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) { /* check y0 to yw radial to get maximum scale */
						sx=(yw/2-dtt-priv->wr*(1.0+sin(priv->centre.th)))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				} else { /* check y2',y3'=yw to get maximum scale */
					sx=drs-dr1*stn;
					if ((sx>DZE)||(sx<NZE)) {
						sx=(stn*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)+yw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					if ((drs>DZE)||(drs<NZE)) {
						sx=(priv->wr*(stx-sin(priv->centre.th))+dtt*ctx+yw/2)/drs;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				}
			} else { /* radial label cw bottom to axis azimuthal label top to axis */
				priv->flaga=GTK_PLOT_POLAR_AXES_CW;
				sx=dr1+drs;
				if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) { /* check y0 to yw radial to get maximum scale */
					sx=(yw/2-dtt-priv->wr*(1.0+sin(priv->centre.th)))/sx;
					if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
				}
				sx=dr1*ctn-drc;
				if ((sx>DZE)||(sx<NZE)) { /* check x2'|x2"=0 to get maximum scale */
					if (thn<=N3_MY_PI_4) sx=(priv->wr*cos(priv->centre.th)-ctn*(priv->wr+dtt)-xw/2)/sx;
					else sx=(priv->wr*(cos(priv->centre.th)-ctn)-dtt*stn-xw/2)/sx;
					if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
				}
				if (thx>=G_PI_2) { /* check y0 to 0 radial to get maximum scale */
					sx=dr1-drs;
					if (sx>DZE) {
						sx=(yw/2-dtt-priv->wr*(1.0-sin(priv->centre.th)))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				} else { /* check y1',y4'=0 to get maximum scale */
					sx=drs-(dr1*stx);
					if ((sx>DZE)||(sx<NZE)) {
						sx=(stx*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)-yw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					if ((drs>DZE)||(drs<NZE)) {
						sx=(priv->wr*(stn-sin(priv->centre.th))-dtt*ctn-yw/2)/drs;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				}
			}
		} else if (dt>0.0) {
			if (thx+thn>=G_PI) { /* radial label ccw top to axis azimuthal label bottom to axis */
				priv->flaga=GTK_PLOT_POLAR_AXES_DN;
				priv->s=drs-dr1*stn; /* check y3'=yw to get maximum scale */
				if ((dr1>=0.0)&&(priv->s<DZE)) priv->s=G_MAXDOUBLE;
				else if ((dr1<=0.0)&&(priv->s>NZE)) priv->s=-G_MAXDOUBLE;
				else priv->s=(stx*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)+yw/2)/priv->s;
				sx=dr1*ctx-drc;
				if ((sx>DZE)||(sx<NZE)) {/* check x1'||x1"=0 to get maximum scale */
					if (thx>=MY_3PI_4) sx=(priv->wr*cos(priv->centre.th)-ctx*(priv->wr+dtt)-xw/2)/sx;
					else sx=(priv->wr*(cos(priv->centre.th)-ctx)+dtt*stx-xw/2)/sx;
					if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
				}
				if (thn>=G_PI_2) {
					sx=drs-dr1*stn;
					if ((sx>DZE)||(sx<NZE)) {/* check y2'=0 to get maximum scale */
						sx=(stn*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)-yw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					if ((drc>DZE)||(drc<NZE)) {/* check x4=xw to get maximum scale */
						sx=(priv->wr*(ctn-cos(priv->centre.th))-xw/2)/drc;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				} else {
					sx=dr1-drs;
					if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check y0 to 0 radial to get maximum scale */
						sx=(yw/2-dtt-priv->wr*(1.0-sin(priv->centre.th)))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					sx=dr1*ctn-drc;
					if ((sx>DZE)||(sx<NZE)) {/* check x2'=xw to get maximum scale */
						sx=(priv->wr*cos(priv->centre.th)-ctn*(priv->wr+dtt)+xw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				}
			} else if (thx+thn>=0.0) { /* radial label cw top to axis azimuthal label bottom to axis */
				priv->flaga=GTK_PLOT_POLAR_AXES_DN|GTK_PLOT_POLAR_AXES_CW;
				if (thn>=0) {/* check y4'=yw to get maximum scale */
					if ((dr1>=0.0)&&(drs<DZE)) priv->s=G_MAXDOUBLE;
					else if ((dr1<=0.0)&&(drs>NZE)) priv->s=-G_MAXDOUBLE;
					else priv->s=(priv->wr*(stn-sin(priv->centre.th))-dtt*ctn+yw/2)/drs;
					sx=dr1*ctn-drc;
					if ((sx>DZE)||(sx<NZE)) {/* check x2"||x2'=xw to get maximum scale */
						if (thn>=G_PI_4) sx=(priv->wr*cos(priv->centre.th)-ctn*(priv->wr+dtt)+xw/2)/sx;
						else sx=(priv->wr*(cos(priv->centre.th)-ctn)-dtt*stn+xw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				} else {/* check x0 to xw radial to get maximum scale */
					priv->s=dr1-drc;
					if ((dr1>=0.0)&&(priv->s<DZE)) priv->s=G_MAXDOUBLE;
					else if ((dr1<=0.0)&&(priv->s>NZE)) priv->s=-G_MAXDOUBLE;
					else priv->s=(xw/2-dtt-priv->wr*(1-cos(priv->centre.th)))/priv->s;
					sx=drs-dr1*stn;
					if ((sx>DZE)||(sx<NZE)) {/* check y2'||y2"=yw to get maximum scale */
						if (thn<=NMY_PI_4) sx=(stn*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)+yw/2)/sx;
						else sx=(priv->wr*(stn-sin(priv->centre.th))-dtt*ctn+yw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				}
				if (thx>=G_PI_2) {
					sx=dr1-drs;
					if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check y0 to 0 radial to get maximum scale */
						sx=(yw/2-dtt-priv->wr*(1-sin(priv->centre.th)))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					sx=dr1*cos(thx)-drc;
					if ((sx>DZE)||(sx<NZE)) {/* check x1'=0 to get maximum scale */
						sx=(priv->wr*cos(priv->centre.th)-ctx*(dtt+priv->wr)-xw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				} else {
					sx=drs-dr1*sin(thx);
					if ((sx>DZE)||(sx<NZE)) {/* check y1'=0 to get maximum scale */
						sx=(stx*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)-yw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					if ((drc>DZE)||(drc<NZE)) {/* check x3=0 to get maximum scale */
						sx=(priv->wr*(ctx-cos(priv->centre.th))+xw/2)/drc;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				}
			} else if (thx+thn>=NMY_PI) { /* radial label ccw bottom to axis azimuthal label top to axis */
				priv->flaga=0;
				if (thx>=0) {/* check x0 to xw radial to get maximum scale */
					priv->s=dr1-drc;
					if ((dr1>=0.0)&&(priv->s<DZE)) priv->s=G_MAXDOUBLE;
					else if ((dr1<=0.0)&&(priv->s>NZE)) priv->s=-G_MAXDOUBLE;
					else priv->s=(xw/2-dtt-priv->wr*(1-cos(priv->centre.th)))/priv->s;
					sx=drs-dr1*stx;
					if ((sx>DZE)||(sx<NZE)) {/* check y1"||y1'=0 to get maximum scale */
						if (thx>=G_PI_4) sx=(priv->wr*(stx-sin(priv->centre.th))-dtt*ctx-yw/2)/sx;
						else sx=(stx*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)-yw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				} else {/* check y3'=0 to get maximum scale */
					if ((dr1>=0.0)&&(drs>NZE)) priv->s=G_MAXDOUBLE;
					else if ((dr1<=0.0)&&(drs<DZE)) priv->s=-G_MAXDOUBLE;
					else priv->s=(priv->wr*(stx-sin(priv->centre.th))+dtt*ctx-yw/2)/drs;
					sx=dr1*ctx-drc;
					if ((sx>DZE)||(sx<NZE)) {/* check x1"||x1'=xw to get maximum scale */
						if (thx<=NMY_PI_4) sx=(priv->wr*(cos(priv->centre.th)-ctx)+dtt*stx-xw/2)/sx;
						else sx=(priv->wr*cos(priv->centre.th)-ctx*(priv->wr+dtt)+xw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				}
				if (thn<=NMY_PI_2) {
					sx=dr1+drs;
					if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check y0 to yw radial to get maximum scale */
						sx=(yw/2-dtt-priv->wr*(1+sin(priv->centre.th)))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					sx=(dr1*cos(thn))-drc;
					if ((sx>DZE)||(sx<NZE)) {/* check x2'=0 to get maximum scale */
						sx=(priv->wr*cos(priv->centre.th)-ctn*(priv->wr+dtt)-xw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				} else {
					sx=drs-dr1*sin(thn);
					if ((sx>DZE)||(sx<NZE)) {/* check y2'=yw to get maximum scale */
						sx=(stn*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)+yw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					if ((drc>DZE)||(drc<NZE)) {/* check x4=0 to get maximum scale */
						sx=(priv->wr*(ctn-cos(priv->centre.th))+(xw/2))/drc;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				}
			} else { /* radial label cw bottom to axis azimuthal label top to axis */
				priv->flaga=GTK_PLOT_POLAR_AXES_CW;
				if ((dr1>=0.0)&&(drs>NZE)) priv->s=G_MAXDOUBLE;
				else if ((dr1<=0.0)&&(drs<DZE)) priv->s=-G_MAXDOUBLE;
				else priv->s=(priv->wr*(stn-sin(priv->centre.th))-dtt*ctn-yw/2)/drs; /* check y4'=0 to get maximum scale */
				sx=dr1*ctn-drc;
				if ((sx>DZE)||(sx<NZE)) {/* check x2'||x2"=0 to get maximum scale */
					if (thn<=N3_MY_PI_4) sx=(priv->wr*cos(priv->centre.th)-ctn*(priv->wr+dtt)-xw/2)/sx;
					else sx=(priv->wr*(cos(priv->centre.th)-ctn)-dtt*stn-xw/2)/sx;
					if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
				}
				if (thx<=NMY_PI_2) {
					sx=drs-dr1*stx;
					if ((sx>DZE)||(sx<NZE)) {/* check y1'=yw to get maximum scale */
						sx=(stx*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)+yw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					if ((drc>DZE)||(drc<NZE)) {/* check x3=xw to get maximum scale */
						sx=(priv->wr*(ctx-cos(priv->centre.th))-xw/2)/drc;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				} else {
					sx=dr1+drs;
					if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check y0 to yw radial to get maximum scale */
						sx=(yw/2-dtt-priv->wr*(1+sin(priv->centre.th)))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					sx=dr1*cos(thx)-drc;
					if ((sx>DZE)||(sx<NZE)) {/* check x1'=xw to get maximum scale */
						sx=(priv->wr*cos(priv->centre.th)-ctx*(priv->wr+dtt)-(xw/2))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				}
			}
		} else {/* check radial x0 to 0 */
			priv->s=dr1+drc;
			if ((dr1>=0.0)&&(drs<DZE)) priv->s=G_MAXDOUBLE;
			else if ((dr1<=0.0)&&(drs>NZE)) priv->s=-G_MAXDOUBLE;
			else priv->s=(xw/2-dtt-priv->wr*(1+cos(priv->centre.th)))/priv->s;
			if (dt>=NMY_PI) {
				if (thx+thn>=G_PI) { /* radial label cw bottom to axis azimuthal label top to axis */
					priv->flaga=GTK_PLOT_POLAR_AXES_CW;
					sx=dr1+drs;
					if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial y0 to yw */
						sx=(yw/2-dtt-priv->wr*(1.0+sin(priv->centre.th)))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					sx=dr1-drc;
					if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial x0 to xw */
						sx=(xw/2-dtt-priv->wr*(1.0-cos(priv->centre.th)))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					if (thx>=G_PI_2) {
						sx=dr1-drs;
						if (sx>DZE)
						{/* check radial y0 to 0 */
							sx=(yw/2-dtt-((priv->wr)*(1.0-sin(priv->centre.th))))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
						}
					} else {
						sx=drs-dr1*stx;
						if ((sx>DZE)||(sx<NZE)) {/* check y1'=0 */
							sx=(stx*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)-yw/2)/sx;
							if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
						}
						if (thn>=MY_3PI_4) {
							sx=drs-dr1*stn;
							if ((sx>DZE)||(sx<NZE)) {/* check y2"=0 to get maximum scale */
								sx=(priv->wr*(stn-sin(priv->centre.th))-dtt*ctn-yw/2)/sx;
								if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
							}
						}
					}
				} else if (thx+thn>=0.0) { /* radial label ccw bottom to axis azimuthal label top to axis */
					priv->flaga=0;
					sx=dr1+drs;
					if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial y0 to yw */
						sx=(yw/2-dtt-priv->wr*(1+sin(priv->centre.th)))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					if (thn>=G_PI_2) {
						sx=drs-dr1*stn;
						if ((sx>DZE)||(sx<NZE)) {/* check y2'=0 */
							sx=(stn*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)-yw/2)/sx;
							if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
						}
						if (thx>=0.0) {
							sx=dr1-drc;
							if (sx>DZE)
							{/* check radial x0 to xw */
								sx=(xw/2-dtt-priv->wr*(1-cos(priv->centre.th)))/sx;
								if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
							}
							if (thx<=G_PI_4) {
								sx=drs-dr1*stx;
								if ((sx>DZE)||(sx<NZE)) {/* check y1"=0 */
									sx=(priv->wr*(stx-sin(priv->centre.th))-dtt*ctx-yw/2)/sx;
									if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
								}
							}
						} else {
							sx=dr1*ctx-drc;
							if ((sx>DZE)||(sx<NZE)) {/* check x1'||x1"=xw to get maximum scale */
								if (thx>=NMY_PI_4) sx=(priv->wr*cos(priv->centre.th)-ctx*(dtt+priv->wr)+xw/2)/sx;
								else sx=(priv->wr*(cos(priv->centre.th)-ctx)+dtt*stx+xw/2)/sx;
								if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
							}
						}
					} else {
						sx=dr1-drs;
						if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial y0 to 0 */
							sx=(yw/2-dtt-priv->wr*(1-sin(priv->centre.th)))/sx;
							if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
						}
						if (thx>=0) {
							sx=dr1-drc;
							if (sx>DZE)
							{/* check radial x0 to xw */
								sx=(xw/2-dtt-priv->wr*(1-cos(priv->centre.th)))/sx;
								if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
							}
						} else {
							sx=dr1*ctx-drc;
							if ((sx>DZE)||(sx<NZE)) {/* check x1'||x1"=xw to get maximum scale */
								if (thx>=NMY_PI_4) sx=(priv->wr*cos(priv->centre.th)-ctx*(dtt+priv->wr)+xw/2)/sx;
								else sx=(priv->wr*(cos(priv->centre.th)-ctx)+dtt*stx+xw/2)/sx;
								if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
							}
						}
					}
				} else if (thx+thn>=NMY_PI) { /* radial label cw top to axis azimuthal label bottom to axis */
					priv->flaga=(GTK_PLOT_POLAR_AXES_DN|GTK_PLOT_POLAR_AXES_CW);
					sx=dr1-drs;
					if (sx>DZE)
					{/* check radial y0 to 0 */
						sx=(yw/2-dtt-priv->wr*(1-sin(priv->centre.th)))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					if (thx<=NMY_PI_2) {
						sx=drs-dr1*stx;
						if ((sx>DZE)||(sx<NZE)) {/* check y1'=yw */
							sx=(stx*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)+yw/2)/sx;
							if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
						}
						if ((thn)<=0) {
							sx=dr1-drc;
							if (sx>DZE)
							{/* check radial x0 to xw */
								sx=(xw/2-dtt-priv->wr*(1-cos(priv->centre.th)))/sx;
								if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
							}
							if (thn>=NMY_PI_4) {
								sx=drs-dr1*stn;
								if ((sx>DZE)||(sx<NZE)) {/* check y2"=yw to get maximum scale */
									sx=(priv->wr*(stn-sin(priv->centre.th))-dtt*ctn+yw/2)/sx;
									if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
								}
							}
						} else {
							sx=dr1*ctn-drc;
							if ((sx>DZE)||(sx<NZE)) {/* check x2'||x2"=xw to get maximum scale */
								if (thn<=G_PI_4) sx=(priv->wr*cos(priv->centre.th)-ctn*(priv->wr+dtt)+xw/2)/sx;
								else sx=(priv->wr*(cos(priv->centre.th)-ctn)-dtt*stn+xw/2)/sx;
								if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
							}
						}
					} else {
						sx=dr1+drs;
						if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial y0 to yw */
							sx=(yw/2-dtt-priv->wr*(1+sin(priv->centre.th)))/sx;
							if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
						}
						if (thn<=0.0) {
							sx=dr1-drc;
							if (sx>DZE) {/* check radial x0 to xw */
								sx=(xw/2-dtt-priv->wr*(1-cos(priv->centre.th)))/sx;
								if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
							}
						} else {
							sx=dr1*ctn-drc;
							if ((sx>DZE)||(sx<NZE)) {/* check x2'||x2"=xw to get maximum scale */
								if (thn<=G_PI_4) sx=(priv->wr*cos(priv->centre.th)-ctn*(priv->wr+dtt)+xw/2)/sx;
								else sx=(priv->wr*(cos(priv->centre.th)-ctn)-dtt*stn+xw/2)/sx;
								if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
							}
						}
					}
				} else { /* radial label ccw top to axis azimuthal label bottom to axis */
					priv->flaga=GTK_PLOT_POLAR_AXES_DN;
					sx=dr1-drs;
					if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial y0 to 0 */
						sx=(yw/2-dtt-priv->wr*(1-sin(priv->centre.th)))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					sx=dr1-drc;
					if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial x0 to xw */
						sx=(xw/2-dtt-priv->wr*(1-cos(priv->centre.th)))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					if (thn>=NMY_PI_2) {
						sx=drs-dr1*sin(thn);
						if ((sx>DZE)||(sx<NZE)) {/* check y2'=yw */
							sx=(stn*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)+yw/2)/sx;
							if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
						}
						if (thx<=N3_MY_PI_4) {
							sx=drs-dr1*stx;
							if ((sx>DZE)||(sx<NZE)) {/* check y1"=yw */
								sx=(priv->wr*(stx-sin(priv->centre.th))-dtt*ctx+(yw/2))/sx;
								if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
							}
						}
					} else {
						sx=dr1+drs;
						if (sx>DZE) {/* check radial y0 to yw */
							sx=(yw/2-dtt-priv->wr*(1+sin(priv->centre.th)))/sx;
							if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
						}
					}
				}
			} else if ((thx+thn)>=0) { /* radial label cw bottom to axis azimuthal label top to axis */
				priv->flaga=GTK_PLOT_POLAR_AXES_CW;
				if (thx<=NMY_PI_2) {
					sx=drs-dr1*stx;
					if ((sx>DZE)||(sx<NZE)) {/* check y1'=yw */
						sx=(stx*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)+yw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					if ((drc>DZE)||(drc<NZE)) {/* check x3=xw to get maximum scale */
						sx=(priv->wr*(ctx-cos(priv->centre.th))-xw/2)/drc;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					if (thn>=MY_3PI_4) {
						sx=drs-dr1*stn;
						if ((sx>DZE)||(sx<NZE)) {/* check y2"=0 to get maximum scale */
							sx=(priv->wr*(stn-sin(priv->centre.th))-dtt*ctn-yw/2)/sx;
							if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
						}
					} else {
						sx=drs-dr1*stn;
						if ((sx>DZE)||(sx<NZE)) {/* check y2'=0 */
							sx=(stn*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)-yw/2)/sx;
							if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
						}
						if ((drc>DZE)||(drc<NZE)) {/* check x4'=xw to get maximum scale */
							sx=(priv->wr*(ctn-cos(priv->centre.th))+dtt*stn-xw/2)/drc;
							if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
						}
					}
				} else {
					sx=dr1+drs;
					if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial y0 to yw */
						sx=(yw/2-dtt-priv->wr*(1+sin(priv->centre.th)))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					sx=dr1*ctx-drc;
					if ((sx>DZE)||(sx<NZE)) {/* check x1'=xw to get maximum scale */
						sx=(priv->wr*cos(priv->centre.th)-ctx*(dtt+priv->wr)+xw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					sx=drs-dr1*stn;
					if ((sx>DZE)||(sx<NZE)) {/* check y2"||y2'=0 */
						if (thn>=MY_3PI_4) sx=(priv->wr*(stn-sin(priv->centre.th))-dtt*ctn-yw/2)/sx;
						else sx=(stn*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)-yw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				}
			} else { /* radial label ccw top to axis azimuthal label bottom to axis */
				priv->flaga=GTK_PLOT_POLAR_AXES_DN;
				if (thn>=G_PI_2) {
					sx=drs-dr1*stn;
					if ((sx>DZE)||(sx<NZE)) {/* check y2'=0 */
						sx=(stn*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)-yw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					if ((drc>DZE)||(drc<NZE)) {/* check x4=xw to get maximum scale */
						sx=(priv->wr*(ctn-cos(priv->centre.th))-xw/2)/drc;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					if (thx<=N3_MY_PI_4) {
						sx=drs-dr1*stx;
						if ((sx>DZE)||(sx<NZE)) {/* check y1"=yw */
							sx=(priv->wr*(stx-sin(priv->centre.th))-dtt*ctx+yw/2)/sx;
							if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
						}
					} else {
						sx=drs-dr1*stx;
						if ((sx>DZE)||(sx<NZE)) {/* check y1'=yw */
							sx=(stx*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)+yw/2)/sx;
							if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
						}
						if ((drc>DZE)||(drc<NZE)) {/* check x3'=xw to get maximum scale */
							sx=(priv->wr*(ctx-cos(priv->centre.th))-dtt*stx-xw/2)/drc;
							if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
						}
					}
				} else {
					sx=dr1-drs;
					if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial y0 to 0 */
						sx=(yw/2-dtt-priv->wr*(1-sin(priv->centre.th)))/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					sx=dr1*ctn-drc;
					if ((sx>DZE)||(sx<NZE)) {/* check x2'=xw to get maximum scale */
						sx=(priv->wr*cos(priv->centre.th)-ctn*(priv->wr+dtt)+xw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
					sx=drs-dr1*stx;
					if ((sx>DZE)||(sx<NZE)) {/* check y1"||y1'=yw */
						if (thx<=N3_MY_PI_4) sx=(priv->wr*(stx-sin(priv->centre.th))-dtt*ctx+yw/2)/sx;
						else sx=(stx*(priv->wr+dtt)-priv->wr*sin(priv->centre.th)+yw/2)/sx;
						if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
					}
				}
			}
		}
		priv->x0=xw/2-drc*priv->s-priv->wr*cos(priv->centre.th);
		priv->y0=yw/2+drs*priv->s+priv->wr*sin(priv->centre.th);
		sx=thn;
		kx=1<<(priv->ticks.z2m);
		if (dt<0) dz=(dt+MY_2PI)/(kx*priv->ticks.zin);
		else dz=dt/(kx*priv->ticks.zin);
		cairo_move_to(cr, priv->x0+priv->wr*ctn, priv->y0-priv->wr*stn);
		cairo_line_to(cr, priv->x0+(priv->wr+priv->s*dr1)*ctn, priv->y0-(priv->wr+priv->s*dr1)*stn);
		if ((plot->flagd&GTK_PLOT_POLAR_DISP_RDN)==0) {
			for (j=1; j<priv->ticks.zin; j++) {
				for (k=1; k<kx; k++) {
					sx+=dz;/* additional azimuthal ticks */
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
					cairo_line_to(cr, priv->x0+(priv->wr+JT+priv->s*dr1)*csx, priv->y0-(priv->wr+JT+priv->s*dr1)*ssx);
				}
				sx+=dz;/* inner radial lines to tick */
				csx=cos(sx);
				ssx=sin(sx);
				cairo_move_to(cr, priv->x0+priv->wr*csx, priv->y0-priv->wr*ssx);
				cairo_line_to(cr, priv->x0+(priv->wr+JT+priv->s*dr1)*csx, priv->y0-(priv->wr+JT+priv->s*dr1)*ssx);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, (plt->afont));
				if (priv->thcs<BFL) g_ascii_dtostr(lbl, priv->thcs, round(sx*I180_MY_PI));/* draw azimuthal tick labels */
				else g_ascii_dtostr(lbl, BFL, round(sx*I180_MY_PI));
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				rl=priv->wr+tt+priv->s*dr1;
				cairo_move_to(cr, priv->x0+rl*csx-wd/2, priv->y0-rl*ssx-hg/2);
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
			}
		} else {
			if (dt<0) dwr=priv->ticks.zin/(2.0+I_MY_PI*dt);
			else dwr=priv->ticks.zin*G_PI/dt;
			csx=fmod(dwr,1);
			if ((csx<DZE)||(csx>(1-DZE))) { /* check if angles can be rationalised */
				j0=round(dwr);
				jl=2+floor(log10(j0));
				g_ascii_dtostr(lbl, jl, j0);
				str2=g_strdup(lbl);
				j0=round(priv->bounds.thmin*dwr*I_MY_PI);
				for (j=1; j<priv->ticks.zin; j++) {
					for (k=1; k<kx; k++) {
						sx+=dz;/* additional azimuthal ticks */
						csx=cos(sx);
						ssx=sin(sx);
						cairo_move_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
						cairo_line_to(cr, priv->x0+(priv->wr+JT+priv->s*dr1)*csx, priv->y0-(priv->wr+JT+priv->s*dr1)*ssx);
					}
					sx+=dz;/* inner radial lines to tick */
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+priv->wr*csx, priv->y0-priv->wr*ssx);
					cairo_line_to(cr, priv->x0+(priv->wr+JT+priv->s*dr1)*csx, priv->y0-(priv->wr+JT+priv->s*dr1)*ssx);
					cairo_stroke(cr);
					if (j+j0==0) str3=g_strdup("0");
					else if (j+j0==1) str3=g_strconcat("π/", str2, NULL);
					else if (j+j0==-1) str3=g_strconcat("-π/", str2, NULL);
					else {
						g_ascii_dtostr(lbl, priv->thcs-jl-1, j+j0);
						str1=g_strdup(lbl);
						str3=g_strconcat(str1, "π/", str2, NULL);
						g_free(str1);
					}
					lyt=pango_cairo_create_layout(cr); /* draw azimuthal tick labels */
					pango_layout_set_font_description(lyt, plt->afont);
					pango_layout_set_text(lyt, str3, -1);
					pango_layout_get_pixel_size(lyt, &wd, &hg);
					rl=priv->wr+tt+priv->s*dr1;
					cairo_move_to(cr, priv->x0+rl*csx-(wd/2), priv->y0-rl*ssx-(hg/2));
					pango_cairo_show_layout(cr, lyt);
					g_object_unref(lyt);
					g_free(str3);
				}
				g_free(str2);
			} else {
				for (j=1; j<priv->ticks.zin; j++) {
					for (k=1; k<kx; k++) {
						sx+=dz;/* additional azimuthal ticks */
						csx=cos(sx);
						ssx=sin(sx);
						cairo_move_to(cr, priv->x0+(((priv->wr)+((priv->s)*dr1))*csx), priv->y0-(((priv->wr)+((priv->s)*dr1))*ssx));
						cairo_line_to(cr, priv->x0+(((priv->wr)+JT+((priv->s)*dr1))*csx), priv->y0-(((priv->wr)+JT+((priv->s)*dr1))*ssx));
					}
					sx+=dz;/* inner radial lines to tick */
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+priv->wr*csx, priv->y0-priv->wr*ssx);
					cairo_line_to(cr, priv->x0+(priv->wr+JT+priv->s*dr1)*csx, priv->y0-(priv->wr+JT+priv->s*dr1)*ssx);
					cairo_stroke(cr);
					lyt=pango_cairo_create_layout(cr);
					pango_layout_set_font_description(lyt, plt->afont);
					g_snprintf(lbl, priv->thcs, "%f", sx);/* output radial value as is */
					pango_layout_set_text(lyt, lbl, -1);
					pango_layout_get_pixel_size(lyt, &wd, &hg);
					rl=priv->wr+tt+priv->s*dr1;
					cairo_move_to(cr, priv->x0+rl*csx-(wd/2), priv->y0-rl*ssx-(hg/2));
					pango_cairo_show_layout(cr, lyt);
					g_object_unref(lyt);
				}
			}
		}
		for (k=1; k<kx; k++) {
			sx+=dz;/* additional azimuthal ticks */
			csx=cos(sx);
			ssx=sin(sx);
			cairo_move_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
			cairo_line_to(cr, priv->x0+(priv->wr+JT+priv->s*dr1)*csx, priv->y0-(priv->wr+JT+priv->s*dr1)*ssx);
		}
		cairo_move_to(cr, priv->x0+priv->wr*ctx, priv->y0-priv->wr*stx);
		cairo_line_to(cr, priv->x0+(priv->wr+priv->s*dr1)*ctx, priv->y0-(priv->wr+priv->s*dr1)*stx);
		cairo_stroke(cr);
		cairo_new_sub_path(cr);
		cairo_arc(cr, priv->x0, priv->y0, priv->wr, -thx, -thn);
		rt=priv->wr;
		dwr=dr1*priv->s/priv->ticks.r;
		kx=1;
		if (priv->flaga==0) {
			{mtr.xx=ctx; mtr.xy=stx; mtr.yx=-stx; mtr.yy=ctx; mtr.x0=0; mtr.y0=0;}
			for (j=1; j<priv->ticks.r; j++) {
				rt+=dwr;
				cairo_new_sub_path(cr);/* draw arc grids */
				cairo_arc(cr, priv->x0, priv->y0, rt, -thx-JT/rt, -thn);
				if (rt*dt>=((priv->ticks.zc)*kx*(priv->ticks.zin))) {/* add additional radials */
					dz=(priv->bounds.thmax-priv->bounds.thmin)/(kx*priv->ticks.zin);/* is bounds right to use here and later? */
					sx=priv->bounds.thmin+(dz/2);
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+rt*csx, priv->y0-rt*ssx);
					cairo_line_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
					for (k=1; k<kx*priv->ticks.zin; k++) {
						sx+=dz;
						csx=cos(sx);
						ssx=sin(sx);
						cairo_move_to(cr, priv->x0+rt*csx, priv->y0-rt*ssx);
						cairo_line_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
					}
					kx*=2;
				}
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, plt->afont);
				g_snprintf(lbl, priv->rcs, "%f", priv->bounds.rmin+(rt-priv->wr)/priv->s);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				sx=rt-(wd/2);
				dz=-JTI-hg;
				cairo_move_to(cr, priv->x0+sx*ctx+dz*stx, priv->y0+dz*ctx-sx*stx);
				cairo_set_matrix(cr, &mtr);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);/* draw radial tick labels*/
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
			}
			rt+=dwr;
			cairo_new_sub_path(cr);
			cairo_arc(cr, priv->x0, priv->y0, rt, -thx, -thn);
			cairo_stroke(cr);
			lyt=pango_cairo_create_layout(cr); /* draw radial label */
			pango_layout_set_font_description(lyt, plt->lfont);
			pango_layout_set_text(lyt, plot->rlab, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			sx=priv->wr+(dr1*priv->s-wd)/2;
			cairo_move_to(cr, priv->x0+sx*ctx-dtr*stx, priv->y0-dtr*ctx-sx*stx);
			cairo_set_matrix(cr, &mtr);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			lyt=pango_cairo_create_layout(cr); /* draw azimuthal label */
			pango_layout_set_font_description(lyt, plt->lfont);
			g_object_unref(lyt);
		} else if (priv->flaga==GTK_PLOT_POLAR_AXES_DN) {
			{mtr.xx=-ctx; mtr.xy=-stx; mtr.yx=stx; mtr.yy=-ctx;}
			for (j=1; j<priv->ticks.r; j++) {
				rt+=dwr;
				cairo_new_sub_path(cr);/* draw arc grids */
				cairo_arc(cr, priv->x0, priv->y0, rt, -thx-JT/rt, -thn);
				if (rt*dt>=priv->ticks.zc*kx*priv->ticks.zin) {/* add additional radials */
					dz=(thx-thn)/(kx*priv->ticks.zin);
					sx=thn+(dz/2);
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+rt*csx, priv->y0-rt*ssx);
					cairo_line_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
					for (k=1; k<kx*priv->ticks.zin; k++) {
						sx+=dz;
						csx=cos(sx);
						ssx=sin(sx);
						cairo_move_to(cr, priv->x0+rt*csx, priv->y0-rt*ssx);
						cairo_line_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
					}
					kx*=2;
				}
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, plt->afont);
				g_snprintf(lbl, priv->rcs, "%f", priv->bounds.rmin+(rt-priv->wr)/priv->s);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				sx=rt+(wd/2);
				cairo_move_to(cr, priv->x0+sx*ctx-JTI*stx, priv->y0-JTI*ctx-sx*stx);
				cairo_set_matrix(cr, &mtr);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);/* draw radial tick labels*/
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
			}
			rt+=dwr;
			cairo_new_sub_path(cr);
			cairo_arc(cr, priv->x0, priv->y0, rt, -thx, -thn);
			cairo_stroke(cr);
			lyt=pango_cairo_create_layout(cr); /* draw radial label */
			pango_layout_set_font_description(lyt, plt->lfont);
			pango_layout_set_text(lyt, plot->rlab, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			sx=priv->wr+(dr1*priv->s+wd)/2;
			dz=hg-dtr;
			cairo_move_to(cr, priv->x0+sx*ctx+dz*stx, priv->y0+dz*ctx-sx*stx);
			cairo_set_matrix(cr, &mtr);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			lyt=pango_cairo_create_layout(cr); /* draw azimuthal label */
			pango_layout_set_font_description(lyt, plt->lfont);
			g_object_unref(lyt);
		} else if (priv->flaga==(GTK_PLOT_POLAR_AXES_DN|GTK_PLOT_POLAR_AXES_CW)) {
			{mtr.xx=ctn; mtr.xy=stn; mtr.yx=-stn; mtr.yy=ctn;}
			for (j=1; j<priv->ticks.r; j++) {
				rt+=dwr;
				cairo_new_sub_path(cr);/* draw arc grids */
				cairo_arc(cr, priv->x0, priv->y0, rt, -thx, -thn+JT/rt);
				if (rt*dt>=priv->ticks.zc*kx*priv->ticks.zin) {/* add additional radials */
					dz=(thx-thn)/(kx*priv->ticks.zin);
					sx=thn+(dz/2);
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+rt*csx, priv->y0-rt*ssx);
					cairo_line_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
					for (k=1; k<kx*priv->ticks.zin; k++) {
						sx+=dz;
						csx=cos(sx);
						ssx=sin(sx);
						cairo_move_to(cr, priv->x0+rt*csx, priv->y0-rt*ssx);
						cairo_line_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
					}
					kx*=2;
				}
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, plt->afont);
				g_snprintf(lbl, priv->rcs, "%f", priv->bounds.rmin+(rt-priv->wr)/priv->s);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				sx=rt-(wd/2);
				cairo_move_to(cr, priv->x0+sx*ctn+JTI*stn, priv->y0+JTI*ctn-sx*stn);
				cairo_set_matrix(cr, &mtr);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);/* draw radial tick labels*/
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
			}
			rt+=dwr;
			cairo_new_sub_path(cr);
			cairo_arc(cr, priv->x0, priv->y0, rt, -thx, -thn);
			cairo_stroke(cr);
			lyt=pango_cairo_create_layout(cr); /* draw radial label */
			pango_layout_set_font_description(lyt, plt->lfont);
			pango_layout_set_text(lyt, plot->rlab, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			sx=priv->wr+(dr1*priv->s-wd)/2;
			dz=dtr-hg;
			cairo_move_to(cr, priv->x0+sx*ctn+dz*stn, priv->y0+dz*ctn-sx*stn);
			cairo_set_matrix(cr, &mtr);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			lyt=pango_cairo_create_layout(cr); /* draw azimuthal label */
			pango_layout_set_font_description(lyt, plt->lfont);
			g_object_unref(lyt);
		} else {
			{mtr.xx=-ctn; mtr.xy=-stn; mtr.yx=stn; mtr.yy=-ctn;}
			for (j=1; j<priv->ticks.r; j++) {
				rt+=dwr;
				cairo_new_sub_path(cr);/* draw arc grids */
				cairo_arc(cr, priv->x0, priv->y0, rt, -thx, -thn+JT/rt);
				if (rt*dt>=priv->ticks.zc*kx*priv->ticks.zin) {/* add additional radials */
					dz=(thx-thn)/(kx*priv->ticks.zin);
					sx=thn+(dz/2);
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+rt*csx, priv->y0-rt*ssx);
					cairo_line_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
					for (k=1; k<kx*priv->ticks.zin; k++) {
						sx+=dz;
						csx=cos(sx);
						ssx=sin(sx);
						cairo_move_to(cr, priv->x0+rt*csx, priv->y0-rt*ssx);
						cairo_line_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
					}
					kx*=2;
				}
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, plt->afont);
				g_snprintf(lbl, priv->rcs, "%f", priv->bounds.rmin+(rt-priv->wr)/priv->s);
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				sx=rt+(wd/2);
				dz=JTI+hg;
				cairo_move_to(cr, priv->x0+sx*ctn+dz*stn, priv->y0+dz*ctn-sx*stn);
				cairo_set_matrix(cr, &mtr);
				pango_cairo_update_layout(cr, lyt);
				pango_cairo_show_layout(cr, lyt);/* draw radial tick labels*/
				g_object_unref(lyt);
				cairo_identity_matrix(cr);
			}
			rt+=dwr;
			cairo_new_sub_path(cr);
			cairo_arc(cr, priv->x0, priv->y0, rt, -thx, -thn);
			cairo_stroke(cr);
			lyt=pango_cairo_create_layout(cr); /* draw radial label */
			pango_layout_set_font_description(lyt, plt->lfont);
			pango_layout_set_text(lyt, plot->rlab, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			sx=priv->wr+(dr1*priv->s+wd)/2;
			cairo_move_to(cr, priv->x0+sx*ctn+dtr*stn, priv->y0+dtr*ctn-sx*stn);
			cairo_set_matrix(cr, &mtr);
			pango_cairo_update_layout(cr, lyt);
			pango_cairo_show_layout(cr, lyt);
			g_object_unref(lyt);
			cairo_identity_matrix(cr);
			lyt=pango_cairo_create_layout(cr); /* draw azimuthal label */
			pango_layout_set_font_description(lyt, plt->lfont);
			g_object_unref(lyt);
		}
	} else if (priv->bounds.rmin==0.0) {
		priv->s=dr1-drc;/* check radial x0 to xw */
		if ((dr1>=0.0)&&(priv->s<DZE)) priv->s=G_MAXDOUBLE;
		else if ((dr1<=0.0)&&(priv->s>NZE)) priv->s=-G_MAXDOUBLE;
		else priv->s=((xw/2)-dtt)/priv->s;
		sx=dr1-drs;
		if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial y0 to 0 */
			sx=((yw/2)-dtt)/sx;
			if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
		}
		sx=dr1+drc;
		if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial x0 to 0 */
			sx=((xw/2)-dtt-priv->wr*(1.0+cos(priv->centre.th)))/sx;
			if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
		}
		sx=dr1+drs;
		if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial y0 to yw */
			sx=((yw/2)-dtt)/sx;
			if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
		}
		priv->flaga=GTK_PLOT_POLAR_AXES_NL;
		priv->x0=(xw/2)-drc*priv->s;
		priv->y0=(yw/2)+drs*priv->s;
		sx=priv->bounds.thmin;
		kx=1<<priv->ticks.z2m;
		dz=MY_2PI/(kx*priv->ticks.zin);
		csx=cos(sx);
		ssx=sin(sx);
		cairo_move_to(cr, priv->x0, priv->y0);
		cairo_line_to(cr, priv->x0+(JT+priv->s*dr1)*csx, priv->y0-(JT+priv->s*dr1)*ssx);
		if ((plot->flagd&GTK_PLOT_POLAR_DISP_RDN)==0) {
			for (j=1; j<priv->ticks.zin; j++) {
				for (k=1; k<kx; k++) {
					sx+=dz;/* additional azimuthal ticks */
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+priv->s*dr1*csx, (priv->y0)-priv->s*dr1*ssx);
					cairo_line_to(cr, priv->x0+(JT+priv->s*dr1)*csx, priv->y0-(JT+priv->s*dr1)*ssx);
				}
				sx+=dz/* inner radial lines to tick */;
				csx=cos(sx);
				ssx=sin(sx);
				cairo_move_to(cr, priv->x0, priv->y0);
				cairo_line_to(cr, priv->x0+(JT+priv->s*dr1)*csx, priv->y0-(JT+priv->s*dr1)*ssx);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr);
				pango_layout_set_font_description(lyt, plt->afont);
				g_snprintf(lbl, priv->thcs, "%d", (gint) round(sx*I180_MY_PI));/* draw azimuthal tick labels */
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				rl=tt+priv->s*dr1;
				cairo_move_to(cr, priv->x0+rl*csx-(wd/2), priv->y0-rl*ssx-(hg/2));
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
			}
		} else if (fmod((priv->ticks.zin),2)==0) {
			jl=2+floor(log10(priv->ticks.zin/2));
			g_ascii_dtostr(lbl, jl, priv->ticks.zin/2);
			str2=g_strdup(lbl);
			j0=priv->bounds.thmin*priv->ticks.zin*I_MY_2_PI;
			for (j=1; j<priv->ticks.zin; j++) {
				for (k=1; k<kx; k++) {
					sx+=dz;/* additional azimuthal ticks */
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+priv->s*dr1*csx, priv->y0-priv->s*dr1*ssx);
					cairo_line_to(cr, priv->x0+(JT+priv->s*dr1)*csx, priv->y0-(JT+priv->s*dr1)*ssx);
				}
				sx+=dz/* inner radial lines to tick */;
				csx=cos(sx);
				ssx=sin(sx);
				cairo_move_to(cr, priv->x0, priv->y0);
				cairo_line_to(cr, priv->x0+(JT+priv->s*dr1)*csx, priv->y0-(JT+priv->s*dr1)*ssx);
				cairo_stroke(cr);
				if (j+j0==0) str3=g_strdup("0");
				else if (j+j0==1) str3=g_strconcat("π/", str2, NULL);
				else if (j+j0==-1) str3=g_strconcat("-π/", str2, NULL);
				else {
					g_ascii_dtostr(lbl, priv->thcs-jl-1, j+j0);
					str1=g_strdup(lbl);
					str3=g_strconcat(str1, "π/", str2, NULL);
					g_free(str1);
				}
				lyt=pango_cairo_create_layout(cr); /* draw azimuthal tick labels */
				pango_layout_set_font_description(lyt, plt->afont);
				pango_layout_set_text(lyt, str3, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				rl=tt+priv->s*dr1;
				cairo_move_to(cr, priv->x0+rl*csx-(wd/2), priv->y0-rl*ssx-(hg/2));
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				g_free(str3);
			}
			g_free(str2);
		} else {
			jl=2+floor(log10(priv->ticks.zin));
			g_ascii_dtostr(lbl, jl, priv->ticks.zin);
			str2=g_strdup(lbl);
			j0=priv->bounds.thmin*priv->ticks.zin*I_MY_PI;
			for (j=2; j<2*priv->ticks.zin; j+=2) {
				for (k=1; k<kx; k++) {
					sx+=dz;/* additional azimuthal ticks */
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+priv->s*dr1*csx, (priv->y0)-priv->s*dr1*ssx);
					cairo_line_to(cr, priv->x0+(JT+priv->s*dr1)*csx, priv->y0-(JT+priv->s*dr1)*ssx);
				}
				sx+=dz/* inner radial lines to tick */;
				csx=cos(sx);
				ssx=sin(sx);
				cairo_move_to(cr, priv->x0, priv->y0);
				cairo_line_to(cr, priv->x0+(JT+priv->s*dr1)*csx, priv->y0-(JT+priv->s*dr1)*ssx);
				cairo_stroke(cr);
				if (j+j0==0) str3=g_strdup("0");
				else if (j+j0==1) str3=g_strconcat("π/", str2, NULL);
				else if (j+j0==-1) str3=g_strconcat("-π/", str2, NULL);
				else {
					g_ascii_dtostr(lbl, priv->thcs-jl-1, j+j0);
					str1=g_strdup(lbl);
					str3=g_strconcat(str1, "π/", str2, NULL);
					g_free(str1);
				}
				lyt=pango_cairo_create_layout(cr); /* draw azimuthal tick labels */
				pango_layout_set_font_description(lyt, plt->afont);
				pango_layout_set_text(lyt, str3, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				rl=tt+priv->s*dr1;
				cairo_move_to(cr, priv->x0+rl*csx-(wd/2), priv->y0-rl*ssx-(hg/2));
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				g_free(str3);
			}
			g_free(str2);
		}
		for (k=1; k<kx; k++) {
			sx+=dz;/* additional azimuthal ticks */
			csx=cos(sx);
			ssx=sin(sx);
			cairo_move_to(cr, priv->x0+priv->s*dr1*csx, priv->y0-priv->s*dr1*ssx);
			cairo_line_to(cr, priv->x0+(JT+priv->s*dr1)*csx, priv->y0-(JT+priv->s*dr1)*ssx);
		}
		cairo_stroke(cr);
		rt=0;
		dwr=dr1*priv->s/priv->ticks.r;
		kx=1;
		for (j=1; j<priv->ticks.r; j++)
		{
			rt+=dwr;
			cairo_new_sub_path(cr);
			cairo_arc(cr, priv->x0, priv->y0, rt, NMY_PI, G_PI);
			if (rt*MY_2PI>=priv->ticks.zc*kx*priv->ticks.zin) {/* add additional radials */
				dz=MY_2PI/(kx*priv->ticks.zin);
				sx=(dz/2);
				csx=cos(sx);
				ssx=sin(sx);
				cairo_move_to(cr, priv->x0+rt*csx, priv->y0-rt*ssx);
				cairo_line_to(cr, priv->x0+priv->s*dr1*csx, priv->y0-priv->s*dr1*ssx);
				for (k=1; k<kx*priv->ticks.zin; k++) {
					sx+=dz;
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+rt*csx, priv->y0-rt*ssx);
					cairo_line_to(cr, priv->x0+priv->s*dr1*csx, priv->y0-priv->s*dr1*ssx);
				}
				kx*=2;
			}
			cairo_stroke(cr);
			lyt=pango_cairo_create_layout(cr);
			pango_layout_set_font_description(lyt, plt->afont);
			g_snprintf(lbl, priv->rcs, "%f", rt/priv->s);
			pango_layout_set_text(lyt, lbl, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			cairo_move_to(cr, priv->x0+rt-(wd/2), priv->y0+JTI);
			pango_cairo_show_layout(cr, lyt);/* draw radial tick labels*/
			g_object_unref(lyt);
		}
		rt+=dwr;
		cairo_new_sub_path(cr);
		cairo_arc(cr, priv->x0, priv->y0, rt, NMY_PI, G_PI);
		cairo_stroke(cr);
		lyt=pango_cairo_create_layout(cr); /* draw radial label */
		pango_layout_set_font_description(lyt, plt->lfont);
		pango_layout_set_text(lyt, plot->rlab, -1);
		pango_layout_get_pixel_size(lyt, &wd, &hg);
		cairo_move_to(cr, priv->x0+(dr1*priv->s-wd)/2, priv->y0+dtr-hg);
		pango_cairo_show_layout(cr, lyt);
		g_object_unref(lyt);
		lyt=pango_cairo_create_layout(cr); /* draw azimuthal label */
		pango_layout_set_font_description(lyt, plt->lfont);
		g_object_unref(lyt);
	} else {
		(priv->s)=dr1-drc;/* check radial x0 to xw */
		if ((dr1>=0.0)&&(priv->s<DZE)) priv->s=G_MAXDOUBLE;
		else if ((dr1<=0.0)&&(priv->s>NZE)) priv->s=-G_MAXDOUBLE;
		else priv->s=((xw/2)-dtt-priv->wr*(1.0-cos(priv->centre.th)))/priv->s;
		sx=dr1-drs;
		if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial y0 to 0 */
			sx=((yw/2)-dtt-priv->wr*(1.0-sin(priv->centre.th)))/sx;
			if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
		}
		sx=dr1+drc;
		if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial x0 to 0 */
			sx=((xw/2)-dtt-priv->wr*(1.0+cos(priv->centre.th)))/sx;
			if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
		}
		sx=dr1+drs;
		if (((dr1>0.0)&&(sx>DZE))||((dr1<0.0)&&(sx<NZE))) {/* check radial y0 to yw */
			sx=((yw/2)-dtt-priv->wr*(1.0+sin(priv->centre.th)))/sx;
			if (((dr1>0.0)&&(sx>0.0)&&(sx<priv->s))||((dr1<0.0)&&(sx<0.0)&&(sx>priv->s))) priv->s=sx;
		}
		priv->flaga=GTK_PLOT_POLAR_AXES_NL;
		priv->x0=(xw/2)-drc*priv->s-priv->wr*cos(priv->centre.th);
		priv->y0=(yw/2)+drs*priv->s+priv->wr*sin(priv->centre.th);
		sx=0;
		kx=1<<priv->ticks.z2m;
		dz=MY_2PI/(kx*priv->ticks.zin);
		dt=4;
		cairo_save(cr);
		cairo_set_dash(cr, &dt, 1, 0);
		cairo_move_to(cr, priv->x0+priv->wr/3, priv->y0);
		cairo_line_to(cr, priv->x0+priv->wr, priv->y0);
		cairo_stroke(cr);
		cairo_restore(cr);
		cairo_line_to(cr, priv->x0+priv->wr+priv->s*dr1, priv->y0);
		if ((plot->flagd&GTK_PLOT_POLAR_DISP_RDN)==0) {
			for (j=1; j<priv->ticks.zin; j++) {
				for (k=1; k<kx; k++) {
					sx+=dz;/* additional azimuthal ticks */
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
					cairo_line_to(cr, priv->x0+(priv->wr+JT+priv->s*dr1)*csx, priv->y0-(priv->wr+JT+priv->s*dr1)*ssx);
				}
				sx+=dz/* inner radial lines to tick */;
				csx=cos(sx);
				ssx=sin(sx);
				cairo_stroke(cr);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, priv->x0+priv->wr*csx/3, priv->y0-priv->wr*ssx/3);
				cairo_line_to(cr, priv->x0+priv->wr*csx, priv->y0-priv->wr*ssx);
				cairo_stroke(cr);
				cairo_restore(cr);
				cairo_line_to(cr, priv->x0+(priv->wr+JT+priv->s*dr1)*csx, priv->y0-(priv->wr+JT+priv->s*dr1)*ssx);
				cairo_stroke(cr);
				lyt=pango_cairo_create_layout(cr); /* draw azimuthal tick labels */
				pango_layout_set_font_description(lyt, plt->afont);
				g_snprintf(lbl, priv->thcs, "%d", (gint) round(sx*I180_MY_PI));
				pango_layout_set_text(lyt, lbl, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				rl=priv->wr+tt+priv->s*dr1;
				cairo_move_to(cr, priv->x0+rl*csx-(wd/2), priv->y0-rl*ssx-(hg/2));
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
			}
		} else if (fmod(priv->ticks.zin,2)==0) {
			jl=2+floor(log10(priv->ticks.zin/2));
			g_ascii_dtostr(lbl, jl, priv->ticks.zin/2);
			str2=g_strdup(lbl);
			j0=priv->bounds.thmin*priv->ticks.zin*I_MY_2_PI;
			for (j=1; j<priv->ticks.zin; j++) {
				for (k=1; k<kx; k++) {
					sx+=dz;/* additional azimuthal ticks */
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
					cairo_line_to(cr, priv->x0+(priv->wr+JT+priv->s*dr1)*csx, priv->y0-(priv->wr+JT+priv->s*dr1)*ssx);
				}
				sx+=dz;/* inner radial lines to tick */
				csx=cos(sx);
				ssx=sin(sx);
				cairo_stroke(cr);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, priv->x0+priv->wr*csx/3, priv->y0-priv->wr*ssx/3);
				cairo_line_to(cr, priv->x0+priv->wr*csx, priv->y0-priv->wr*ssx);
				cairo_stroke(cr);
				cairo_restore(cr);
				cairo_move_to(cr, priv->x0+priv->wr*csx, priv->y0-priv->wr*ssx);
				cairo_line_to(cr, priv->x0+(priv->wr+JT+priv->s*dr1)*csx, priv->y0-(priv->wr+JT+priv->s*dr1)*ssx);
				cairo_stroke(cr);
				if (j+j0==0) str3=g_strdup("0");
				else if (j+j0==1) str3=g_strconcat("π/", str2, NULL);
				else if (j+j0==-1) str3=g_strconcat("-π/", str2, NULL);
				else {
					g_ascii_dtostr(lbl, priv->thcs-jl-1, j+j0);
					str1=g_strdup(lbl);
					str3=g_strconcat(str1, "π/", str2, NULL);
					g_free(str1);
				}
				lyt=pango_cairo_create_layout(cr); /* draw azimuthal tick labels */
				pango_layout_set_font_description(lyt, plt->afont);
				pango_layout_set_text(lyt, str3, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				rl=priv->wr+tt+priv->s*dr1;
				cairo_move_to(cr, priv->x0+rl*csx-(wd/2), priv->y0-rl*ssx-(hg/2));
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				g_free(str3);
			}
			g_free(str2);
		} else {
			jl=2+floor(log10(priv->ticks.zin));
			g_ascii_dtostr(lbl, jl, priv->ticks.zin);
			str2=g_strdup(lbl);
			j0=priv->bounds.thmin*priv->ticks.zin*I_MY_PI;
			for (j=2; j<2*priv->ticks.zin; j+=2) {
				for (k=1; k<kx; k++) {
					sx+=dz;/* additional azimuthal ticks */
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
					cairo_line_to(cr, priv->x0+(priv->wr+JT+priv->s*dr1)*csx, priv->y0-(priv->wr+JT+priv->s*dr1)*ssx);
				}
				sx+=dz;/* inner radial lines to tick */
				csx=cos(sx);
				ssx=sin(sx);
				cairo_stroke(cr);
				cairo_save(cr);
				cairo_set_dash(cr, &dt, 1, 0);
				cairo_move_to(cr, priv->x0+priv->wr*csx/3, priv->y0-priv->wr*ssx/3);
				cairo_line_to(cr, priv->x0+priv->wr*csx, priv->y0-priv->wr*ssx);
				cairo_stroke(cr);
				cairo_restore(cr);
				cairo_move_to(cr, priv->x0+priv->wr*csx, priv->y0-priv->wr*ssx);
				cairo_line_to(cr, priv->x0+(priv->wr+JT+priv->s*dr1)*csx, (priv->y0)-(priv->wr+JT+priv->s*dr1)*ssx);
				cairo_stroke(cr);
				if (j+j0==0) str3=g_strdup("0");
				else if (j+j0==1) str3=g_strconcat("π/", str2, NULL);
				else if (j+j0==-1) str3=g_strconcat("-π/", str2, NULL);
				else {
					g_ascii_dtostr(lbl, priv->thcs-jl-1, j+j0);
					str1=g_strdup(lbl);
					str3=g_strconcat(str1, "π/", str2, NULL);
					g_free(str1);
				}
				lyt=pango_cairo_create_layout(cr); /* draw azimuthal tick labels */
				pango_layout_set_font_description(lyt, plt->afont);
				pango_layout_set_text(lyt, str3, -1);
				pango_layout_get_pixel_size(lyt, &wd, &hg);
				rl=priv->wr+tt+priv->s*dr1;
				cairo_move_to(cr, priv->x0+rl*csx-(wd/2), priv->y0-rl*ssx-(hg/2));
				pango_cairo_show_layout(cr, lyt);
				g_object_unref(lyt);
				g_free(str3);
			}
			g_free(str2);
		}
		for (k=1; k<kx; k++) {
			sx+=dz;/* additional azimuthal ticks */
			csx=cos(sx);
			ssx=sin(sx);
			cairo_move_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
			cairo_line_to(cr, priv->x0+(priv->wr+JT+priv->s*dr1)*csx, priv->y0-(priv->wr+JT+priv->s*dr1)*ssx);
		}
		cairo_stroke(cr);
		cairo_new_sub_path(cr);
		cairo_arc(cr, priv->x0, priv->y0, priv->wr, NMY_PI, G_PI);
		rt=priv->wr;
		dwr=dr1*priv->s/priv->ticks.r;
		kx=1;
		for (j=1; j<priv->ticks.r; j++) {
			rt+=dwr;
			cairo_new_sub_path(cr);
			cairo_arc(cr, priv->x0, priv->y0, rt, NMY_PI, G_PI);
			if (rt*MY_2PI>=priv->ticks.zc*kx*priv->ticks.zin) {/* add additional radials */
				dz=MY_2PI/(kx*(priv->ticks.zin));
				sx=(dz/2);
				csx=cos(sx);
				ssx=sin(sx);
				cairo_move_to(cr, priv->x0+rt*csx, priv->y0-rt*ssx);
				cairo_line_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
				for (k=1; k<kx*priv->ticks.zin; k++) {
					sx+=dz;
					csx=cos(sx);
					ssx=sin(sx);
					cairo_move_to(cr, priv->x0+rt*csx, priv->y0-rt*ssx);
					cairo_line_to(cr, priv->x0+(priv->wr+priv->s*dr1)*csx, priv->y0-(priv->wr+priv->s*dr1)*ssx);
				}
				kx*=2;
			}
			cairo_stroke(cr);
			lyt=pango_cairo_create_layout(cr);
			pango_layout_set_font_description(lyt, plt->afont);
			g_snprintf(lbl, priv->rcs, "%f", priv->bounds.rmin+(rt-priv->wr)/priv->s);
			pango_layout_set_text(lyt, lbl, -1);
			pango_layout_get_pixel_size(lyt, &wd, &hg);
			cairo_move_to(cr, priv->x0+rt-(wd/2), priv->y0+JTI);
			pango_cairo_show_layout(cr, lyt);/* draw radial tick labels*/
			g_object_unref(lyt);
		}
		rt+=dwr;
		cairo_new_sub_path(cr);
		cairo_arc(cr, (priv->x0), (priv->y0), rt, NMY_PI, G_PI);
		cairo_stroke(cr);
		lyt=pango_cairo_create_layout(cr); /* draw radial label */
		pango_layout_set_font_description(lyt, plt->lfont);
		pango_layout_set_text(lyt, plot->rlab, -1);
		pango_layout_get_pixel_size(lyt, &wd, &hg);
		cairo_move_to(cr, priv->x0+priv->wr+(dr1*priv->s-wd)/2, priv->y0+dtr-hg);
		pango_cairo_show_layout(cr, lyt);
		g_object_unref(lyt);
		lyt=pango_cairo_create_layout(cr); /* draw azimuthal label */
		pango_layout_set_font_description(lyt, plt->lfont);
		g_object_unref(lyt);
	}
  if (!(plot->rdata) || !(plot->thdata)) return; /* else plot data */
  if (priv->hv) {/* take thdata from second set offset by the length of rdata */
    if ((plot->flagd&GTK_PLOT_POLAR_DISP_LIN)!=0) {
      cairo_set_line_width(cr, plot->linew);
      if ((plot->flagd&GTK_PLOT_POLAR_DISP_PTS)!=0) {/* lines and points */
        if (TRUE||(plot->flagd&GTK_PLOT_POLAR_DISP_PNT)==0) {/* straight lines - force this case for the time being */
          for (k=0; k<plt->ind->len; k++) {
            cairo_set_dash(cr, NULL, 0, 0);
            if (plot->kdata && k<(plot->kdata->len)) {
              fg=g_array_index(plot->kdata, guint, k);
              if (fg&GTK_PLOT_POLAR_KEY_CLR) continue;
              switch ((fg&GTK_PLOT_POLAR_KEY_DSH)>>GTK_PLOT_POLAR_KEY_DSO) {
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
            ft=fmod(k,plt->rd->len);
            vv=g_array_index(plt->rd, gdouble, ft);
            wv=g_array_index(plt->gr, gdouble, ft);
            xv=g_array_index(plt->bl, gdouble, ft);
            yv=g_array_index(plt->al, gdouble, ft);
            cairo_set_source_rgba(cr, vv, wv, xv, yv);
            ft=g_array_index(plt->ind, gint, k);
            if (ft>=plot->rdata->len) break;
            if (ft>=plot->thdata->len-plot->rdata->len) break;
            st=g_array_index(plt->stride, gint, k);
            lt=g_array_index(plt->sizes, gint, k)*st+ft;
            if (lt>plot->rdata->len) lt=plot->rdata->len;
            if (lt>plot->thdata->len-plot->rdata->len) lt=plot->thdata->len-plot->rdata->len;
            for (ssx=MY_2PI; ssx>-10.0; ssx-=MY_2PI) {
							r=g_array_index(plot->rdata, gdouble, ft)-priv->bounds.rmin;
							th=ssx+g_array_index(plot->thdata, gdouble, ft+plot->rdata->len);
							xt=0;
							if (th<priv->bounds.thmin) xt|=GTK_PLOT_POLAR_BORDERS_CW;
							else if (th>priv->bounds.thmax) xt|=GTK_PLOT_POLAR_BORDERS_CCW;
					        if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(r<0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(r>0.0))) xt|=GTK_PLOT_POLAR_BORDERS_IN;
					        else if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(r>priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(r<priv->bounds.rmax-priv->bounds.rmin))) xt|=GTK_PLOT_POLAR_BORDERS_OUT;
							if (xt==0) {
								drs=priv->wr+r*priv->s;
								x=priv->x0+drs*cos(th);
								y=priv->y0-drs*sin(th);
								cairo_arc(cr, x, y, (plot->ptsize), 0, MY_2PI);
								cairo_fill(cr);
								cairo_move_to(cr, x, y);
							}
							for (j=st+ft; j<lt; j+=st) {
								rn=g_array_index(plot->rdata, gdouble, j)-priv->bounds.rmin;
								tn=ssx+g_array_index(plot->thdata, gdouble, j+(plot->rdata->len));
					        	if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(rn<0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(rn>0.0))) {
									if (tn<priv->bounds.thmin) {
										if (xt==0) {
											drs=r-rn;
											if ((drs<DZE)&&(drs>NZE)) {
												x=priv->x0+priv->wr*cos(priv->bounds.thmin);
												y=priv->y0-priv->wr*sin(priv->bounds.thmin);
											} else {
												drs=((tn-th)*r/drs)+th;
												if (drs>=thn) { /* check wrapping */
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												} else {
													drs=(rn-r)*(th-priv->bounds.thmin)/(th-tn);
													drs=priv->wr+(drs+r)*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												}
											}
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
											drs=th-tn;
											if (drs>DZE) {
												drs=rn+(r-rn)*(priv->bounds.thmin-tn)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													csx=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(csx);
													y=priv->y0-drc*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmin);
														y=priv->y0-drs*sin(priv->bounds.thmin);
													} else {
														drs=tn-(th-tn)*rn/(r-rn);
														x=priv->x0+priv->wr*cos(drs);
														y=priv->y0-priv->wr*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										}
										xt=(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CW);
									} else if (tn>(priv->bounds.thmax)) {
										if (xt==0) {
											drs=r-rn;
											if ((drs<DZE)&&(drs>NZE)) {
												x=priv->x0+priv->wr*cos(priv->bounds.thmax);
												y=priv->y0-priv->wr*sin(priv->bounds.thmax);
											} else {
												drs=th+(tn-th)*r/drs;
												if (drs<=thx) {
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												} else {
													drs=(rn-r)*(priv->bounds.thmax-th)/(tn-th);
													drs=priv->wr+(drs+r)*(priv->s);
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												}
											}
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
											drs=tn-th;
											if (drs>DZE) {
												drs=r+(rn-r)*(priv->bounds.thmax-th)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													csx=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(csx);
													y=priv->y0-drc*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmax);
														y=priv->y0-drs*sin(priv->bounds.thmax);
													} else {
														drs=tn-(th-tn)*rn/(r-rn);
														x=priv->x0+priv->wr*cos(drs);
														y=priv->y0-priv->wr*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										}
										xt=(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CCW);
									} else {
										if (xt==0) {
											drs=th+(tn-th)*r/(r-rn);
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
											csx=(th-tn)/(r-rn);
											drs=th+csx*(priv->bounds.rmax-priv->bounds.rmin-r);
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
											cairo_move_to(cr, x, y);
											drs=tn-csx*rn;
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_CW) {
											drs=r+(priv->bounds.thmin-th)*(rn-r)/(tn-th);
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
												cairo_move_to(cr, x, y);
												drs=th-r*(tn-th)/(rn-r);
												x=priv->x0+priv->wr*cos(drs);
												y=priv->y0-priv->wr*sin(drs);
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										} else if (xt==(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CW)) {
											drs=tn-th;
											if (drs>DZE) {
												drs=r+(rn-r)*(priv->bounds.thmin-th)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													csx=tn-(th-tn)*rn/(r-rn);
													x=priv->x0+priv->wr*cos(csx);
													y=priv->y0-priv->wr*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmin);
														y=priv->y0-drs*sin(priv->bounds.thmin);
													} else {
														drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
														drc=priv->wr+dr1*(priv->s);
														x=priv->x0+drc*cos(drs);
														y=priv->y0-drc*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										} else if (xt==GTK_PLOT_POLAR_BORDERS_CCW) {
											drs=r+(priv->bounds.thmax-th)*(rn-r)/(tn-th);
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
												cairo_move_to(cr, x, y);
												drs=th-r*(tn-th)/(rn-r);
												x=priv->x0+priv->wr*cos(drs);
												y=priv->y0-priv->wr*sin(drs);
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										} else if (xt==(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CCW)) {
											drs=th-tn;
											if (drs>DZE) {
												drs=rn+(r-rn)*(priv->bounds.thmax-tn)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													csx=tn-(th-tn)*rn/(r-rn);
													x=priv->x0+priv->wr*cos(csx);
													y=priv->y0-priv->wr*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmax);
														y=priv->y0-drs*sin(priv->bounds.thmax);
													} else {
														drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
														drc=priv->wr+dr1*priv->s;
														x=priv->x0+drc*cos(drs);
														y=priv->y0-drc*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										}
										xt=GTK_PLOT_POLAR_BORDERS_IN;
									}
								} else if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(rn>priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(rn<priv->bounds.rmax-priv->bounds.rmin))) {
									if (tn<priv->bounds.thmin) {
										if (xt==0) {
											drs=rn-r;
											if ((drs<DZE)&&(drs>NZE)) {
												drs=priv->wr+dr1*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
											} else {
												drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/drs;
												if (drs>=thn) {
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												} else {
													drs=r+(rn-r)*(th-priv->bounds.thmin)/(th-tn);
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												}
											}
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
											drs=th-tn;
											if (drs>DZE) {
												drs=rn+(r-rn)*(priv->bounds.thmin-tn)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													csx=th-(tn-th)*r/(rn-r);
													x=priv->x0+priv->wr*cos(csx);
													y=priv->y0-priv->wr*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmin);
														y=priv->y0-drs*sin(priv->bounds.thmin);
													} else {
														drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
														drc=priv->wr+dr1*priv->s;
														x=priv->x0+drc*cos(drs);
														y=priv->y0-drc*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										}
										xt=(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CW);
									} else if (tn>(priv->bounds.thmax)) {
										if (xt==0) {
											drs=rn-r;
											if ((drs<DZE)&&(drs>NZE)) {
												drs=priv->wr+dr1*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
											} else {
												drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/drs;
												if (drs<=thx) {
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												} else {
													drs=r+(rn-r)*(priv->bounds.thmax-th)/(tn-th);
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												}
											}
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
											drs=tn-th;
											if (drs>DZE) {
												drs=r+(rn-r)*(priv->bounds.thmax-th)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													csx=th-(tn-th)*r/(rn-r);
													x=priv->x0+priv->wr*cos(csx);
													y=priv->y0-priv->wr*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmax);
														y=priv->y0-drs*sin(priv->bounds.thmax);
													} else {
														drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
														drc=priv->wr+dr1*priv->s;
														x=priv->x0+drc*cos(drs);
														y=priv->y0-drc*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										}
										xt=(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CCW);
									} else {
										if (xt==0) {
											drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
											csx=(tn-th)/(rn-r);
											drs=tn+csx*(priv->bounds.rmax-priv->bounds.rmin-rn);
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
											cairo_move_to(cr, x, y);
											drs=th-csx*r;
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_CW) {
											drs=r+(priv->bounds.thmin-th)*(rn-r)/(tn-th);
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<dr1))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>dr1))) {
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
												cairo_move_to(cr, x, y);
												drs=th+(priv->bounds.rmax-priv->bounds.rmin-r)*(tn-th)/(rn-r);
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(drs);
												y=priv->y0-drc*sin(drs);
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										} else if (xt==(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CW)) {
											drs=tn-th;
											if (drs>DZE) {
												drs=r+(rn-r)*(priv->bounds.thmin-th)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													csx=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(csx);
													y=priv->y0-drc*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmin);
														y=priv->y0-drs*sin(priv->bounds.thmin);
													} else {
														drs=th-(tn-th)*r/(rn-r);
														x=priv->x0+priv->wr*cos(drs);
														y=priv->y0-priv->wr*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										} else if (xt==GTK_PLOT_POLAR_BORDERS_CCW) {
											drs=r+(priv->bounds.thmax-th)*(rn-r)/(tn-th);
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<dr1))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>dr1))) {
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
												cairo_move_to(cr, x, y);
												drs=th+(priv->bounds.rmax-priv->bounds.rmin-r)*(tn-th)/(rn-r);
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(drs);
												y=priv->y0-drc*sin(drs);
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										} else if (xt==(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CCW)) {
											drs=th-tn;
											if (drs>DZE) {
												drs=rn+(r-rn)*(priv->bounds.thmax-tn)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													csx=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(csx);
													y=priv->y0-drc*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmax);
														y=priv->y0-drs*sin(priv->bounds.thmax);
													} else {
														drs=th-(tn-th)*r/(rn-r);
														x=priv->x0+priv->wr*cos(drs);
														y=priv->y0-priv->wr*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										}
										xt=GTK_PLOT_POLAR_BORDERS_OUT;
									}
								} else if (tn<priv->bounds.thmin) {
									if (xt==0) {
										drs=r+(rn-r)*(th-priv->bounds.thmin)/(th-tn);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmin);
										y=priv->y0-drs*sin(priv->bounds.thmin);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
										drs=tn-rn*(tn-th)/(rn-r);
										if (drs>priv->bounds.thmin) {
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
											cairo_move_to(cr, x, y);
											drs=rn+(priv->bounds.thmin-tn)*(rn-r)/(tn-th);
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmin);
											y=priv->y0-drs*sin(priv->bounds.thmin);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
										drs=tn+(priv->bounds.rmax-priv->bounds.rmin-rn)*(tn-th)/(rn-r);
										if (drs>priv->bounds.thmin) {
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
											cairo_move_to(cr, x, y);
											drs=rn+(priv->bounds.thmin-tn)*(rn-r)/(tn-th);
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmin);
											y=priv->y0-drs*sin(priv->bounds.thmin);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									}
									xt=GTK_PLOT_POLAR_BORDERS_CW;
								} else if (tn>priv->bounds.thmax) {
									if (xt==0) {
										drs=r+(rn-r)*(priv->bounds.thmax-th)/(tn-th);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmax);
										y=priv->y0-drs*sin(priv->bounds.thmax);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
										drs=tn-rn*(tn-th)/(rn-r);
										if (drs<priv->bounds.thmax) {
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
											cairo_move_to(cr, x, y);
											drs=rn+(priv->bounds.thmax-tn)*(rn-r)/(tn-th);
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmax);
											y=priv->y0-drs*sin(priv->bounds.thmax);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
										drs=tn+(priv->bounds.rmax-priv->bounds.rmin-rn)*(tn-th)/(rn-r);
										if (drs<priv->bounds.thmax) {
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
											cairo_move_to(cr, x, y);
											drs=rn+(priv->bounds.thmax-tn)*(rn-r)/(tn-th);
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmax);
											y=priv->y0-drs*sin(priv->bounds.thmax);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									}
									xt=GTK_PLOT_POLAR_BORDERS_CCW;
								} else { /* within range */
									if ((xt&GTK_PLOT_POLAR_BORDERS_IN)!=0) {
										drs=rn-r;
										if ((xt&GTK_PLOT_POLAR_BORDERS_CW)!=0) {
											if ((drs<DZE)&&(drs>NZE)) {
												x=priv->x0+priv->wr*cos(priv->bounds.thmin);
												y=priv->y0-priv->wr*sin(priv->bounds.thmin);
											} else {
												drs=tn+(th-tn)*rn/drs;
												if (drs>=thn) { /* check wrapping */
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												} else {
													drs=rn+(r-rn)*(tn-priv->bounds.thmin)/(tn-th);
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												}
											}
										} else if ((xt&GTK_PLOT_POLAR_BORDERS_CCW)!=0) {
											if ((drs<DZE)&&(drs>NZE)) {
												x=priv->x0+priv->wr*cos(priv->bounds.thmax);
												y=priv->y0-priv->wr*sin(priv->bounds.thmax);
											} else {
												drs=tn+(th-tn)*rn/drs;
												if (drs<=thx) {
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												} else {
													drs=rn+(r-rn)*(priv->bounds.thmax-tn)/(th-tn);
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												}
											}
										} else {
											drs=tn+(th-tn)*rn/drs;
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
										}
										cairo_move_to(cr, x, y);
									} else if ((xt&GTK_PLOT_POLAR_BORDERS_OUT)!=0) {
										drs=r-rn;
										if ((xt&GTK_PLOT_POLAR_BORDERS_CW)!=0) {
											if ((drs<DZE)&&(drs>NZE)) {
												drs=priv->wr+dr1*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
											} else {
												drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/drs;
												if (drs>=thn) {
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												} else {
													drs=rn+(r-rn)*(tn-priv->bounds.thmin)/(tn-th);
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												}
											}
										} else if ((xt&GTK_PLOT_POLAR_BORDERS_CCW)!=0) {
											if ((drs<DZE)&&(drs>NZE)) {
												drs=priv->wr+dr1*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
											} else {
												drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/drs;
												if (drs<=thx) {
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												} else {
													drs=rn+(r-rn)*(priv->bounds.thmax-tn)/(th-tn);
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												}
											}
										} else {
											drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/drs;
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
										}
										cairo_move_to(cr, x, y);
									} else if ((xt&GTK_PLOT_POLAR_BORDERS_CW)!=0) {
										drs=rn+(r-rn)*(tn-priv->bounds.thmin)/(tn-th);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmin);
										y=priv->y0-drs*sin(priv->bounds.thmin);
										cairo_move_to(cr, x, y);
									} else if ((xt&GTK_PLOT_POLAR_BORDERS_CCW)!=0) {
										drs=rn+(r-rn)*(priv->bounds.thmax-tn)/(th-tn);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmax);
										y=priv->y0-drs*sin(priv->bounds.thmax);
										cairo_move_to(cr, x, y);
									}
									drs=priv->wr+rn*priv->s;
									x=priv->x0+drs*cos(tn);
									y=priv->y0-drs*sin(tn);
									cairo_line_to(cr, x, y);
									cairo_stroke(cr);
									cairo_arc(cr, x, y, plot->ptsize, 0, MY_2PI);
									cairo_fill(cr);
									cairo_move_to(cr, x, y);
									xt=0;
								}
								r=rn;
								th=tn;
							}
            }
          }
//        } else {/* spline interpolation to polar transform - placeholder */
        }	
      } else if (TRUE||((plot->flagd)&GTK_PLOT_POLAR_DISP_PNT)==0) { /* straight lines only - force */
        for (k=0; k<(plt->ind->len); k++) {
          cairo_set_dash(cr, NULL, 0, 0);
          if (plot->kdata && k<(plot->kdata->len)) {
            fg=g_array_index(plot->kdata, guint, k);
            if (fg&GTK_PLOT_POLAR_KEY_CLR) continue;
            switch ((fg&GTK_PLOT_POLAR_KEY_DSH)>>GTK_PLOT_POLAR_KEY_DSO) {
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
          ft=fmod(k,plt->rd->len);
          vv=g_array_index(plt->rd, gdouble, ft);
          wv=g_array_index(plt->gr, gdouble, ft);
          xv=g_array_index(plt->bl, gdouble, ft);
          yv=g_array_index(plt->al, gdouble, ft);
          cairo_set_source_rgba(cr, vv, wv, xv, yv);
          ft=g_array_index(plt->ind, gint, k);
          if (ft>=(plot->rdata->len)) break;
          if (ft>=(plot->thdata->len)-(plot->rdata->len)) break;
          st=g_array_index(plt->stride, gint, k);
          lt=g_array_index(plt->sizes, gint, k)*st+ft;
          if (lt>(plot->rdata->len)) lt=plot->rdata->len;
          if (lt>(plot->thdata->len)-(plot->rdata->len)) lt=plot->thdata->len-plot->rdata->len;
          for (ssx=MY_2PI; ssx>-10; ssx-=MY_2PI) {
						r=g_array_index(plot->rdata, gdouble, ft)-priv->bounds.rmin;
						th=ssx+g_array_index(plot->thdata, gdouble, ft+plot->rdata->len);
						xt=0;
						if (th<priv->bounds.thmin) xt|=GTK_PLOT_POLAR_BORDERS_CW;
						else if (th>priv->bounds.thmax) xt|=GTK_PLOT_POLAR_BORDERS_CCW;
						if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(r<0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(r>0.0))) xt|=GTK_PLOT_POLAR_BORDERS_IN;
						else if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(r>priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(r<priv->bounds.rmax-priv->bounds.rmin))) xt|=GTK_PLOT_POLAR_BORDERS_OUT;
						if (xt==0) {
							drs=priv->wr+r*priv->s;
							x=priv->x0+drs*cos(th);
							y=priv->y0-drs*sin(th);
							cairo_move_to(cr, x, y);
						}
						for (j=st+ft; j<lt; j+=st) {
							rn=g_array_index(plot->rdata, gdouble, j)-priv->bounds.rmin;
							tn=ssx+g_array_index(plot->thdata, gdouble, j+plot->rdata->len);
					        if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(rn<0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(rn>0.0))) {
								if (tn<priv->bounds.thmin) {
									if (xt==0) {
										drs=r-rn;
										if ((drs<DZE)&&(drs>NZE)) {
											x=priv->x0+priv->wr*cos(priv->bounds.thmin);
											y=priv->y0-priv->wr*sin(priv->bounds.thmin);
										} else {
											drs=((tn-th)*r/drs)+th;
											if (drs>=thn) {/* check wrapping */
												x=priv->x0+priv->wr*cos(drs);
												y=priv->y0-priv->wr*sin(drs);
											} else {
												drs=(rn-r)*(th-priv->bounds.thmin)/(th-tn);
												drs=priv->wr+(drs+r)*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
											}
										}
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
										drs=th-tn;
										if (drs>DZE) {
											drs=rn+(r-rn)*(priv->bounds.thmin-tn)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
												csx=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(csx);
												y=priv->y0-drc*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												} else {
													drs=tn-(th-tn)*rn/(r-rn);
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									}
									xt=(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CW);
								} else if (tn>(priv->bounds.thmax)) {
									if (xt==0) {
										drs=r-rn;
										if ((drs<DZE)&&(drs>NZE)) {
											x=priv->x0+priv->wr*cos(priv->bounds.thmax);
											y=priv->y0-priv->wr*sin(priv->bounds.thmax);
										} else {
											drs=th+(tn-th)*r/drs;
											if (drs<=thx) {
												x=priv->x0+priv->wr*cos(drs);
												y=priv->y0-priv->wr*sin(drs);
											} else {
												drs=(rn-r)*(priv->bounds.thmax-th)/(tn-th);
												drs=priv->wr+(drs+r)*(priv->s);
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
											}
										}
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
										drs=tn-th;
										if (drs>DZE) {
											drs=r+(rn-r)*(priv->bounds.thmax-th)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
												csx=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(csx);
												y=priv->y0-drc*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												} else {
													drs=tn-(th-tn)*rn/(r-rn);
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									}
									xt=(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CCW);
								} else {
									if (xt==0) {
										drs=th+(tn-th)*r/(r-rn);
										x=priv->x0+priv->wr*cos(drs);
										y=priv->y0-priv->wr*sin(drs);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
										csx=(th-tn)/(r-rn);
										drs=th+csx*(priv->bounds.rmax-priv->bounds.rmin-r);
										drc=priv->wr+dr1*priv->s;
										x=priv->x0+drc*cos(drs);
										y=priv->y0-drc*sin(drs);
										cairo_move_to(cr, x, y);
										drs=tn-csx*rn;
										x=priv->x0+priv->wr*cos(drs);
										y=priv->y0-priv->wr*sin(drs);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_CW) {
										drs=r+(priv->bounds.thmin-th)*(rn-r)/(tn-th);
										if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmin);
											y=priv->y0-drs*sin(priv->bounds.thmin);
											cairo_move_to(cr, x, y);
											drs=th-r*(tn-th)/(rn-r);
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									} else if (xt==(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CW)) {
										drs=tn-th;
										if (drs>DZE) {
											drs=r+(rn-r)*(priv->bounds.thmin-th)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
												csx=tn-(th-tn)*rn/(r-rn);
												x=priv->x0+priv->wr*cos(csx);
												y=priv->y0-priv->wr*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												} else {
													drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
													drc=priv->wr+dr1*(priv->s);
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									} else if (xt==GTK_PLOT_POLAR_BORDERS_CCW) {
										drs=r+(priv->bounds.thmax-th)*(rn-r)/(tn-th);
										if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmax);
											y=priv->y0-drs*sin(priv->bounds.thmax);
											cairo_move_to(cr, x, y);
											drs=th-r*(tn-th)/(rn-r);
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									} else if (xt==(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CCW)) {
										drs=th-tn;
										if (drs>DZE) {
											drs=rn+(r-rn)*(priv->bounds.thmax-tn)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
												csx=tn-(th-tn)*rn/(r-rn);
												x=priv->x0+priv->wr*cos(csx);
												y=priv->y0-priv->wr*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												} else {
													drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									}
									xt=GTK_PLOT_POLAR_BORDERS_IN;
								}
							} else if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(rn>priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(rn<priv->bounds.rmax-priv->bounds.rmin))) {
								if (tn<priv->bounds.thmin) {
									if (xt==0) {
										drs=rn-r;
										if ((drs<DZE)&&(drs>NZE)) {
											drs=priv->wr+dr1*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmin);
											y=priv->y0-drs*sin(priv->bounds.thmin);
										} else {
											drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/drs;
											if (drs>=thn) {
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(drs);
												y=priv->y0-drc*sin(drs);
											} else {
												drs=r+(rn-r)*(th-priv->bounds.thmin)/(th-tn);
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
											}
										}
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
										drs=th-tn;
										if (drs>DZE) {
											drs=rn+(r-rn)*(priv->bounds.thmin-tn)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
												csx=th-(tn-th)*r/(rn-r);
												x=priv->x0+priv->wr*cos(csx);
												y=priv->y0-priv->wr*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												} else {
													drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									}
									xt=(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CW);
								} else if (tn>(priv->bounds.thmax)) {
									if (xt==0) {
										drs=rn-r;
										if ((drs<DZE)&&(drs>NZE)) {
											drs=priv->wr+dr1*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmax);
											y=priv->y0-drs*sin(priv->bounds.thmax);
										} else {
											drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/drs;
											if (drs<=thx) {
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(drs);
												y=priv->y0-drc*sin(drs);
											} else {
												drs=r+(rn-r)*(priv->bounds.thmax-th)/(tn-th);
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
											}
										}
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
										drs=tn-th;
										if (drs>DZE) {
											drs=r+(rn-r)*(priv->bounds.thmax-th)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
												csx=th-(tn-th)*r/(rn-r);
												x=priv->x0+priv->wr*cos(csx);
												y=priv->y0-priv->wr*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												} else {
													drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									}
									xt=(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CCW);
								} else {
									if (xt==0) {
										drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
										drc=priv->wr+dr1*priv->s;
										x=priv->x0+drc*cos(drs);
										y=priv->y0-drc*sin(drs);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
										csx=(tn-th)/(rn-r);
										drs=tn+csx*(priv->bounds.rmax-priv->bounds.rmin-rn);
										drc=priv->wr+dr1*priv->s;
										x=priv->x0+drc*cos(drs);
										y=priv->y0-drc*sin(drs);
										cairo_move_to(cr, x, y);
										drs=th-csx*r;
										x=priv->x0+priv->wr*cos(drs);
										y=priv->y0-priv->wr*sin(drs);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_CW) {
										drs=r+(priv->bounds.thmin-th)*(rn-r)/(tn-th);
										if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<dr1))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>dr1))) {
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmin);
											y=priv->y0-drs*sin(priv->bounds.thmin);
											cairo_move_to(cr, x, y);
											drs=th+(priv->bounds.rmax-priv->bounds.rmin-r)*(tn-th)/(rn-r);
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									} else if (xt==(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CW)) {
										drs=tn-th;
										if (drs>DZE) {
											drs=r+(rn-r)*(priv->bounds.thmin-th)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
												csx=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(csx);
												y=priv->y0-drc*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												} else {
													drs=th-(tn-th)*r/(rn-r);
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									} else if (xt==GTK_PLOT_POLAR_BORDERS_CCW) {
										drs=r+(priv->bounds.thmax-th)*(rn-r)/(tn-th);
										if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<dr1))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>dr1))) {
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmax);
											y=priv->y0-drs*sin(priv->bounds.thmax);
											cairo_move_to(cr, x, y);
											drs=th+(priv->bounds.rmax-priv->bounds.rmin-r)*(tn-th)/(rn-r);
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									} else if (xt==(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CCW)) {
										drs=th-tn;
										if (drs>DZE) {
											drs=rn+(r-rn)*(priv->bounds.thmax-tn)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
												csx=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(csx);
												y=priv->y0-drc*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												} else {
													drs=th-(tn-th)*r/(rn-r);
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									}
									xt=GTK_PLOT_POLAR_BORDERS_OUT;
								}
							} else if (tn<priv->bounds.thmin) {
								if (xt==0) {
									drs=r+(rn-r)*(th-priv->bounds.thmin)/(th-tn);
									drs=priv->wr+drs*priv->s;
									x=priv->x0+drs*cos(priv->bounds.thmin);
									y=priv->y0-drs*sin(priv->bounds.thmin);
									cairo_line_to(cr, x, y);
									cairo_stroke(cr);
								} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
									drs=tn-rn*(tn-th)/(rn-r);
									if (drs>priv->bounds.thmin) {
										x=priv->x0+priv->wr*cos(drs);
										y=priv->y0-priv->wr*sin(drs);
										cairo_move_to(cr, x, y);
										drs=rn+(priv->bounds.thmin-tn)*(rn-r)/(tn-th);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmin);
										y=priv->y0-drs*sin(priv->bounds.thmin);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									}
								} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
									drs=tn+(priv->bounds.rmax-priv->bounds.rmin-rn)*(tn-th)/(rn-r);
									if (drs>priv->bounds.thmin) {
										drc=priv->wr+dr1*priv->s;
										x=priv->x0+drc*cos(drs);
										y=priv->y0-drc*sin(drs);
										cairo_move_to(cr, x, y);
										drs=rn+(priv->bounds.thmin-tn)*(rn-r)/(tn-th);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmin);
										y=priv->y0-drs*sin(priv->bounds.thmin);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									}
								}
								xt=GTK_PLOT_POLAR_BORDERS_CW;
							} else if (tn>priv->bounds.thmax) {
								if (xt==0) {
									drs=r+(rn-r)*(priv->bounds.thmax-th)/(tn-th);
									drs=priv->wr+drs*priv->s;
									x=priv->x0+drs*cos(priv->bounds.thmax);
									y=priv->y0-drs*sin(priv->bounds.thmax);
									cairo_line_to(cr, x, y);
									cairo_stroke(cr);
								} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
									drs=tn-rn*(tn-th)/(rn-r);
									if (drs<priv->bounds.thmax) {
										x=priv->x0+priv->wr*cos(drs);
										y=priv->y0-priv->wr*sin(drs);
										cairo_move_to(cr, x, y);
										drs=rn+(priv->bounds.thmax-tn)*(rn-r)/(tn-th);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmax);
										y=priv->y0-drs*sin(priv->bounds.thmax);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									}
								} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
									drs=tn+(priv->bounds.rmax-priv->bounds.rmin-rn)*(tn-th)/(rn-r);
									if (drs<priv->bounds.thmax) {
										drc=priv->wr+dr1*priv->s;
										x=priv->x0+drc*cos(drs);
										y=priv->y0-drc*sin(drs);
										cairo_move_to(cr, x, y);
										drs=rn+(priv->bounds.thmax-tn)*(rn-r)/(tn-th);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmax);
										y=priv->y0-drs*sin(priv->bounds.thmax);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									}
								}
								xt=GTK_PLOT_POLAR_BORDERS_CCW;
							} else { /* within range */
								if ((xt&GTK_PLOT_POLAR_BORDERS_IN)!=0) {
									drs=rn-r;
									if ((xt&GTK_PLOT_POLAR_BORDERS_CW)!=0) {
										if ((drs<DZE)&&(drs>NZE)) {
											x=priv->x0+priv->wr*cos(priv->bounds.thmin);
											y=priv->y0-priv->wr*sin(priv->bounds.thmin);
										} else {
											drs=tn+(th-tn)*rn/drs;
											if (drs>=thn) { /* check wrapping */
												x=priv->x0+priv->wr*cos(drs);
												y=priv->y0-priv->wr*sin(drs);
											} else {
												drs=rn+(r-rn)*(tn-priv->bounds.thmin)/(tn-th);
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
											}
										}
									} else if ((xt&GTK_PLOT_POLAR_BORDERS_CCW)!=0) {
										if ((drs<DZE)&&(drs>NZE)) {
											x=priv->x0+priv->wr*cos(priv->bounds.thmax);
											y=priv->y0-priv->wr*sin(priv->bounds.thmax);
										} else {
											drs=tn+(th-tn)*rn/drs;
											if (drs<=thx) {
												x=priv->x0+priv->wr*cos(drs);
												y=priv->y0-priv->wr*sin(drs);
											} else {
												drs=rn+(r-rn)*(priv->bounds.thmax-tn)/(th-tn);
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
											}
										}
									} else {
										drs=tn+(th-tn)*rn/drs;
										x=priv->x0+priv->wr*cos(drs);
										y=priv->y0-priv->wr*sin(drs);
									}
									cairo_move_to(cr, x, y);
								} else if ((xt&GTK_PLOT_POLAR_BORDERS_OUT)!=0) {
									drs=r-rn;
									if ((xt&GTK_PLOT_POLAR_BORDERS_CW)!=0) {
										if ((drs<DZE)&&(drs>NZE)) {
											drs=priv->wr+dr1*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmin);
											y=priv->y0-drs*sin(priv->bounds.thmin);
										} else {
											drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/drs;
											if (drs>=thn) {
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(drs);
												y=priv->y0-drc*sin(drs);
											} else {
												drs=rn+(r-rn)*(tn-priv->bounds.thmin)/(tn-th);
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
											}
										}
									} else if ((xt&GTK_PLOT_POLAR_BORDERS_CCW)!=0) {
										if ((drs<DZE)&&(drs>NZE)) {
											drs=priv->wr+dr1*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmax);
											y=priv->y0-drs*sin(priv->bounds.thmax);
										} else {
											drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/drs;
											if (drs<=thx) {
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(drs);
												y=priv->y0-drc*sin(drs);
											} else {
												drs=rn+(r-rn)*(priv->bounds.thmax-tn)/(th-tn);
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
											}
										}
									} else {
										drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/drs;
										drc=priv->wr+dr1*priv->s;
										x=priv->x0+drc*cos(drs);
										y=priv->y0-drc*sin(drs);
									}
									cairo_move_to(cr, x, y);
								} else if ((xt&GTK_PLOT_POLAR_BORDERS_CW)!=0) {
									drs=rn+(r-rn)*(tn-priv->bounds.thmin)/(tn-th);
									drs=priv->wr+drs*priv->s;
									x=priv->x0+drs*cos(priv->bounds.thmin);
									y=priv->y0-drs*sin(priv->bounds.thmin);
									cairo_move_to(cr, x, y);
								} else if ((xt&GTK_PLOT_POLAR_BORDERS_CCW)!=0) {
									drs=rn+(r-rn)*(priv->bounds.thmax-tn)/(th-tn);
									drs=priv->wr+drs*priv->s;
									x=priv->x0+drs*cos(priv->bounds.thmax);
									y=priv->y0-drs*sin(priv->bounds.thmax);
									cairo_move_to(cr, x, y);
								}
								drs=priv->wr+rn*priv->s;
								x=priv->x0+drs*cos(tn);
								y=priv->y0-drs*sin(tn);
								cairo_line_to(cr, x, y);
								xt=0;
							}
							r=rn;
							th=tn;
						}
            cairo_stroke(cr);
          }
        }
//			} else { /* spline interpolation to polar transform - placeholder */
      }
    } else if ((plot->flagd&GTK_PLOT_POLAR_DISP_PTS)!=0) {/* points only */
      for (k=0; k<(plt->ind->len); k++) {
        if (plot->kdata && k<(plot->kdata->len) && (g_array_index(plot->kdata, guint, k)&GTK_PLOT_POLAR_KEY_CLR)) continue;
        ft=fmod(k,(plt->rd->len));
        vv=g_array_index(plt->rd, gdouble, ft);
        wv=g_array_index(plt->gr, gdouble, ft);
        xv=g_array_index(plt->bl, gdouble, ft);
        yv=g_array_index(plt->al, gdouble, ft);
        cairo_set_source_rgba(cr, vv, wv, xv, yv);
        ft=g_array_index(plt->ind, gint, k);
        if (ft>=(plot->rdata->len)) break;
        if (ft>=(plot->thdata->len)-(plot->rdata->len)) break;
        st=g_array_index(plt->stride, gint, k);
        lt=g_array_index(plt->sizes, gint, k)*st+ft;
        if (lt>(plot->rdata->len)) lt=plot->rdata->len;
        if (lt>(plot->thdata->len)-(plot->rdata->len)) lt=plot->thdata->len-plot->rdata->len;
        for (j=ft+st; j<lt; j+=st) {
          r=g_array_index(plot->rdata, gdouble, j)-priv->bounds.rmin;
          th=g_array_index(plot->thdata, gdouble, j+(plot->rdata->len));
          if ((th<priv->bounds.thmin-MY_2PI)||(th>priv->bounds.thmax+MY_2PI)||((th<priv->bounds.thmin)&&(th>priv->bounds.thmax-MY_2PI))||((th>priv->bounds.thmax)&&(th<priv->bounds.thmin+MY_2PI))) continue;
          if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&((r>priv->bounds.rmax-priv->bounds.rmin)||(r<0.0)))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&((r<priv->bounds.rmax-priv->bounds.rmin)||(r>0.0)))) continue;
          drs=priv->wr+r*priv->s;
          x=priv->x0+drs*cos(th);
          y=priv->y0-drs*sin(th);
          cairo_move_to(cr, x, y);
          cairo_arc(cr, x, y, plot->ptsize, 0, MY_2PI);
          cairo_fill(cr);
        }
      }
    }
  } else {/* {take thdata from first set */
    if (((plot->flagd)&GTK_PLOT_POLAR_DISP_LIN)!=0) {
      cairo_set_line_width(cr, plot->linew);
      if (((plot->flagd)&GTK_PLOT_POLAR_DISP_PTS)!=0) { /* lines and points */
        if (((plot->flagd)&GTK_PLOT_POLAR_DISP_PNT)==0) { /* straight lines */
          for (k=0; k<(plt->ind->len); k++) {
            cairo_set_dash(cr, NULL, 0, 0);
            if (plot->kdata && k<(plot->kdata->len)) {
              fg=g_array_index(plot->kdata, guint, k);
              if (fg&GTK_PLOT_POLAR_KEY_CLR) continue;
              switch ((fg&GTK_PLOT_POLAR_KEY_DSH)>>GTK_PLOT_POLAR_KEY_DSO) {
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
            ft=fmod(k,plt->rd->len);
            vv=g_array_index(plt->rd, gdouble, ft);
            wv=g_array_index(plt->gr, gdouble, ft);
            xv=g_array_index(plt->bl, gdouble, ft);
            yv=g_array_index(plt->al, gdouble, ft);
            cairo_set_source_rgba(cr, vv, wv, xv, yv);
            ft=g_array_index(plt->ind, gint, k);
            if (ft>=(plot->rdata->len)) break;
            st=g_array_index(plt->stride, gint, k);
            lt=g_array_index(plt->sizes, gint, k)*st+ft;
            if (lt>(plot->rdata->len)) lt=plot->rdata->len;
            for (ssx=MY_2PI; ssx>-10; ssx-=MY_2PI) {
							r=g_array_index(plot->rdata, gdouble, ft)-priv->bounds.rmin;
							th=ssx+g_array_index(plot->thdata, gdouble, ft);
							xt=0;
							if (th<priv->bounds.thmin) xt|=GTK_PLOT_POLAR_BORDERS_CW;
							else if (th>priv->bounds.thmax) xt|=GTK_PLOT_POLAR_BORDERS_CCW;
							if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(r<0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(r>0.0))) xt|=GTK_PLOT_POLAR_BORDERS_IN;
							else if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(r>priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(r<priv->bounds.rmax-priv->bounds.rmin))) xt|=GTK_PLOT_POLAR_BORDERS_OUT;
							if (xt==0) {
								drs=priv->wr+r*priv->s;
								x=priv->x0+drs*cos(th);
								y=priv->y0-drs*sin(th);
								cairo_arc(cr, x, y, plot->ptsize, 0, MY_2PI);
								cairo_fill(cr);
								cairo_move_to(cr, x, y);
							}
							for (j=st+ft; j<lt; j+=st) {
								rn=g_array_index(plot->rdata, gdouble, j)-priv->bounds.rmin;
								tn=ssx+g_array_index(plot->thdata, gdouble, j);
								if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(rn<0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(rn>0.0))) {
									if (tn<priv->bounds.thmin) {
										if (xt==0) {
											drs=r-rn;
											if ((drs<DZE)&&(drs>NZE)) {
												x=priv->x0+priv->wr*cos(priv->bounds.thmin);
												y=priv->y0-priv->wr*sin(priv->bounds.thmin);
											} else {
												drs=((tn-th)*r/drs)+th;
												if (drs>=thn) {/* check wrapping */
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												} else {
													drs=(rn-r)*(th-priv->bounds.thmin)/(th-tn);
													drs=priv->wr+(drs+r)*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												}
											}
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
											drs=th-tn;
											if (drs>DZE) {
												drs=rn+(r-rn)*(priv->bounds.thmin-tn)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													csx=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(csx);
													y=priv->y0-drc*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmin);
														y=priv->y0-drs*sin(priv->bounds.thmin);
													} else {
														drs=tn-(th-tn)*rn/(r-rn);
														x=priv->x0+priv->wr*cos(drs);
														y=priv->y0-priv->wr*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										}
										xt=(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CW);
									} else if (tn>(priv->bounds.thmax)) {
										if (xt==0) {
											drs=r-rn;
											if ((drs<DZE)&&(drs>NZE)) {
												x=priv->x0+priv->wr*cos(priv->bounds.thmax);
												y=priv->y0-priv->wr*sin(priv->bounds.thmax);
											} else {
												drs=th+(tn-th)*r/drs;
												if (drs<=thx) {
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												} else {
													drs=(rn-r)*(priv->bounds.thmax-th)/(tn-th);
													drs=priv->wr+(drs+r)*(priv->s);
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												}
											}
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
											drs=tn-th;
											if (drs>DZE) {
												drs=r+(rn-r)*(priv->bounds.thmax-th)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													csx=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(csx);
													y=priv->y0-drc*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmax);
														y=priv->y0-drs*sin(priv->bounds.thmax);
													} else {
														drs=tn-(th-tn)*rn/(r-rn);
														x=priv->x0+priv->wr*cos(drs);
														y=priv->y0-priv->wr*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										}
										xt=(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CCW);
									} else {
										if (xt==0) {
											drs=th+(tn-th)*r/(r-rn);
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
											csx=(th-tn)/(r-rn);
											drs=th+csx*(priv->bounds.rmax-priv->bounds.rmin-r);
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
											cairo_move_to(cr, x, y);
											drs=tn-csx*rn;
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_CW) {
											drs=r+(priv->bounds.thmin-th)*(rn-r)/(tn-th);
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
												cairo_move_to(cr, x, y);
												drs=th-r*(tn-th)/(rn-r);
												x=priv->x0+priv->wr*cos(drs);
												y=priv->y0-priv->wr*sin(drs);
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										} else if (xt==(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CW)) {
											drs=tn-th;
											if (drs>DZE) {
												drs=r+(rn-r)*(priv->bounds.thmin-th)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													csx=tn-(th-tn)*rn/(r-rn);
													x=priv->x0+priv->wr*cos(csx);
													y=priv->y0-priv->wr*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmin);
														y=priv->y0-drs*sin(priv->bounds.thmin);
													} else {
														drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
														drc=priv->wr+dr1*(priv->s);
														x=priv->x0+drc*cos(drs);
														y=priv->y0-drc*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										} else if (xt==GTK_PLOT_POLAR_BORDERS_CCW) {
											drs=r+(priv->bounds.thmax-th)*(rn-r)/(tn-th);
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
												cairo_move_to(cr, x, y);
												drs=th-r*(tn-th)/(rn-r);
												x=priv->x0+priv->wr*cos(drs);
												y=priv->y0-priv->wr*sin(drs);
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										} else if (xt==(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CCW)) {
											drs=th-tn;
											if (drs>DZE) {
												drs=rn+(r-rn)*(priv->bounds.thmax-tn)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													csx=tn-(th-tn)*rn/(r-rn);
													x=priv->x0+priv->wr*cos(csx);
													y=priv->y0-priv->wr*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmax);
														y=priv->y0-drs*sin(priv->bounds.thmax);
													} else {
														drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
														drc=priv->wr+dr1*priv->s;
														x=priv->x0+drc*cos(drs);
														y=priv->y0-drc*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										}
										xt=GTK_PLOT_POLAR_BORDERS_IN;
									}
								} else if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(rn>priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(rn<priv->bounds.rmax-priv->bounds.rmin))) {
									if (tn<priv->bounds.thmin) {
										if (xt==0) {
											drs=rn-r;
											if ((drs<DZE)&&(drs>NZE)) {
												drs=priv->wr+dr1*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
											} else {
												drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/drs;
												if (drs>=thn) {
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												} else {
													drs=r+(rn-r)*(th-priv->bounds.thmin)/(th-tn);
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												}
											}
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
											drs=th-tn;
											if (drs>DZE) {
												drs=rn+(r-rn)*(priv->bounds.thmin-tn)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													csx=th-(tn-th)*r/(rn-r);
													x=priv->x0+priv->wr*cos(csx);
													y=priv->y0-priv->wr*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmin);
														y=priv->y0-drs*sin(priv->bounds.thmin);
													} else {
														drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
														drc=priv->wr+dr1*priv->s;
														x=priv->x0+drc*cos(drs);
														y=priv->y0-drc*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										}
										xt=(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CW);
									} else if (tn>(priv->bounds.thmax)) {
										if (xt==0) {
											drs=rn-r;
											if ((drs<DZE)&&(drs>NZE)) {
												drs=priv->wr+dr1*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
											} else {
												drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/drs;
												if (drs<=thx) {
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												} else {
													drs=r+(rn-r)*(priv->bounds.thmax-th)/(tn-th);
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												}
											}
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
											drs=tn-th;
											if (drs>DZE) {
												drs=r+(rn-r)*(priv->bounds.thmax-th)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													csx=th-(tn-th)*r/(rn-r);
													x=priv->x0+priv->wr*cos(csx);
													y=priv->y0-priv->wr*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmax);
														y=priv->y0-drs*sin(priv->bounds.thmax);
													} else {
														drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
														drc=priv->wr+dr1*priv->s;
														x=priv->x0+drc*cos(drs);
														y=priv->y0-drc*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										}
										xt=(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CCW);
									} else {
										if (xt==0) {
											drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
											csx=(tn-th)/(rn-r);
											drs=tn+csx*(priv->bounds.rmax-priv->bounds.rmin-rn);
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
											cairo_move_to(cr, x, y);
											drs=th-csx*r;
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										} else if (xt==GTK_PLOT_POLAR_BORDERS_CW) {
											drs=r+(priv->bounds.thmin-th)*(rn-r)/(tn-th);
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<dr1))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>dr1))) {
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
												cairo_move_to(cr, x, y);
												drs=th+(priv->bounds.rmax-priv->bounds.rmin-r)*(tn-th)/(rn-r);
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(drs);
												y=priv->y0-drc*sin(drs);
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										} else if (xt==(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CW)) {
											drs=tn-th;
											if (drs>DZE) {
												drs=r+(rn-r)*(priv->bounds.thmin-th)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													csx=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(csx);
													y=priv->y0-drc*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmin);
														y=priv->y0-drs*sin(priv->bounds.thmin);
													} else {
														drs=th-(tn-th)*r/(rn-r);
														x=priv->x0+priv->wr*cos(drs);
														y=priv->y0-priv->wr*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										} else if (xt==GTK_PLOT_POLAR_BORDERS_CCW) {
											drs=r+(priv->bounds.thmax-th)*(rn-r)/(tn-th);
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<dr1))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>dr1))) {
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
												cairo_move_to(cr, x, y);
												drs=th+(priv->bounds.rmax-priv->bounds.rmin-r)*(tn-th)/(rn-r);
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(drs);
												y=priv->y0-drc*sin(drs);
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										} else if (xt==(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CCW)) {
											drs=th-tn;
											if (drs>DZE) {
												drs=rn+(r-rn)*(priv->bounds.thmax-tn)/drs;
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													csx=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(csx);
													y=priv->y0-drc*sin(csx);
													cairo_move_to(cr, x, y);
													if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
														drs=priv->wr+drs*priv->s;
														x=priv->x0+drs*cos(priv->bounds.thmax);
														y=priv->y0-drs*sin(priv->bounds.thmax);
													} else {
														drs=th-(tn-th)*r/(rn-r);
														x=priv->x0+priv->wr*cos(drs);
														y=priv->y0-priv->wr*sin(drs);
													}
													cairo_line_to(cr, x, y);
													cairo_stroke(cr);
												}
											}
										}
										xt=GTK_PLOT_POLAR_BORDERS_OUT;
									}
								} else if (tn<priv->bounds.thmin) {
									if (xt==0) {
										drs=r+(rn-r)*(th-priv->bounds.thmin)/(th-tn);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmin);
										y=priv->y0-drs*sin(priv->bounds.thmin);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
										drs=tn-rn*(tn-th)/(rn-r);
										if (drs>priv->bounds.thmin) {
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
											cairo_move_to(cr, x, y);
											drs=rn+(priv->bounds.thmin-tn)*(rn-r)/(tn-th);
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmin);
											y=priv->y0-drs*sin(priv->bounds.thmin);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
										drs=tn+(priv->bounds.rmax-priv->bounds.rmin-rn)*(tn-th)/(rn-r);
										if (drs>priv->bounds.thmin) {
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
											cairo_move_to(cr, x, y);
											drs=rn+(priv->bounds.thmin-tn)*(rn-r)/(tn-th);
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmin);
											y=priv->y0-drs*sin(priv->bounds.thmin);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									}
									xt=GTK_PLOT_POLAR_BORDERS_CW;
								} else if (tn>priv->bounds.thmax) {
									if (xt==0) {
										drs=r+(rn-r)*(priv->bounds.thmax-th)/(tn-th);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmax);
										y=priv->y0-drs*sin(priv->bounds.thmax);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
										drs=tn-rn*(tn-th)/(rn-r);
										if (drs<priv->bounds.thmax) {
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
											cairo_move_to(cr, x, y);
											drs=rn+(priv->bounds.thmax-tn)*(rn-r)/(tn-th);
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmax);
											y=priv->y0-drs*sin(priv->bounds.thmax);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
										drs=tn+(priv->bounds.rmax-priv->bounds.rmin-rn)*(tn-th)/(rn-r);
										if (drs<priv->bounds.thmax) {
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
											cairo_move_to(cr, x, y);
											drs=rn+(priv->bounds.thmax-tn)*(rn-r)/(tn-th);
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmax);
											y=priv->y0-drs*sin(priv->bounds.thmax);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									}
									xt=GTK_PLOT_POLAR_BORDERS_CCW;
								} else { /* within range */
									if ((xt&GTK_PLOT_POLAR_BORDERS_IN)!=0) {
										drs=rn-r;
										if ((xt&GTK_PLOT_POLAR_BORDERS_CW)!=0) {
											if ((drs<DZE)&&(drs>NZE)) {
												x=priv->x0+priv->wr*cos(priv->bounds.thmin);
												y=priv->y0-priv->wr*sin(priv->bounds.thmin);
											} else {
												drs=tn+(th-tn)*rn/drs;
												if (drs>=thn) { /* check wrapping */
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												} else {
													drs=rn+(r-rn)*(tn-priv->bounds.thmin)/(tn-th);
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												}
											}
										} else if ((xt&GTK_PLOT_POLAR_BORDERS_CCW)!=0) {
											if ((drs<DZE)&&(drs>NZE)) {
												x=priv->x0+priv->wr*cos(priv->bounds.thmax);
												y=priv->y0-priv->wr*sin(priv->bounds.thmax);
											} else {
												drs=tn+(th-tn)*rn/drs;
												if (drs<=thx) {
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												} else {
													drs=rn+(r-rn)*(priv->bounds.thmax-tn)/(th-tn);
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												}
											}
										} else {
											drs=tn+(th-tn)*rn/drs;
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
										}
										cairo_move_to(cr, x, y);
									} else if ((xt&GTK_PLOT_POLAR_BORDERS_OUT)!=0) {
										drs=r-rn;
										if ((xt&GTK_PLOT_POLAR_BORDERS_CW)!=0) {
											if ((drs<DZE)&&(drs>NZE)) {
												drs=priv->wr+dr1*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
											} else {
												drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/drs;
												if (drs>=thn) {
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												} else {
													drs=rn+(r-rn)*(tn-priv->bounds.thmin)/(tn-th);
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												}
											}
										} else if ((xt&GTK_PLOT_POLAR_BORDERS_CCW)!=0) {
											if ((drs<DZE)&&(drs>NZE)) {
												drs=priv->wr+dr1*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
											} else {
												drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/drs;
												if (drs<=thx) {
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												} else {
													drs=rn+(r-rn)*(priv->bounds.thmax-tn)/(th-tn);
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												}
											}
										} else {
											drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/drs;
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
										}
										cairo_move_to(cr, x, y);
									} else if ((xt&GTK_PLOT_POLAR_BORDERS_CW)!=0) {
										drs=rn+(r-rn)*(tn-priv->bounds.thmin)/(tn-th);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmin);
										y=priv->y0-drs*sin(priv->bounds.thmin);
										cairo_move_to(cr, x, y);
									} else if ((xt&GTK_PLOT_POLAR_BORDERS_CCW)!=0) {
										drs=rn+(r-rn)*(priv->bounds.thmax-tn)/(th-tn);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmax);
										y=priv->y0-drs*sin(priv->bounds.thmax);
										cairo_move_to(cr, x, y);
									}
									drs=priv->wr+rn*priv->s;
									x=priv->x0+drs*cos(tn);
									y=priv->y0-drs*sin(tn);
									cairo_line_to(cr, x, y);
									cairo_stroke(cr);
									cairo_arc(cr, x, y, plot->ptsize, 0, MY_2PI);
									cairo_fill(cr);
									cairo_move_to(cr, x, y);
									xt=0;
								}
								r=rn;
								th=tn;
							}
            }
          }
        } else { /* spline interpolation to polar transform - placeholder */
        }	
      } else if (TRUE||((plot->flagd)&GTK_PLOT_POLAR_DISP_PNT)==0) { /* straight lines only - force */
        for (k=0; k<(plt->ind->len); k++) {
          cairo_set_dash(cr, NULL, 0, 0);
          if (plot->kdata && k<(plot->kdata->len)) {
            fg=g_array_index(plot->kdata, guint, k);
            if (fg&GTK_PLOT_POLAR_KEY_CLR) continue;
            switch ((fg&GTK_PLOT_POLAR_KEY_DSH)>>GTK_PLOT_POLAR_KEY_DSO) {
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
          ft=fmod(k,plt->rd->len);
          vv=g_array_index(plt->rd, gdouble, ft);
          wv=g_array_index(plt->gr, gdouble, ft);
          xv=g_array_index(plt->bl, gdouble, ft);
          yv=g_array_index(plt->al, gdouble, ft);
          cairo_set_source_rgba(cr, vv, wv, xv, yv);
          ft=g_array_index(plt->ind, gint, k);
          if (ft>=(plot->rdata->len)) break;
          st=g_array_index(plt->stride, gint, k);
          lt=(g_array_index(plt->sizes, gint, k)*st)+ft;
          if (lt>(plot->rdata->len)) lt=plot->rdata->len;
          for (ssx=MY_2PI; ssx>-10; ssx-=MY_2PI) {
						r=g_array_index(plot->rdata, gdouble, ft)-priv->bounds.rmin;
						th=ssx+g_array_index(plot->thdata, gdouble, ft);
						xt=0;
						if (th<priv->bounds.thmin) xt|=GTK_PLOT_POLAR_BORDERS_CW;
						else if (th>priv->bounds.thmax) xt|=GTK_PLOT_POLAR_BORDERS_CCW;
						if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(r<0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(r>0.0))) xt|=GTK_PLOT_POLAR_BORDERS_IN;
						else if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(r>priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(r<priv->bounds.rmax-priv->bounds.rmin))) xt|=GTK_PLOT_POLAR_BORDERS_OUT;
						if (xt==0) {
							drs=priv->wr+r*priv->s;
							x=priv->x0+drs*cos(th);
							y=priv->y0-drs*sin(th);
							cairo_move_to(cr, x, y);
						}
						for (j=st+ft; j<lt; j+=st) {
							rn=g_array_index(plot->rdata, gdouble, j)-priv->bounds.rmin;
							tn=ssx+g_array_index(plot->thdata, gdouble, j);
					        if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(rn<0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(rn>0.0))) {
								if (tn<priv->bounds.thmin) {
									if (xt==0) {
										drs=r-rn;
										if ((drs<DZE)&&(drs>NZE)) {
											x=priv->x0+priv->wr*cos(priv->bounds.thmin);
											y=priv->y0-priv->wr*sin(priv->bounds.thmin);
										} else {
											drs=((tn-th)*r/drs)+th;
											if (drs>=thn) {/* check wrapping */
												x=priv->x0+priv->wr*cos(drs);
												y=priv->y0-priv->wr*sin(drs);
											} else {
												drs=(rn-r)*(th-priv->bounds.thmin)/(th-tn);
												drs=priv->wr+(drs+r)*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
											}
										}
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
										drs=th-tn;
										if (drs>DZE) {
											drs=rn+(r-rn)*(priv->bounds.thmin-tn)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
												csx=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(csx);
												y=priv->y0-drc*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												} else {
													drs=tn-(th-tn)*rn/(r-rn);
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									}
									xt=(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CW);
								} else if (tn>(priv->bounds.thmax)) {
									if (xt==0) {
										drs=r-rn;
										if ((drs<DZE)&&(drs>NZE)) {
											x=priv->x0+priv->wr*cos(priv->bounds.thmax);
											y=priv->y0-priv->wr*sin(priv->bounds.thmax);
										} else {
											drs=th+(tn-th)*r/drs;
											if (drs<=thx) {
												x=priv->x0+priv->wr*cos(drs);
												y=priv->y0-priv->wr*sin(drs);
											} else {
												drs=(rn-r)*(priv->bounds.thmax-th)/(tn-th);
												drs=priv->wr+(drs+r)*(priv->s);
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
											}
										}
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
										drs=tn-th;
										if (drs>DZE) {
											drs=r+(rn-r)*(priv->bounds.thmax-th)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
												csx=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(csx);
												y=priv->y0-drc*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												} else {
													drs=tn-(th-tn)*rn/(r-rn);
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									}
									xt=(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CCW);
								} else {
									if (xt==0) {
										drs=th+(tn-th)*r/(r-rn);
										x=priv->x0+priv->wr*cos(drs);
										y=priv->y0-priv->wr*sin(drs);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
										csx=(th-tn)/(r-rn);
										drs=th+csx*(priv->bounds.rmax-priv->bounds.rmin-r);
										drc=priv->wr+dr1*priv->s;
										x=priv->x0+drc*cos(drs);
										y=priv->y0-drc*sin(drs);
										cairo_move_to(cr, x, y);
										drs=tn-csx*rn;
										x=priv->x0+priv->wr*cos(drs);
										y=priv->y0-priv->wr*sin(drs);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_CW) {
										drs=r+(priv->bounds.thmin-th)*(rn-r)/(tn-th);
										if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmin);
											y=priv->y0-drs*sin(priv->bounds.thmin);
											cairo_move_to(cr, x, y);
											drs=th-r*(tn-th)/(rn-r);
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									} else if (xt==(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CW)) {
										drs=tn-th;
										if (drs>DZE) {
											drs=r+(rn-r)*(priv->bounds.thmin-th)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
												csx=tn-(th-tn)*rn/(r-rn);
												x=priv->x0+priv->wr*cos(csx);
												y=priv->y0-priv->wr*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												} else {
													drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
													drc=priv->wr+dr1*(priv->s);
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									} else if (xt==GTK_PLOT_POLAR_BORDERS_CCW) {
										drs=r+(priv->bounds.thmax-th)*(rn-r)/(tn-th);
										if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmax);
											y=priv->y0-drs*sin(priv->bounds.thmax);
											cairo_move_to(cr, x, y);
											drs=th-r*(tn-th)/(rn-r);
											x=priv->x0+priv->wr*cos(drs);
											y=priv->y0-priv->wr*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									} else if (xt==(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CCW)) {
										drs=th-tn;
										if (drs>DZE) {
											drs=rn+(r-rn)*(priv->bounds.thmax-tn)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
												csx=tn-(th-tn)*rn/(r-rn);
												x=priv->x0+priv->wr*cos(csx);
												y=priv->y0-priv->wr*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												} else {
													drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									}
									xt=GTK_PLOT_POLAR_BORDERS_IN;
								}
							} else if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(rn>priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(rn<priv->bounds.rmax-priv->bounds.rmin))) {
								if (tn<priv->bounds.thmin) {
									if (xt==0) {
										drs=rn-r;
										if ((drs<DZE)&&(drs>NZE)) {
											drs=priv->wr+dr1*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmin);
											y=priv->y0-drs*sin(priv->bounds.thmin);
										} else {
											drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/drs;
											if (drs>=thn) {
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(drs);
												y=priv->y0-drc*sin(drs);
											} else {
												drs=r+(rn-r)*(th-priv->bounds.thmin)/(th-tn);
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
											}
										}
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
										drs=th-tn;
										if (drs>DZE) {
											drs=rn+(r-rn)*(priv->bounds.thmin-tn)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
												csx=th-(tn-th)*r/(rn-r);
												x=priv->x0+priv->wr*cos(csx);
												y=priv->y0-priv->wr*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												} else {
													drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									}
									xt=(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CW);
								} else if (tn>(priv->bounds.thmax)) {
									if (xt==0) {
										drs=rn-r;
										if ((drs<DZE)&&(drs>NZE)) {
											drs=priv->wr+dr1*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmax);
											y=priv->y0-drs*sin(priv->bounds.thmax);
										} else {
											drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/drs;
											if (drs<=thx) {
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(drs);
												y=priv->y0-drc*sin(drs);
											} else {
												drs=r+(rn-r)*(priv->bounds.thmax-th)/(tn-th);
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
											}
										}
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
										drs=tn-th;
										if (drs>DZE) {
											drs=r+(rn-r)*(priv->bounds.thmax-th)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
												csx=th-(tn-th)*r/(rn-r);
												x=priv->x0+priv->wr*cos(csx);
												y=priv->y0-priv->wr*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												} else {
													drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
													drc=priv->wr+dr1*priv->s;
													x=priv->x0+drc*cos(drs);
													y=priv->y0-drc*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									}
									xt=(GTK_PLOT_POLAR_BORDERS_OUT|GTK_PLOT_POLAR_BORDERS_CCW);
								} else {
									if (xt==0) {
										drs=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
										drc=priv->wr+dr1*priv->s;
										x=priv->x0+drc*cos(drs);
										y=priv->y0-drc*sin(drs);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
										csx=(tn-th)/(rn-r);
										drs=tn+csx*(priv->bounds.rmax-priv->bounds.rmin-rn);
										drc=priv->wr+dr1*priv->s;
										x=priv->x0+drc*cos(drs);
										y=priv->y0-drc*sin(drs);
										cairo_move_to(cr, x, y);
										drs=th-csx*r;
										x=priv->x0+priv->wr*cos(drs);
										y=priv->y0-priv->wr*sin(drs);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									} else if (xt==GTK_PLOT_POLAR_BORDERS_CW) {
										drs=r+(priv->bounds.thmin-th)*(rn-r)/(tn-th);
										if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<dr1))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>dr1))) {
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmin);
											y=priv->y0-drs*sin(priv->bounds.thmin);
											cairo_move_to(cr, x, y);
											drs=th+(priv->bounds.rmax-priv->bounds.rmin-r)*(tn-th)/(rn-r);
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									} else if (xt==(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CW)) {
										drs=tn-th;
										if (drs>DZE) {
											drs=r+(rn-r)*(priv->bounds.thmin-th)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
												csx=th+(tn-th)*(priv->bounds.rmax-priv->bounds.rmin-r)/(rn-r);
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(csx);
												y=priv->y0-drc*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmin);
													y=priv->y0-drs*sin(priv->bounds.thmin);
												} else {
													drs=th-(tn-th)*r/(rn-r);
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									} else if (xt==GTK_PLOT_POLAR_BORDERS_CCW) {
										drs=r+(priv->bounds.thmax-th)*(rn-r)/(tn-th);
										if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<dr1))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>dr1))) {
											drs=priv->wr+drs*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmax);
											y=priv->y0-drs*sin(priv->bounds.thmax);
											cairo_move_to(cr, x, y);
											drs=th+(priv->bounds.rmax-priv->bounds.rmin-r)*(tn-th)/(rn-r);
											drc=priv->wr+dr1*priv->s;
											x=priv->x0+drc*cos(drs);
											y=priv->y0-drc*sin(drs);
											cairo_line_to(cr, x, y);
											cairo_stroke(cr);
										}
									} else if (xt==(GTK_PLOT_POLAR_BORDERS_IN|GTK_PLOT_POLAR_BORDERS_CCW)) {
										drs=th-tn;
										if (drs>DZE) {
											drs=rn+(r-rn)*(priv->bounds.thmax-tn)/drs;
											if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs<priv->bounds.rmax-priv->bounds.rmin))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs>priv->bounds.rmax-priv->bounds.rmin))) {
												csx=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/(r-rn);
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(csx);
												y=priv->y0-drc*sin(csx);
												cairo_move_to(cr, x, y);
												if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&(drs>0.0))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&(drs<0.0))) {
													drs=priv->wr+drs*priv->s;
													x=priv->x0+drs*cos(priv->bounds.thmax);
													y=priv->y0-drs*sin(priv->bounds.thmax);
												} else {
													drs=th-(tn-th)*r/(rn-r);
													x=priv->x0+priv->wr*cos(drs);
													y=priv->y0-priv->wr*sin(drs);
												}
												cairo_line_to(cr, x, y);
												cairo_stroke(cr);
											}
										}
									}
									xt=GTK_PLOT_POLAR_BORDERS_OUT;
								}
							} else if (tn<priv->bounds.thmin) {
								if (xt==0) {
									drs=r+(rn-r)*(th-priv->bounds.thmin)/(th-tn);
									drs=priv->wr+drs*priv->s;
									x=priv->x0+drs*cos(priv->bounds.thmin);
									y=priv->y0-drs*sin(priv->bounds.thmin);
									cairo_line_to(cr, x, y);
									cairo_stroke(cr);
								} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
									drs=tn-rn*(tn-th)/(rn-r);
									if (drs>priv->bounds.thmin) {
										x=priv->x0+priv->wr*cos(drs);
										y=priv->y0-priv->wr*sin(drs);
										cairo_move_to(cr, x, y);
										drs=rn+(priv->bounds.thmin-tn)*(rn-r)/(tn-th);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmin);
										y=priv->y0-drs*sin(priv->bounds.thmin);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									}
								} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
									drs=tn+(priv->bounds.rmax-priv->bounds.rmin-rn)*(tn-th)/(rn-r);
									if (drs>priv->bounds.thmin) {
										drc=priv->wr+dr1*priv->s;
										x=priv->x0+drc*cos(drs);
										y=priv->y0-drc*sin(drs);
										cairo_move_to(cr, x, y);
										drs=rn+(priv->bounds.thmin-tn)*(rn-r)/(tn-th);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmin);
										y=priv->y0-drs*sin(priv->bounds.thmin);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									}
								}
								xt=GTK_PLOT_POLAR_BORDERS_CW;
							} else if (tn>priv->bounds.thmax) {
								if (xt==0) {
									drs=r+(rn-r)*(priv->bounds.thmax-th)/(tn-th);
									drs=priv->wr+drs*priv->s;
									x=priv->x0+drs*cos(priv->bounds.thmax);
									y=priv->y0-drs*sin(priv->bounds.thmax);
									cairo_line_to(cr, x, y);
									cairo_stroke(cr);
								} else if (xt==GTK_PLOT_POLAR_BORDERS_IN) {
									drs=tn-rn*(tn-th)/(rn-r);
									if (drs<priv->bounds.thmax) {
										x=priv->x0+priv->wr*cos(drs);
										y=priv->y0-priv->wr*sin(drs);
										cairo_move_to(cr, x, y);
										drs=rn+(priv->bounds.thmax-tn)*(rn-r)/(tn-th);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmax);
										y=priv->y0-drs*sin(priv->bounds.thmax);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									}
								} else if (xt==GTK_PLOT_POLAR_BORDERS_OUT) {
									drs=tn+(priv->bounds.rmax-priv->bounds.rmin-rn)*(tn-th)/(rn-r);
									if (drs<priv->bounds.thmax) {
										drc=priv->wr+dr1*priv->s;
										x=priv->x0+drc*cos(drs);
										y=priv->y0-drc*sin(drs);
										cairo_move_to(cr, x, y);
										drs=rn+(priv->bounds.thmax-tn)*(rn-r)/(tn-th);
										drs=priv->wr+drs*priv->s;
										x=priv->x0+drs*cos(priv->bounds.thmax);
										y=priv->y0-drs*sin(priv->bounds.thmax);
										cairo_line_to(cr, x, y);
										cairo_stroke(cr);
									}
								}
								xt=GTK_PLOT_POLAR_BORDERS_CCW;
							} else { /* within range */
								if ((xt&GTK_PLOT_POLAR_BORDERS_IN)!=0) {
									drs=rn-r;
									if ((xt&GTK_PLOT_POLAR_BORDERS_CW)!=0) {
										if ((drs<DZE)&&(drs>NZE)) {
											x=priv->x0+priv->wr*cos(priv->bounds.thmin);
											y=priv->y0-priv->wr*sin(priv->bounds.thmin);
										} else {
											drs=tn+(th-tn)*rn/drs;
											if (drs>=thn) { /* check wrapping */
												x=priv->x0+priv->wr*cos(drs);
												y=priv->y0-priv->wr*sin(drs);
											} else {
												drs=rn+(r-rn)*(tn-priv->bounds.thmin)/(tn-th);
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
											}
										}
									} else if ((xt&GTK_PLOT_POLAR_BORDERS_CCW)!=0) {
										if ((drs<DZE)&&(drs>NZE)) {
											x=priv->x0+priv->wr*cos(priv->bounds.thmax);
											y=priv->y0-priv->wr*sin(priv->bounds.thmax);
										} else {
											drs=tn+(th-tn)*rn/drs;
											if (drs<=thx) {
												x=priv->x0+priv->wr*cos(drs);
												y=priv->y0-priv->wr*sin(drs);
											} else {
												drs=rn+(r-rn)*(priv->bounds.thmax-tn)/(th-tn);
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
											}
										}
									} else {
										drs=tn+(th-tn)*rn/drs;
										x=priv->x0+priv->wr*cos(drs);
										y=priv->y0-priv->wr*sin(drs);
									}
									cairo_move_to(cr, x, y);
								} else if ((xt&GTK_PLOT_POLAR_BORDERS_OUT)!=0) {
									drs=r-rn;
									if ((xt&GTK_PLOT_POLAR_BORDERS_CW)!=0) {
										if ((drs<DZE)&&(drs>NZE)) {
											drs=priv->wr+dr1*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmin);
											y=priv->y0-drs*sin(priv->bounds.thmin);
										} else {
											drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/drs;
											if (drs>=thn) {
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(drs);
												y=priv->y0-drc*sin(drs);
											} else {
												drs=rn+(r-rn)*(tn-priv->bounds.thmin)/(tn-th);
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmin);
												y=priv->y0-drs*sin(priv->bounds.thmin);
											}
										}
									} else if ((xt&GTK_PLOT_POLAR_BORDERS_CCW)!=0) {
										if ((drs<DZE)&&(drs>NZE)) {
											drs=priv->wr+dr1*priv->s;
											x=priv->x0+drs*cos(priv->bounds.thmax);
											y=priv->y0-drs*sin(priv->bounds.thmax);
										} else {
											drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/drs;
											if (drs<=thx) {
												drc=priv->wr+dr1*priv->s;
												x=priv->x0+drc*cos(drs);
												y=priv->y0-drc*sin(drs);
											} else {
												drs=rn+(r-rn)*(priv->bounds.thmax-tn)/(th-tn);
												drs=priv->wr+drs*priv->s;
												x=priv->x0+drs*cos(priv->bounds.thmax);
												y=priv->y0-drs*sin(priv->bounds.thmax);
											}
										}
									} else {
										drs=tn+(th-tn)*(priv->bounds.rmax-priv->bounds.rmin-rn)/drs;
										drc=priv->wr+dr1*priv->s;
										x=priv->x0+drc*cos(drs);
										y=priv->y0-drc*sin(drs);
									}
									cairo_move_to(cr, x, y);
								} else if ((xt&GTK_PLOT_POLAR_BORDERS_CW)!=0) {
									drs=rn+(r-rn)*(tn-priv->bounds.thmin)/(tn-th);
									drs=priv->wr+drs*priv->s;
									x=priv->x0+drs*cos(priv->bounds.thmin);
									y=priv->y0-drs*sin(priv->bounds.thmin);
									cairo_move_to(cr, x, y);
								} else if ((xt&GTK_PLOT_POLAR_BORDERS_CCW)!=0) {
									drs=rn+(r-rn)*(priv->bounds.thmax-tn)/(th-tn);
									drs=priv->wr+drs*priv->s;
									x=priv->x0+drs*cos(priv->bounds.thmax);
									y=priv->y0-drs*sin(priv->bounds.thmax);
									cairo_move_to(cr, x, y);
								}
								drs=priv->wr+rn*priv->s;
								x=priv->x0+drs*cos(tn);
								y=priv->y0-drs*sin(tn);
								cairo_line_to(cr, x, y);
								xt=0;
							}
							r=rn;
							th=tn;
						}
						cairo_stroke(cr);
          }
        }
      } else { /* spline interpolation to polar transform - placeholder */
      }
    } else if (((plot->flagd)&GTK_PLOT_POLAR_DISP_PTS)!=0) { /* points only */
      for (k=0; k<(plt->ind->len); k++) {
        cairo_set_dash(cr, NULL, 0, 0);
        if (plot->kdata && k<(plot->kdata->len) && (g_array_index(plot->kdata, guint, k)&GTK_PLOT_POLAR_KEY_CLR)) continue;
        ft=fmod(k,plt->rd->len);
        vv=g_array_index(plt->rd, gdouble, ft);
        wv=g_array_index(plt->gr, gdouble, ft);
        xv=g_array_index(plt->bl, gdouble, ft);
        yv=g_array_index(plt->al, gdouble, ft);
        cairo_set_source_rgba(cr, vv, wv, xv, yv);
        ft=g_array_index(plt->ind, gint, k);
        if (ft>=(plot->rdata->len)) break;
        st=g_array_index(plt->stride, gint, k);
        lt=g_array_index(plt->sizes, gint, k)*st+ft;
        if (lt>(plot->rdata->len)) lt=plot->rdata->len;
        for (j=ft+st; j<lt; j+=st) {
          r=g_array_index(plot->rdata, gdouble, j)-priv->bounds.rmin;
          th=g_array_index(plot->thdata, gdouble, j);
          if ((th<priv->bounds.thmin-MY_2PI)||(th>priv->bounds.thmax+MY_2PI)||((th<priv->bounds.thmin)&&(th>priv->bounds.thmax-MY_2PI))||((th>priv->bounds.thmax)&&(th<priv->bounds.thmin+MY_2PI))) continue;
          if (((priv->bounds.rmax-priv->bounds.rmin>=0.0)&&((r>priv->bounds.rmax-priv->bounds.rmin)||(r<0.0)))||((priv->bounds.rmax-priv->bounds.rmin<=0.0)&&((r<priv->bounds.rmax-priv->bounds.rmin)||(r>0.0)))) continue;
          drs=priv->wr+r*priv->s;
          x=priv->x0+drs*cos(th);
          y=priv->y0-drs*sin(th);
          cairo_move_to(cr, x, y);
          cairo_arc(cr, x, y, plot->ptsize, 0, MY_2PI);
          cairo_fill(cr);
        }
      }
    }
  }
}

static void gtk_plot_polar_redraw(GtkWidget *widget) {
  GdkRegion *region;
  GdkWindow *wdw;

  if (!(wdw=gtk_widget_get_window(widget))) return;
  region=gdk_drawable_get_clip_region(wdw);
  gdk_window_invalidate_region(wdw, region, TRUE);
  gdk_window_process_updates(wdw, TRUE);
  gdk_region_destroy(region);
}

gboolean gtk_plot_polar_update_scale(GtkWidget *widget, gdouble rn, gdouble rx, gdouble thn, gdouble thx, gdouble rcn, gdouble thc) {
  GtkPlotPolarPrivate *priv;

  priv=GTK_PLOT_POLAR_GET_PRIVATE(widget);
  priv->centre.r   =rcn;
  priv->bounds.rmin=rn;
  priv->bounds.rmax=rx;
  if (((GTK_PLOT_POLAR(widget)->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) {
    priv->centre.th   =thc*MY_PI_180;
    priv->bounds.thmin=thn*MY_PI_180;
    priv->bounds.thmax=thx*MY_PI_180;
  } else {
    priv->centre.th   =thc;
    priv->bounds.thmin=thn;
    priv->bounds.thmax=thx;
  }
  gtk_plot_polar_redraw(widget);
  return FALSE;
}

gboolean gtk_plot_polar_update_scale_pretty(GtkWidget *widget, gdouble xn, gdouble xx, gdouble yn, gdouble yx) {
  GtkPlot *plt;
  GtkPlotPolar *plot;
  GtkPlotPolarPrivate *priv;
  gdouble num, num3, thn, thx, dtt;
  gint num2, lt, ut, tk;

  priv=GTK_PLOT_POLAR_GET_PRIVATE(widget);
  plot=GTK_PLOT_POLAR(widget);
  plt=GTK_PLOT(widget);
  priv->ticks.zc=WGS;
  if ((xx+xn)<0) {
    xx=-xx;
    xn=-xn;
  }
  if (xx<xn) {
    num=xx;
    xx=xn;
    xn=num;
  }
  dtt=((pango_font_description_get_size(plt->afont)+pango_font_description_get_size(plt->lfont))/PANGO_SCALE+JTI)*(xx-xn)/((priv->bounds.rmax-priv->bounds.rmin)*priv->s);
  priv->rcs=2; /* determine number of characters needed for radial labels and pretty max/min values */
  if (xn<=0) {
    priv->bounds.rmin=0;
    num=log10(xx);
    if (num>=0) {
      num2=(gint)num;
      num=fmod(num,1);
      priv->rcs+=num2;
      if (num==0) {
        priv->ticks.r=5;
        priv->bounds.rmax=xx;
        num2--;
      } else if (num<L10_2PT5) {
        priv->ticks.r=ceil(2*exp(G_LN10*num));
        priv->bounds.rmax=5*priv->ticks.r*exp(G_LN10*(--num2));
      } else if (num<L10_5) {
        priv->ticks.r=ceil(exp(G_LN10*num));
        priv->bounds.rmax=priv->ticks.r*exp(G_LN10*num2);
      } else {
        priv->ticks.r=ceil(0.5*exp(G_LN10*num));
        priv->bounds.rmax=2*priv->ticks.r*exp(G_LN10*num2);
        if (priv->ticks.r==5) priv->rcs++;
      }
    } else {
      num2=floor(num);
      num=fmod(num,1);
      num++;
      if (num==0) {
        priv->ticks.r=5;
        priv->bounds.rmax=xx;
        num2--;
      } else if (num<L10_2PT5) {
        priv->ticks.r=ceil(2*exp(G_LN10*num));
        priv->bounds.rmax=5*priv->ticks.r*exp(G_LN10*(--num2));
      } else if (num<L10_5) {
        priv->ticks.r=ceil(exp(G_LN10*num));
        priv->bounds.rmax=priv->ticks.r*exp(G_LN10*num2);
      } else {
        priv->ticks.r=ceil(0.5*exp(G_LN10*num));
        priv->bounds.rmax=2*priv->ticks.r*exp(G_LN10*num2);
      }
    }
    if (num2<0) (priv->rcs)+=1-num2;
  } else {
    num3=(xx-xn)/5;
    num=log10(num3);
    if (num>=0) {
      num2=(gint)num;
      num=fmod(num,1);
    } else {
      num2=floor(num);
      num=fmod(num,1);
      num++;
    }
    if ((num<=0)||(num>=1)) {
      lt=floor(FAC+xn/num3);
      ut=ceil(xx/num3-FAC);
      tk=ut-lt;
      if (tk>5) {
        num3*=2;
        lt=floor(FAC+xn/num3);
        ut=ceil(xx/num3-FAC);
        tk=ut-lt;
      }
    } else if (num<L10_2PT5) {
      num=exp(G_LN10*num2);
      num3=2*num;
      lt=floor(FAC+xn/num3);
      ut=ceil(xx/num3-FAC);
      tk=ut-lt;
      if (tk>5) {
        num3=5*num;
        lt=floor(FAC+xn/num3);
        ut=ceil(xx/num3-FAC);
        tk=ut-lt;
        if (tk>5) {
          num3*=2;
          lt=floor(FAC+xn/num3);
          ut=ceil(xx/num3-FAC);
          tk=ut-lt;
        }
      }
      num=fmod(num,1);
    } else if (num<L10_5) {
      num=exp(G_LN10*num2);
      num3=5*num;
      lt=floor(FAC+xn/num3);
      ut=ceil(xx/num3-FAC);
      tk=ut-lt;
      if (tk>5) {
        num3*=2;
        lt=floor(FAC+xn/num3);
        ut=ceil(xx/num3-FAC);
        tk=ut-lt;
        if (tk>5) {
          num3*=2;
          lt=floor(FAC+xn/num3);
          ut=ceil(xx/num3-FAC);
          tk=ut-lt;
        }
      }
    } else {
      num=G_LN10*(++num2);
      num3=exp(num);
      lt=floor(FAC+xn/num3);
      ut=ceil(xx/num3-FAC);
      tk=ut-lt;
      if (tk>5) {
        num3*=2;
        lt=floor(FAC+xn/num3);
        ut=ceil(xx/num3-FAC);
        tk=ut-lt;
      }
    }
    priv->bounds.rmin=num3*(gdouble)lt;
    priv->bounds.rmax=num3*(gdouble)ut;
    priv->ticks.r=tk;
    if (priv->bounds.rmax>=10) priv->rcs+=floor(log10(xx));
    if (num3<1) {
      priv->rcs++;
      num=-log10(num3);
      if (fmod(num,1)<FA2) {
        num2=(gint)num;
        plot->rdp=(guint)num;
      } else {
        num2=(gint)ceil(num);
        plot->rdp=(guint)ceil(num);
      }
      priv->rcs+=num2;
    } else plot->rdp=0;
  }
  if (priv->rcs>8) priv->rcs=8;
  if (((plot->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) {
    num=(yx-yn);
    if (num<0) {
      num+=360;
      if (yx+yn<0) {
        thx=yx+360;
        thn=yn;
      } else {
        thn=yn-360;
        thx=yx;
      }
    } else {
      thx=yx;
      thn=yn;
    }
    if (num>=360) {
      if (yx>=360) {
        priv->bounds.thmax=MY_2PI;
        priv->bounds.thmin=0;
        priv->thcs=4;
      } else if (yn<=-360) {
        priv->bounds.thmax=0;
        priv->bounds.thmin=N2_MY_PI;
        priv->thcs=5;
      } else {
        priv->bounds.thmax=G_PI;
        priv->bounds.thmin=NMY_PI;
        priv->thcs=5;
      }
      priv->centre.r=0;
      priv->centre.th=0;
      priv->ticks.zin=12;
      priv->ticks.z2m=2;
    } else if (num>150) {
      ut=ceil(thx/30-FA2);
      lt=floor(thn/30+FA2);
      priv->bounds.thmax=ut*MY_PI_6;
      priv->bounds.thmin=lt*MY_PI_6;
      priv->centre.r=priv->bounds.rmin+(priv->bounds.rmax-priv->bounds.rmin)*(1+cos(num*MY_PI_360))/2;
      priv->centre.th=(ut+lt)*MY_PI_12;
      if (thn<-100) priv->thcs=5;
      else priv->thcs=4;
      priv->ticks.zin=ut-lt;
      priv->ticks.z2m=2;
    } else if (num>60) {
      ut=ceil(thx/10-FA2);
      lt=floor(thn/10+FA2);
      priv->bounds.thmax=ut*MY_PI_18;
      priv->bounds.thmin=lt*MY_PI_18;
      priv->centre.r=(priv->bounds.rmax+dtt+priv->bounds.rmin)/2;
      priv->centre.th=(ut+lt)*MY_PI_36;
      if (thn<-100) priv->thcs=5;
      else if ((thn>=-10) && (thx<=100)) priv->thcs=3;
      else priv->thcs=4;
      priv->ticks.zin=ut-lt;
      priv->ticks.z2m=1;
    } else if (num>30) {
      ut=ceil(thx/5-FA2);
      lt=floor(thn/5+FA2);
      priv->bounds.thmax=ut*MY_PI_36;
      priv->bounds.thmin=lt*MY_PI_36;
      priv->centre.r=(priv->bounds.rmax+dtt+priv->bounds.rmin)/2;
      priv->centre.th=(ut+lt)*MY_PI_72;
      if (thn<-100) priv->thcs=5;
      else if ((thn>=-10) && (thx<=100)) priv->thcs=3;
      else priv->thcs=4;
      priv->ticks.zin=ut-lt;
      priv->ticks.z2m=0;
    } else if (num>12) {
      ut=ceil(thx/2-FA2);
      lt=floor(thn/2+FA2);
      priv->bounds.thmax=ut*MY_PI_90;
      priv->bounds.thmin=lt*MY_PI_90;
      priv->centre.r=(priv->bounds.rmax+dtt+priv->bounds.rmin)/2;
      priv->centre.th=(ut+lt)*MY_PI_180;
      if (thn<-100) priv->thcs=5;
      else if ((thn>=-10) && (thx<=100)) priv->thcs=3;
      else priv->thcs=4;
      priv->ticks.zin=ut-lt;
      priv->ticks.z2m=0;
    } else {
      ut=ceil(thx-FA2);
      lt=floor(thn+FA2);
      priv->bounds.thmax=ut*MY_PI_180;
      priv->bounds.thmin=lt*MY_PI_180;
      priv->centre.r=(priv->bounds.rmax+dtt+priv->bounds.rmin)/2;
      priv->centre.th=(ut+lt)*MY_PI_360;
      if (thn<-100) priv->thcs=5;
      else if (thn>=-10) {
        if (thx<=100) {
          if ((thn>=0) && (thx<=10)) priv->thcs=2;
          else priv->thcs=3;
        } else priv->thcs=4;
      } else priv->thcs=4;
      priv->ticks.zin=ut-lt;
      priv->ticks.z2m=0;
    }
  } else {
    thn=yn*I_MY_PI;
    thx=yx*I_MY_PI;
    num=thx-thn;
    if (num<0) {
      num+=2;
      if (thn+thx<0) thx+=2;
      else thn-=2;
    }
    if (num>=2) {
      if (thx>=2) {
        priv->bounds.thmax=MY_2PI;
        priv->bounds.thmin=0;
        priv->thcs=6;
      } else if (yn<=-2) {
        priv->bounds.thmax=0;
        priv->bounds.thmin=N2_MY_PI;
        priv->thcs=7;
      } else {
        priv->bounds.thmax=G_PI;
        priv->bounds.thmin=NMY_PI;
        priv->thcs=6;
      }
      priv->centre.r=0;
      priv->centre.th=0;
      priv->ticks.zin=12;
      priv->ticks.z2m=2;
    } else if (6*num>5) {
      ut=ceil(thx*6-FA2);
      lt=floor(thn*6+FA2);
      priv->bounds.thmax=MY_PI_6*ut;
      priv->bounds.thmin=MY_PI_6*lt;
      priv->centre.r=priv->bounds.rmin+(priv->bounds.rmax-priv->bounds.rmin)*(1+cos(num*G_PI_2))/2;
      priv->centre.th=(ut+lt)*MY_PI_12;
      if (lt<-10) priv->thcs=7;
      else if ((lt>=-1)&&(ut<=10)) priv->thcs=5;
      else (priv->thcs)=6;
      priv->ticks.zin=ut-lt;
      priv->ticks.z2m=2;
    } else if (3*num>1) {
      ut=ceil(thx*18-FA2);
      lt=floor(thn*18+FA2);
      priv->bounds.thmax=MY_PI_18*ut;
      priv->bounds.thmin=MY_PI_18*lt;
      priv->centre.r=(priv->bounds.rmax+dtt+priv->bounds.rmin)/2;
      priv->centre.th=(ut+lt)*MY_PI_36;
      if (lt<-10) priv->thcs=8;
      else if ((lt>=-1)&&(ut<=10)) priv->thcs=6;
      else priv->thcs=7;
      priv->ticks.zin=ut-lt;
      priv->ticks.z2m=1;
    } else if (6*num>1) {
      ut=ceil(thx*36-FA2);
      lt=floor(thn*36+FA2);
      priv->bounds.thmax=MY_PI_36*ut;
      priv->bounds.thmin=MY_PI_36*lt;
      priv->centre.r=(priv->bounds.rmax+dtt+priv->bounds.rmin)/2;
      priv->centre.th=(ut+lt)*MY_PI_72;
      if (lt<-10) priv->thcs=8;
      else if ((lt>=-1)&&(ut<=10)) priv->thcs=6;
      else (priv->thcs)=7;
      priv->ticks.zin=ut-lt;
      priv->ticks.z2m=0;
    } else if (15*num>1) {
      ut=ceil(thx*90-FA2);
      lt=floor(thn*90+FA2);
      priv->bounds.thmax=MY_PI_90*ut;
      priv->bounds.thmin=MY_PI_90*lt;
      priv->centre.r=(priv->bounds.rmax+dtt+priv->bounds.rmin)/2;
      priv->centre.th=(ut+lt)*MY_PI_180;
      if (lt<-100) priv->thcs=9;
      else if ((lt<-10)||(ut>100)) priv->thcs=8;
      else if ((lt>=-1)&&(ut<=10)) priv->thcs=6;
      else priv->thcs=7;
      priv->ticks.zin=ut-lt;
      priv->ticks.z2m=0;
    } else {
      ut=ceil(thx*180-FA2);
      lt=floor(thn*180+FA2);
      priv->bounds.thmax=MY_PI_180*ut;
      priv->bounds.thmin=MY_PI_180*lt;
      priv->centre.r=(priv->bounds.rmax+dtt+priv->bounds.rmin)/2;
      priv->centre.th=(ut+lt)*MY_PI_360;
      if (lt<-100) priv->thcs=10;
      else if ((lt<-10)||(ut>100)) priv->thcs=9;
      else if ((lt>=-1)&&(ut<=10)) priv->thcs=7;
      else priv->thcs=8;
      priv->ticks.zin=ut-lt;
      priv->ticks.z2m=0;
    }
  }
  gtk_plot_polar_redraw(widget);
  return FALSE;
}

gboolean gtk_plot_polar_print(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer data) {
  cairo_t* cr;

  cr=gtk_print_context_get_cairo_context(context);
  drawk(GTK_WIDGET(data), cr);
  draw(GTK_WIDGET(data), cr);
  return FALSE;
}

gboolean gtk_plot_polar_print_eps(GtkWidget *plot, gchar* fout) {
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

gboolean gtk_plot_polar_print_png(GtkWidget *plot, gchar* fout) {
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

gboolean gtk_plot_polar_print_svg(GtkWidget *plot, gchar* fout) {
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

static gboolean gtk_plot_polar_button_press(GtkWidget *widget, GdkEventButton *event) {
  gdouble dy, dx, dt, s;
  gint d, hg, j, wd, xw;
  GtkPlot *plt;
  GtkPlotPolar *plot;
  GtkPlotPolarPrivate *priv;
  PangoLayout *lyt;

  priv=GTK_PLOT_POLAR_GET_PRIVATE(widget);
  if ((priv->flagr)==0) {
    xw=widget->allocation.width;
    j=(xw<400)?0:(xw<1000)?1:2;
    plt=GTK_PLOT(widget);
    plot=GTK_PLOT_POLAR(widget);
    if ((event->y)<=2*SZ[j]+1.0) {
      if ((event->x)>=xw-2*SZ[j]-1.0) {
        priv->flagr=2;
	    return FALSE;
      } else if ((event->x)>=xw-4*SZ[j]-2.0) {
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
    priv->rescale.rmax=priv->bounds.rmax;
    priv->rescale.thmax=priv->bounds.thmax;
    dy=priv->y0-event->y;
    dx=event->x-priv->x0;
    if (((plot->zmode)&(GTK_PLOT_POLAR_ZOOM_SGL|GTK_PLOT_POLAR_ZOOM_AZM))!=0) {
      dt=atan2(dy, dx); /* divide by zero handling */
      if ((dt<=(priv->bounds.thmax))&&(dt>=(priv->bounds.thmin))) {
        priv->rescale.thmax=dt;
        priv->flagr=1;
      } else if ((dt+MY_2PI<=(priv->bounds.thmax))&&(dt+MY_2PI<=(priv->bounds.thmin))) {
        priv->rescale.thmax=dt+MY_2PI;
        priv->flagr=1;
      } else if ((dt-MY_2PI<=(priv->bounds.thmax))&&(dt-MY_2PI<=(priv->bounds.thmin))) {
        priv->rescale.thmax=dt-MY_2PI;
        priv->flagr=1;
      } else return FALSE;
    }
    if (((plot->zmode)&(GTK_PLOT_POLAR_ZOOM_SGL|GTK_PLOT_POLAR_ZOOM_RDL))!=0) {
      dy=(sqrt(dy*dy+dx*dx)-priv->wr)/priv->s+priv->bounds.rmin;
      if ((dy<=(priv->bounds.rmax))&&(dy>=(priv->bounds.rmin))) {
        priv->rescale.rmax=dy;
        priv->flagr=1;
      } else priv->flagr=0;
    }
  }
  return FALSE;
}

static gboolean gtk_plot_polar_motion_notify(GtkWidget *widget, GdkEventMotion *event)
{
	GtkPlotPolarPrivate *priv;
	GtkPlotPolar *plot;
	gdouble dx, dy, dt;

	priv=GTK_PLOT_POLAR_GET_PRIVATE(widget);
	plot=GTK_PLOT_POLAR(widget);
	dy=(priv->y0)-(event->y);
	dx=(event->x)-(priv->x0);
	dt=atan2(dy, dx); /* divide by zero handling */
	if ((dt<=(priv->bounds.thmax))&&(dt>=(priv->bounds.thmin)))
	{
		dy=((sqrt((dy*dy)+(dx*dx))-(priv->wr))/(priv->s))+(priv->bounds.rmin);
		if ((dy>=(priv->bounds.rmin))&&(dy<=(priv->bounds.rmax)))
		{
			(plot->rps)=dy;
			if (((plot->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) (plot->thps)=dt*I180_MY_PI;
			else (plot->thps)=dt;
			g_signal_emit(plot, gtk_plot_polar_signals[MOVED], 0);
		}
	}
	else if (((dt+MY_2PI)<=(priv->bounds.thmax))&&((dt+MY_2PI)>=(priv->bounds.thmin)))
	{
		dy=((sqrt((dy*dy)+(dx*dx))-(priv->wr))/(priv->s))+(priv->bounds.rmin);
		if ((dy>=(priv->bounds.rmin))&&(dy<=(priv->bounds.rmax)))
		{
			(plot->rps)=dy;
			if (((plot->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) (plot->thps)=(dt*I180_MY_PI)+360;
			else (plot->thps)=dt+MY_2PI;
			g_signal_emit(plot, gtk_plot_polar_signals[MOVED], 0);
		}
	}
	else if (((dt-MY_2PI)<=(priv->bounds.thmax))&&((dt-MY_2PI)>=(priv->bounds.thmin)))
	{
		dy=((sqrt((dy*dy)+(dx*dx))-(priv->wr))/(priv->s))+(priv->bounds.rmin);
		if ((dy>=(priv->bounds.rmin))&&(dy<=(priv->bounds.rmax)))
		{
			(plot->rps)=dy;
			if (((plot->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) (plot->thps)=(dt*I180_MY_PI)-360;
			else (plot->thps)=dt-MY_2PI;
			g_signal_emit(plot, gtk_plot_polar_signals[MOVED], 0);
		}
	}
  return FALSE;
}

static gboolean gtk_plot_polar_button_release(GtkWidget *widget, GdkEventButton *event) {
  gdouble dy, dx, dt, xn, xx, yn, yx, s;
  gint d, hg, j, wd, xw;
  GtkPlot *plt;
  GtkPlotPolar *plot;
  GtkPlotPolarPrivate *priv;
  guint *ptr;
  PangoLayout *lyt;

  priv=GTK_PLOT_POLAR_GET_PRIVATE(widget);
  plot=GTK_PLOT_POLAR(widget);
  xw=widget->allocation.width;
  j=(xw<400)?0:(xw<1000)?1:2;
  switch (priv->flagr) {
    case 1:
      if (((plot->zmode)&GTK_PLOT_POLAR_ZOOM_SGL)==0) {
			(priv->rescale.rmin)=(priv->bounds.rmin);
			(priv->rescale.thmin)=(priv->bounds.thmin);
			dy=(priv->y0)-(event->y);
			dx=(event->x)-(priv->x0);
			if (((plot->zmode)&GTK_PLOT_POLAR_ZOOM_AZM)!=0)
			{
				dt=atan2(dy, dx); /* divide by zero handling */
				if ((dt<=(priv->bounds.thmax))&&(dt>=(priv->bounds.thmin))) (priv->rescale.thmin)=dt;
				else if (((dt+MY_2PI)<=(priv->bounds.thmax))&&((dt+MY_2PI)<=(priv->bounds.thmin))) (priv->rescale.thmin)=dt+MY_2PI;
				else if (((dt-MY_2PI)<=(priv->bounds.thmax))&&((dt-MY_2PI)<=(priv->bounds.thmin))) (priv->rescale.thmin)=dt-MY_2PI;
			}
			if (((plot->zmode)&GTK_PLOT_POLAR_ZOOM_RDL)!=0)
			{
				dy=((sqrt((dy*dy)+(dx*dx))-(priv->wr))/(priv->s))+(priv->bounds.rmin);
				if ((dy<=(priv->bounds.rmax))&&(dy>=(priv->bounds.rmin))) (priv->rescale.rmin)=dy;
			}
			if (((plot->zmode)&GTK_PLOT_POLAR_ZOOM_OUT)==0)
			{
				if ((priv->rescale.thmax)!=(priv->rescale.thmin))
				{
					if ((priv->rescale.rmax)>(priv->rescale.rmin))
					{
						{xn=(priv->rescale.rmin); xx=(priv->rescale.rmax); yn=(priv->rescale.thmin); yx=(priv->rescale.thmax);}
						if (((plot->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) {yn*=I180_MY_PI; yx*=I180_MY_PI;}
						gtk_plot_polar_update_scale_pretty(widget, xn, xx, yn, yx);
					}
					else if ((priv->rescale.rmax)<(priv->rescale.rmin))
					{
						{xn=(priv->rescale.rmax); xx=(priv->rescale.rmin); yn=(priv->rescale.thmin); yx=(priv->rescale.thmax);}
						if (((plot->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) {yn*=I180_MY_PI; yx*=I180_MY_PI;}
						gtk_plot_polar_update_scale_pretty(widget, xn, xx, yn, yx);
					}
				}
			}
			else if (((priv->rescale.rmax)!=(priv->rescale.rmin))&&((priv->rescale.thmax)!=(priv->rescale.thmin)))
			{
				s=((priv->bounds.rmax)-(priv->bounds.rmin))/((priv->rescale.rmax)-(priv->rescale.rmin));
				if (s>0) {xn=((priv->bounds.rmin)-(priv->rescale.rmin))*s; xx=((priv->bounds.rmax)-(priv->rescale.rmax))*s;}
				else {xn=((priv->rescale.rmax)-(priv->bounds.rmin))*s; xx=((priv->rescale.rmin)-(priv->bounds.rmax))*s;}
				{xn+=(priv->bounds.rmin); xx+=(priv->bounds.rmax);}
				s=((priv->bounds.thmax)-(priv->bounds.thmin))/((priv->rescale.thmax)-(priv->rescale.thmin));
				if (s>0) {yn=((priv->bounds.thmin)-(priv->rescale.thmin))*s; yx=((priv->bounds.thmax)-(priv->rescale.thmax))*s;}
				else if (s<0) {yn=((priv->rescale.thmax)-(priv->bounds.thmin))*s; yx=((priv->rescale.thmin)-(priv->bounds.thmax))*s;}
				else return FALSE;
				{yn+=(priv->bounds.thmin); yx+=(priv->bounds.thmax);}
				if (((plot->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) {yn*=I180_MY_PI; yx*=I180_MY_PI;}
				gtk_plot_polar_update_scale_pretty(widget, xn, xx, yn, yx);
			}
		}
		else if (((plot->zmode)&GTK_PLOT_POLAR_ZOOM_OUT)==0)
		{
			xn=((priv->rescale.rmax)*ZS)+(ZSC*(priv->bounds.rmin));
			xx=((priv->rescale.rmax)*ZS)+(ZSC*(priv->bounds.rmax));
			yn=((priv->rescale.thmax)*ZS)+(ZSC*(priv->bounds.thmin));
			yx=((priv->rescale.thmax)*ZS)+(ZSC*(priv->bounds.thmax));
			if (((plot->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) {yn*=I180_MY_PI; yx*=I180_MY_PI;}
			gtk_plot_polar_update_scale_pretty(widget, xn, xx, yn, yx);
		}
		else
		{
			xn=((priv->bounds.rmin)*UZ)-(UZC*(priv->rescale.rmax));
			xx=((priv->bounds.rmax)*UZ)-(UZC*(priv->rescale.rmax));
			yn=((priv->bounds.thmin)*UZ)-(UZC*(priv->rescale.thmax));
			yx=((priv->bounds.thmax)*UZ)-(UZC*(priv->rescale.thmax));
			if (((plot->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) {yn*=I180_MY_PI; yx*=I180_MY_PI;}
			gtk_plot_polar_update_scale_pretty(widget, xn, xx, yn, yx);
		}
      break;
    case 2:
      if (((event->y)<=2*SZ[j]+1.0)&&((event->x)>=xw-2*SZ[j]-1.0)) {
        plot->zmode^=GTK_PLOT_POLAR_ZOOM_OUT;
        gtk_widget_queue_draw_area(widget, xw-2*SZ[j]-1.0, 0, 2*SZ[j]+1.0, 2*SZ[j]+1.0);
      }
      break;
    case 3:
      if (((event->y)<=2*SZ[j]+1.0)&&((event->x)>=xw-4*SZ[j]-2.0)&&((event->x)<=xw-2*SZ[j]-1.0)) {
        plot->zmode-=GTK_PLOT_POLAR_ZOOM_RDL;
        if (((plot->zmode)&(GTK_PLOT_POLAR_ZOOM_SGL|GTK_PLOT_POLAR_ZOOM_AZM|GTK_PLOT_POLAR_ZOOM_RDL))==0) plot->zmode|=GTK_PLOT_POLAR_ZOOM_SGL;
        gtk_widget_queue_draw_area(widget, xw-4*SZ[j]-2.0, 0, 2*SZ[j]+1.0, 2*SZ[j]+1.0);
      }
      break;
    case 0:
      break;
    default:
      plt=GTK_PLOT(widget);
      if ((plot->kdata)&&(plot->kdata->len+4>priv->flagr)&&(plt->ky)&&(*(plt->ky))&&((event->x)>=xw-2*SZ[j]-1.0)&&((event->y)>=2*SZ[j]+3.0)) {
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
	          (*ptr)^=GTK_PLOT_POLAR_KEY_CLR;
              g_signal_emit(plot, gtk_plot_polar_signals[KEY_CHANGED], 0);
              gtk_plot_polar_redraw(widget);
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

void gtk_plot_polar_set_label(GtkPlotPolar *plot, gchar *rl, gchar *tl) {
  if (plot->rlab)  g_free(plot->rlab);
  if (plot->thlab) g_free(plot->thlab);
  plot->rlab  = g_strdup(rl);
  plot->thlab = g_strdup(tl);
}

void gtk_plot_polar_set_data(GtkPlotPolar *plot, GArray *rd, GArray *td, GArray *nd, GArray *sz, GArray *st, GArray *kd) {
  if (plot->rdata)  g_array_unref(plot->rdata);
  plot->rdata  = g_array_ref(rd);
  if (plot->thdata) g_array_unref(plot->thdata);
  plot->thdata = g_array_ref(td);
  if (plot->kdata)  g_array_unref(plot->kdata);
  plot->kdata  = kd?g_array_ref(kd):NULL;
  gtk_plot_set_indices(GTK_PLOT(plot), nd, sz, st);
}

static void gtk_plot_polar_finalise(GtkPlotPolar *plot) {
  if (plot->rlab) {
    g_free(plot->rlab);
    plot->rlab=NULL;
  }
  if (plot->thlab) {
    g_free(plot->thlab);
    plot->thlab=NULL;
  }
  if (plot->rdata) {
    g_array_unref(plot->rdata);
    plot->rdata=NULL;
  }
  if (plot->thdata) {
    g_array_unref(plot->thdata);
    plot->thdata=NULL;
  }
  if (plot->kdata) {
    g_array_unref(plot->kdata);
    plot->kdata=NULL;
  }
}

static void gtk_plot_polar_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch (prop_id)
	{
		case PROP_BRN:
			GTK_PLOT_POLAR_GET_PRIVATE(object)->bounds.rmin=g_value_get_double(value);
			break;
		case PROP_BRX:
			GTK_PLOT_POLAR_GET_PRIVATE(object)->bounds.rmax=g_value_get_double(value);
			break;
		case PROP_BTN:
			if (((GTK_PLOT_POLAR(object)->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) (GTK_PLOT_POLAR_GET_PRIVATE(object)->bounds.thmin)=MY_PI_180*g_value_get_double(value);
			else GTK_PLOT_POLAR_GET_PRIVATE(object)->bounds.thmin=g_value_get_double(value);
			break;
		case PROP_BTX:
			if (((GTK_PLOT_POLAR(object)->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) (GTK_PLOT_POLAR_GET_PRIVATE(object)->bounds.thmax)=MY_PI_180*g_value_get_double(value);
			else GTK_PLOT_POLAR_GET_PRIVATE(object)->bounds.thmax=g_value_get_double(value);
			break;
		case PROP_CR:
			GTK_PLOT_POLAR_GET_PRIVATE(object)->centre.r=g_value_get_double(value);
			break;
		case PROP_CT:
			if (((GTK_PLOT_POLAR(object)->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) (GTK_PLOT_POLAR_GET_PRIVATE(object)->centre.th)=MY_PI_180*g_value_get_double(value);
			else GTK_PLOT_POLAR_GET_PRIVATE(object)->centre.th=g_value_get_double(value);
			break;
		case PROP_RT:
			GTK_PLOT_POLAR_GET_PRIVATE(object)->ticks.r=g_value_get_uint(value);
			break;
		case PROP_ZIT:
			GTK_PLOT_POLAR_GET_PRIVATE(object)->ticks.zin=g_value_get_uint(value);
			break;
		case PROP_ZTM:
			GTK_PLOT_POLAR_GET_PRIVATE(object)->ticks.z2m=g_value_get_uint(value);
			break;
		case PROP_ZC:
			GTK_PLOT_POLAR_GET_PRIVATE(object)->ticks.zc=g_value_get_uint(value);
			break;
		case PROP_RC:
			GTK_PLOT_POLAR_GET_PRIVATE(object)->rcs=g_value_get_uint(value);
			break;
		case PROP_TC:
			GTK_PLOT_POLAR_GET_PRIVATE(object)->thcs=g_value_get_uint(value);
			break;
		case PROP_HV:
			GTK_PLOT_POLAR_GET_PRIVATE(object)->hv=g_value_get_boolean(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void gtk_plot_polar_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch (prop_id)
	{
		case PROP_BRN:
			g_value_set_double(value, (GTK_PLOT_POLAR_GET_PRIVATE(object)->bounds.rmin));
			break;
		case PROP_BRX:
			g_value_set_double(value, (GTK_PLOT_POLAR_GET_PRIVATE(object)->bounds.rmax));
			break;
		case PROP_BTN:
			if (((GTK_PLOT_POLAR(object)->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) g_value_set_double(value, I180_MY_PI*(GTK_PLOT_POLAR_GET_PRIVATE(object)->bounds.thmin));
			else g_value_set_double(value, (GTK_PLOT_POLAR_GET_PRIVATE(object)->bounds.thmin));
			break;
		case PROP_BTX:
			if (((GTK_PLOT_POLAR(object)->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) g_value_set_double(value, I180_MY_PI*(GTK_PLOT_POLAR_GET_PRIVATE(object)->bounds.thmax));
			else g_value_set_double(value, (GTK_PLOT_POLAR_GET_PRIVATE(object)->bounds.thmax));
			break;
		case PROP_CR:
			g_value_set_double(value, (GTK_PLOT_POLAR_GET_PRIVATE(object)->centre.r));
			break;
		case PROP_CT:
			if (((GTK_PLOT_POLAR(object)->flagd)&GTK_PLOT_POLAR_DISP_RDN)==0) g_value_set_double(value, (GTK_PLOT_POLAR_GET_PRIVATE(object)->centre.th));
			else g_value_set_double(value, I180_MY_PI*(GTK_PLOT_POLAR_GET_PRIVATE(object)->centre.th));
			break;
		case PROP_RT:
			g_value_set_uint(value, (GTK_PLOT_POLAR_GET_PRIVATE(object)->ticks.r));
			break;
		case PROP_ZIT:
			g_value_set_uint(value, (GTK_PLOT_POLAR_GET_PRIVATE(object)->ticks.zin));
			break;
		case PROP_ZTM:
			g_value_set_uint(value, (GTK_PLOT_POLAR_GET_PRIVATE(object)->ticks.z2m));
			break;
		case PROP_ZC:
		 	g_value_set_uint(value, (GTK_PLOT_POLAR_GET_PRIVATE(object)->ticks.zc));
			break;
		case PROP_RC: 
			g_value_set_uint(value, (GTK_PLOT_POLAR_GET_PRIVATE(object)->rcs));
			break;
		case PROP_TC:
		 	g_value_set_uint(value, (GTK_PLOT_POLAR_GET_PRIVATE(object)->thcs));
			break;
		case PROP_HV: 
			g_value_set_boolean(value, (GTK_PLOT_POLAR_GET_PRIVATE(object)->hv));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static gboolean gtk_plot_polar_expose(GtkWidget *widget, GdkEventExpose *event) {
  cairo_t *cr;

  cr=gdk_cairo_create(gtk_widget_get_window(widget));
  cairo_rectangle(cr, (event->area.x), (event->area.y), (event->area.width), (event->area.height));
  cairo_clip(cr);
  drawz(widget, cr);
  drawk(widget, cr);
  draw(widget, cr);
  cairo_destroy(cr);
  return FALSE;
}

static void gtk_plot_polar_class_init(GtkPlotPolarClass *klass) {
  GObjectClass *obj_klass;
  GtkWidgetClass *widget_klass;

  obj_klass=G_OBJECT_CLASS(klass);
  g_type_class_add_private(obj_klass, sizeof(GtkPlotPolarPrivate));
  obj_klass->finalize     = (GObjectFinalizeFunc) gtk_plot_polar_finalise;
  obj_klass->set_property = gtk_plot_polar_set_property;
  obj_klass->get_property = gtk_plot_polar_get_property;
  g_object_class_install_property(obj_klass, PROP_BRN, g_param_spec_double( "rmin",      "Minimum r value",             "Minimum value for the radial scale", -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_BRX, g_param_spec_double( "rmax",      "Maximum r value",             "Maximum value for the radial scale", -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_BTN, g_param_spec_double( "thmin",     "Minimum theta value",         "Minimum value for the azimuthal scale", -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_BTX, g_param_spec_double( "thmax",     "Maximum theta value",         "Maximum value for the azimuthal scale", -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_CR,  g_param_spec_double( "rcnt",      "Centre r value",              "Radial value at the centre of the plot", -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_CT,  g_param_spec_double( "thcnt",     "Centre theta value",          "Azimuthal value at the centre of the plot", -G_MAXDOUBLE, G_MAXDOUBLE, 0, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_RT,  g_param_spec_uint(   "rticks",    "Radial ticks-1",              "Number of grid arcs drawn-1", 1, G_MAXINT, 4, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_ZIT, g_param_spec_uint(   "thticksin", "Inner radial lines-1",        "Number of radial lines at the centre of the circle-1", 1, G_MAXINT, 4, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_ZTM, g_param_spec_uint(   "thtickmul", "log2((ticks-1)/thticksin)+1", "Number of times rticks must be doubled to equal the number of ticks on the outer arc -1", 1, G_MAXINT, 5, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_ZC,  g_param_spec_double( "mulcrit",   "length for new radial line",  "distance an arc segment between two neighbouring radial lines needs to be before a new radial line forks off", 1, G_MAXDOUBLE, 5, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_RC,  g_param_spec_uint(   "rchar",     "radial label characters",     "Number of characters to store radial label strings", 1, 10, 5, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_TC,  g_param_spec_uint(   "thchar",    "theta label characters",      "Number of characters to store azimuthal label strings", 1, 10, 5, G_PARAM_READWRITE));
  g_object_class_install_property(obj_klass, PROP_HV,  g_param_spec_boolean("transpose", "alternate display",           "Display second set from thdata", FALSE, G_PARAM_READWRITE));
  widget_klass=GTK_WIDGET_CLASS(klass);
  widget_klass->button_press_event   = gtk_plot_polar_button_press;
  widget_klass->motion_notify_event  = gtk_plot_polar_motion_notify;
  widget_klass->button_release_event = gtk_plot_polar_button_release;
  widget_klass->expose_event         = gtk_plot_polar_expose;
  gtk_plot_polar_signals[MOVED]       = g_signal_new("moved",       G_OBJECT_CLASS_TYPE(obj_klass), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(GtkPlotPolarClass, moved),       NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
  gtk_plot_polar_signals[KEY_CHANGED] = g_signal_new("key_changed", G_OBJECT_CLASS_TYPE(obj_klass), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(GtkPlotPolarClass, key_changed), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void gtk_plot_polar_init(GtkPlotPolar *plot) {
  GtkPlotPolarPrivate *priv;

  gtk_widget_add_events(GTK_WIDGET(plot), GDK_BUTTON_PRESS_MASK|GDK_POINTER_MOTION_MASK|GDK_BUTTON_RELEASE_MASK);
  plot->rdata=plot->thdata=plot->kdata=NULL;
  plot->rlab=g_strdup("Amplitude");
  plot->thlab=g_strdup("Azimuth");
  plot->flagd=GTK_PLOT_POLAR_DISP_PTS|GTK_PLOT_POLAR_DISP_LIN;
  plot->ptsize=5;
  plot->linew=2;
  plot->zmode=GTK_PLOT_POLAR_ZOOM_RDL|GTK_PLOT_POLAR_ZOOM_AZM;
  plot->rps=plot->thps=0;
  plot->rdp=plot->thdp=2;
  priv=GTK_PLOT_POLAR_GET_PRIVATE(plot);
  priv->bounds.rmin=0.0;
  priv->bounds.rmax=1.0;
  priv->bounds.thmin=NMY_PI;
  priv->bounds.thmax=G_PI;
  priv->centre.r=0.5;
  priv->centre.th=0;
  priv->ticks.r=4;
  priv->ticks.zin=12;
  priv->ticks.z2m=2;
  priv->ticks.zc=40.0;
  priv->rcs=5;
  priv->thcs=6;
  priv->flagr=priv->flaga=0;
  priv->hv=FALSE;
}

GtkWidget *gtk_plot_polar_new(void) {return g_object_new(GTK_PLOT_TYPE_POLAR, NULL);}
