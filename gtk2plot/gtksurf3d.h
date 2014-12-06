/***************************************************************************
 *            gtksurf3d.h
 *
 *  A GTK+ widget that provides rendering of a 3D surf
 *
 *  Sat Dec 04 16:15:00 2014
 *  Copyright  2014  Paul Childs
 *  <pchilds@physics.org>
 ****************************************************************************/

#ifndef __GTK_SURF_3D_H__
# define __GTK_SURF_3D_H__
# include <gtk/gtk.h>
  G_BEGIN_DECLS
# define GTK_SURF_TYPE_3D               gtk_surf_3d_get_type()
# define GTK_SURF_3D(obj)               G_TYPE_CHECK_INSTANCE_CAST(obj, GTK_SURF_TYPE_3D, GtkSurf3d)
# define GTK_SURF_IS_3D(obj)            G_TYPE_CHECK_INSTANCE_TYPE(obj, GTK_SURF_TYPE_3D)
# define GTK_SURF_3D_CLASS(klass)       G_TYPE_CHECK_CLASS_CAST(klass,  GTK_SURF_TYPE_3D, GtkSurf3dClass)
# define GTK_SURF_IS_3D_CLASS(klass)    G_TYPE_CHECK_CLASS_TYPE(klass,  GTK_SURF_TYPE_3D)
# define GTK_SURF_GET_3D_CLASS(obj)     G_TYPE_INSTANCE_GET_CLASS(obj,  GTK_SURF_TYPE_3D, GtkSurf3dClass)
  typedef struct _GtkSurf3d GtkSurf3d;
  typedef struct _GtkSurf3dClass GtkSurf3dClass;
  typedef enum {
    GTK_SURF_3D_MODE_POLAR  = 1 << 0,
    GTK_SURF_3D_MODE_FILL   = 1 << 1
  } GtkSurf3dMode;
  struct _GtkSurf3d {
    GtkDrawingArea  parent;
    GArray          *data;
    guint           dim, nd, st1, st2, st3, sz1, sz2;
    guint           mode;
  };
  struct _GtkSurf3dClass {GtkDrawingAreaClass parent_class;};
  gboolean      gtk_surf_3d_refresh(GtkWidget *widget);
  void          gtk_surf_3d_set_data(GtkSurf3d *surf, GArray *pd);
  void          gtk_surf_3d_init_gl(int *argc, char ***argv);
  GtkWidget*    gtk_surf_3d_new(void);
  extern GType  gtk_surf_3d_get_type(void);
  G_END_DECLS
#endif
