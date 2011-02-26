/***************************************************************************
 *            testplotpolar0-1-0.c
 *
 *  A program to show the capabilities of the plotpolar widget
 *  version 0.1.0
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include "plotpolar0-1-0.h"

#define NMY_PI -3.1415926535897932384626433832795028841971693993751

GtkWidget *window, *plot, *statusbar;

void prt(GtkWidget *widget, gpointer data)
  {
  GtkWidget *wfile;
  GtkFileFilter *filter;
  gchar *fout=NULL;

  wfile=gtk_file_chooser_dialog_new("Select Image File", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
  g_signal_connect(G_OBJECT(wfile), "destroy", G_CALLBACK(gtk_widget_destroy), G_OBJECT(wfile));
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(wfile), TRUE);
  filter=gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "Encapsulated Postscript (EPS)");
  gtk_file_filter_add_pattern(filter, "*.eps");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(wfile), filter);
  if (gtk_dialog_run(GTK_DIALOG(wfile))==GTK_RESPONSE_ACCEPT)
    {
    fout=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(wfile));
    plot_polar_print_eps(plot, fout);
    g_free(fout);
    }
  gtk_widget_destroy(wfile);
  }

void opd(GtkWidget *widget, gpointer data)
  {
  PlotPolar *plt;
  GtkWidget *wfile;
  GArray *x, *yb;
  gdouble xi, xf, lcl, mny, mxy;
  guint k, sal;
  gint lc;
  gchar *contents, *str, *fin=NULL;
  gchar **strary, **strat;
  GError *Err;

  wfile=gtk_file_chooser_dialog_new("Select Data File", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
  g_signal_connect(G_OBJECT(wfile), "destroy", G_CALLBACK(gtk_widget_destroy), G_OBJECT(wfile));
  if (gtk_dialog_run(GTK_DIALOG(wfile))==GTK_RESPONSE_ACCEPT)
    {
    fin=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(wfile));
    if (g_file_get_contents(fin, &contents, NULL, &Err))
      {
      strary=g_strsplit_set(contents, "\r\n", 0);
      sal=g_strv_length(strary);
      x=g_array_new(FALSE, FALSE, sizeof(gdouble));
      yb=g_array_new(FALSE, FALSE, sizeof(gdouble));
      k=0;
      lc=0;
      for (k=0; k<sal; k++)
        {
        if (!strary[k]) continue;
        if (!g_strcmp0("", strary[k])) continue;
        if (!(g_ascii_isdigit(strary[k][0])|(g_str_has_prefix(strary[k],"-")))) continue;
        strat=g_strsplit_set(strary[k], "\t,", 0);
        lcl=g_ascii_strtod(g_strstrip(strat[0]), NULL);
        g_array_append_val(x, lcl);
        if (lc==0)
          {
          if (!strat[1]) lcl=0;
          else lcl=g_ascii_strtod(g_strstrip(strat[1]), NULL);
          mny=lcl;
          mxy=lcl;
          }
        else
          {
          if (!strat[1]) lcl=0;
          else lcl=g_ascii_strtod(g_strstrip(strat[1]), NULL);
          if (lcl<mny) mny=lcl;
          else if (lcl>mxy) mxy=lcl;
          }
        g_array_append_val(yb, lcl);
        g_strfreev(strat);
        lc++;
        }
      g_strfreev(strary);
      str=g_strdup_printf("File: %s successfully loaded", fin);
      gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), str), str);
      g_free(str);
      plt=PLOT_POLAR(plot);
      plt->size=lc;
      plt->thdata=x;
      plt->rdata=yb;
      xi=g_array_index(x, gdouble, 0);
      xf=g_array_index(x, gdouble, (lc-1));
      g_array_free(x, TRUE);
      g_array_free(yb, TRUE);
      plot_polar_update_scale(plot, mny, mxy, xi, xf, 0, 0);
      }
    else
      {
      str=g_strdup_printf("Loading failed for file: %s", fin);
      gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), str), str);
      g_free(str);
      }
    g_free(contents);
    g_free(fin);
    }
  gtk_widget_destroy(wfile);
  }

void pltmv(PlotPolar *plt, gpointer data)
  {
  gchar *str;

  str=g_strdup_printf("r: %f, th: %f", plt->rps, plt->thps);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), str), str);
  g_free(str);
  }

void upg(GtkWidget *widget, gpointer data)
  {
  PlotPolar *plt;
  gint d;
  gdouble xn, xx, yn, yx, r0, th0;

  plt=PLOT_POLAR(plot);
  d=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  d++;
  (plt->flags)=(2*d)+1;
  g_object_get(G_OBJECT(plot), "rmin", &xn, "rmax", &xx, "thmin", &yn, "thmax", &yx, "rcnt", &r0, "thcnt", &th0, NULL);
  plot_polar_update_scale(plot, xn, xx, yn, yx, r0, th0);
  }

int main(int argc, char *argv[])
  {
  PlotPolar *plt;
  GtkWidget *vbox, *vbox2, *mnb, *mnu, *mni, *hpane, *butt;
  GArray *x, *y;
  guint j;
  gdouble valx, valy;
  GtkAccelGroup *accel_group=NULL;

  gtk_init(&argc, &argv);
  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "PlotPolar tester");
  g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  vbox=gtk_vbox_new(FALSE, 0); 
  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_widget_show(vbox);
  mnb=gtk_menu_bar_new();
  gtk_box_pack_start(GTK_BOX(vbox), mnb, FALSE, FALSE, 2);
  gtk_widget_show(mnb);
  mnu=gtk_menu_new();
  accel_group=gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
  mni=gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
  gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(opd), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(mnu), mni);
  gtk_widget_show(mni);
  mni=gtk_image_menu_item_new_from_stock(GTK_STOCK_PRINT, NULL);
  gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(prt), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(mnu), mni);
  gtk_widget_show(mni);
  mni=gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(mnu), mni);
  gtk_widget_show(mni);
  mni=gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group);
  gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(gtk_main_quit), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(mnu), mni);
  gtk_widget_show(mni);
  mni=gtk_menu_item_new_with_mnemonic("_File");
  gtk_widget_show(mni);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(mni), mnu);
  gtk_menu_shell_append(GTK_MENU_SHELL(mnb), mni);
  hpane=gtk_hpaned_new();
  gtk_widget_show(hpane);
  gtk_container_add(GTK_CONTAINER(vbox), hpane);
  vbox2=gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox2);
  butt=gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(butt), "lines");
  gtk_combo_box_append_text(GTK_COMBO_BOX(butt), "points");
  gtk_combo_box_append_text(GTK_COMBO_BOX(butt), "both");
  g_signal_connect(G_OBJECT(butt), "changed", G_CALLBACK(upg), NULL);
  gtk_box_pack_start(GTK_BOX(vbox2), butt, FALSE, FALSE, 2);
  gtk_widget_show(butt);
  gtk_paned_add1(GTK_PANED(hpane), vbox2);
  plot=plot_polar_new();
  g_signal_connect(plot, "moved", G_CALLBACK(pltmv), NULL);
  x=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 1024);
  y=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 1024);
  for (j=0; j<=50; j++)
    {
    valx=((gdouble)j-25)*G_PI/25;
    g_array_append_val(x, valx);
    valy=cos(32*valx);
    valy++;
    valy*=(3+cos(16*valx));
    valy*=(1+cos(4*valx))/8;
    valy++;
    valy/=3;
    g_array_append_val(y, valy);
    }
  plt=PLOT_POLAR(plot);
  (plt->flags)=7;
  (plt->size)=j;
  (plt->rdata)=y;
  (plt->thdata)=x;
  (plt->ptsize)=4;
  (plt->lfsize)=14;
  gtk_widget_show(plot);
  gtk_paned_add2(GTK_PANED(hpane), plot);
  statusbar=gtk_statusbar_new();
  gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 2);
  gtk_widget_show(statusbar);
  gtk_widget_show(window);
  /*plot_polar_update_scale_pretty(plot, 0, 1, -1, 2);*/
  gtk_main();
  return 0;
  }
