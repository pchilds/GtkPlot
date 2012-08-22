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
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include "gtkplotpolar.h"

#define NMY_PI -3.1415926535897932384626433832795028841971693993751

GdkColormap *cmp;
GtkWidget *helpwin, *window, *plot, *statusbar, *colour, *jind, *entry1, *entry2, *butt1, *butt2, *jix;
gchar* fol=NULL;

void dpa(GtkWidget *widget, gpointer data)
{
	GArray *car, *cag, *cab, *caa;
	GtkPlotPolar *plt;
	GdkColor cl;
	gchar *str, *str2;
	gdouble iv, xi, xf, mny, mxy, r0, th0;
	gdouble *ptr;
	guint16 alp;
	gint dx, j, k;
	PangoFontDescription *ds1, *ds2;

	plt=GTK_PLOT_POLAR(plot);
	{str=g_strdup(gtk_entry_get_text(GTK_ENTRY(entry2))); str2=g_strdup(gtk_entry_get_text(GTK_ENTRY(entry1)));}
	gtk_plot_polar_set_label(plt, str, str2);
	{g_free(str); g_free(str2);}
	ds1=pango_font_description_from_string(gtk_font_button_get_font_name(GTK_FONT_BUTTON(butt1)));
	ds2=pango_font_description_from_string(gtk_font_button_get_font_name(GTK_FONT_BUTTON(butt2)));
	gtk_plot_polar_set_font(plt, ds1, ds2);
	pango_font_description_free(ds1); pango_font_description_free(ds2);
	k=(plt->ind->len);
	car=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), k);
	cag=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), k);
	cab=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), k);
	caa=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), k);
	for(j=0; j<k; j++)
	{
		dx=fmod(j, (plt->rd->len));
		iv=g_array_index((plt->rd), gdouble, dx);
		g_array_append_val(car, iv);
		iv=g_array_index((plt->gr), gdouble, dx);
		g_array_append_val(cag, iv);
		iv=g_array_index((plt->bl), gdouble, dx);
		g_array_append_val(cab, iv);
		iv=g_array_index((plt->al), gdouble, dx);
		g_array_append_val(caa, iv);
	}
	j=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(jix));
	gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(colour), &cl);
	ptr=&g_array_index(car, gdouble, j);
	iv=((gdouble) (cl.red))/65535;
	*ptr=iv;
	ptr=&g_array_index(cag, gdouble, j);
	iv=((gdouble) (cl.green))/65535;
	*ptr=iv;
	ptr=&g_array_index(cab, gdouble, j);
	iv=((gdouble) (cl.blue))/65535;
	*ptr=iv;
	alp=gtk_color_selection_get_current_alpha(GTK_COLOR_SELECTION(colour));
	ptr=&g_array_index(caa, gdouble, j);
	iv=((gdouble) alp)/65535;
	*ptr=iv;
	gtk_plot_polar_set_colour(plt, car, cag, cab, caa);
	{g_array_unref(car); g_array_unref(cag); g_array_unref(cab); g_array_unref(caa);}
	g_object_get(G_OBJECT(plot), "thmin", &xi, "thmax", &xf, "rmin", &mny, "rmax", &mxy, "rcnt", &r0, "thcnt", &th0, NULL);
	gtk_plot_polar_update_scale(plot, mny, mxy, xi, xf, r0, th0);
}

void dpo(GtkWidget *widget, gpointer data)
{
	dpa(NULL, NULL);
	gtk_widget_destroy(helpwin);
}

void upj(GtkWidget *widget, gpointer data)
{
	GdkColor cl;
	GtkPlotPolar *plt;
	guint alp;
	gint dx, jdm;

	plt=GTK_PLOT_POLAR(plot);
	jdm=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	dx=fmod(jdm, (plt->rd->len));
	(cl.red)=(guint16) (65535*g_array_index((plt->rd), gdouble, dx));
	(cl.green)=(guint16) (65535*g_array_index((plt->gr), gdouble, dx));
	(cl.blue)=(guint16) (65535*g_array_index((plt->bl), gdouble, dx));
	alp=(guint16) (65535*g_array_index((plt->al), gdouble, dx));
	gdk_colormap_alloc_color(cmp, &cl, FALSE, TRUE);
	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(colour), &cl);
	gtk_color_selection_set_current_alpha(GTK_COLOR_SELECTION(colour), alp);
}

void dpr(GtkWidget *widget, gpointer data)
{
	AtkObject *atk_widget, *atk_label;
	GtkWidget *content, *table, *butt, *label;
	GtkAdjustment *adj;
	GdkColor cl;
	GtkPlotPolar *plt;
	gdouble *ptr;
	gint j;
	guint alp;
	gchar *str;
	
	helpwin=gtk_dialog_new_with_buttons("Display Properties", GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
	g_signal_connect_swapped(G_OBJECT(helpwin), "destroy", G_CALLBACK(gtk_widget_destroy), G_OBJECT(helpwin));
	butt=gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_widget_show(butt);
	g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(gtk_widget_destroy), G_OBJECT(helpwin));
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(helpwin)->action_area), butt);
	butt=gtk_button_new_from_stock(GTK_STOCK_APPLY);
	gtk_widget_show(butt);
	g_signal_connect(G_OBJECT(butt), "clicked", G_CALLBACK(dpa), NULL);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(helpwin)->action_area), butt);
	butt=gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_widget_show(butt);
	g_signal_connect(G_OBJECT(butt), "clicked", G_CALLBACK(dpo), NULL);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(helpwin)->action_area), butt);
	content=gtk_dialog_get_content_area(GTK_DIALOG(helpwin));
	table=gtk_table_new(10, 2, FALSE);
	gtk_widget_show(table);
	plt=GTK_PLOT_POLAR(plot);
	label=gtk_label_new("Azimuthal axis text:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	entry1=gtk_entry_new();
	str=g_strdup(plt->thlab);
	gtk_entry_set_text(GTK_ENTRY(entry1), str);
	g_free(str);
	gtk_widget_show(entry1);
	gtk_table_attach(GTK_TABLE(table), entry1, 0, 1, 1, 2, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	atk_widget=gtk_widget_get_accessible(entry1);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	label=gtk_label_new("Radial axis text:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	entry2=gtk_entry_new();
	str=g_strdup(plt->rlab);
	gtk_entry_set_text(GTK_ENTRY(entry2), str);
	g_free(str);
	gtk_widget_show(entry2);
	gtk_table_attach(GTK_TABLE(table), entry2, 0, 1, 3, 4, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	atk_widget=gtk_widget_get_accessible(entry2);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	label=gtk_label_new("Text size:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	str=pango_font_description_to_string(plt->lfont);
	butt1=gtk_font_button_new_with_font(str);
	g_free(str);
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_show_size(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_use_font(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_use_size(GTK_FONT_BUTTON(butt1), FALSE);
	gtk_font_button_set_title(GTK_FONT_BUTTON(butt1), "Font Selection for Axis Labels");
	gtk_widget_show(butt1);
	gtk_table_attach(GTK_TABLE(table), butt1, 0, 1, 5, 6, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	atk_widget=gtk_widget_get_accessible(butt1);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	label=gtk_label_new("Tick label size:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 6, 7, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	str=pango_font_description_to_string(plt->afont);
	butt2=gtk_font_button_new_with_font(str);
	g_free(str);
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_show_size(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_use_font(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_use_size(GTK_FONT_BUTTON(butt2), FALSE);
	gtk_font_button_set_title(GTK_FONT_BUTTON(butt2), "Font Selection for Tick Mark Labels");
	gtk_widget_show(butt2);
	gtk_table_attach(GTK_TABLE(table), butt2, 0, 1, 7, 8, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	atk_widget=gtk_widget_get_accessible(butt2);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	label=gtk_label_new("Index of Plot:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 8, 9, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	j=(plt->ind->len);
	adj=(GtkAdjustment*) gtk_adjustment_new(0, 0, j-1, 1.0, 5.0, 0.0);
	jix=gtk_spin_button_new(adj, 0, 0);
	g_signal_connect(G_OBJECT(jix), "value-changed", G_CALLBACK(upj), NULL);
	gtk_table_attach(GTK_TABLE(table), jix, 0, 1, 9, 10, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	gtk_widget_show(jix);
	atk_widget=gtk_widget_get_accessible(jix);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	colour=gtk_color_selection_new();
	gtk_color_selection_set_has_opacity_control(GTK_COLOR_SELECTION(colour), TRUE);
	(cl.red)=(guint16) (65535*g_array_index((plt->rd), gdouble, 0));
	(cl.green)=(guint16) (65535*g_array_index((plt->gr), gdouble, 0));
	(cl.blue)=(guint16) (65535*g_array_index((plt->bl), gdouble, 0));
	alp=(guint16) (65535*g_array_index((plt->al), gdouble, 0));
	cmp=gdk_colormap_get_system();
	gdk_colormap_alloc_color(cmp, &cl, FALSE, TRUE);
	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(colour), &cl);
	gtk_color_selection_set_has_palette(GTK_COLOR_SELECTION(colour), TRUE);
	gtk_color_selection_set_current_alpha(GTK_COLOR_SELECTION(colour), alp);
	gtk_widget_show(colour);
	gtk_table_attach(GTK_TABLE(table), colour, 1, 2, 0, 7, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	gtk_container_add(GTK_CONTAINER(content), table);
	gtk_widget_show(helpwin);
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
			if (g_str_has_suffix(fout, ".eps")) gtk_plot_polar_print_eps(plot, fout);
			else
			{
				fout2=g_strconcat(fout, ".eps", NULL);
				gtk_plot_polar_print_eps(plot, fout2);
				g_free(fout2);
			}
		}
		else if (filt==svgfilt)
		{
			if (g_str_has_suffix(fout, ".svg")) gtk_plot_polar_print_svg(plot, fout);
			else
			{
				fout2=g_strconcat(fout, ".svg", NULL);
				gtk_plot_polar_print_svg(plot, fout2);
				g_free(fout2);
			}
		}
		else if (g_str_has_suffix(fout, ".png")) gtk_plot_polar_print_png(plot, fout);
		else
		{
			fout2=g_strconcat(fout, ".png", NULL);
			gtk_plot_polar_print_png(plot, fout2);
			g_free(fout2);
		}
		g_free(fout);
	}
	gtk_widget_destroy(wfile);
}

void opd(GtkWidget *widget, gpointer data)
{
	GArray *x, *y, *sz, *nx;
	GtkPlotPolar *plt;
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
			x=g_array_new(FALSE, FALSE, sizeof(gdouble));
			y=g_array_new(FALSE, FALSE, sizeof(gdouble));
			sz=g_array_new(FALSE, FALSE, sizeof(gint));
			nx=g_array_new(FALSE, FALSE, sizeof(gint));
			lc=0;
			for (k=0; k<sal; k++)
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
				else if (lcl<mny) mny=lcl;
				else if (lcl>mxy) mxy=lcl;
				g_array_append_val(y, lcl);
				g_strfreev(strat);
				lc++;
			}
			g_strfreev(strary);
			str=g_strdup_printf("File: %s successfully loaded", fin);
			gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), str), str);
			g_free(str);
			plt=GTK_PLOT_POLAR(plot);
			g_array_append_val(sz, lc);
			k=0;
			g_array_append_val(nx, k);
			xi=g_array_index(x, gdouble, 0);
			xf=g_array_index(x, gdouble, (lc-1));
			gtk_plot_polar_set_data(plt, x, y, nx, sz);
			{g_array_unref(x); g_array_unref(y); g_array_unref(nx); g_array_unref(sz);}
			gtk_plot_polar_update_scale(plot, mny, mxy, xi, xf, 0, 0);
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
	GArray *x, *y, *sz, *nx;
	GtkPlotPolar *plt;
	GtkWidget *wfile;
	gdouble xi, xf, lcl, mny, mxy;
	guint k, sal;
	gint lc, lc2;
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
			plt=GTK_PLOT_POLAR(plot);
			x=g_array_new(FALSE, FALSE, sizeof(gdouble));
			y=g_array_new(FALSE, FALSE, sizeof(gdouble));
			sz=g_array_new(FALSE, FALSE, sizeof(gint));
			nx=g_array_new(FALSE, FALSE, sizeof(gint));
			{sal=0; lc=0; lc2=0;}
			while (sal<(plt->sizes->len))
			{
				lc2=g_array_index((plt->ind), gint, sal);
				g_array_append_val(nx, lc2);
				lc=g_array_index((plt->sizes), gint, sal);
				g_array_append_val(sz, lc);
				lc+=lc2;
				for (k=lc2;k<lc;k++)
				{
					lcl=g_array_index((plt->thdata), gdouble, k);
					g_array_append_val(x, lcl);
					lcl=g_array_index((plt->rdata), gdouble, k);
					g_array_append_val(y, lcl);
				}
				sal++;
			}
			g_array_append_val(nx, lc);
			g_object_get(G_OBJECT(plot), "thmin", &xi, "thmax", &xf, "rmin", &mny, "rmax", &mxy, NULL);
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
			gtk_plot_polar_set_data(plt, x, y, nx, sz);
			{g_array_unref(x); g_array_unref(y); g_array_unref(nx); g_array_unref(sz);}
			gtk_plot_polar_update_scale(plot, mny, mxy, xi, xf, 0, 0);
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

void pltmv(GtkPlotPolar *plt, gpointer data)
{
	gchar *str;

	str=g_strdup_printf("r: %f, th: %f", plt->rps, plt->thps);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), str), str);
	g_free(str);
}

void upg(GtkWidget *widget, gpointer data)
{
	GtkPlotPolar *plt;
	gint d;
	gdouble xn, xx, yn, yx, r0, th0;

	plt=GTK_PLOT_POLAR(plot);
	d=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	d++;
	(plt->flagd)=(2*d)+1;
	g_object_get(G_OBJECT(plot), "rmin", &xn, "rmax", &xx, "thmin", &yn, "thmax", &yx, "rcnt", &r0, "thcnt", &th0, NULL);
	gtk_plot_polar_update_scale(plot, xn, xx, yn, yx, r0, th0);
}

int main(int argc, char *argv[])
{
	AtkObject *atk_widget, *atk_label;
	GArray *x, *y, *sz, *nx, *car, *cab, *cag, *caa;
	GtkPlotPolar *plt;
	GtkWidget *vbox, *vbox2, *mnb, *mnu, *mni, *hpane, *butt, *label;
	GtkAdjustment *adj;
	guint j;
	gdouble fll, valx, valy;
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
	gtk_combo_box_set_active(GTK_COMBO_BOX(butt), 2);
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
	atk_widget=gtk_widget_get_accessible(jind);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	gtk_paned_add1(GTK_PANED(hpane), vbox2);
	plot=gtk_plot_polar_new();
	g_signal_connect(plot, "moved", G_CALLBACK(pltmv), NULL);
	x=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 51);
	y=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 51);
	sz=g_array_new(FALSE, FALSE, sizeof(gint));
	nx=g_array_new(FALSE, FALSE, sizeof(gint));
	plt=GTK_PLOT_POLAR(plot);
	(plt->flagd)=7;
	j=0;
	g_array_append_val(nx, j);
	while (j<=50)
	{
		valx=((gdouble)(j++)-25)*G_PI/25;
		g_array_append_val(x, valx);
		valy=cos(32*valx);
		valy++;
		valy*=(3+cos(16*valx));
		valy*=(1+cos(4*valx))/8;
		valy++;
		valy/=3;
		g_array_append_val(y, valy);
	}
	g_array_append_val(sz, j);
	gtk_plot_polar_set_data(plt, x, y, nx, sz);
	{g_array_unref(x); g_array_unref(y); g_array_unref(nx); g_array_unref(sz);}
	(plt->ptsize)=4;
	car=g_array_new(FALSE, FALSE, sizeof(gdouble));
	cag=g_array_new(FALSE, FALSE, sizeof(gdouble));
	cab=g_array_new(FALSE, FALSE, sizeof(gdouble));
	caa=g_array_new(FALSE, FALSE, sizeof(gdouble));
	fll=0;
	{g_array_append_val(car, fll); g_array_append_val(cag, fll); g_array_append_val(cab, fll); g_array_append_val(cag, fll); g_array_append_val(cab, fll); g_array_append_val(cab, fll);}
	fll++;
	{g_array_append_val(car, fll); g_array_append_val(cag, fll); g_array_append_val(cab, fll);}
	fll--;
	{g_array_append_val(car, fll); g_array_append_val(cag, fll); g_array_append_val(car, fll);}
	fll=0.8;
	{g_array_append_val(caa, fll); g_array_append_val(caa, fll); g_array_append_val(caa, fll); g_array_append_val(caa, fll);}
	gtk_plot_polar_set_colour(plt, car, cag, cab, caa);
	{g_array_unref(car); g_array_unref(cag); g_array_unref(cab); g_array_unref(caa);}
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
