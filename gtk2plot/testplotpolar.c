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

gchar* fol=NULL;
GdkColormap *cmp;
GtkPrintSettings *prst=NULL;
GtkWidget *butt1, *butt2, *colour, *entry1, *entry2, *helpwin, *jind, *jix, *plot, *sbr;

void dpa(GtkWidget *widget, gpointer data)
{
	GArray *car, *cag, *cab, *caa;
	GtkPlot *pt;
	GtkPlotPolar *plt;
	GdkColor cl;
	gchar *str, *str2;
	gdouble iv, xi, xf, mny, mxy, r0, th0;
	gdouble *ptr;
	guint16 alp;
	gint dx, j, k;
	PangoFontDescription *ds1, *ds2;

	plt=GTK_PLOT_POLAR(plot);
	pt=GTK_PLOT(plot);
	{str=g_strdup(gtk_entry_get_text(GTK_ENTRY(entry2))); str2=g_strdup(gtk_entry_get_text(GTK_ENTRY(entry1)));}
	gtk_plot_polar_set_label(plt, str, str2);
	{g_free(str); g_free(str2);}
	ds1=pango_font_description_from_string(gtk_font_button_get_font_name(GTK_FONT_BUTTON(butt1)));
	ds2=pango_font_description_from_string(gtk_font_button_get_font_name(GTK_FONT_BUTTON(butt2)));
	gtk_plot_set_font(pt, ds1, ds2, NULL);
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
	gtk_plot_set_colour(pt, car, cag, cab, caa, NULL);
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
	GtkPlot *pt;
	guint alp;
	gint dx, jdm;

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
	GtkPlotPolar *plt;
	gint j;
	guint alp;
	gchar *str;
	
	helpwin=gtk_dialog_new_with_buttons("Display Properties", GTK_WINDOW(data), GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
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
	pt=GTK_PLOT(plot);
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
	str=pango_font_description_to_string(pt->lfont);
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
	str=pango_font_description_to_string(pt->afont);
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
	j=(pt->ind->len);
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
	gtk_table_attach(GTK_TABLE(table), colour, 1, 2, 0, 7, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
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
	g_signal_connect(prto, "draw_page", G_CALLBACK(gtk_plot_polar_print), (gpointer) plot);
	res=gtk_print_operation_run(prto, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, GTK_WINDOW(data), &Err);
	if (res==GTK_PRINT_OPERATION_RESULT_ERROR)
	{
		str=g_strdup_printf("An error occured while printing: %s.", (Err->message));
		gtk_statusbar_push(GTK_STATUSBAR(sbr), gtk_statusbar_get_context_id(GTK_STATUSBAR(sbr), str), str);
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

	wfile=gtk_file_chooser_dialog_new("Select Image File", GTK_WINDOW(data), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
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
	GArray *ky, *nx, *st, *sz, *x, *y;
	gchar *contents, *fin=NULL, *str;
	gchar **strary, **strat;
	gdouble lcl, mny=0.0, mxy=1.0, xi, xf;
	GError *Err;
	gint lc;
	GtkPlotPolar *plt;
	GtkWidget *wfile;
	guint k, sal;

	wfile=gtk_file_chooser_dialog_new("Select Data File", GTK_WINDOW(data), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
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
			st=g_array_sized_new(FALSE, FALSE, sizeof(gint), 1);
			sz=g_array_sized_new(FALSE, FALSE, sizeof(gint), 1);
			nx=g_array_sized_new(FALSE, FALSE, sizeof(gint), 1);
			ky=g_array_sized_new(FALSE, FALSE, sizeof(guint), 1);
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
			gtk_statusbar_push(GTK_STATUSBAR(sbr), gtk_statusbar_get_context_id(GTK_STATUSBAR(sbr), str), str);
			g_free(str);
			plt=GTK_PLOT_POLAR(plot);
			g_array_append_val(sz, lc);
			k=0;
			g_array_append_val(nx, k);
			g_array_append_val(ky, k);
			k=1;
			g_array_append_val(st, k);
			xi=g_array_index(x, gdouble, 0);
			xf=g_array_index(x, gdouble, (lc-1));
			gtk_plot_polar_set_data(plt, x, y, nx, sz, st, ky);
			{g_array_unref(x); g_array_unref(y); g_array_unref(nx); g_array_unref(sz); g_array_unref(st); g_array_unref(ky);}
			gtk_plot_polar_update_scale(plot, mny, mxy, xi, xf, 0, 0);
		}
		else
		{
			str=g_strdup_printf("Loading failed for file: %s", fin);
			gtk_statusbar_push(GTK_STATUSBAR(sbr), gtk_statusbar_get_context_id(GTK_STATUSBAR(sbr), str), str);
			g_free(str);
		}
		g_free(contents);
		g_free(fin);
	}
	gtk_widget_destroy(wfile);
}

void ad(GtkWidget *widget, gpointer data)
{
	GArray *ky, *nx, *st, *sz, *x, *y;
	gchar *contents, *fin=NULL, *str;
	gchar **strary, **strat;
	gdouble lcl, mny, mxy, xi, xf;
	GError *Err;
	gint lc, lc2, lc3, lc4;
	GtkPlot *pt;
	GtkPlotPolar *plt;
	GtkWidget *wfile;
	guint k, lc5, sal;

	wfile=gtk_file_chooser_dialog_new("Select Data File", GTK_WINDOW(data), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
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
			pt=GTK_PLOT(plot);
			x=g_array_new(FALSE, FALSE, sizeof(gdouble));
			y=g_array_new(FALSE, FALSE, sizeof(gdouble));
			st=g_array_new(FALSE, FALSE, sizeof(gint));
			sz=g_array_new(FALSE, FALSE, sizeof(gint));
			nx=g_array_new(FALSE, FALSE, sizeof(gint));
			ky=g_array_new(FALSE, FALSE, sizeof(guint));
			sal=lc=0;
			while (sal<(pt->sizes->len))
			{
				g_array_append_val(nx, lc);
				lc2=1;
				g_array_append_val(st, lc2);
				lc5=g_array_index(plt->kdata, guint, sal);
				g_array_append_val(ky, lc5);
				lc2=g_array_index(pt->ind, gint, sal);
				lc3=g_array_index(pt->sizes, gint, sal);
				lc4=g_array_index(pt->stride, gint, sal);
				g_array_append_val(sz, lc3);
				lc+=lc3;
				lc3=lc2+lc3*lc4;
				for (k=lc2;k<lc3;k+=lc4)
				{
					lcl=g_array_index(plt->thdata, gdouble, k);
					g_array_append_val(x, lcl);
					lcl=g_array_index(plt->rdata, gdouble, k);
					g_array_append_val(y, lcl);
				}
				sal++;
			}
			g_array_append_val(nx, lc);
			lc5=0;
			g_array_append_val(ky, lc5);
			lc2=1;
			g_array_append_val(st, lc2);
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
			gtk_statusbar_push(GTK_STATUSBAR(sbr), gtk_statusbar_get_context_id(GTK_STATUSBAR(sbr), str), str);
			g_free(str);
			g_array_append_val(sz, lc);
			gtk_plot_polar_set_data(plt, x, y, nx, sz, st, ky);
			{g_array_unref(x); g_array_unref(y); g_array_unref(nx); g_array_unref(sz); g_array_unref(st); g_array_unref(ky);}
			gtk_plot_polar_update_scale(plot, mny, mxy, xi, xf, 0, 0);
		}
		else
		{
			str=g_strdup_printf("Loading failed for file: %s", fin);
			gtk_statusbar_push(GTK_STATUSBAR(sbr), gtk_statusbar_get_context_id(GTK_STATUSBAR(sbr), str), str);
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
	gtk_statusbar_push(GTK_STATUSBAR(sbr), gtk_statusbar_get_context_id(GTK_STATUSBAR(sbr), str), str);
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
	AtkObject *atk_label, *atk_widget;
	GArray *caa, *cab, *cag, *car, *ky, *nx, *st, *sz, *x, *y;
	gchar* klb[3] = {"Spiral", "Sinusoidal", NULL};
	gdouble fll, valx, valy;
	GtkAccelGroup *accel_group=NULL;
	GtkAdjustment *adj;
	GtkPlot *pt;
	GtkPlotPolar *plt;
	GtkWidget *vbox, *vbox2, *mnb, *mnu, *mni, *hpane, *butt, *label, *wdw;
	guint j;

	gtk_init(&argc, &argv);
	wdw=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(wdw), "PlotPolar tester");
	g_signal_connect_swapped(G_OBJECT(wdw), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	vbox=gtk_vbox_new(FALSE, 0); 
	gtk_container_add(GTK_CONTAINER(wdw), vbox);
	gtk_widget_show(vbox);
	mnb=gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), mnb, FALSE, FALSE, 2);
	gtk_widget_show(mnb);
	mnu=gtk_menu_new();
	accel_group=gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(wdw), accel_group);
	mni=gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
	gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(opd), (gpointer) wdw);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnu), mni);
	gtk_widget_show(mni);
	mni=gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(ad), (gpointer) wdw);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnu), mni);
	gtk_widget_show(mni);
	mni=gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE, NULL);
	gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(prg), (gpointer) wdw);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnu), mni);
	gtk_widget_show(mni);
	mni=gtk_image_menu_item_new_from_stock(GTK_STOCK_PRINT, NULL);
	gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(prt), (gpointer) wdw);
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
	g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(dpr), (gpointer) wdw);
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
	adj=(GtkAdjustment*) gtk_adjustment_new(5, 0, G_MAXINT, 1.0, 5.0, 0.0);
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
  st=g_array_sized_new(FALSE, FALSE, sizeof(gint), 2);
  sz=g_array_sized_new(FALSE, FALSE, sizeof(gint), 2);
  nx=g_array_sized_new(FALSE, FALSE, sizeof(gint), 2);
  ky=g_array_sized_new(FALSE, FALSE, sizeof(guint), 2);
  plt=GTK_PLOT_POLAR(plot);
  pt=GTK_PLOT(plot);
  plt->flagd=7;
  j=2;
  g_array_append_val(st, j);
  g_array_append_val(st, j);
  j=1<<1;
  g_array_append_val(ky, j);
  j=2<<1;
  g_array_append_val(ky, j);
  j=0;
  g_array_append_val(nx, j);
  j=1;
  g_array_append_val(nx, j);
  for (j=0;j<=50;j++) {
    valx=(gdouble)j/50.0;
    g_array_append_val(y, valx);
    valx=G_PI*(2*valx-1);
    g_array_append_val(x, valx);
    g_array_append_val(x, valx);
    valy=(1+(1+cos(32*valx))*(3+cos(16*valx))*(1+cos(4*valx))/8)/3.0;
    g_array_append_val(y, valy);
  }
  g_array_append_val(sz, j);
  g_array_append_val(sz, j);
  gtk_plot_polar_set_data(plt, y, x, nx, sz, st, ky);
  g_array_unref(x);
  g_array_unref(y);
  g_array_unref(nx);
  g_array_unref(sz);
  g_array_unref(st);
  g_array_unref(ky);
  plt->ptsize=4;
  car=g_array_new(FALSE, FALSE, sizeof(gdouble));
  cag=g_array_new(FALSE, FALSE, sizeof(gdouble));
  cab=g_array_new(FALSE, FALSE, sizeof(gdouble));
  caa=g_array_new(FALSE, FALSE, sizeof(gdouble));
  fll=0;
  g_array_append_val(car, fll);
  g_array_append_val(cag, fll);
  g_array_append_val(cab, fll);
  g_array_append_val(cag, fll);
  g_array_append_val(cab, fll);
  g_array_append_val(cab, fll);
  fll++;
  g_array_append_val(car, fll);
  g_array_append_val(cag, fll);
  g_array_append_val(cab, fll);
  fll--;
  g_array_append_val(car, fll);
  g_array_append_val(cag, fll);
  g_array_append_val(car, fll);
  fll=0.8;
  g_array_append_val(caa, fll);
  g_array_append_val(caa, fll);
  g_array_append_val(caa, fll);
  g_array_append_val(caa, fll);
  gtk_plot_set_colour(pt, car, cag, cab, caa, klb);
  g_array_unref(car);
  g_array_unref(cag);
  g_array_unref(cab);
  g_array_unref(caa);
  gtk_widget_show(plot);
  gtk_paned_add2(GTK_PANED(hpane), plot);
  sbr=gtk_statusbar_new();
  gtk_box_pack_start(GTK_BOX(vbox), sbr, FALSE, FALSE, 2);
  gtk_widget_show(sbr);
  gtk_widget_show(wdw);
  fol=g_strdup("/home");
  gtk_main();
  return 0;
}
