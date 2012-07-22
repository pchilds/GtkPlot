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
#include "gtkplotlinear.h"

#define CM 65535
#define CMI 0.000015259021897
#define HM 32767

GtkWidget *helpwin, *window, *plot, *statusbar, *colour, *jind, *entry1, *entry2, *butt1, *butt2, *jix;
GArray *x, *y, *sz, *nx, *cla;
gchar* fol=NULL;

void dpa(GtkWidget *widget, gpointer data)
{
	GArray *ca;
	GtkPlotLinear *plt;
	GdkRGBA cl, iv;
	gchar *str;
	gdouble xi, xf, mny, mxy;
	GdkRGBA *ptr;
	gint dx, j;

	plt=GTK_PLOT_LINEAR(plot);
	g_free(plt->xlab);
	(plt->xlab)=g_strdup(gtk_entry_get_text(GTK_ENTRY(entry1)));
	g_free(plt->ylab);
	(plt->ylab)=g_strdup(gtk_entry_get_text(GTK_ENTRY(entry2)));
	pango_font_description_free(plt->lfont);
	str=g_strdup(gtk_font_button_get_font_name(GTK_FONT_BUTTON(butt1)));
	(plt->lfont)=pango_font_description_from_string(str);
	g_free(str);
	pango_font_description_free(plt->afont);
	str=g_strdup(gtk_font_button_get_font_name(GTK_FONT_BUTTON(butt2)));
	(plt->afont)=pango_font_description_from_string(str);
	g_free(str);
	gtk_color_selection_get_current_rgba(GTK_COLOR_SELECTION(colour), &cl);
	g_object_get(G_OBJECT(plot), "xmin", &xi, "xmax", &xf, "ymin", &mny, "ymax", &mxy, NULL);
	ca=g_array_sized_new(FALSE, FALSE, sizeof(GdkRGBA), (nx->len));
	for(j=0; j<(nx->len); j++)
	{
		dx=fmod(j, (plt->cl->len));
		iv=g_array_index((plt->cl), GdkRGBA, dx);
		g_array_append_val(ca, iv);
	}
	{g_array_free(cla, TRUE);}
	j=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(jix));
	ptr=&g_array_index(ca, gdouble, j);
	*ptr=cl;
	cla=g_array_new(FALSE, FALSE, sizeof(GdkRGBA));
	for (j=0; j<(ca->len); j++)
	{
		iv=g_array_index(ca, GdkRGBA, j);
		g_array_append_val(cla, iv);
	}
	g_array_free(car, TRUE);
	(plt->cl)=cla;
	gtk_plot_linear_update_scale(plot, xi, xf, mny, mxy);
}

void dpo(GtkWidget *widget, gpointer data)
{
	dpa(NULL, NULL);
	gtk_widget_destroy(helpwin);
}

void upj(GtkWidget *widget, gpointer data)
{
	GdkRGBA cl;
	gint dx, jdm;

	jdm=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	dx=fmod(jdm, (cla->len));
	cl=g_array_index(cla, GdkRGBA, dx);
	gtk_color_selection_set_current_rgba(GTK_COLOR_SELECTION(colour), &cl);
}

void dpr(GtkWidget *widget, gpointer data)
{
	AtkObject *atk_widget, *atk_label;
	GtkWidget *content, *grid, *butt, *label;
	GtkAdjustment *adj;
	GdkRGBA cl;
	GtkPlotLinear *plt;
	gdouble *ptr;
	gint j;
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
	grid=gtk_grid_new();
	gtk_widget_show(grid);
	plt=GTK_PLOT_LINEAR(plot);
	label=gtk_label_new("X axis text:");
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(), label, 0, 0, 1, 1);
	entry1=gtk_entry_new();
	str=g_strdup(plt->xlab);
	gtk_entry_set_text(GTK_ENTRY(entry1), str);
	g_free(str);
	gtk_widget_show(entry1);
	gtk_grid_attach(GTK_GRID(grid), entry1, 0, 1, 1, 1);
	atk_widget=gtk_widget_get_accessible(entry1);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	label=gtk_label_new("Y axis text:");
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 2, 1, 1);
	entry2=gtk_entry_new();
	str=g_strdup(plt->ylab);
	gtk_entry_set_text(GTK_ENTRY(entry2), str);
	g_free(str);
	gtk_widget_show(entry2);
	gtk_grid_attach(GTK_GRID(grid), entry2, 0, 3, 1, 1);
	atk_widget=gtk_widget_get_accessible(entry1);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	label=gtk_label_new("Text size:");
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 4, 1, 1);
	str=pango_font_description_to_string(plt->lfont);
	butt1=gtk_font_button_new_with_font(str);
	g_free(str);
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_show_size(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_use_font(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_use_size(GTK_FONT_BUTTON(butt1), FALSE);
	gtk_font_button_set_title(GTK_FONT_BUTTON(butt1), "Font Selection for Axis Labels");
	gtk_widget_show(butt1);
	gtk_grid_attach(GTK_GRID(grid), butt1, 0, 5, 1, 1);
	atk_widget=gtk_widget_get_accessible(butt1);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	label=gtk_label_new("Tick label size:");
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 6, 1, 1);
	str=pango_font_description_to_string(plt->afont);
	butt2=gtk_font_button_new_with_font(str);
	g_free(str);
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_show_size(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_use_font(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_use_size(GTK_FONT_BUTTON(butt2), FALSE);
	gtk_font_button_set_title(GTK_FONT_BUTTON(butt2), "Font Selection for Tick Mark Labels");
	gtk_widget_show(butt2);
	gtk_grid_attach(GTK_GRID(grid), butt2, 0, 7, 1, 1);
	atk_widget=gtk_widget_get_accessible(butt2);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	label=gtk_label_new("Index of Plot:");
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 8, 1, 1, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	adj=(GtkAdjustment*) gtk_adjustment_new(0, 0, (nx->len)-1, 1.0, 5.0, 0.0);
	jix=gtk_spin_button_new(adj, 0, 0);
	g_signal_connect(G_OBJECT(jix), "value-changed", G_CALLBACK(upj), NULL);
	gtk_grid_attach(GTK_GRID(grid), jix, 0, 9, 1, 1, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	gtk_widget_show(jix);
	atk_widget=gtk_widget_get_accessible(jix);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	colour=gtk_color_selection_new();
	gtk_color_selection_set_has_opacity_control(GTK_COLOR_SELECTION(colour), TRUE);
	cl=g_array_index(cla, GdkRGBA, 0);
	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(colour), &cl);
	gtk_color_selection_set_has_palette(GTK_COLOR_SELECTION(colour), TRUE);
	gtk_widget_show(colour);
	gtk_grid_attach(GTK_GRID(grid), colour, 1, 0, 1, 9, GTK_FILL|GTK_SHRINK|GTK_EXPAND, GTK_FILL|GTK_SHRINK|GTK_EXPAND, 2, 2);
	gtk_container_add(GTK_CONTAINER(content), grid);
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
			if (g_str_has_suffix(fout, ".eps")) gtk_plot_linear_print_eps(plot, fout);
			else
			{
				fout2=g_strconcat(fout, ".eps", NULL);
				gtk_plot_linear_print_eps(plot, fout2);
				g_free(fout2);
			}
		}
		else if (filt==svgfilt)
		{
			if (g_str_has_suffix(fout, ".svg")) gtk_plot_linear_print_svg(plot, fout);
			else
			{
				fout2=g_strconcat(fout, ".svg", NULL);
				gtk_plot_linear_print_svg(plot, fout2);
				g_free(fout2);
			}
		}
		else if (g_str_has_suffix(fout, ".png")) gtk_plot_linear_print_png(plot, fout);
		else
		{
			fout2=g_strconcat(fout, ".png", NULL);
			gtk_plot_linear_print_png(plot, fout2);
			g_free(fout2);
		}
		g_free(fout);
	}
	gtk_widget_destroy(wfile);
}

void opd(GtkWidget *widget, gpointer data)
{
	GtkPlotLinear *plt;
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
			plt=GTK_PLOT_LINEAR(plot);
			g_array_append_val(sz, lc);
			(plt->sizes)=sz;
			k=0;
			g_array_append_val(nx, k);
			(plt->xdata)=x;
			(plt->ydata)=y;
			(plt->ind)=nx;
			xi=g_array_index(x, gdouble, 0);
			xf=g_array_index(x, gdouble, (lc-1));
			gtk_plot_linear_update_scale(plot, xi, xf, mny, mxy);
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
	GtkPlotLinear *plt;
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
			plt=GTK_PLOT_LINEAR(plot);
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
			gtk_plot_linear_update_scale(plot, xi, xf, mny, mxy);
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

void pltmv(GtkPlotLinear *plt, gpointer data)
{
	gchar *str;
	
	str=g_strdup_printf("x: %f, y: %f", plt->xps, plt->yps);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), str), str);
	g_free(str);
}

void upg(GtkWidget *widget, gpointer data)
{
	GtkPlotLinear *plt;
	gint d;
	gdouble xn, xx, yn, yx;
	
	plt=GTK_PLOT_LINEAR(plot);
	d=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	d++;
	(plt->flagd)=d;
	g_object_get(G_OBJECT(plot), "xmin", &xn, "xmax", &xx, "ymin", &yn, "ymax", &yx, NULL);
	gtk_plot_linear_update_scale_pretty(plot, xn, xx, yn, yx);
}

int main(int argc, char *argv[])
{
	AtkObject *atk_widget, *atk_label;
	GdkColor cl;
	GtkPlotLinear *plt;
	GtkWidget *grid, *grid2, *mnb, *mnu, *mni, *pane, *butt, *label;
	GtkAdjustment *adj;
	guint j;
	gdouble val;
	GtkAccelGroup *accel_group=NULL;
	
	gtk_init(&argc, &argv);
	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "PlotLinear tester");
	g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	grid=gtk_grid_new(); 
	gtk_container_add(GTK_CONTAINER(window), grid);
	gtk_widget_show(grid);
	mnb=gtk_menu_bar_new();
	gtk_grid_attach(GTK_GRID(grid), mnb, 0, 0, 1, 1);
	gtk_widget_show(mnb);
	mnu=gtk_menu_new();
	accel_group=gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
	mni=gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
	gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(opd), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnu), mni);
	gtk_widget_show(mni);
	mni=gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_KEY_a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(ad), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnu), mni);
	gtk_widget_show(mni);
	mni=gtk_image_menu_item_new_from_stock(GTK_STOCK_PRINT, NULL);
	gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_KEY_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(prt), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnu), mni);
	gtk_widget_show(mni);
	mni=gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(mnu), mni);
	gtk_widget_show(mni);
	mni=gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group);
	gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(gtk_main_quit), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnu), mni);
	gtk_widget_show(mni);
	mni=gtk_menu_item_new_with_mnemonic("_File");
	gtk_widget_show(mni);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mni), mnu);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnb), mni);
	mnu=gtk_menu_new();
	mni=gtk_menu_item_new_with_label("Display Properties:");
	gtk_widget_add_accelerator(mni, "activate", accel_group, GDK_KEY_F2, 0, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(mni), "activate", G_CALLBACK(dpr), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnu), mni);
	gtk_widget_show(mni);
	mni=gtk_menu_item_new_with_mnemonic("_Properties");
	gtk_widget_show(mni);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mni), mnu);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnb), mni);
	pane=gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show(pane);
	gtk_grid_attach(GTK_GRID(grid), pane, 0, 1, 1, 1);
	grid2=gtk_grid_new();
	gtk_widget_show(grid2);
	butt=gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(butt), "lines");
	gtk_combo_box_append_text(GTK_COMBO_BOX(butt), "points");
	gtk_combo_box_append_text(GTK_COMBO_BOX(butt), "both");
	gtk_combo_box_set_active(GTK_COMBO_BOX(butt), 3);
	g_signal_connect(G_OBJECT(butt), "changed", G_CALLBACK(upg), NULL);
	gtk_grid_attach(GTK_GRID(grid2), butt, 0, 0, 1, 1);
	gtk_widget_show(butt);
	label=gtk_label_new("Header Lines:");
	gtk_grid_attach(GTK_GRID(grid2), label, 0, 1, 1, 1);
	gtk_widget_show(label);
	adj=(GtkAdjustment *) gtk_adjustment_new(5, 0, G_MAXINT, 1.0, 5.0, 0.0);
	jind=gtk_spin_button_new(adj, 0, 0);
	gtk_grid_attach(GTK_GRID(grid2), jind, 0, 2, 1, 1);
	gtk_widget_show(jind);
	atk_widget=gtk_widget_get_accessible(jind);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	gtk_paned_add1(GTK_PANED(pane), grid2);
	plot=gtk_plot_linear_new();
	g_signal_connect(plot, "moved", G_CALLBACK(pltmv), NULL);
	x=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 1024);
	y=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 1024);
	sz=g_array_new(FALSE, FALSE, sizeof(gint));
	nx=g_array_new(FALSE, FALSE, sizeof(gint));
	plt=GTK_PLOT_LINEAR(plot);
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
	cla=g_array_new(FALSE, FALSE, sizeof(GdkRGBA));
	{cl.red=0; cl.green=0; cl.blue=0; cl.alpha=0.8;}
	g_array_append_val(cla, cl);
	{cl.red=1; cl.green=0; cl.blue=0; cl.alpha=0.8;}
	g_array_append_val(cla, cl);
	{cl.red=0; cl.green=1; cl.blue=0; cl.alpha=0.8;}
	g_array_append_val(cla, cl);
	{cl.red=0; cl.green=0; cl.blue=1; cl.alpha=0.8;}
	g_array_append_val(cla, cl);
	(plt->cl)=cla;
	gtk_widget_show(plot);
	gtk_paned_add2(GTK_PANED(pane), plot);
	statusbar=gtk_statusbar_new();
	gtk_grid_attach(GTK_GRID(grid), 0, 2, 1, 1);
	gtk_widget_show(statusbar);
	gtk_widget_show(window);
	fol=g_strdup("/home");
	gtk_main();
	return 0;
}
