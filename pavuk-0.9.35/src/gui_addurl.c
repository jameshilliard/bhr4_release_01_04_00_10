/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "gui.h"

#ifdef GTK_FACE
#include <string.h>
#include <gdk/gdkkeysyms.h>

#include "ainterface.h"

#include "icons/append.xpm"
#include "icons/cancel.xpm"

/*** DROP SIGNAL HANDLER ***/
static void window_drop_append_url(GtkWidget * widget,
  GdkDragContext * context, gint x, gint y, GtkSelectionData * seldata,
  guint info, guint time, gpointer data)
{
  gchar *p;
  gchar *drag_url;
  gchar *p_seldata;
  url_info *ui;

  if(!seldata || !seldata->data)
  {
    gtk_drag_finish(context, FALSE, FALSE, time);
    return;
  }

  p_seldata = (gchar *) seldata->data;

  /* strip away '\n' */
  p = strchr(p_seldata, '\n');
  if(p)
    drag_url = new_n_string(p_seldata, p - p_seldata);
  else
    drag_url = new_string(p_seldata);

  ui = url_info_new(drag_url);
  if(gui_cfg.config_shell)
  {
    url_info *cui;
    int row = gtk_clist_append(GTK_CLIST(gui_cfg.url_list), &drag_url);
    cui = url_info_duplicate(ui);
    gtk_clist_set_row_data_full(GTK_CLIST(gui_cfg.url_list), row, cui,
      (GtkDestroyNotify) url_info_free);
  }

  cfg.request = dllist_append(cfg.request, (dllist_t) ui);

  if(cfg.mode_started)
  {
    if(!append_starting_url(ui, NULL))
    {
      gdk_beep();
    }
  }

#ifdef DEBUG
  if(cfg.debug)
    xprintf(0, gettext("Dropped URL : %s\n"), drag_url);
#endif
  gtk_entry_set_text(GTK_ENTRY(data), drag_url);
  gtk_entry_select_region(GTK_ENTRY(data), 0, strlen(drag_url));
  _free(drag_url);

  gtk_drag_finish(context, TRUE, FALSE, time);
}

static void AppendURLCB(GtkObject * object, gpointer func_data)
{
  url_info *ui;
  char *p = (gchar *) gtk_entry_get_text(GTK_ENTRY(func_data));

  if(!p || !*p)
  {
    gdk_beep();
    return;
  }

  ui = url_info_new(p);
  if(gui_cfg.config_shell)
  {
    url_info *cui;
    int row = gtk_clist_append(GTK_CLIST(gui_cfg.url_list), &p);
    cui = url_info_duplicate(ui);
    gtk_clist_set_row_data_full(GTK_CLIST(gui_cfg.url_list), row, cui,
      (GtkDestroyNotify) url_info_free);
  }
  cfg.request = dllist_append(cfg.request, (dllist_t) ui);

  if(cfg.mode_started)
  {
    if(!append_starting_url(ui, NULL))
    {
      gdk_beep();
    }
  }
  gtk_entry_select_region(GTK_ENTRY(func_data), 0, strlen(p));
}

void gui_build_addurl(int popup)
{
  static GtkWidget *append_url_tl = NULL;
  GtkAccelGroup *accel_group;

  if(!append_url_tl)
  {
    GtkWidget *label, *box, *ptab, *button, *entry;

    append_url_tl = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_usize(append_url_tl, 300, -1);
    gtk_window_set_title(GTK_WINDOW(append_url_tl),
      gettext("Pavuk: Append URL"));
    gtk_signal_connect(GTK_OBJECT(append_url_tl), "destroy",
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &append_url_tl);

    box = gtk_vbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(append_url_tl), box);
    gtk_widget_show(box);

    ptab = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), ptab, FALSE, FALSE, 2);
    gtk_widget_show(ptab);

    label = gtk_label_new(gettext("New URL:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(ptab), label, FALSE, FALSE, 2);
    gtk_widget_show(label);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(ptab), entry, TRUE, TRUE, 2);
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
      GTK_SIGNAL_FUNC(AppendURLCB), (gpointer) entry);
    gtk_widget_show(entry);

  /*** DRAG'N'DROP ***/
    gtk_drag_dest_set(append_url_tl, GTK_DEST_DEFAULT_ALL,
      dragtypes, NUM_ELEM(dragtypes) - 1, GDK_ACTION_COPY | GDK_ACTION_MOVE);
    gtk_signal_connect(GTK_OBJECT(append_url_tl),
      "drag_data_received", GTK_SIGNAL_FUNC(window_drop_append_url), entry);

    label = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 2);
    gtk_widget_show(label);

    ptab = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(box), ptab, FALSE, FALSE, 2);
    gtk_widget_show(ptab);

    button = guitl_pixmap_button(append_xpm, NULL, gettext("Append"));
    gtk_container_add(GTK_CONTAINER(ptab), button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(AppendURLCB), (gpointer) entry);
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(button);
    gtk_widget_show(button);

    button = guitl_pixmap_button(cancel_xpm, NULL, gettext("Cancel"));
    accel_group = gtk_accel_group_new();
    gtk_widget_add_accelerator(button, "clicked", accel_group,
      GDK_Escape, 0, GTK_ACCEL_VISIBLE);
    gtk_window_add_accel_group(GTK_WINDOW(append_url_tl), accel_group);
    gtk_container_add(GTK_CONTAINER(ptab), button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(guitl_PopdownW), (gpointer) append_url_tl);
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_widget_show(button);
  }
  gtk_widget_show(append_url_tl);
  if(GTK_WIDGET_REALIZED(append_url_tl))
    gdk_window_raise(append_url_tl->window);
}

#endif /* GTK_FACE */
