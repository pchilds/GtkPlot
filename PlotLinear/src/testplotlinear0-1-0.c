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

#define CM 65535
#define CMI 0.000015259021897
#define HM 32767

GtkWidget *window, *plot, *statusbar, *jind, *label;
GArray *x, *y, *sz, *nx;
gchar* fol=NULL;

void dpr(GtkWidget *widget, gpointer data)
{
	GtkWidget *helpwin, *content, *table, *entry1, *entry2, *label, *butt1, *butt2, *colour;
	GtkAdjustment *adj1, *adj2;
	GdkColor cl;
	PlotLinear *plt;
	gdouble *ptr;
	gint j;
	gdouble xi, xf, mny, mxy;
	gchar *str;
	
	helpwin=gtk_dialog_new_with_buttons("Dsiplay Properties", GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL, GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, NULL);
	g_signal_connect_swapped(G_OBJECT(helpwin), "destroy", G_CALLBACK(gtk_widget_destroy), G_OBJECT(helpwin));
	gtk_widget_show(helpwin);
	content=gtk_dialog_get_content_area(GTK_DIALOG(helpwin));
	table=gtk_table_new(8, 2, FALSE);
	gtk_widget_show(table);
	label=gtk_label_new("X axis text:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	label=gtk_label_new("Y axis text:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	label=gtk_label_new("Text size:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	label=gtk_label_new("Tick label size:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 6, 7, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	entry1=gtk_entry_new();
	entry2=gtk_entry_new();
	plt=PLOT_LINEAR(plot);
	str=g_strdup(plt->xlab);
	gtk_entry_set_text(GTK_ENTRY(entry1), str);
	g_free(str);
	str=g_strdup(plt->ylab);
	gtk_entry_set_text(GTK_ENTRY(entry2), str);
	g_free(str);
	str=pango_font_description_to_string(plt->lfont);
	butt1=gtk_font_button_new_with_font(str);
	g_free(str);
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_show_size(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_use_font(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_use_size(GTK_FONT_BUTTON(butt1), FALSE);
	gtk_font_button_set_title(GTK_FONT_BUTTON(butt1), "Font Selection for Axis Labels");
	str=pango_font_description_to_string(plt->afont);
	butt2=gtk_font_button_new_with_font(str);
	g_free(str);
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_show_size(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_use_font(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_use_size(GTK_FONT_BUTTON(butt2), FALSE);
	gtk_font_button_set_title(GTK_FONT_BUTTON(butt2), "Font Selection for Tick Mark Labels");
	gtk_widget_show(entry1);
	gtk_widget_show(entry2);
	gtk_widget_show(butt1);
	gtk_widget_show(butt2);
	gtk_table_attach(GTK_TABLE(table), entry1, 0, 1, 1, 2, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	gtk_table_attach(GTK_TABLE(table), entry2, 0, 1, 3, 4, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	gtk_table_attach(GTK_TABLE(table), butt1, 0, 1, 5, 6, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	gtk_table_attach(GTK_TABLE(table), butt2, 0, 1, 7, 8, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	colour=gtk_color_selection_new();
	gtk_color_selection_set_has_opacity_control(GTK_COLOR_SELECTION(colour), TRUE);
	gtk_widget_show(colour);
	gtk_table_attach(GTK_TABLE(table), colour, 1, 2, 0, 7, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	gtk_container_add(GTK_CONTAINER(content), table);
	if (gtk_dialog_run(GTK_DIALOG(helpwin))==GTK_RESPONSE_APPLY)
	{
		g_free(plt->xlab);
		g_free(plt->ylab);
		pango_font_description_free(plt->lfont);
		pango_font_description_free(plt->afont);
		(plt->xlab)=g_strdup(gtk_entry_get_text(GTK_ENTRY(entry1)));
		(plt->ylab)=g_strdup(gtk_entry_get_text(GTK_ENTRY(entry2)));
		str=g_strdup(gtk_font_button_get_font_name(GTK_FONT_BUTTON(butt1)));
		(plt->lfont)=pango_font_description_from_string(str);
		g_free(str);
		str=g_strdup(gtk_font_button_get_font_name(GTK_FONT_BUTTON(butt2)));
		(plt->afont)=pango_font_description_from_string(str);
		g_free(str);
		g_object_get(G_OBJECT(plot), "xmin", &xi, "xmax", &xf, "ymin", &mny, "ymax", &mxy, NULL);
		plot_linear_update_scale(plot, xi, xf, mny, mxy);
	}
	gtk_widget_destroy(helpwin);
}

void prt(GtkWidget *widget, gpointer data)
{
	GtkWidget *wfile;
	GtkFileFilter *epsfilt, *pngfilt, *svgfilt, *filt;
	gchar *fout=NULL, *fout2=NULL;
	
	wfile=gtk_file_chooser_dialog_new("Select Image File", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	g_signal_connect(G_OBJECT(wfile), "destroy", G_CALLBACK(gtk_widget_destroy), G_OBJECT(wfile));
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(wfile), TRUE);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(wfile), fol);
	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(wfile), FALSE);
	epsfilt=gtk_file_filter_new();
	gtk_file_filter_set_name(epsfilt, "Encapsulated Postscript (EPS)");
	gtk_file_filter_add_mime_type(epsfilt, "application/postscript");
	gtk_file_filter_add_mime_type(epsfilt, "application/eps");
	gtk_file_filter_add_mime_type(epsfilt, "image/eps");
	gtk_file_filter_add_mime_type(epsfilt, "image/x-eps");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(wfile), epsfilt);
	pngfilt=gtk_file_filter_new();
	gtk_file_filter_set_name(pngfilt, "Portable Network Graphics (PNG)");
	gtk_file_filter_add_mime_type(pngfilt, "image/png");
	gtk_file_filter_add_mime_type(pngfilt, "application/png");
	gtk_file_filter_add_mime_type(pngfilt, "application/x-png");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(wfile), pngfilt);
	svgfilt=gtk_file_filter_new();
	gtk_file_filter_set_name(svgfilt, "Scalable Vector Graphics (SVG)");
	gtk_file_filter_add_mime_type(svgfilt, "image/svg+xml");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(wfile), svgfilt);
	if (gtk_dialog_run(GTK_DIALOG(wfile))==GTK_RESPONSE_ACCEPT)
	{
		g_free(fol);
		fol=gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(wfile));
		fout=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(wfile));
		filt=gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(wfile));
		if (filt==epsfilt)
		{
			if (g_str_has_suffix(fout, ".eps")) plot_linear_print_eps(plot, fout);
			else
			{
				fout2=g_strconcat(fout, ".eps", NULL);
				plot_linear_print_eps(plot, fout2);
				g_free(fout2);
			}
		}
		else if (filt==svgfilt)
		{
			if (g_str_has_suffix(fout, ".svg")) plot_linear_print_svg(plot, fout);
			else
			{
				fout2=g_strconcat(fout, ".svg", NULL);
				plot_linear_print_svg(plot, fout2);
				g_free(fout2);
			}
		}
		else if (g_str_has_suffix(fout, ".png")) plot_linear_print_png(plot, fout);
		else
		{
			fout2=g_strconcat(fout, ".png", NULL);
			plot_linear_print_png(plot, fout2);
			g_free(fout2);
		}
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
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(wfile), FALSE);
	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(wfile), FALSE);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(wfile), fol);
	if (gtk_dialog_run(GTK_DIALOG(wfile))==GTK_RESPONSE_ACCEPT)
	{
		g_free(fol);
		fol=gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(wfile));
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
				g_strchug(strary[k]);
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
			(plt->xdata)=x;
			(plt->ydata)=y;
			(plt->ind)=nx;
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
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(wfile), FALSE);
	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(wfile), FALSE);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(wfile), fol);
	if (gtk_dialog_run(GTK_DIALOG(wfile))==GTK_RESPONSE_ACCEPT)
	{
		g_free(fol);
		fol=gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(wfile));
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
				g_strchug(strary[k]);
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
	gtk_combo_box_set_active(GTK_COMBO_BOX(butt), 3);
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
	gtk_widget_show(plot);
	gtk_paned_add2(GTK_PANED(hpane), plot);
	statusbar=gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 2);
	gtk_widget_show(statusbar);
	gtk_widget_show(window);
	fol=g_strdup("/home");
	gtk_main();
	return 0;
}
