/***************************************************************************
 *            gtksurf3d.c
 *
 *  A GTK+ widget that provides rendering of a 3D surface
 *
 *  Sat Dec 04 16:15:00 2014
 *  Copyright  2014  Paul Childs
 *  <pchilds@physics.org>
 ****************************************************************************/

#include "gtksurf3d.h"
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>
#include <math.h>

#define DZE         0.0001  /* divide by zero threshold */
#define CZE         1-DZE
#define MY_180_PI   180.0/G_PI
#define GTK_SURF_3D_GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_SURF_TYPE_3D, GtkSurf3dPrivate)
G_DEFINE_TYPE(GtkSurf3d, gtk_surf_3d, GTK_TYPE_DRAWING_AREA);
typedef struct _GtkSurf3dPrivate GtkSurf3dPrivate;
struct _GtkSurf3dPrivate {gdouble rsx, rsy, rsz, sc; guint flagr;};

static void draw(GtkSurf3d *surf) {
  gdouble           r, t, p, x, y, z;
  guint             j, k;
  GtkSurf3dPrivate  *priv;

  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glColor3f(0.0, 0.0, 0.0);
  glBegin(GL_LINES);
   glVertex3f(0., 0., 0.);
   glVertex3f(1., 0., 0.);
   glVertex3f(0., 0., 0.);
   glVertex3f(0., 1., 0.);
   glVertex3f(0., 0., 0.);
   glVertex3f(0., 0., 1.);
  glEnd();
  glBegin(GL_TRIANGLES);
   glVertex3f(1.08, 0., 0.);
   glVertex3f(1., -0.02, 0.02);
   glVertex3f(1., 0.02, -0.02);
   glVertex3f(0., 1.08, 0.);
   glVertex3f(-0.02, 1., 0.02);
   glVertex3f(0.02, 1., -0.02);
   glVertex3f(0., 0., 1.08);
   glVertex3f(-0.02, 0.02, 1.);
   glVertex3f(0.02, -0.02, 1.);
  glEnd();
  if ((surf->data)&&(surf->sz1>1)&&(surf->sz2>1)&&(surf->data->len>surf->nd+surf->st1+surf->st2+(surf->dim-1)*surf->st3)) {
    priv=GTK_SURF_3D_GET_PRIVATE(surf);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glScalef(priv->sc,priv->sc,priv->sc);
    glPushAttrib (GL_POLYGON_BIT);
    if (!(surf->mode&GTK_SURF_3D_MODE_FILL)) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if (surf->mode&GTK_SURF_3D_MODE_POLAR) {
      glColor3f(0.0, 0.0, 1.0);
      for (k=0;k<surf->sz2;k++) {
        if (surf->nd+surf->st1+(k+1)*surf->st2+(surf->dim-1)*surf->st3>=surf->data->len) break;
        glBegin(GL_QUAD_STRIP);
        for (j=0;j<surf->sz1;j++) {
          if (surf->nd+(j+1)*surf->st1+(k+1)*surf->st2+(surf->dim-1)*surf->st3>=surf->data->len) break;
          t=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+k*surf->st2);
          p=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+k*surf->st2+surf->st3);
          r=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+k*surf->st2+2*surf->st3);
          x=r*cos(p)*cos(t);
          y=r*cos(p)*sin(t);
          z=r*sin(p);
          if (r<0) r=-r;
          if (r>=1.0) glColor3f(0.75, 0, 0.25);
          else if (r>0.9) glColor3f(2.5*(1.3-r), 0, 2.5*(r-0.9));
          else if (r>0.5) glColor3f(2.5*(r-0.5), 2.5*(0.9-r), 0);
          else if (r>0.1) glColor3f(0, 2.5*(r-0.1), 2.5*(0.5-r));
          else glColor3f(2.5*(0.1-r), 0, 2.5*(r+0.3));
          glVertex3f(x, y, z);
          t=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+(k+1)*surf->st2);
          p=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+(k+1)*surf->st2+surf->st3);
          r=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+(k+1)*surf->st2+2*surf->st3);
          x=r*cos(p)*cos(t);
          y=r*cos(p)*sin(t);
          z=r*sin(p);
          if (r<0) r=-r;
          if (r>=1.0) glColor3f(0.75, 0, 0.25);
          else if (r>0.9) glColor3f(2.5*(1.3-r), 0, 2.5*(r-0.9));
          else if (r>0.5) glColor3f(2.5*(r-0.5), 2.5*(0.9-r), 0);
          else if (r>0.1) glColor3f(0, 2.5*(r-0.1), 2.5*(0.5-r));
          else glColor3f(2.5*(0.1-r), 0, 2.5*(r+0.3));
          glVertex3f(x, y, z);
        }
        glEnd();
      }
    } else {
      glBegin(GL_QUAD_STRIP);
      for (k=0;k<surf->sz2;k++) {
        if (surf->nd+surf->st1+(k+1)*surf->st2+(surf->dim-1)*surf->st3>=surf->data->len) break;
        for (j=0;j<surf->sz1;j++) {
          if (surf->nd+(j+1)*surf->st1+(k+1)*surf->st2+(surf->dim-1)*surf->st3>=surf->data->len) break;
          x=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+k*surf->st2);
          y=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+k*surf->st2+surf->st3);
          z=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+k*surf->st2+2*surf->st3);
          glVertex3f(x, y, z);
          x=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+(k+1)*surf->st2);
          y=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+(k+1)*surf->st2+surf->st3);
          z=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+(k+1)*surf->st2+2*surf->st3);
          glVertex3f(x, y, z);
        }
        glEnd();
      }
    }
    glPopAttrib ();
    glPopMatrix();
  }
}

gboolean gtk_surf_3d_refresh(GtkWidget *widget) {
  GdkWindow *wdw;

  if (!(wdw=gtk_widget_get_window(widget))) return FALSE;
  gdk_window_invalidate_rect(wdw, &widget->allocation, FALSE);
  gdk_window_process_updates(wdw, TRUE);
  return FALSE;
}

static gboolean gtk_surf_3d_button_press(GtkWidget *widget, GdkEventButton *event) {
  gint              xw, yw;
  GtkSurf3dPrivate  *priv;

  priv=GTK_SURF_3D_GET_PRIVATE(widget);
  if ((priv->flagr)==0) {
    if (((xw=widget->allocation.width)<DZE)||((yw=widget->allocation.height)<DZE)) return FALSE;
    if (xw>yw) {
      priv->rsx=(2*event->x-xw)/yw;
      priv->rsy=1-2*event->y/yw;
    } else {
      priv->rsx=2*event->x/xw-1;
      priv->rsy=(yw-2*event->y)/xw;
    }
    if ((priv->rsz=priv->rsx*priv->rsx+priv->rsy*priv->rsy)<CZE) {
      priv->rsz=sqrt(1-priv->rsz);
      priv->flagr=1;
    }
  }
  return FALSE;
}

static gboolean gtk_surf_3d_button_release(GtkWidget *widget, GdkEventButton *event) {
  gdouble           dx, dy, dz;
  gint              xw, yw;
  GLfloat           mtx[16];
  GtkSurf3dPrivate  *priv;

  priv=GTK_SURF_3D_GET_PRIVATE(widget);
  if (priv->flagr) {
    if (((xw=widget->allocation.width)<DZE)||((yw=widget->allocation.height)<DZE)) return FALSE;
    if (xw>yw) {
      dx=(2*event->x-xw)/yw;
      dy=1-2*event->y/yw;
    } else {
      dx=2*event->x/xw-1;
      dy=(yw-2*event->y)/xw;
    }
    if ((dz=dx*dx+dy*dy)<CZE) {
      dz=sqrt(1-dz);
      glMatrixMode(GL_PROJECTION);
      glGetFloatv(GL_PROJECTION_MATRIX, &mtx[0]);
      glLoadIdentity();
      glRotatef(MY_180_PI*acos(dx*priv->rsx+dy*priv->rsy+dz*priv->rsz), dz*priv->rsy-dy*priv->rsz, dx*priv->rsz-dz*priv->rsx, dy*priv->rsx-dx*priv->rsy);
      glMultMatrixf(mtx);
      gtk_surf_3d_refresh(widget);
    }
    priv->flagr=0;
  }
  return FALSE;
}

static gboolean gtk_surf_3d_scroll(GtkWidget *widget, GdkEventScroll *event) {
  gdouble           dx, dy;
  gint              xw, yw;
  GtkSurf3dPrivate  *priv;

  if ((event->time<5)||((xw=widget->allocation.width)<DZE)||((yw=widget->allocation.height)<DZE)) return FALSE;
  if (xw>yw) {
    dx=(2*event->x-xw)/yw;
    dy=1-2*event->y/yw;
  } else {
    dx=2*event->x/xw-1;
    dy=(yw-2*event->y)/xw;
  }
  if (dx*dx+dy*dy>=CZE) return FALSE;
  priv=GTK_SURF_3D_GET_PRIVATE(widget);
  glMatrixMode(GL_MODELVIEW);
  if (event->direction==GDK_SCROLL_UP) priv->sc*=1.25;
  else priv->sc*=0.8;
  gtk_surf_3d_refresh(widget);
  return FALSE;
}

static gboolean gtk_surf_3d_expose(GtkWidget *widget, GdkEventExpose *event) {
  GdkGLContext  *glc;
  GdkGLDrawable *gld;

  glc=gtk_widget_get_gl_context(widget);
  gld=gtk_widget_get_gl_drawable(widget);
  if (gdk_gl_drawable_gl_begin(gld, glc)) {
    draw(GTK_SURF_3D(widget));
    if (gdk_gl_drawable_is_double_buffered(gld)) gdk_gl_drawable_swap_buffers(gld);
    else glFlush();
    gdk_gl_drawable_gl_end(gld);
  }
  return TRUE;
}

static gboolean gtk_surf_3d_configure(GtkWidget *widget, GdkEventConfigure *event) {
  GdkGLContext          *glc;
  GdkGLDrawable         *gld;
  static const GLfloat  mtx[16]={-0.707106781,-0.353553391,0.612372436,0.,0.707106781,-0.353553391,0.612372436,0.,0.,0.866025404,0.5,0.,0.,0.,0.,1.};

  glc=gtk_widget_get_gl_context(widget);
  gld=gtk_widget_get_gl_drawable(widget);
  if (gdk_gl_drawable_gl_begin(gld, glc)) {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClearDepth(1.0);
    (widget->allocation.width>widget->allocation.height)?glViewport((widget->allocation.width-widget->allocation.height)/2, 0, widget->allocation.height, widget->allocation.height):glViewport(0, (widget->allocation.height-widget->allocation.width)/2, widget->allocation.width, widget->allocation.width);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(mtx);
/* alternately:
    glRotatef(30,1.,0.,0.);
    glRotatef(-45,0.,1.,0.);
    glRotatef(-120,1.,1.,1.);
** OR:
    glRotatef(-141.290807697,0.20280301,0.489609778,0.848029011);
*/
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    gdk_gl_drawable_gl_end(gld);
  }
  return TRUE;
}

void gtk_surf_3d_set_data(GtkSurf3d *surf, GArray *pdata) {
  if (surf->data)   g_array_unref(surf->data);
  surf->data      = g_array_ref(pdata);
}

static void gtk_surf_3d_finalise(GtkSurf3d *surf) {
  if (surf->data) {
    g_array_unref(surf->data);
    surf->data=NULL;
  }
}

static void gtk_surf_3d_class_init(GtkSurf3dClass *klass) {
  GObjectClass   *obj_klass;
  GtkWidgetClass *widget_klass;

  obj_klass=G_OBJECT_CLASS(klass);
  g_type_class_add_private(obj_klass, sizeof(GtkSurf3dPrivate));
  obj_klass->finalize                = (GObjectFinalizeFunc) gtk_surf_3d_finalise;
  widget_klass=GTK_WIDGET_CLASS(klass);
  widget_klass->button_press_event   = gtk_surf_3d_button_press;
  widget_klass->button_release_event = gtk_surf_3d_button_release;
  widget_klass->scroll_event         = gtk_surf_3d_scroll;
  widget_klass->expose_event         = gtk_surf_3d_expose;
  widget_klass->configure_event      = gtk_surf_3d_configure;
}

static void gtk_surf_3d_init(GtkSurf3d *surf) {
  GtkSurf3dPrivate  *priv;
  GdkGLConfig       *glc;

  gtk_widget_add_events(GTK_WIDGET(surf), GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_SCROLL_MASK);
  glc=gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA|GDK_GL_MODE_DEPTH|GDK_GL_MODE_DOUBLE);
  gtk_widget_set_gl_capability(GTK_WIDGET(surf), glc, NULL, TRUE, GDK_GL_RGBA_TYPE);
  surf->data=NULL;
  surf->nd=0;
  surf->dim=3;
  surf->st1=3;
  surf->st2=0;
  surf->st3=1;
  surf->sz1=0;
  surf->sz2=0;
  surf->mode=0;
  priv=GTK_SURF_3D_GET_PRIVATE(surf);
  priv->flagr=0;
  priv->rsx=priv->rsy=priv->rsz=0.0;
  priv->sc=1.0;
}

void gtk_surf_3d_init_gl(int *argc, char ***argv) {gtk_gl_init(argc, argv);}

GtkWidget *gtk_surf_3d_new(void) {return g_object_new(GTK_SURF_TYPE_3D, NULL);}
