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

/*  GtkPlot3 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include "gtkplotpolar.h"
#define NMY_PI -3.1415926535897932384626433832795028841971693993751

gchar* fol=NULL;
GtkPrintSettings *prst=NULL;
GtkWidget *butt1, *butt2, *colour, *entry1, *entry2, *helpwin, *jind, *jix, *plot, *statusbar, *window;

void dph(GtkDialog *dlg, gint response, gpointer data)
{
	GArray *cla;
	GtkPlot *pt;
	GtkPlotPolar *plt;
	GdkRGBA cl, iv;
	gchar *str, *str2;
	gdouble xi, xf, mny, mxy, r0, th0;
	GdkRGBA *ptr;
	guint16 alp;
	gint dx, j, k;
	PangoFontDescription *ds1, *ds2;

	if (response!=GTK_RESPONSE_CLOSE)
	{
		plt=GTK_PLOT_POLAR(plot);
		pt=GTK_PLOT(plot);
		{str=g_strdup(gtk_entry_get_text(GTK_ENTRY(entry2))); str2=g_strdup(gtk_entry_get_text(GTK_ENTRY(entry1)));}
		gtk_plot_polar_set_label(plt, str, str2);
		{g_free(str); g_free(str2);}
		ds1=pango_font_description_from_string(gtk_font_button_get_font_name(GTK_FONT_BUTTON(butt1)));
		ds2=pango_font_description_from_string(gtk_font_button_get_font_name(GTK_FONT_BUTTON(butt2)));
		gtk_plot_set_font(pt, ds1, ds2);
		pango_font_description_free(ds1); pango_font_description_free(ds2);
		k=(pt->ind->len);
		cla=g_array_sized_new(FALSE, FALSE, sizeof(GdkRGBA), k);
		for(j=0; j<k; j++)
		{
			dx=fmod(j, (pt->cl->len));
			iv=g_array_index((pt->cl), GdkRGBA, dx);
			g_array_append_val(cla, iv);
		}
		j=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(jix));
		gtk_color_selection_get_current_rgba(GTK_COLOR_SELECTION(colour), &cl);
		ptr=&g_array_index(cla, GdkRGBA, j);
		*ptr=cl;
		gtk_plot_set_colour(pt, cla);
		g_array_unref(cla);
		g_object_get(G_OBJECT(plot), "thmin", &xi, "thmax", &xf, "rmin", &mny, "rmax", &mxy, "rcnt", &r0, "thcnt", &th0, NULL);
		gtk_plot_polar_update_scale(plot, mny, mxy, xi, xf, r0, th0);
	}
	if (response!=GTK_RESPONSE_APPLY) gtk_widget_destroy(helpwin);
}

void upj(GtkWidget *widget, gpointer data)
{
	GdkRGBA cl;
	gint dx, jdm;
	GtkPlot *pt;

	pt=GTK_PLOT(plot);
	jdm=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	dx=fmod(jdm, (pt->cl->len));
	cl=g_array_index(pt->cl, GdkRGBA, dx);
	gtk_color_selection_set_current_rgba(GTK_COLOR_SELECTION(colour), &cl);
}

void dpr(GtkWidget *widget, gpointer data)
{
	AtkObject *atk_widget, *atk_label;
	GtkWidget *content, *grid, *label;
	GtkAdjustment *adj;
	GdkRGBA cl;
	GtkPlot *pt;
	GtkPlotPolar *plt;
	gdouble *ptr;
	gint j;
	gchar *str;
	
	helpwin=gtk_dialog_new_with_buttons("Display Properties", GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, GTK_STOCK_APPLY, GTK_RESPONSE_APPLY, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	g_signal_connect_swapped(G_OBJECT(helpwin), "destroy", G_CALLBACK(gtk_widget_destroy), G_OBJECT(helpwin));
	g_signal_connect(GTK_DIALOG(helpwin), "response", G_CALLBACK(dph), NULL);
	content=gtk_dialog_get_content_area(GTK_DIALOG(helpwin));
	grid=gtk_grid_new();
	gtk_widget_show(grid);
	plt=GTK_PLOT_POLAR(plot);
	pt=GTK_PLOT(plot);
	label=gtk_label_new("Azimuthal axis text:");
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
	entry1=gtk_entry_new();
	str=g_strdup(plt->thlab);
	gtk_entry_set_text(GTK_ENTRY(entry1), str);
	g_free(str);
	gtk_widget_show(entry1);
	gtk_grid_attach(GTK_GRID(grid), entry1, 0, 1, 1, 1);
	atk_widget=gtk_widget_get_accessible(entry1);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	label=gtk_label_new("Radial axis text:");
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 2, 1, 1);
	entry2=gtk_entry_new();
	str=g_strdup(plt->rlab);
	gtk_entry_set_text(GTK_ENTRY(entry2), str);
	g_free(str);
	gtk_widget_show(entry2);
	gtk_grid_attach(GTK_GRID(grid), entry2, 0, 3, 1, 1);
	atk_widget=gtk_widget_get_accessible(entry2);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	label=gtk_label_new("Text size:");
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 4, 1, 1);
	str=pango_font_description_to_string(pt->lfont);
	butt1=gtk_font_button_new_with_font(str);
	g_free(str);
	gtk_widget_show(butt1);
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_show_size(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_use_font(GTK_FONT_BUTTON(butt1), TRUE);
	gtk_font_button_set_use_size(GTK_FONT_BUTTON(butt1), FALSE);
	gtk_font_button_set_title(GTK_FONT_BUTTON(butt1), "Font Selection for Axis Labels");
	gtk_grid_attach(GTK_GRID(grid), butt1, 0, 5, 1, 1);
	atk_widget=gtk_widget_get_accessible(butt1);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	label=gtk_label_new("Tick label size:");
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 6, 1, 1);
	str=pango_font_description_to_string(pt->afont);
	butt2=gtk_font_button_new_with_font(str);
	g_free(str);
	gtk_widget_show(butt2);
	gtk_font_button_set_show_style(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_show_size(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_use_font(GTK_FONT_BUTTON(butt2), TRUE);
	gtk_font_button_set_use_size(GTK_FONT_BUTTON(butt2), FALSE);
	gtk_font_button_set_title(GTK_FONT_BUTTON(butt2), "Font Selection for Tick Mark Labels");
	gtk_grid_attach(GTK_GRID(grid), butt2, 0, 7, 1, 1);
	atk_widget=gtk_widget_get_accessible(butt2);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	label=gtk_label_new("Index of Plot:");
	gtk_widget_show(label);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 8, 1, 1);
	adj=(GtkAdjustment*) gtk_adjustment_new(0, 0, (pt->ind->len)-1, 1.0, 5.0, 0.0);
	jix=gtk_spin_button_new(adj, 0, 0);
	g_signal_connect(G_OBJECT(jix), "value-changed", G_CALLBACK(upj), NULL);
	gtk_grid_attach(GTK_GRID(grid), jix, 0, 9, 1, 1);
	gtk_widget_show(jix);
	atk_widget=gtk_widget_get_accessible(jix);
	atk_label=gtk_widget_get_accessible(GTK_WIDGET(label));
	atk_object_add_relationship(atk_label, ATK_RELATION_LABEL_FOR, atk_widget);
	atk_object_add_relationship(atk_widget, ATK_RELATION_LABELLED_BY, atk_label);
	colour=gtk_color_selection_new();
	gtk_color_selection_set_has_opacity_control(GTK_COLOR_SELECTION(colour), TRUE);
	cl=g_array_index(pt->cl, GdkRGBA, 0);
	gtk_color_selection_set_current_rgba(GTK_COLOR_SELECTION(colour), &cl);
	gtk_color_selection_set_has_palette(GTK_COLOR_SELECTION(colour), TRUE);
	gtk_widget_show(colour);
	gtk_grid_attach(GTK_GRID(grid), colour, 1, 0, 1, 9);
	gtk_container_add(GTK_CONTAINER(content), grid);
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
	gtk_page_setup_set_orientation(prps, GTK_PAGE_ORIENTATION_LANDSCAPE);
	gtk_print_operation_set_default_page_setup(prto, prps);
	g_signal_connect(prto, "begin_print", G_CALLBACK(prb), NULL);
	g_signal_connect(prto, "draw_page", G_CALLBACK(gtk_plot_polar_print), (gpointer) plot);
	res=gtk_print_operation_run(prto, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, GTK_WINDOW(data), &Err);
	if (res==GTK_PRINT_OPERATION_RESULT_ERROR)
	{
		str=g_strdup_printf(_("An error occured while printing: %s."), (Err->message));
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
	GArray *nx, *st, *sz, *x, *y;
	gchar *contents, *fin=NULL, *str;
	gchar **strary, **strat;
	gdouble lcl, mny, mxy, xi, xf;
	GError *Err;
	gint lc;
	GtkPlotPolar *plt;
	GtkWidget *wfile;
	guint k, sal;

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
			st=g_array_sized_new(FALSE, FALSE, sizeof(gint), 1);
			sz=g_array_sized_new(FALSE, FALSE, sizeof(gint), 1);
			nx=g_array_sized_new(FALSE, FALSE, sizeof(gint), 1);
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
			k=1;
			g_array_append_val(st, k);
			xi=g_array_index(x, gdouble, 0);
			xf=g_array_index(x, gdouble, (lc-1));
			gtk_plot_polar_set_data(plt, x, y, nx, sz, st);
			{g_array_unref(x); g_array_unref(y); g_array_unref(nx); g_array_unref(sz); g_array_unref(st);}
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
	GArray *nx, *st, *sz, *x, *y;
	gchar *contents, *fin=NULL, *str;
	gchar **strary, **strat;
	gdouble lcl, mny, mxy, xi, xf;
	GError *Err;
	gint lc, lc2;
	GtkPlot *pt;
	GtkPlotPolar *plt;
	GtkWidget *wfile;
	guint k, sal;

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
			pt=GTK_PLOT(plot);
			x=g_array_new(FALSE, FALSE, sizeof(gdouble));
			y=g_array_new(FALSE, FALSE, sizeof(gdouble));
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
			gtk_plot_polar_set_data(plt, x, y, nx, sz, st);
			{g_array_unref(x); g_array_unref(y); g_array_unref(nx); g_array_unref(sz); g_array_unref(st);}
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
	AtkObject *atk_label, *atk_widget;
	GArray *cla, *nx, *st, *sz, *x, *y;
	GdkRGBA cl;
	gdouble valx, valy;
	GtkAccelGroup *accel_group=NULL;
	GtkAdjustment *adj;
	GtkPlot *pt;
	GtkPlotPolar *plt;
	GtkWidget *butt, *grid, *grid2, *label, *mnb, *mni, *mnu, *pane;
	guint j;

	gtk_init(&argc, &argv);
	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "PlotPolar tester");
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
	{gtk_widget_set_hexpand(pane, TRUE); gtk_widget_set_vexpand(pane, TRUE); gtk_widget_set_halign(pane, GTK_ALIGN_FILL); gtk_widget_set_valign(pane, GTK_ALIGN_FILL);}
	gtk_widget_show(pane);
	gtk_grid_attach(GTK_GRID(grid), pane, 0, 1, 1, 1);
	grid2=gtk_grid_new();
	gtk_widget_show(grid2);
	butt=gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(butt), "lines");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(butt), "points");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(butt), "both");
	gtk_combo_box_set_active(GTK_COMBO_BOX(butt), 2);
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
	plot=gtk_plot_polar_new();
	g_signal_connect(plot, "moved", G_CALLBACK(pltmv), NULL);
	x=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 51);
	y=g_array_sized_new(FALSE, FALSE, sizeof(gdouble), 51);
	st=g_array_sized_new(FALSE, FALSE, sizeof(gint), 2);
	sz=g_array_sized_new(FALSE, FALSE, sizeof(gint), 2);
	nx=g_array_sized_new(FALSE, FALSE, sizeof(gint), 2);
	plt=GTK_PLOT_POLAR(plot);
	pt=GTK_PLOT(plot);
	(plt->flagd)=7;
	j=1;
	g_array_append_val(st, j);
	g_array_append_val(st, j);
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
	gtk_plot_polar_set_data(plt, y, x, nx, sz, st);
	{g_array_unref(x); g_array_unref(y); g_array_unref(nx); g_array_unref(sz); g_array_unref(st);}
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
	gtk_plot_set_colour(pt, cla);
	g_array_unref(cla);
	gtk_widget_show(plot);
	gtk_paned_add2(GTK_PANED(pane), plot);
	statusbar=gtk_statusbar_new();
	gtk_grid_attach(GTK_GRID(grid), statusbar, 0, 2, 1, 1);
	gtk_widget_show(statusbar);
	gtk_widget_show(window);
	fol=g_strdup("/home");
	gtk_main();
	return 0;
}
