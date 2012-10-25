/***************************************************************************
 *            gtkplotaccessible.c
 *
 *  An ATK base class for data plotting
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
#include "gtkplotaccessible.h"
#define TOOLTIP_KEY "tooltip"

static gboolean gtk_plot_accessible_on_screen(GtkWidget *widget)
{
	gboolean return_value;
	GtkAllocation allocation;
	GtkWidget *viewport;

	gtk_widget_get_allocation(widget, &allocation);
	viewport=gtk_widget_get_ancestor(widget, GTK_TYPE_VIEWPORT);
	if (viewport)
	{
		GtkAllocation viewport_allocation;
		GtkAdjustment *adjustment;
		GdkRectangle visible_rect;

		gtk_widget_get_allocation(viewport, &viewport_allocation);
		adjustment=gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(viewport));
		visible_rect.y=gtk_adjustment_get_value(adjustment);
		adjustment=gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(viewport));
		{visible_rect.x=gtk_adjustment_get_value(adjustment); visible_rect.width=viewport_allocation.width; visible_rect.height=viewport_allocation.height;}
		if (((allocation.x+allocation.width)<visible_rect.x)||((allocation.y+allocation.height)<visible_rect.y)||(allocation.x>(visible_rect.x+visible_rect.width))||(allocation.y>(visible_rect.y+visible_rect.height))) return_value = FALSE;
		else return_value=TRUE;
	}
	else
	{
		if ((allocation.x+allocation.width)<=0&&(allocation.y+allocation.height)<=0) return_value=FALSE;
		else return_value=TRUE;
	}
	return return_value;
}

static gboolean gtk_plot_accessible_all_parents_visible(GtkWidget *widget)
{
	gboolean result=TRUE;
	GtkWidget *iter_parent=NULL;

	for (iter_parent=gtk_widget_get_parent(widget); iter_parent; iter_parent=gtk_widget_get_parent(iter_parent))
	{
		if (!gtk_widget_get_visible(iter_parent))
		{
			result=FALSE;
			break;
		}
	}
	return result;
}

static void gtk_plot_accessible_get_extents(AtkComponent *component, gint *x, gint *y, gint *width, gint *height, AtkCoordType coord_type)
{
	GdkWindow *window;
	gint x_window, y_window;
	gint x_toplevel, y_toplevel;
	GtkWidget *widget;
	GtkAllocation allocation;

	widget=gtk_accessible_get_widget(GTK_ACCESSIBLE(component));
	if (widget==NULL) return;
	gtk_widget_get_allocation(widget, &allocation);
	{*width = allocation.width; *height=allocation.height;}
	if (!gtk_plot_accessible_on_screen(widget)||(!gtk_widget_is_drawable(widget)))
	{
		{*x=G_MININT; *y=G_MININT;}
		return;
	}
	if (gtk_widget_get_parent(widget)) {*x=allocation.x; *y =allocation.y; window=gtk_widget_get_parent_window(widget);}
	else {*x=0; *y=0; window=gtk_widget_get_window(widget);}
	gdk_window_get_origin(window, &x_window, &y_window);
	{*x+=x_window; *y+=y_window;}
	if (coord_type==ATK_XY_WINDOW)
	{
		window=gdk_window_get_toplevel(gtk_widget_get_window(widget));
		gdk_window_get_origin(window, &x_toplevel, &y_toplevel);
		{*x-=x_toplevel; *y-=y_toplevel;}
	}
}

static AtkLayer gtk_plot_accessible_get_layer(AtkComponent *component) {return (GTK_PLOT_ACCESSIBLE(component))->layer;}

static void gtk_plot_accessible_get_size(AtkComponent *component, gint *width, gint *height)
{
	GtkWidget *widget;

	widget=gtk_accessible_get_widget(GTK_ACCESSIBLE(component));
	if (widget==NULL) return;
	{*width=gtk_widget_get_allocated_width(widget); *height=gtk_widget_get_allocated_height(widget);}
}

static gboolean gtk_plot_accessible_grab_focus(AtkComponent *component)
{
	GtkWidget *widget;
	GtkWidget *toplevel;

	widget=gtk_accessible_get_widget(GTK_ACCESSIBLE(component));
	if (!widget||!gtk_widget_get_can_focus (widget)) return FALSE;
	gtk_widget_grab_focus(widget);
	toplevel=gtk_widget_get_toplevel(widget);
	if (gtk_widget_is_toplevel(toplevel))
	{
		#ifdef GDK_WINDOWING_X11
		gtk_window_present_with_time(GTK_WINDOW(toplevel),
		gdk_x11_get_server_time(gtk_widget_get_window(widget)));
		#else
		gtk_window_present(GTK_WINDOW(toplevel));
		#endif
	}
	return TRUE;
}

static void atk_component_interface_init(AtkComponentIface *iface)
{
	iface->get_extents=gtk_plot_accessible_get_extents;
	iface->get_layer=gtk_plot_accessible_get_layer;
	iface->get_size=gtk_plot_accessible_get_size;
	iface->grab_focus=gtk_plot_accessible_grab_focus;
}

G_DEFINE_TYPE_WITH_CODE(GtkPlotAccessible, _gtk_plot_accessible, GTK_TYPE_ACCESSIBLE, G_IMPLEMENT_INTERFACE(ATK_TYPE_COMPONENT, atk_component_interface_init))

static gboolean focus_cb(GtkWidget *widget, GdkEventFocus *event)
{
	AtkObject *obj;

	obj=gtk_widget_get_accessible(widget);
	g_signal_emit_by_name(obj, "focus-event", event->in);
	return FALSE;
}

static void notify_cb(GObject *obj, GParamSpec *pspec)
{
	GtkPlotAccessibleClass *klass;

	klass=GTK_PLOT_ACCESSIBLE_GET_CLASS(GTK_PLOT_ACCESSIBLE(gtk_widget_get_accessible(GTK_WIDGET(obj))));
	if (klass->notify_gtk) klass->notify_gtk(obj, pspec);
}

static void size_allocate_cb(GtkWidget *widget, GtkAllocation *allocation)
{
	AtkObject* accessible;
	AtkRectangle rect;

	accessible=gtk_widget_get_accessible(widget);
	if (ATK_IS_COMPONENT(accessible))
	{
		{rect.x=allocation->x; rect.y=allocation->y; rect.width=allocation->width; rect.height=allocation->height;}
		g_signal_emit_by_name (accessible, "bounds-changed", &rect);
	}
}

static gint map_cb(GtkWidget *widget)
{
	AtkObject *accessible;

	accessible=gtk_widget_get_accessible(widget);
	atk_object_notify_state_change(accessible, ATK_STATE_SHOWING, gtk_widget_get_mapped(widget));
	return 1;
}

static void gtk_plot_accessible_notify_gtk(GObject *obj, GParamSpec *pspec)
{
	GtkWidget* widget=GTK_WIDGET (obj);
	AtkObject* atk_obj=gtk_widget_get_accessible(widget);
	AtkState state;
	gboolean value;

	if (g_strcmp0(pspec->name, "has-focus")==0) return;
	else if (g_strcmp0(pspec->name, "visible")==0) {state=ATK_STATE_VISIBLE; value=gtk_widget_get_visible(widget);}
	else if (g_strcmp0(pspec->name, "sensitive")==0) {state=ATK_STATE_SENSITIVE; value=gtk_widget_get_sensitive (widget);}
	else if (g_strcmp0(pspec->name, "tooltip-text")==0) g_object_set_data_full(G_OBJECT(GTK_PLOT_ACCESSIBLE(atk_obj)), TOOLTIP_KEY, gtk_widget_get_tooltip_text(widget), g_free);
	return;
	atk_object_notify_state_change(atk_obj, state, value);
	if (state==ATK_STATE_SENSITIVE) atk_object_notify_state_change(atk_obj, ATK_STATE_ENABLED, value);
	if (state==ATK_STATE_HORIZONTAL) atk_object_notify_state_change(atk_obj, ATK_STATE_VERTICAL, !value);
}

static const gchar* gtk_plot_accessible_get_description(AtkObject* accessible)
{
	GtkWidget *widget;

	widget=gtk_accessible_get_widget(GTK_ACCESSIBLE(accessible));
	if (widget==NULL) return NULL;
	if (accessible->description) return accessible->description;
	return g_object_get_data(G_OBJECT(accessible), TOOLTIP_KEY);
}

static AtkObject* gtk_plot_accessible_get_parent(AtkObject *accessible)
{
	AtkObject *parent;
	GtkWidget *widget, *parent_widget;

	widget=gtk_accessible_get_widget(GTK_ACCESSIBLE(accessible));
	if (widget==NULL) return NULL;
	parent=accessible->accessible_parent;
	if (parent!=NULL) return parent;
	parent_widget=gtk_widget_get_parent(widget);
	if (parent_widget==NULL) return NULL;
	if (GTK_IS_NOTEBOOK(parent_widget))
	{
		gint page_num=0;
		GtkWidget *child;
		GtkNotebook *notebook;

		notebook=GTK_NOTEBOOK(parent_widget);
		while (TRUE)
		{
			child=gtk_notebook_get_nth_page(notebook, page_num);
			if (!child) break;
			if (child==widget)
			{
				parent=gtk_widget_get_accessible(parent_widget);
				parent=atk_object_ref_accessible_child(parent, page_num);
				g_object_unref(parent);
				return parent;
			}
			page_num++;
		}
	}
	parent=gtk_widget_get_accessible(parent_widget);
	return parent;
}

static AtkRelationSet* gtk_plot_accessible_ref_relation_set(AtkObject *obj)
{
	AtkObject *array[1];
	AtkRelation* relation;
	AtkRelationSet *relation_set;
	GtkWidget *widget;

	widget=gtk_accessible_get_widget(GTK_ACCESSIBLE(obj));
	if (widget==NULL) return NULL;
	relation_set=ATK_OBJECT_CLASS(_gtk_plot_accessible_parent_class)->ref_relation_set(obj);
	if (!atk_relation_set_contains(relation_set, ATK_RELATION_LABELLED_BY))
	{
		GList *labels, *ptr;

		labels=gtk_widget_list_mnemonic_labels(widget);
		ptr=labels;
		while (ptr)
		{
			if (ptr->data)
			{
				array[0]=gtk_widget_get_accessible(ptr->data);
				relation=atk_relation_new(array, 1, ATK_RELATION_LABELLED_BY);
				atk_relation_set_add(relation_set, relation);
				g_object_unref(relation);
				break;
			}
			ptr=ptr->next;
		}
		g_list_free(labels);
	}
	return relation_set;
}

static AtkStateSet *gtk_plot_accessible_ref_state_set(AtkObject *accessible)
{
	GtkWidget *widget;
	AtkStateSet *state_set;

	state_set=ATK_OBJECT_CLASS(_gtk_plot_accessible_parent_class)->ref_state_set(accessible);
	widget=gtk_accessible_get_widget(GTK_ACCESSIBLE(accessible));
	if (widget == NULL) atk_state_set_add_state(state_set, ATK_STATE_DEFUNCT);
	else
	{
		if (gtk_widget_is_sensitive(widget))
		{
			atk_state_set_add_state(state_set, ATK_STATE_SENSITIVE);
			atk_state_set_add_state(state_set, ATK_STATE_ENABLED);
		}
		if (gtk_widget_get_can_focus(widget)) atk_state_set_add_state(state_set, ATK_STATE_FOCUSABLE);
		if (gtk_widget_get_visible(widget))
		{
			atk_state_set_add_state(state_set, ATK_STATE_VISIBLE);
			if (gtk_plot_accessible_on_screen(widget)&&gtk_widget_get_mapped(widget)&&gtk_plot_accessible_all_parents_visible(widget)) atk_state_set_add_state(state_set, ATK_STATE_SHOWING);
		}
		if (gtk_widget_has_focus(widget)&&!g_object_get_data(G_OBJECT(accessible), "gail-focus-object")) atk_state_set_add_state(state_set, ATK_STATE_FOCUSED);
		if (gtk_widget_has_default(widget)) atk_state_set_add_state(state_set, ATK_STATE_DEFAULT);
	}
	return state_set;
}

static gint gtk_plot_accessible_get_index_in_parent(AtkObject *accessible)
{
	gint index;
	GList *children;
	GtkWidget *widget;
	GtkWidget *parent_widget;

	widget=gtk_accessible_get_widget(GTK_ACCESSIBLE(accessible));
	if (widget==NULL) return -1;
	if (accessible->accessible_parent)
	{
		AtkObject *parent;
		gint n_children, i;
		gboolean found=FALSE;

		parent=accessible->accessible_parent;
		n_children=atk_object_get_n_accessible_children(parent);
		for (i=0;i<n_children;i++)
		{
			AtkObject *child;

			child=atk_object_ref_accessible_child(parent, i);
			if (child==accessible) found=TRUE;
			g_object_unref(child);
			if (found) return i;
		}
	}
	if (!GTK_IS_WIDGET(widget)) return -1;
	parent_widget=gtk_widget_get_parent(widget);
	if (!GTK_IS_CONTAINER(parent_widget)) return -1;
	children=gtk_container_get_children(GTK_CONTAINER(parent_widget));
	index=g_list_index(children, widget);
	g_list_free(children);
	return index;
}

static AtkAttributeSet *gtk_plot_accessible_get_attributes(AtkObject *obj)
{
	AtkAttribute *toolkit;
	AtkAttributeSet *attributes;

	toolkit=g_new(AtkAttribute, 1);
	toolkit->name=g_strdup("toolkit");
	toolkit->value=g_strdup("gtk");
	attributes=g_slist_append(NULL, toolkit);
	return attributes;
}

static void gtk_plot_accessible_focus_event(AtkObject *obj, gboolean focus_in)
{
	AtkObject *focus_obj;

	focus_obj=g_object_get_data(G_OBJECT (obj), "gail-focus-object");
	if (focus_obj==NULL) focus_obj=obj;
	atk_object_notify_state_change(focus_obj, ATK_STATE_FOCUSED, focus_in);
}

static void gtk_plot_accessible_initialise(AtkObject *obj, gpointer data)
{
	GtkWidget *widget;

	(ATK_OBJECT_CLASS(_gtk_plot_accessible_parent_class))->initialize(obj, data);
	widget=GTK_WIDGET(data);
	g_signal_connect_after(widget, "focus-in-event", G_CALLBACK(focus_cb), NULL);
	g_signal_connect_after(widget, "focus-out-event", G_CALLBACK(focus_cb), NULL);
	g_signal_connect(widget, "notify", G_CALLBACK(notify_cb), NULL);
	g_signal_connect(widget, "size-allocate", G_CALLBACK(size_allocate_cb), NULL);
	g_signal_connect(widget, "map", G_CALLBACK(map_cb), NULL);
	g_signal_connect(widget, "unmap", G_CALLBACK(map_cb), NULL);
	GTK_PLOT_ACCESSIBLE(obj)->layer=ATK_LAYER_WIDGET;
	obj->role=ATK_ROLE_CHART;
	g_object_set_data_full(G_OBJECT(GTK_PLOT_ACCESSIBLE(obj)), TOOLTIP_KEY, gtk_widget_get_tooltip_text(widget), g_free);
}

static void gtk_plot_accessible_finalise(GObject *obj) {(G_OBJECT_CLASS(_gtk_plot_accessible_parent_class))->finalize(obj);}

static void _gtk_plot_accessible_class_init(GtkPlotAccessibleClass *klass)
{
	AtkObjectClass* atk_klass;

	klass->notify_gtk=gtk_plot_accessible_notify_gtk;
	atk_klass=ATK_OBJECT_CLASS(klass);
	atk_klass->get_description=gtk_plot_accessible_get_description;
	atk_klass->get_parent=gtk_plot_accessible_get_parent;
	atk_klass->ref_relation_set=gtk_plot_accessible_ref_relation_set;
	atk_klass->ref_state_set=gtk_plot_accessible_ref_state_set;
	atk_klass->get_index_in_parent=gtk_plot_accessible_get_index_in_parent;
	atk_klass->get_attributes=gtk_plot_accessible_get_attributes;
	atk_klass->focus_event=gtk_plot_accessible_focus_event;
	atk_klass->initialize=gtk_plot_accessible_initialise;
	(G_OBJECT_CLASS(klass))->finalize=gtk_plot_accessible_finalise;
}

static void _gtk_plot_accessible_init(GtkPlotAccessible *plot) {}