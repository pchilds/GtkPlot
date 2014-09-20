/***************************************************************************
 *            testplotdonutr0-1-0.c
 *
 *  A program to show the capabilities of the plotdonut widget
 *  version 0.1.0
 *
 *  Sat Dec  4 17:18:14 2013
 *  Copyright  2013  Paul Childs
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
#include "gtkplotdonut.h"

gchar* fol=NULL;
GdkColormap *cmp;
GtkPrintSettings *prst=NULL;
GtkWidget *helpwin, *window, *plot, *statusbar, *colour, *jind, *butt1, *butt2, *jix;

void dpa(GtkWidget *widget, gpointer data)
{
	GArray *car, *cag, *cab, *caa;
	GtkPlot *pt;
	GdkColor cl;
	gdouble iv;
	gdouble *ptr;
	guint16 alp;
	gint dx, j, k;
	PangoFontDescription *ds1, *ds2;

	pt=GTK_PLOT(plot);
	ds1=pango_font_description_from_string(gtk_font_button_get_font_name(GTK_FONT_BUTTON(butt1)));
	ds2=pango_font_description_from_string(gtk_font_button_get_font_name(GTK_FONT_BUTTON(butt2)));
	gtk_plot_set_font(pt, ds1, ds2);
	pango_font_description_free(ds1); pango_font_description_free(ds2);
	k=(pt->ind->len);
	car=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), k);
	cag=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), k);
	cab=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), k);
	caa=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), k);
	for(j=0; j<k; j++)
	{
		dx=fmod(j, (pt->rd->len));
		iv=g_array_index((pt->rd), gdouble, dx);
		g_array_append_val(car, iv);
		iv=g_array_index((pt->gr), gdouble, dx);
		g_array_append_val(cag, iv);
		iv=g_array_index((pt->bl), gdouble, dx);
		g_array_append_val(cab, iv);
		iv=g_array_index((pt->al), gdouble, dx);
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
	gtk_plot_set_colour(pt, car, cag, cab, caa);
	{g_array_unref(car); g_array_unref(cag); g_array_unref(cab); g_array_unref(caa);}
	gtk_plot_donut_refresh(plot);
}

void dpo(GtkWidget *widget, gpointer data)
{
	dpa(NULL, NULL);
	gtk_widget_destroy(helpwin);
}

void upj(GtkWidget *widget, gpointer data)
{
	GdkColor cl;
	GtkPlot *pt;
	guint alp;
	gint jdm, dx;

	pt=GTK_PLOT(plot);
	jdm=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	dx=fmod(jdm, (pt->rd->len));
	(cl.red)=(guint16) (65535*g_array_index((pt->rd), gdouble, dx));
	(cl.green)=(guint16) (65535*g_array_index((pt->gr), gdouble, dx));
	(cl.blue)=(guint16) (65535*g_array_index((pt->bl), gdouble, dx));
	alp=(guint16) (65535*g_array_index((pt->al), gdouble, dx));
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
	GtkPlot *pt;
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
	table=gtk_table_new(6, 2, FALSE);
	gtk_widget_show(table);
	pt=GTK_PLOT(plot);
	label=gtk_label_new("Text size:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	str=pango_font_description_to_string(pt->lfont);
	butt1=gtk_font_button_new_with_font(str);
	g_free(str);
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_show_size(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_use_font(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_use_size(GTK_FONT_BUTTON(butt1), FALSE);
	gtk_font_button_set_title(GTK_FONT_BUTTON(butt1), "Font Selection for Axis Labels");
	gtk_widget_show(butt1);
	gtk_table_attach(GTK_TABLE(table), butt1, 0, 1, 1, 2, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	atk_widget=gtk_widget_get_accessible(butt1);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	label=gtk_label_new("Tick label size:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	str=pango_font_description_to_string(pt->afont);
	butt2=gtk_font_button_new_with_font(str);
	g_free(str);
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_show_size(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_use_font(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_use_size(GTK_FONT_BUTTON(butt2), FALSE);
	gtk_font_button_set_title(GTK_FONT_BUTTON(butt2), "Font Selection for Tick Mark Labels");
	gtk_widget_show(butt2);
	gtk_table_attach(GTK_TABLE(table), butt2, 0, 1, 3, 4, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	atk_widget=gtk_widget_get_accessible(butt2);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	label=gtk_label_new("Index of Plot:");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	j=(pt->ind->len);
	adj=(GtkAdjustment*) gtk_adjustment_new(0, 0, j-1, 1.0, 5.0, 0.0);
	jix=gtk_spin_button_new(adj, 0, 0);
	g_signal_connect(G_OBJECT(jix), "value-changed", G_CALLBACK(upj), NULL);
	gtk_table_attach(GTK_TABLE(table), jix, 0, 1, 5, 6, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	gtk_widget_show(jix);
	atk_widget=gtk_widget_get_accessible(jix);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	colour=gtk_color_selection_new();
	gtk_color_selection_set_has_opacity_control(GTK_COLOR_SELECTION(colour), TRUE);
	(cl.red)=(guint16) (65535*g_array_index((pt->rd), gdouble, 0));
	(cl.green)=(guint16) (65535*g_array_index((pt->gr), gdouble, 0));
	(cl.blue)=(guint16) (65535*g_array_index((pt->bl), gdouble, 0));
	alp=(guint16) (65535*g_array_index((pt->al), gdouble, 0));
	cmp=gdk_colormap_get_system();
	gdk_colormap_alloc_color(cmp, &cl, FALSE, TRUE);
	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(colour), &cl);
	gtk_color_selection_set_has_palette(GTK_COLOR_SELECTION(colour), TRUE);
	gtk_color_selection_set_current_alpha(GTK_COLOR_SELECTION(colour), alp);
	gtk_widget_show(colour);
	gtk_table_attach(GTK_TABLE(table), colour, 1, 2, 0, 6, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	gtk_container_add(GTK_CONTAINER(content), table);
	gtk_widget_show(helpwin);
}

void prb(GtkPrintOperation *prto, GtkPrintContext *ctex, int page_nr) {gtk_print_operation_set_current_page(prto, 0); gtk_print_operation_set_has_selection(prto, FALSE);}

void prt(GtkWidget *widget, gpointer data)
{
	gchar *str;
	GError *Err=NULL;
	GtkPageSetup *prps=NULL;
	GtkPrintOperation *prto;
	GtkPrintOperationResult res;

	prto=gtk_print_operation_new();
	if (prst!=NULL) gtk_print_operation_set_print_settings(prto, prst);
	prps=gtk_print_operation_get_default_page_setup(prto);
	if (prps==NULL) prps=gtk_page_setup_new();
	gtk_page_setup_set_orientation(prps, GTK_PAGE_ORIENTATION_LANDSCAPE);
	gtk_print_operation_set_default_page_setup(prto, prps);
	g_signal_connect(prto, "begin_print", G_CALLBACK(prb), NULL);
	g_signal_connect(prto, "draw_page", G_CALLBACK(gtk_plot_donut_print), (gpointer) plot);
	res=gtk_print_operation_run(prto, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, GTK_WINDOW(data), &Err);
	if (res==GTK_PRINT_OPERATION_RESULT_ERROR)
	{
		str=g_strdup_printf("An error occured while printing: %s.", (Err->message));
		gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), str), str);
		g_free(str);
		g_error_free(Err);
	}
	else if (res==GTK_PRINT_OPERATION_RESULT_APPLY)
	{
		if (prst!=NULL) g_object_unref(prst);
		prst=g_object_ref(gtk_print_operation_get_print_settings(prto));
	}
	g_object_unref(prto);
}

void prg(GtkWidget *widget, gpointer data)
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
			if (g_str_has_suffix(fout, ".eps")) gtk_plot_donut_print_eps(plot, fout);
			else
			{
				fout2=g_strconcat(fout, ".eps", NULL);
				gtk_plot_donut_print_eps(plot, fout2);
				g_free(fout2);
			}
		}
		else if (filt==svgfilt)
		{
			if (g_str_has_suffix(fout, ".svg")) gtk_plot_donut_print_svg(plot, fout);
			else
			{
				fout2=g_strconcat(fout, ".svg", NULL);
				gtk_plot_donut_print_svg(plot, fout2);
				g_free(fout2);
			}
		}
		else if (g_str_has_suffix(fout, ".png")) gtk_plot_donut_print_png(plot, fout);
		else
		{
			fout2=g_strconcat(fout, ".png", NULL);
			gtk_plot_donut_print_png(plot, fout2);
			g_free(fout2);
		}
		g_free(fout);
	}
	gtk_widget_destroy(wfile);
}

void opd(GtkWidget *widget, gpointer data)
{
	GArray *k, *nx, *st, *sz, *v;
	gchar *contents, *fin=NULL, *str;
	gchar **strary, **strat;
	gdouble lcl;
	GError *Err;
	gint lc;
	GtkWidget *wfile;
	guint j, sal;

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
			{k=g_array_new(FALSE, FALSE, sizeof(gchar*)); v=g_array_new(FALSE, FALSE, sizeof(gdouble)); st=g_array_sized_new(FALSE, FALSE, sizeof(gint), 1); sz=g_array_sized_new(FALSE, FALSE, sizeof(gint), 1); nx=g_array_sized_new(FALSE, FALSE, sizeof(gint), 1);}
			j=0;
			g_array_append_val(nx, j);
			j=1;
			g_array_append_val(st, j);
			lc=0;
			for (j=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(jind)); j<sal; j++)
			{
				if (!strary[j]) continue;
				g_strchug(strary[j]);
				if (!g_strcmp0("", strary[j])) continue;
				strat=g_strsplit_set(strary[j], "\t,", 0);
				if (!strat[1]) lcl=0;
				else if (!(g_ascii_isdigit((g_strstrip(strat[1]))[0])||(g_str_has_prefix(strat[1],"-"))))
				{
					g_strfreev(strat);
					continue;
				}
				else lcl=g_ascii_strtod(g_strstrip(strat[1]), NULL);
				g_array_append_val(v, lcl);
				str=g_strdup(g_strstrip(strat[0]));
				g_array_append_val(k, str);
				g_strfreev(strat);
				lc++;
			}
			g_strfreev(strary);
			str=g_strdup_printf("File: %s successfully loaded", fin);
			gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), str), str);
			g_free(str);
			g_array_append_val(sz, lc);
			gtk_plot_donut_set_data(GTK_PLOT_DONUT(plot), k, v, nx, sz, st);
			{g_array_unref(k); g_array_unref(v); g_array_unref(nx); g_array_unref(sz); g_array_unref(st);}
			gtk_plot_donut_refresh(plot);
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
	GArray *k, *nx, *st, *sz, *v;
	gchar *contents, *fin=NULL, *str;
	gchar **strary, **strat;
	gdouble lcl;
	GError *Err;
	gint lc, lc2;
	GtkPlot *pt;
	GtkPlotDonut *plt;
	GtkWidget *wfile;
	guint j, sal;

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
			plt=GTK_PLOT_DONUT(plot);
			pt=GTK_PLOT(plot);
			k=g_array_new(FALSE, FALSE, sizeof(gchar*));
			v=g_array_new(FALSE, FALSE, sizeof(gdouble));
			st=g_array_new(FALSE, FALSE, sizeof(gint));
			sz=g_array_new(FALSE, FALSE, sizeof(gint));
			nx=g_array_new(FALSE, FALSE, sizeof(gint));
			lc=1;
			g_array_append_val(st, lc);
			{sal=0; lc=0;}
			while (sal<(pt->sizes->len))
			{
				lc=1;
				g_array_append_val(st, lc);
				lc2=g_array_index((pt->ind), gint, sal);
				g_array_append_val(nx, lc2);
				lc=g_array_index((pt->sizes), gint, sal);
				g_array_append_val(sz, lc);
				lc+=lc2;
				for (j=lc2;j<lc;j++)
				{
					str=g_strdup(g_array_index((plt->kdata), gchar*, j));
					g_array_append_val(k, str);
					lcl=g_array_index((plt->vdata), gdouble, j);
					g_array_append_val(v, lcl);
				}
				sal++;
			}
			g_array_append_val(nx, lc);
			strary=g_strsplit_set(contents, "\r\n", 0);
			sal=g_strv_length(strary);
			lc=0;
			for (j=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(jind)); j<sal; j++)
			{
				if (!strary[j]) continue;
				g_strchug(strary[j]);
				if (!g_strcmp0("", strary[j])) continue;
				strat=g_strsplit_set(strary[j], "\t,", 0);
				if (!strat[1]) lcl=0;
				else if (!(g_ascii_isdigit((g_strstrip(strat[1]))[0])||(g_str_has_prefix(strat[1],"-"))))
				{
					g_strfreev(strat);
					continue;
				}
				else lcl=g_ascii_strtod(g_strstrip(strat[1]), NULL);
				g_array_append_val(v, lcl);
				str=g_strdup(g_strstrip(strat[0]));
				g_array_append_val(k, str);
				g_strfreev(strat);
				lc++;
			}
			g_strfreev(strary);
			str=g_strdup_printf("File: %s successfully loaded", fin);
			gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), str), str);
			g_free(str);
			g_array_append_val(sz, lc);
			gtk_plot_donut_set_data(plt, k, v, nx, sz, st);
			{g_array_unref(k); g_array_unref(v); g_array_unref(nx); g_array_unref(sz); g_array_unref(st);}
			gtk_plot_donut_refresh(plot);
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

void upg(GtkWidget *widget, gpointer data)
{
	gint d;
	
	d=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	((GTK_PLOT_DONUT(plot))->flagd)=(++d);
	gtk_plot_donut_refresh(plot);
}

int main(int argc, char *argv[])
{
	AtkObject *atk_label, *atk_widget;
	GArray *caa, *cab, *cag, *car, *k, *nx, *st, *sz, *v;
	gchar str[9];
	gchar *str2;
	gdouble fll, val;
	GtkAccelGroup *accel_group=NULL;
	GtkAdjustment *adj;
	GtkPlot *pt;
	GtkPlotDonut *plt;
	GRand *rand;
	GtkWidget *vbox, *vbox2, *mnb, *mnu, *mni, *hpane, *butt, *label;
	guint j;
	
	gtk_init(&argc, &argv);
	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "PlotDonut tester");
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
	mni=gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE, NULL);
	gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(prg), NULL);
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
	gtk_combo_box_append_text(GTK_COMBO_BOX(butt), "KEYs");
	gtk_combo_box_append_text(GTK_COMBO_BOX(butt), "VALUEs");
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
	atk_widget=gtk_widget_get_accessible(jind);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	gtk_paned_add1(GTK_PANED(hpane), vbox2);
	plot=gtk_plot_donut_new();
	k=g_array_sized_new(FALSE, FALSE, sizeof(gchar*), 10);
	v=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 10);
	st=g_array_sized_new(FALSE, FALSE, sizeof(gint), 2);
	sz=g_array_sized_new(FALSE, FALSE, sizeof(gint), 2);
	nx=g_array_sized_new(FALSE, FALSE, sizeof(gint), 2);
	plt=GTK_PLOT_DONUT(plot);
	pt=GTK_PLOT(plot);
	(plt->flagd)=3;
	rand=g_rand_new();
	j=1;
	g_array_append_val(st, j);
	g_array_append_val(st, j);
	j=0;
	g_array_append_val(nx, j);
	while (j<5)
	{
	    g_snprintf(str, 9, "Group %d", ++j);
		str2=g_strdup(str);
		g_array_append_val(k, str2);
		val=g_rand_double_range(rand, 1, 8);
		g_array_append_val(v, val);
		
	}
	g_array_append_val(sz, j);
	g_array_append_val(nx, j);
	g_array_append_val(sz, j);
	while (j<10)
	{
	    g_snprintf(str, 9, "Group %d", ++j);
		str2=g_strdup(str);
		g_array_append_val(k, str2);
		val=g_rand_double_range(rand, 1, 8);
		g_array_append_val(v, val);
		
	}
	gtk_plot_donut_set_data(plt, k, v, nx, sz, st);
	{g_array_unref(k); g_array_unref(v); g_array_unref(nx); g_array_unref(sz); g_array_unref(st);}
	car=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 6);
	cag=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 6);
	cab=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 6);
	caa=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 6);
	fll=0;
	{g_array_append_val(cag, fll); g_array_append_val(cab, fll); g_array_append_val(cab, fll);}
	fll++;
	{g_array_append_val(car, fll); g_array_append_val(cag, fll); g_array_append_val(cab, fll);}
	fll--;
	{g_array_append_val(car, fll); g_array_append_val(car, fll); g_array_append_val(cag, fll); g_array_append_val(cab, fll);}
	fll++;
	{g_array_append_val(car, fll); g_array_append_val(cag, fll); g_array_append_val(car, fll); g_array_append_val(cab, fll); g_array_append_val(cab, fll);}
	fll--;
	{g_array_append_val(cag, fll); g_array_append_val(car, fll);}
	fll++;
	{g_array_append_val(cag, fll);}
	fll=0.8;
	{g_array_append_val(caa, fll); g_array_append_val(caa, fll); g_array_append_val(caa, fll); g_array_append_val(caa, fll); g_array_append_val(caa, fll); g_array_append_val(caa, fll);}
	gtk_plot_set_colour(pt, car, cag, cab, caa);
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
