/***************************************************************************
 *            testplotlinear0-1-0.c
 *
 *  A program to show the capabilities of the plotlinear widget
 *  version 0.1.0
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
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include "plotlinear0-1-0.h"

GtkWidget *window, *plot, *statusbar, *jind, *label;
GArray *x, *y, *sz, *nx;

void dpr(GtkWidget *widget, gpointer data)
{
	GtkWidget *helpwin, *content, *table, *entry1, *entry2, *label, *spin1, *spin2;
	GtkAdjustment *adj1, *adj2;
	PlotLinear *plt;
	gdouble xi, xf, mny, mxy;
	
	helpwin=gtk_dialog_new_with_buttons("Dsiplay Properties", GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL, GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, NULL);
	g_signal_connect_swapped(G_OBJECT(helpwin), "destroy", G_CALLBACK(gtk_widget_destroy), G_OBJECT(helpwin));
	gtk_widget_show(helpwin);
	content=gtk_dialog_get_content_area(GTK_DIALOG(helpwin));
	table=gtk_table_new(4, 2, FALSE);
	gtk_widget_show(table);
	label=gtk_label_new("X axis text:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	label=gtk_label_new("Text size:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	label=gtk_label_new("Y axis text:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	label=gtk_label_new("Tick label size:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, 2, 3, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	entry1=gtk_entry_new();
	entry2=gtk_entry_new();
	plt=PLOT_LINEAR(plot);
	gtk_entry_set_text(GTK_ENTRY(entry1), g_strdup(plt->xlab));
	gtk_entry_set_text(GTK_ENTRY(entry2), g_strdup(plt->ylab));
	adj1=(GtkAdjustment *) gtk_adjustment_new((plt->lfsize), 8, 64, 1.0, 5.0, 0.0);
	adj2=(GtkAdjustment *) gtk_adjustment_new((plt->afsize), 8, 64, 1.0, 5.0, 0.0);
	spin1=gtk_spin_button_new(adj1, 0, 0);
	spin2=gtk_spin_button_new(adj2, 0, 0);
	gtk_widget_show(entry1);
	gtk_widget_show(entry2);
	gtk_widget_show(spin1);
	gtk_widget_show(spin2);
	gtk_table_attach(GTK_TABLE(table), entry1, 0, 1, 1, 2, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	gtk_table_attach(GTK_TABLE(table), entry2, 0, 1, 3, 4, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	gtk_table_attach(GTK_TABLE(table), spin1, 1, 2, 1, 2, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	gtk_table_attach(GTK_TABLE(table), spin2, 1, 2, 3, 4, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	gtk_container_add(GTK_CONTAINER(content), table);
	if (gtk_dialog_run(GTK_DIALOG(helpwin))==GTK_RESPONSE_APPLY)
	{
		(plt->xlab)=g_strdup(gtk_entry_get_text(GTK_ENTRY(entry1)));
		(plt->ylab)=g_strdup(gtk_entry_get_text(GTK_ENTRY(entry2)));
		(plt->lfsize)=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin1));
		(plt->afsize)=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin2));
		g_object_get(G_OBJECT(plot), "xmin", &xi, "xmax", &xf, "ymin", &mny, "ymax", &mxy, NULL);
		plot_linear_update_scale(plot, xi, xf, mny, mxy);
	}
	gtk_widget_destroy(helpwin);
}

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
		plot_linear_print_eps(plot, fout);
		g_free(fout);
	}
	gtk_widget_destroy(wfile);
}

void opd(GtkWidget *widget, gpointer data)
{
	PlotLinear *plt;
	GtkWidget *wfile;
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
			g_array_free(x, TRUE);
			g_array_free(y, TRUE);
			g_array_free(sz, TRUE);
			g_array_free(nx, TRUE);
			x=g_array_new(FALSE, FALSE, sizeof(gdouble));
			y=g_array_new(FALSE, FALSE, sizeof(gdouble));
			sz=g_array_new(FALSE, FALSE, sizeof(gint));
			nx=g_array_new(FALSE, FALSE, sizeof(gint));
			lc=0;
			for (k=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(jind)); k<sal; k++)
			{
				if (!strary[k]) continue;
				if (!g_strcmp0("", strary[k])) continue;
				if (!(g_ascii_isdigit(strary[k][0])|(g_str_has_prefix(strary[k],"-")))) continue;
				strat=g_strsplit_set(strary[k], "\t,", 0);
				lcl=g_ascii_strtod(g_strstrip(strat[0]), NULL);
				g_array_append_val(x, lcl);
				if (!strat[1]) lcl=0;
				else lcl=g_ascii_strtod(g_strstrip(strat[1]), NULL);
				if (lc==0) {mny=lcl; mxy=lcl;}
				else
				{
					if (!strat[1]) lcl=0;
					else lcl=g_ascii_strtod(g_strstrip(strat[1]), NULL);
					if (lcl<mny) mny=lcl;
					else if (lcl>mxy) mxy=lcl;
				}
				g_array_append_val(y, lcl);
				g_strfreev(strat);
				lc++;
			}
			g_strfreev(strary);
			str=g_strdup_printf("File: %s successfully loaded", fin);
			gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), str), str);
			g_free(str);
			plt=PLOT_LINEAR(plot);
			g_array_append_val(sz, lc);
			(plt->sizes)=sz;
			k=0;
			g_array_append_val(nx, k);
			(plt->ind)=nx;
			(plt->xdata)=x;
			(plt->ydata)=y;
			xi=g_array_index(x, gdouble, 0);
			xf=g_array_index(x, gdouble, (lc-1));
			plot_linear_update_scale(plot, xi, xf, mny, mxy);
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

void ad(GtkWidget *widget, gpointer data)
{
	PlotLinear *plt;
	GtkWidget *wfile;
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
			plt=PLOT_LINEAR(plot);
			g_object_get(G_OBJECT(plot), "xmin", &xi, "xmax", &xf, "ymin", &mny, "ymax", &mxy, NULL);
			k=(x->len);
			g_array_append_val(nx, k);
			strary=g_strsplit_set(contents, "\r\n", 0);
			sal=g_strv_length(strary);
			lc=0;
			for (k=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(jind)); k<sal; k++)
			{
				if (!strary[k]) continue;
				if (!g_strcmp0("", strary[k])) continue;
				if (!(g_ascii_isdigit(strary[k][0])|(g_str_has_prefix(strary[k],"-")))) continue;
				strat=g_strsplit_set(strary[k], "\t,", 0);
				if (!strat[1]) lcl=0;
				else lcl=g_ascii_strtod(g_strstrip(strat[1]), NULL);
				if (lcl<mny) mny=lcl;
				else if (lcl>mxy) mxy=lcl;
				g_array_append_val(y, lcl);
				lcl=g_ascii_strtod(g_strstrip(strat[0]), NULL);
				if ((lc==0)&&(lcl<xi)) xi=lcl;
				g_array_append_val(x, lcl);
				g_strfreev(strat);
				lc++;
			}
			if (lcl>xf) xf=lcl;
			g_strfreev(strary);
			str=g_strdup_printf("File: %s successfully loaded", fin);
			gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), str), str);
			g_free(str);
			g_array_append_val(sz, lc);
			(plt->sizes)=sz;
			(plt->ind)=nx;
			(plt->xdata)=x;
			(plt->ydata)=y;
			xi=g_array_index(x, gdouble, 0);
			xf=g_array_index(x, gdouble, (lc-1));
			plot_linear_update_scale(plot, xi, xf, mny, mxy);
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

void pltmv(PlotLinear *plt, gpointer data)
{
	gchar *str;
	
	str=g_strdup_printf("x: %f, y: %f", plt->xps, plt->yps);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), str), str);
	g_free(str);
}

void upg(GtkWidget *widget, gpointer data)
{
	PlotLinear *plt;
	gint d;
	gdouble xn, xx, yn, yx;
	
	plt=PLOT_LINEAR(plot);
	d=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	d++;
	(plt->flagd)=d;
	g_object_get(G_OBJECT(plot), "xmin", &xn, "xmax", &xx, "ymin", &yn, "ymax", &yx, NULL);
	plot_linear_update_scale_pretty(plot, xn, xx, yn, yx);
}

int main(int argc, char *argv[])
{
	PlotLinear *plt;
	GtkWidget *vbox, *vbox2, *mnb, *mnu, *mni, *hpane, *butt;
	GtkAdjustment *adj;
	guint j;
	gdouble val;
	GtkAccelGroup *accel_group=NULL;
	
	gtk_init(&argc, &argv);
	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "PlotLinear tester");
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
	mni=gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(ad), NULL);
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
	mnu=gtk_menu_new();
	mni=gtk_menu_item_new_with_label("Display Properties:");
	gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_F2, 0, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(dpr), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnu), mni);
	gtk_widget_show(mni);
	mni=gtk_menu_item_new_with_mnemonic("_Properties");
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
	label=gtk_label_new("Header Lines:");
	gtk_box_pack_start(GTK_BOX(vbox2), label, FALSE, FALSE, 2);
	gtk_widget_show(label);
	adj=(GtkAdjustment *) gtk_adjustment_new(5, 0, G_MAXINT, 1.0, 5.0, 0.0);
	jind=gtk_spin_button_new(adj, 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), jind, FALSE, FALSE, 2);
	gtk_widget_show(jind);
	gtk_paned_add1(GTK_PANED(hpane), vbox2);
	plot=plot_linear_new();
	g_signal_connect(plot, "moved", G_CALLBACK(pltmv), NULL);
	x=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 1024);
	y=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 1024);
	sz=g_array_new(FALSE, FALSE, sizeof(gint));
	nx=g_array_new(FALSE, FALSE, sizeof(gint));
	plt=PLOT_LINEAR(plot);
	(plt->flagd)=3;
	j=0;
	g_array_append_val(nx, j);
	while (j<50)
	{
		val=(((gdouble)(j++))-10)/32;
		g_array_append_val(x, val);
		val*=val;
		val=-val;
		val++;
		g_array_append_val(y, val);
		
	}
	g_array_append_val(sz, j);
	g_array_append_val(nx, j);
	for (j=0; j<25; j++)
	{
		val=((gdouble)j-2)/15;
		g_array_append_val(x, val);
		val--;
		val*=sin(G_PI*val);
		g_array_append_val(y, val);
	}
	g_array_append_val(sz, j);
	(plt->sizes)=sz;
	(plt->ind)=nx;
	(plt->xdata)=x;
	(plt->ydata)=y;
	(plt->ptsize)=4;
	(plt->lfsize)=14;
	gtk_widget_show(plot);
	gtk_paned_add2(GTK_PANED(hpane), plot);
	statusbar=gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 2);
	gtk_widget_show(statusbar);
	gtk_widget_show(window);
	gtk_main();
	return 0;
}
