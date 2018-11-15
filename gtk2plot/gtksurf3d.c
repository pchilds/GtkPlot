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
#include <glib/gprintf.h> // for writing ppm
#include <glib/gstdio.h> // for writing ppm

#define DZE         0.0001  /* divide by zero threshold */
#define CZE         1-DZE
#define MY_180_PI   180.0/G_PI
#define GTK_SURF_3D_GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_SURF_TYPE_3D, GtkSurf3dPrivate)
G_DEFINE_TYPE(GtkSurf3d, gtk_surf_3d, GTK_TYPE_DRAWING_AREA);
typedef struct _GtkSurf3dPrivate GtkSurf3dPrivate;
struct _GtkSurf3dPrivate {gdouble rsx, rsy, rsz, scp; guint flagr; GTimer* tm;};

static void draw(GtkSurf3d *surf) {
  gdouble           a1, r, r2, t, p, x1, x2, x3, x4, y1, y2, y3, y4, z1, z2, z3, z4;
  guint             j, k;

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
  if ((surf->data)&&((((surf->mode&GTK_SURF_3D_MODE_EDGES)==0)&&(surf->sz1>0)&&(surf->data->len>surf->nd+(surf->dim-1)*surf->st3))||((surf->sz1>1)&&(surf->sz2>1)&&(surf->data->len>surf->nd+surf->st1+surf->st2+(surf->dim-1)*surf->st3)))) {
    glPushAttrib(GL_POLYGON_BIT);
    if ((surf->mode&GTK_SURF_3D_MODE_EDGES)) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      if (surf->mode&GTK_SURF_3D_MODE_POLAR) {
        for (k=0;k<surf->sz2;k++) {
          if (surf->nd+surf->st1+(k+1)*surf->st2+(surf->dim-1)*surf->st3>=surf->data->len) break;
          glBegin(GL_QUAD_STRIP);
          for (j=0;j<surf->sz1;j++) {
            if (surf->nd+(j+1)*surf->st1+(k+1)*surf->st2+(surf->dim-1)*surf->st3>=surf->data->len) break;
            t=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+k*surf->st2);
            p=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+k*surf->st2+surf->st3);
            r=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+k*surf->st2+2*surf->st3);
            x1=r*cos(p)*cos(t);
            y1=r*cos(p)*sin(t);
            z1=r*sin(p);
            if (r<0) r=-r;
            if (r>=1.0) glColor3f(0.75, 0, 0.25);
            else if (r>0.9) glColor3f(2.5*(1.3-r), 0, 2.5*(r-0.9));
            else if (r>0.5) glColor3f(2.5*(r-0.5), 2.5*(0.9-r), 0);
            else if (r>0.1) glColor3f(0, 2.5*(r-0.1), 2.5*(0.5-r));
            else glColor3f(2.5*(0.1-r), 0, 2.5*(r+0.3));
            t=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+(k+1)*surf->st2);
            p=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+(k+1)*surf->st2+surf->st3);
            r2=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+(k+1)*surf->st2+2*surf->st3);
            x2=r2*cos(p)*cos(t);
            y2=r2*cos(p)*sin(t);
            z2=r2*sin(p);
            if (j==0) {
              t=g_array_index(surf->data, gdouble, surf->nd+surf->st1+k*surf->st2);
              p=g_array_index(surf->data, gdouble, surf->nd+surf->st1+k*surf->st2+surf->st3);
              r=g_array_index(surf->data, gdouble, surf->nd+surf->st1+k*surf->st2+2*surf->st3);
              x3=r*cos(p)*cos(t);
              y3=r*cos(p)*sin(t);
              z3=r*sin(p);
              glNormal3f((y2-y1)*(z3-z1)+(y3-y1)*(z1-z2), (z2-z1)*(x3-x1)+(z3-z1)*(x1-x2), (x2-x1)*(y3-y1)+(x3-x1)*(y1-y2));
            } else if (j+1==surf->sz1) {
              t=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+k*surf->st2);
              p=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+k*surf->st2+surf->st3);
              r=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+k*surf->st2+2*surf->st3);
              x4=r*cos(p)*cos(t);
              y4=r*cos(p)*sin(t);
              z4=r*sin(p);
              glNormal3f((y2-y1)*(z1-z4)+(y1-y4)*(z1-z2), (z2-z1)*(x1-x4)+(z1-z4)*(x1-x2), (x2-x1)*(y1-y4)+(x1-x4)*(y1-y2));
            } else {
              t=g_array_index(surf->data, gdouble, surf->nd+(j+1)*surf->st1+k*surf->st2);
              p=g_array_index(surf->data, gdouble, surf->nd+(j+1)*surf->st1+k*surf->st2+surf->st3);
              r=g_array_index(surf->data, gdouble, surf->nd+(j+1)*surf->st1+k*surf->st2+2*surf->st3);
              x3=r*cos(p)*cos(t);
              y3=r*cos(p)*sin(t);
              z3=r*sin(p);
              t=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+k*surf->st2);
              p=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+k*surf->st2+surf->st3);
              r=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+k*surf->st2+2*surf->st3);
              x4=r*cos(p)*cos(t);
              y4=r*cos(p)*sin(t);
              z4=r*sin(p);
              glNormal3f((y2-y1)*(z3-z4)+(y3-y4)*(z1-z2), (z2-z1)*(x3-x4)+(z3-z4)*(x1-x2), (x2-x1)*(y3-y4)+(x3-x4)*(y1-y2));
            }
            glVertex3f(x1, y1, z1);
            if (r2<0) r2=-r2;
            if (r2>=1.0) glColor3f(0.75, 0, 0.25);
            else if (r2>0.9) glColor3f(2.5*(1.3-r2), 0, 2.5*(r2-0.9));
            else if (r2>0.5) glColor3f(2.5*(r2-0.5), 2.5*(0.9-r2), 0);
            else if (r2>0.1) glColor3f(0, 2.5*(r2-0.1), 2.5*(0.5-r2));
            else glColor3f(2.5*(0.1-r2), 0, 2.5*(r2+0.3));
            if (j==0) {
              t=g_array_index(surf->data, gdouble, surf->nd+surf->st1+(k+1)*surf->st2);
              p=g_array_index(surf->data, gdouble, surf->nd+surf->st1+(k+1)*surf->st2+surf->st3);
              r=g_array_index(surf->data, gdouble, surf->nd+surf->st1+(k+1)*surf->st2+2*surf->st3);
              x3=r*cos(p)*cos(t);
              y3=r*cos(p)*sin(t);
              z3=r*sin(p);
              glNormal3f((y2-y1)*(z3-z2)+(y3-y2)*(z1-z2), (z2-z1)*(x3-x2)+(z3-z2)*(x1-x2), (x2-x1)*(y3-y2)+(x3-x2)*(y1-y2));
            } else if (j+1==surf->sz1) {
              t=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+(k+1)*surf->st2);
              p=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+(k+1)*surf->st2+surf->st3);
              r=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+(k+1)*surf->st2+2*surf->st3);
              x4=r*cos(p)*cos(t);
              y4=r*cos(p)*sin(t);
              z4=r*sin(p);
              glNormal3f((y2-y1)*(z2-z4)+(y2-y4)*(z1-z2), (z2-z1)*(x2-x4)+(z2-z4)*(x1-x2), (x2-x1)*(y2-y4)+(x2-x4)*(y1-y2));
            } else {
              t=g_array_index(surf->data, gdouble, surf->nd+(j+1)*surf->st1+(k+1)*surf->st2);
              p=g_array_index(surf->data, gdouble, surf->nd+(j+1)*surf->st1+(k+1)*surf->st2+surf->st3);
              r=g_array_index(surf->data, gdouble, surf->nd+(j+1)*surf->st1+(k+1)*surf->st2+2*surf->st3);
              x3=r*cos(p)*cos(t);
              y3=r*cos(p)*sin(t);
              z3=r*sin(p);
              t=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+(k+1)*surf->st2);
              p=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+(k+1)*surf->st2+surf->st3);
              r=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+(k+1)*surf->st2+2*surf->st3);
              x4=r*cos(p)*cos(t);
              y4=r*cos(p)*sin(t);
              z4=r*sin(p);
              glNormal3f((y2-y1)*(z3-z4)+(y3-y4)*(z1-z2), (z2-z1)*(x3-x4)+(z3-z4)*(x1-x2), (x2-x1)*(y3-y4)+(x3-x4)*(y1-y2));
            }
            glVertex3f(x2, y2, z2);
          }
          glEnd();
        }
      } else {
        for (k=0;k<surf->sz2;k++) {
          if (surf->nd+surf->st1+(k+1)*surf->st2+(surf->dim-1)*surf->st3>=surf->data->len) break;
          glBegin(GL_QUAD_STRIP);
          for (j=0;j<surf->sz1;j++) {
            if (surf->nd+(j+1)*surf->st1+(k+1)*surf->st2+(surf->dim-1)*surf->st3>=surf->data->len) break;
            x1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+k*surf->st2);
            y1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+k*surf->st2+surf->st3);
            z1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+k*surf->st2+2*surf->st3);
            x2=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+(k+1)*surf->st2);
            y2=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+(k+1)*surf->st2+surf->st3);
            z2=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+(k+1)*surf->st2+2*surf->st3);
            r=sqrt(x1*x1+y1*y1+z1*z1);
            if (r>=1.0) glColor3f(0.75, 0, 0.25);
            else if (r>0.9) glColor3f(2.5*(1.3-r), 0, 2.5*(r-0.9));
            else if (r>0.5) glColor3f(2.5*(r-0.5), 2.5*(0.9-r), 0);
            else if (r>0.1) glColor3f(0, 2.5*(r-0.1), 2.5*(0.5-r));
            else glColor3f(2.5*(0.1-r), 0, 2.5*(r+0.3));
            if (j==0) {
              x3=g_array_index(surf->data, gdouble, surf->nd+surf->st1+k*surf->st2);
              y3=g_array_index(surf->data, gdouble, surf->nd+surf->st1+k*surf->st2+surf->st3);
              z3=g_array_index(surf->data, gdouble, surf->nd+surf->st1+k*surf->st2+2*surf->st3);
              glNormal3f((y2-y1)*(z3-z1)+(y3-y1)*(z1-z2), (z2-z1)*(x3-x1)+(z3-z1)*(x1-x2), (x2-x1)*(y3-y1)+(x3-x1)*(y1-y2));
            } else if (j+1==surf->sz1) {
              x4=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+k*surf->st2);
              y4=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+k*surf->st2+surf->st3);
              z4=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+k*surf->st2+2*surf->st3);
              glNormal3f((y2-y1)*(z1-z4)+(y1-y4)*(z1-z2), (z2-z1)*(x1-x4)+(z1-z4)*(x1-x2), (x2-x1)*(y1-y4)+(x1-x4)*(y1-y2));
            } else {
              x3=g_array_index(surf->data, gdouble, surf->nd+(j+1)*surf->st1+k*surf->st2);
              y3=g_array_index(surf->data, gdouble, surf->nd+(j+1)*surf->st1+k*surf->st2+surf->st3);
              z3=g_array_index(surf->data, gdouble, surf->nd+(j+1)*surf->st1+k*surf->st2+2*surf->st3);
              x4=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+k*surf->st2);
              y4=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+k*surf->st2+surf->st3);
              z4=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+k*surf->st2+2*surf->st3);
              glNormal3f((y2-y1)*(z3-z4)+(y3-y4)*(z1-z2), (z2-z1)*(x3-x4)+(z3-z4)*(x1-x2), (x2-x1)*(y3-y4)+(x3-x4)*(y1-y2));
            }
            glVertex3f(x1, y1, z1);
            r=sqrt(x2*x2+y2*y2+z2*z2);
            if (r>=1.0) glColor3f(0.75, 0, 0.25);
            else if (r>0.9) glColor3f(2.5*(1.3-r), 0, 2.5*(r-0.9));
            else if (r>0.5) glColor3f(2.5*(r-0.5), 2.5*(0.9-r), 0);
            else if (r>0.1) glColor3f(0, 2.5*(r-0.1), 2.5*(0.5-r));
            else glColor3f(2.5*(0.1-r), 0, 2.5*(r+0.3));
            if (j==0) {
              x3=g_array_index(surf->data, gdouble, surf->nd+surf->st1+(k+1)*surf->st2);
              y3=g_array_index(surf->data, gdouble, surf->nd+surf->st1+(k+1)*surf->st2+surf->st3);
              z3=g_array_index(surf->data, gdouble, surf->nd+surf->st1+(k+1)*surf->st2+2*surf->st3);
              glNormal3f((y2-y1)*(z3-z2)+(y3-y2)*(z1-z2), (z2-z1)*(x3-x2)+(z3-z2)*(x1-x2), (x2-x1)*(y3-y2)+(x3-x2)*(y1-y2));
            } else if (j+1==surf->sz1) {
              x4=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+(k+1)*surf->st2);
              y4=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+(k+1)*surf->st2+surf->st3);
              z4=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+(k+1)*surf->st2+2*surf->st3);
              glNormal3f((y2-y1)*(z2-z4)+(y2-y4)*(z1-z2), (z2-z1)*(x2-x4)+(z2-z4)*(x1-x2), (x2-x1)*(y2-y4)+(x2-x4)*(y1-y2));
            } else {
              x3=g_array_index(surf->data, gdouble, surf->nd+(j+1)*surf->st1+(k+1)*surf->st2);
              y3=g_array_index(surf->data, gdouble, surf->nd+(j+1)*surf->st1+(k+1)*surf->st2+surf->st3);
              z3=g_array_index(surf->data, gdouble, surf->nd+(j+1)*surf->st1+(k+1)*surf->st2+2*surf->st3);
              x4=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+(k+1)*surf->st2);
              y4=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+(k+1)*surf->st2+surf->st3);
              z4=g_array_index(surf->data, gdouble, surf->nd+(j-1)*surf->st1+(k+1)*surf->st2+2*surf->st3);
              glNormal3f((y2-y1)*(z3-z4)+(y3-y4)*(z1-z2), (z2-z1)*(x3-x4)+(z3-z4)*(x1-x2), (x2-x1)*(y3-y4)+(x3-x4)*(y1-y2));
            }
            glVertex3f(x2, y2, z2);
          }
          glEnd();
        }
      }
    } else {
      glDisable(GL_PROGRAM_POINT_SIZE);
      if (surf->dim>3) {
        switch (surf->mode&GTK_SURF_3D_MODE_DIM4_MASK) {
          case GTK_SURF_3D_MODE_SIZE:
            if (surf->mode&GTK_SURF_3D_MODE_POLAR) {
              for (j=0;j<surf->sz1;j++) {
                if (surf->nd+j*surf->st1+(surf->dim-1)*surf->st3>=surf->data->len) break;
                t=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1);
                p=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+surf->st3);
                r=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+2*surf->st3);
                a1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+3*surf->st3);
                x1=r*cos(p)*cos(t);
                y1=r*cos(p)*sin(t);
                z1=r*sin(p);
                glPointSize(a1*GTK_SURF_3D_GET_PRIVATE(surf)->scp*surf->szp);
                glBegin(GL_POINTS);
                glColor3f(0.f, 1.f, 0.f);
                glVertex3f(x1, y1, z1);
                glEnd();
              }
            } else {
              for (j=0;j<surf->sz1;j++) {
                if (surf->nd+j*surf->st1+(surf->dim-1)*surf->st3>=surf->data->len) break;
                x1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1);
                y1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+surf->st3);
                z1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+2*surf->st3);
                a1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+3*surf->st3);
                glPointSize(a1*GTK_SURF_3D_GET_PRIVATE(surf)->scp*surf->szp);
                glBegin(GL_POINTS);
                glColor3f(0.f, 1.f, 0.f);
                glVertex3f(x1, y1, z1);
                glEnd();
              }
            }
            break;
          case GTK_SURF_3D_MODE_COLOUR:
            glPointSize((surf->szp>0)?GTK_SURF_3D_GET_PRIVATE(surf)->scp*surf->szp:1);
            glBegin(GL_POINTS);
            if (surf->mode&GTK_SURF_3D_MODE_POLAR) {
              for (j=0;j<surf->sz1;j++) {
                if (surf->nd+j*surf->st1+(surf->dim-1)*surf->st3>=surf->data->len) break;
                t=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1);
                p=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+surf->st3);
                r=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+2*surf->st3);
                a1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+3*surf->st3);
                x1=r*cos(p)*cos(t);
                y1=r*cos(p)*sin(t);
                z1=r*sin(p);
                glColor3f((a1 < 0.5) ? 1.-2.*a1 : 0.f, (a1<0.5) ? 2.*a1 : 2.-2.*a1, (a1<0.5) ? 0.f : 2.*a1-1.);
                glVertex3f(x1, y1, z1);
              }
            } else {
              for (j=0;j<surf->sz1;j++) {
                if (surf->nd+j*surf->st1+(surf->dim-1)*surf->st3>=surf->data->len) break;
                x1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1);
                y1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+surf->st3);
                z1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+2*surf->st3);
                a1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+3*surf->st3);
                glColor3f((a1 < 0.5) ? 1.-2.*a1 : 0.f, (a1<0.5) ? 2.*a1 : 2.-2.*a1, (a1<0.5) ? 0.f : 2.*a1-1.);
                glVertex3f(x1, y1, z1);
              }
            }
            glEnd();
            break;
          case GTK_SURF_3D_MODE_ALPHA:
            glPointSize((surf->szp>0)?GTK_SURF_3D_GET_PRIVATE(surf)->scp*surf->szp:1);
            glBegin(GL_POINTS);
            glEnd();
            break;
          default :
            break;
        }
      } else {
        glPointSize((surf->szp>0)?GTK_SURF_3D_GET_PRIVATE(surf)->scp*surf->szp:1);
        glBegin(GL_POINTS);
        glColor3f(0.f, 1.f, 0.f);
        if (surf->mode&GTK_SURF_3D_MODE_POLAR) {
          for (j=0;j<surf->sz1;j++) {
            if (surf->nd+j*surf->st1+(surf->dim-1)*surf->st3>=surf->data->len) break;
            t=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1);
            p=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+surf->st3);
            r=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+2*surf->st3);
            x1=r*cos(p)*cos(t);
            y1=r*cos(p)*sin(t);
            z1=r*sin(p);
            glVertex3f(x1, y1, z1);
          }
        } else {
          for (j=0;j<surf->sz1;j++) {
            if (surf->nd+j*surf->st1+(surf->dim-1)*surf->st3>=surf->data->len) break;
            x1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1);
            y1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+surf->st3);
            z1=g_array_index(surf->data, gdouble, surf->nd+j*surf->st1+2*surf->st3);
            glVertex3f(x1, y1, z1);
          }
        }
        glEnd();
      }
    }
    glPopAttrib();
  }
}

gboolean gtk_surf_3d_refresh(GtkWidget *widget) {
  GdkWindow *wdw;

  if (!(wdw=gtk_widget_get_window(widget))) return FALSE;
  gdk_window_invalidate_rect(wdw, &widget->allocation, FALSE);
  gdk_window_process_updates(wdw, TRUE);
  return FALSE;
}

gboolean gtk_surf_3d_print_png(GtkWidget *widget, gchar* fout) {
  cairo_matrix_t mat;
  cairo_pattern_t *pat;
  cairo_surface_t *dst, *src;
  cairo_t *cr;
  GdkGLContext  *glc;
  GdkGLDrawable *gld;
  gint height, width, width2;
  unsigned char * pixels = 0;

  glc=gtk_widget_get_gl_context(widget);
  gld=gtk_widget_get_gl_drawable(widget);
  if (gdk_gl_drawable_gl_begin(gld, glc)) {
    draw(GTK_SURF_3D(widget));
    if (gdk_gl_drawable_is_double_buffered(gld)) gdk_gl_drawable_swap_buffers(gld);
    else glFlush();
    gdk_gl_drawable_get_size(gld,&width,&height);
    width2 = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, width);
    pixels = (unsigned char *)g_malloc(4*width2*height);
    glReadPixels(0,0,width,height,GL_BGRA,GL_UNSIGNED_BYTE,pixels);
    src=cairo_image_surface_create_for_data(pixels, CAIRO_FORMAT_RGB24, width, height, width2);
    pat=cairo_pattern_create_for_surface(src);
    cairo_matrix_init(&mat,1.,0.,0.,-1.,0.,height);
    cairo_pattern_set_matrix(pat,&mat);
    dst=cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
    cr=cairo_create(dst);
    cairo_set_source(cr,pat);
    cairo_paint(cr);
    cairo_surface_write_to_png(dst, fout);
    cairo_pattern_destroy(pat);
    cairo_surface_destroy(dst);
    cairo_surface_destroy(src);
    g_free(pixels);
    gdk_gl_drawable_gl_end(gld);
  }
  return FALSE;
}

gboolean gtk_surf_3d_print_ppm(GtkWidget *widget, gchar* fout) {
  FILE *f = 0;
  GdkGLContext  *glc;
  GdkGLDrawable *gld;
  gint j, k, width, height;
  unsigned char * pixels = 0;
  unsigned char * pixelIt;

  glc=gtk_widget_get_gl_context(widget);
  gld=gtk_widget_get_gl_drawable(widget);
  if (gdk_gl_drawable_gl_begin(gld, glc)) {
    draw(GTK_SURF_3D(widget));
    if (gdk_gl_drawable_is_double_buffered(gld)) gdk_gl_drawable_swap_buffers(gld);
    else glFlush();
    gdk_gl_drawable_get_size(gld,&width,&height);
    pixels = (unsigned char *)g_malloc(4*width*height);
    glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    f = g_fopen(fout, "w");
    g_fprintf(f, "P3\n%d %d\n255\n", width, height);
    pixelIt = pixels + 4 * (height + 1) * width;
    for (j = 0; j < height; j++) {
      pixelIt -= 8 * width;
      for (k = 0; k < width; k++) {
        g_fprintf(f, "%3d ", *(pixelIt++));
        g_fprintf(f, "%3d ", *(pixelIt++));
        g_fprintf(f, "%3d ", *(pixelIt++));
        pixelIt++;
      }
      g_fprintf(f, "\n");
    }
    fclose(f);
    g_free(pixels);
    gdk_gl_drawable_gl_end(gld);
  }
  return FALSE;
}

static gboolean gtk_surf_3d_rotate(GtkWidget *widget, gdouble dx, gdouble dy) {
  gdouble           dz;
  gdouble           dot;
  GLfloat           mtx[16];
  GtkSurf3dPrivate  *priv;

  if ((dz=dx*dx+dy*dy)<CZE) {
    dz=sqrt(1-dz);
    priv=GTK_SURF_3D_GET_PRIVATE(widget);
    dot=dx*priv->rsx+dy*priv->rsy+dz*priv->rsz;
    if (dot<CZE && dot>-CZE) {
      glMatrixMode(GL_PROJECTION);
      glGetFloatv(GL_PROJECTION_MATRIX, mtx);
      glLoadIdentity();
      glRotatef(MY_180_PI*acos(dot), dy*priv->rsz-dz*priv->rsy, dz*priv->rsx-dx*priv->rsz, dy*priv->rsx-dx*priv->rsy);
      glMultMatrixf(mtx);
      priv->rsx=dx;
      priv->rsy=dy;
      priv->rsz=dz;
      gtk_surf_3d_refresh(widget);
    }
  }
  return FALSE;
}

static gboolean gtk_surf_3d_translate(GtkWidget *widget, gdouble dx, gdouble dy) {
  GLfloat           mtx[16];
  GtkSurf3dPrivate  *priv;

  if ((dx*dx+dy*dy)<CZE) {
    priv=GTK_SURF_3D_GET_PRIVATE(widget);
    glMatrixMode(GL_PROJECTION);
    glGetFloatv(GL_PROJECTION_MATRIX, mtx);
    mtx[12]+=dx-priv->rsx;
    mtx[13]+=dy-priv->rsy;
    priv->rsx=dx;
    priv->rsy=dy;
    glLoadMatrixf(mtx);
    gtk_surf_3d_refresh(widget);
  }
  return FALSE;
}

static gboolean gtk_surf_3d_translate_or_rotate(GtkWidget *widget, gdouble x, gdouble y) {
  gdouble           dx, dy;
  gint              xw, yw;

  if (((xw=widget->allocation.width)<DZE)||((yw=widget->allocation.height)<DZE)) return FALSE;
  if (xw>yw) {
    dx=(2*x-xw)/yw;
    dy=1-2*y/yw;
  } else {
    dx=2*x/xw-1;
    dy=(yw-2*y)/xw;
  }
  if (GTK_SURF_3D_GET_PRIVATE(widget)->flagr==1) gtk_surf_3d_rotate(widget,dx,dy);
  else gtk_surf_3d_translate(widget,dx,dy);
  return FALSE;
}

static gboolean gtk_surf_3d_scroll(GtkWidget *widget, GdkEventScroll *event) {
  gdouble              dx, dy;
  gint                 xw, yw;
  GLfloat              mtx[16];
  static const GLfloat scaleUpMtx[16] = {1.25f,0.f,0.f,0.f,0.f,1.25f,0.f,0.f,0.f,0.f,1.25f,0.f,0.f,0.f,0.f,1.f};
  static const GLfloat scaleDnMtx[16] = {0.8f ,0.f,0.f,0.f,0.f,0.8f ,0.f,0.f,0.f,0.f,0.8f ,0.f,0.f,0.f,0.f,1.f};

  if ((event->time<5)||((xw=widget->allocation.width)<DZE)||((yw=widget->allocation.height)<DZE)) return FALSE;
  if (xw>yw) {
    dx=(2*event->x-xw)/yw;
    dy=1-2*event->y/yw;
  } else {
    dx=2*event->x/xw-1;
    dy=(yw-2*event->y)/xw;
  }
  if (dx*dx+dy*dy>=CZE) return FALSE;
  glMatrixMode(GL_MODELVIEW);
  glGetFloatv(GL_MODELVIEW_MATRIX, mtx);
  if (event->direction==GDK_SCROLL_UP) {
    glLoadMatrixf(scaleUpMtx);
    GTK_SURF_3D_GET_PRIVATE(widget)->scp*=1.25;
  } else {
    glLoadMatrixf(scaleDnMtx);
    GTK_SURF_3D_GET_PRIVATE(widget)->scp*=0.8;
  }
  glMultMatrixf(mtx);
  gtk_surf_3d_refresh(widget);
  return FALSE;
}

static gboolean gtk_surf_3d_motion_notify(GtkWidget *widget, GdkEventMotion *event) {
  GtkSurf3dPrivate  *priv;

  priv=GTK_SURF_3D_GET_PRIVATE(widget);
  if (priv->flagr && g_timer_elapsed(priv->tm,NULL) > 0.25) {
    gtk_surf_3d_translate_or_rotate(widget,event->x,event->y);
    g_timer_reset(priv->tm);
  }
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
      priv->flagr=event->button;
      g_timer_start(priv->tm);
    } else priv->rsz=0;
  }
  return FALSE;
}

static gboolean gtk_surf_3d_button_release(GtkWidget *widget, GdkEventButton *event) {
  GtkSurf3dPrivate  *priv;

  priv=GTK_SURF_3D_GET_PRIVATE(widget);
  if (priv->flagr) {
    gtk_surf_3d_translate_or_rotate(widget,event->x,event->y);
    priv->flagr=0;
    g_timer_stop(priv->tm);
  }
  return FALSE;
}

static gboolean gtk_surf_3d_expose(GtkWidget *widget, GdkEventExpose *event) {
  GdkGLContext  *glc;
  GdkGLDrawable *gld;

  (void)event;
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
  static const GLfloat  mtx[16]={-0.707106781f,-0.353553391f,-0.612372436f,0.f,0.707106781f,-0.353553391f,-0.612372436,0.f,0.f,0.866025404f,-0.5f,0.f,0.f,0.f,0.f,1.f};

  (void)event;
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
  g_timer_destroy(GTK_SURF_3D_GET_PRIVATE(surf)->tm);
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
  widget_klass->configure_event      = gtk_surf_3d_configure;
  widget_klass->expose_event         = gtk_surf_3d_expose;
  widget_klass->motion_notify_event  = gtk_surf_3d_motion_notify;
  widget_klass->scroll_event         = gtk_surf_3d_scroll;
}

static void gtk_surf_3d_init(GtkSurf3d *surf) {
  GtkSurf3dPrivate  *priv;
  GdkGLConfig       *glc;

  gtk_widget_add_events(GTK_WIDGET(surf), GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|GDK_POINTER_MOTION_MASK|GDK_BUTTON_RELEASE_MASK|GDK_SCROLL_MASK);
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
  surf->szp=1;
  surf->mode=0;
  priv=GTK_SURF_3D_GET_PRIVATE(surf);
  priv->flagr=0;
  priv->rsx=priv->rsy=priv->rsz=0.;
  priv->scp=1.;
  priv->tm=g_timer_new();
}

void gtk_surf_3d_init_gl(int *argc, char ***argv) {gtk_gl_init(argc, argv);}

GtkWidget *gtk_surf_3d_new(void) {return g_object_new(GTK_SURF_TYPE_3D, NULL);}
