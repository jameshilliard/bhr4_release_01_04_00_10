/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#ifdef GTK_FACE
#include <stdio.h>
#include <string.h>

#include "authinfo.h"
#include "gauthinfo.h"
#include "gui.h"

#include "icons/ok.xpm"
#include "icons/cancel.xpm"
#include "icons/append.xpm"
#include "icons/apply.xpm"
#include "icons/clear.xpm"
#include "icons/delete.xpm"
#include "icons/modify.xpm"
#include "icons/save.xpm"
#include "icons/load.xpm"


static GtkWidget *list;
static GtkWidget *topl = NULL;
static GtkWidget *w_prot;
static GtkWidget *w_host;
static GtkWidget *w_user;
static GtkWidget *w_pass;
static GtkWidget *w_base;
static GtkWidget *w_realm;
static GtkWidget *w_type[4];

static void fill_list(GtkWidget * clist)
{
  dllist *ptr;

  LOCK_AUTHINFO;
  ptr = authdata;

  while(ptr)
  {
    authinfo *ai = (authinfo *) ptr->data;
    char *ad[7];
    char pom[10];
    char host[100];

    ad[0] = prottable[ai->prot].urlid;
    snprintf(host, sizeof(host), "%s:%d", ai->host, ai->port);
    ad[1] = host;
    ad[2] = ai->user ? ai->user : "";
    ad[3] = ai->pass ? ai->pass : "";
    ad[4] = ai->base ? ai->base : "";
    ad[5] = ai->realm ? ai->realm : "";

    sprintf(pom, "%d", ai->type);
    ad[6] = pom;

    gtk_clist_append(GTK_CLIST(clist), ad);

    ptr = ptr->next;
  }
  UNLOCK_AUTHINFO;
}

static void PopdownW(GtkObject * object, gpointer func_data)
{
  gtk_widget_destroy(GTK_WIDGET(func_data));
}

static void SaveAI(GtkWidget * w, gpointer data)
{
  char *fn =
    (gchar *) gtk_file_selection_get_filename(GTK_FILE_SELECTION(data));

  if(authinfo_save(fn))
  {
    gdk_beep();
  }
  else
    gtk_widget_destroy(GTK_WIDGET(data));
}

static void Save(GtkWidget * w, gpointer data)
{
  static GtkWidget *fn = NULL;

  if(fn)
  {
    gtk_widget_show_all(fn);
    if(GTK_WIDGET_REALIZED(fn))
      gdk_window_raise(fn->window);

    return;
  }

  fn = gtk_file_selection_new(gettext("Pavuk: Save auth. info file"));

  gtk_signal_connect(GTK_OBJECT(fn), "destroy",
    GTK_SIGNAL_FUNC(gtk_widget_destroyed), &fn);

  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fn)->ok_button),
    "clicked", GTK_SIGNAL_FUNC(SaveAI), fn);

  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fn)->cancel_button),
    "clicked", GTK_SIGNAL_FUNC(PopdownW), fn);

  if(cfg.auth_file)
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(fn), cfg.auth_file);

  gtk_widget_show_all(fn);
}

static void LoadAI(GtkWidget * w, gpointer data)
{
  char *fn =
    (gchar *) gtk_file_selection_get_filename(GTK_FILE_SELECTION(data));

  if(authinfo_load(fn))
  {
    gdk_beep();
  }
  else
  {
    gtk_clist_freeze(GTK_CLIST(list));
    gtk_clist_clear(GTK_CLIST(list));
    fill_list(list);
    gtk_clist_thaw(GTK_CLIST(list));
    gtk_widget_destroy(GTK_WIDGET(data));
  }
}

static void Load(GtkWidget * w, gpointer data)
{
  static GtkWidget *fn = NULL;

  if(fn)
  {
    gtk_widget_show_all(fn);
    if(GTK_WIDGET_REALIZED(fn))
      gdk_window_raise(fn->window);

    return;
  }

  fn = gtk_file_selection_new(gettext("Pavuk: Load auth. info file"));

  gtk_signal_connect(GTK_OBJECT(fn), "destroy",
    GTK_SIGNAL_FUNC(gtk_widget_destroyed), &fn);

  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fn)->ok_button),
    "clicked", GTK_SIGNAL_FUNC(LoadAI), fn);

  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fn)->cancel_button),
    "clicked", GTK_SIGNAL_FUNC(PopdownW), fn);

  if(cfg.auth_file)
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(fn), cfg.auth_file);

  gtk_widget_show_all(fn);
}

static void Apply(GtkObject * object, gpointer func_data)
{
  int i, j;

  LOCK_AUTHINFO;
  while(authdata)
  {
    authinfo *ai = (authinfo *) authdata->data;

    free_deep_authinfo(ai);
    authdata = dllist_remove_entry(authdata, authdata);
  }

  for(i = 0; i < GTK_CLIST(func_data)->rows; i++)
  {
    char *ld[7];
    char *p;
    authinfo *ai;

    for(j = 0; j < 7; j++)
      gtk_clist_get_text(GTK_CLIST(func_data), i, j, &ld[j]);

    ai = _malloc(sizeof(authinfo));

    for(j = 0; j < NUM_ELEM(prottable); j++)
    {
      if(prottable[j].urlid && !strcmp(ld[0], prottable[j].urlid))
      {
        ai->prot = j;
        break;
      }
    }
    p = strchr(ld[1], ':');
    ai->host = p ? new_n_string(ld[1], p - ld[1]) : new_string(ld[1]);
    ai->port = p ? atoi(p + 1) : prottable[ai->prot].default_port;
    ai->user = new_string(ld[2]);
    ai->pass = new_string(ld[3]);
    ai->base = ld[4] && *ld[4] ? new_string(ld[4]) : NULL;
    ai->realm = ld[5] && *ld[5] ? new_string(ld[5]) : NULL;
    ai->type = atoi(ld[6]);

    authdata = dllist_append(authdata, (dllist_t)ai);
  }
  UNLOCK_AUTHINFO;
}

static void CopyFromList(GtkObject * object, int row, int col,
  GdkEvent * event, gpointer data)
{
  char *p;
  int i, j;

  if(GTK_CLIST(object)->selection)
  {
    gtk_clist_get_text(GTK_CLIST(object), row, 0, &p);
    for(i = 0, j = 0; i < NUM_ELEM(prottable); i++)
    {
      if(prottable[i].urlid && !strcmp(p, prottable[i].urlid))
      {
        gtk_option_menu_set_history(GTK_OPTION_MENU(w_prot), j);
        break;
      }

      if(prottable[i].supported)
        j++;
    }

    gtk_clist_get_text(GTK_CLIST(object), row, 1, &p);
    gtk_entry_set_text(GTK_ENTRY(w_host), p);

    gtk_clist_get_text(GTK_CLIST(object), row, 2, &p);
    gtk_entry_set_text(GTK_ENTRY(w_user), p);

    gtk_clist_get_text(GTK_CLIST(object), row, 3, &p);
    gtk_entry_set_text(GTK_ENTRY(w_pass), p);

    gtk_clist_get_text(GTK_CLIST(object), row, 4, &p);
    gtk_entry_set_text(GTK_ENTRY(w_base), p);

    gtk_clist_get_text(GTK_CLIST(object), row, 5, &p);
    gtk_entry_set_text(GTK_ENTRY(w_realm), p);

    gtk_clist_get_text(GTK_CLIST(object), row, 6, &p);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(w_type[atoi(p) ? atoi(p) -
          1 : 1]), TRUE);
  }
}

static void NewRow(int row, GtkWidget * list)
{
  char *ad[7];
  char pom[10];

  ad[0] =
    prottable[(long)
    gtk_object_get_user_data(GTK_OBJECT(GTK_OPTION_MENU(w_prot)->menu_item))].
    urlid;

  ad[1] = (gchar *) gtk_entry_get_text(GTK_ENTRY(w_host));
  ad[2] = (gchar *) gtk_entry_get_text(GTK_ENTRY(w_user));
  ad[3] = (gchar *) gtk_entry_get_text(GTK_ENTRY(w_pass));
  ad[4] = (gchar *) gtk_entry_get_text(GTK_ENTRY(w_base));
  ad[5] = (gchar *) gtk_entry_get_text(GTK_ENTRY(w_realm));

#ifdef ENABLE_NTLM
  sprintf(pom, "%d",
    GTK_TOGGLE_BUTTON(w_type[0])->active ? 1 : (GTK_TOGGLE_BUTTON(w_type[1])->
      active ? 2 : (GTK_TOGGLE_BUTTON(w_type[2])->active ? 3 : 4)));
#else
  sprintf(pom, "%d",
    GTK_TOGGLE_BUTTON(w_type[0])->active ? 1 : (GTK_TOGGLE_BUTTON(w_type[1])->
      active ? 2 : 3));
#endif
  ad[6] = pom;

  if(ad[1] && *ad[1] && ad[2] && *ad[2] && ad[3] && *ad[3])
  {
    if(row < 0)
      gtk_clist_append(GTK_CLIST(list), ad);
    else
    {
      int i;

      for(i = 0; i < 7; i++)
        gtk_clist_set_text(GTK_CLIST(list), row, i, ad[i]);
    }
  }
  else
  {
    gdk_beep();
  }
}

static void Append(GtkObject * object, gpointer func_data)
{
  NewRow(-1, func_data);
}

static void Modify(GtkObject * object, gpointer func_data)
{
  if(!GTK_CLIST(func_data)->selection)
  {
    gdk_beep();
    return;
  }

  NewRow(GPOINTER_TO_INT(GTK_CLIST(func_data)->selection->data), func_data);
}

void gauthinfo_run(void)
{
  GtkWidget *box, *swin, *tbox, *frame, *label, *pbox;
  GtkWidget *menu, *mi, *brow, *button;
  GSList *rg;
  int i;

  if(!topl)
  {
    topl = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_border_width(GTK_CONTAINER(topl), 3);
    gtk_window_set_title(GTK_WINDOW(topl),
      gettext("Pavuk: Authorization info editor"));

    gtk_signal_connect(GTK_OBJECT(topl), "destroy",
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &topl);

    box = gtk_vbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(topl), box);
    gtk_widget_show(box);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_usize(swin, 550, 300);
    gtk_box_pack_start(GTK_BOX(box), swin, TRUE, TRUE, 2);
    gtk_widget_show(swin);

    list = gtk_clist_new(7);
    gtk_clist_set_column_title(GTK_CLIST(list), 0, gettext("Protocol"));
    gtk_clist_set_column_title(GTK_CLIST(list), 1, gettext("Host"));
    gtk_clist_set_column_title(GTK_CLIST(list), 2, gettext("User"));
    gtk_clist_set_column_title(GTK_CLIST(list), 3, gettext("Password"));
    gtk_clist_set_column_title(GTK_CLIST(list), 4, gettext("Base dir."));
    gtk_clist_set_column_title(GTK_CLIST(list), 5, gettext("Realm"));
    gtk_clist_set_column_title(GTK_CLIST(list), 6, gettext("Scheme"));
    gtk_clist_column_titles_show(GTK_CLIST(list));
    gtk_clist_set_column_auto_resize(GTK_CLIST(list), 0, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(list), 1, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(list), 2, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(list), 3, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(list), 4, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(list), 5, TRUE);
    gtk_container_add(GTK_CONTAINER(swin), list);

    fill_list(list);

    gtk_widget_show(list);

    gtk_signal_connect(GTK_OBJECT(list), "select_row",
      GTK_SIGNAL_FUNC(CopyFromList), NULL);

    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 1);
    gtk_widget_show(frame);

    tbox = gtk_table_new(4, 5, FALSE);
    gtk_container_add(GTK_CONTAINER(frame), tbox);
    gtk_widget_show(tbox);

    label = gtk_label_new(gettext("Protocol: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(tbox), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL,
      5, 2);
    gtk_widget_show(label);

    w_prot = gtk_option_menu_new();

    menu = gtk_menu_new();
    gtk_widget_realize(menu);

    for(i = 0; i < NUM_ELEM(prottable); i++)
    {
      if(prottable[i].supported)
      {
        mi = gtk_menu_item_new_with_label(prottable[i].urlid);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) prottable[i].id);
        gtk_widget_show(mi);
      }
    }

    gtk_option_menu_set_menu(GTK_OPTION_MENU(w_prot), menu);
    gtk_table_attach(GTK_TABLE(tbox), w_prot, 1, 2, 0, 1, GTK_EXPAND,
      GTK_FILL, 5, 2);
    gtk_widget_show(w_prot);

    label = gtk_label_new(gettext("Host: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(tbox), label, 2, 3, 0, 1, GTK_FILL, GTK_FILL,
      5, 2);
    gtk_widget_show(label);

    w_host = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(tbox), w_host, 3, 4, 0, 1, GTK_EXPAND,
      GTK_FILL, 5, 2);
    gtk_widget_show(w_host);

    label = gtk_label_new(gettext("User: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(tbox), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL,
      5, 2);
    gtk_widget_show(label);

    w_user = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(tbox), w_user, 1, 2, 1, 2, GTK_EXPAND,
      GTK_FILL, 5, 2);
    gtk_widget_show(w_user);

    label = gtk_label_new(gettext("Password: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(tbox), label, 2, 3, 1, 2, GTK_FILL, GTK_FILL,
      5, 2);
    gtk_widget_show(label);

    w_pass = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(tbox), w_pass, 3, 4, 1, 2, GTK_EXPAND,
      GTK_FILL, 5, 2);
    gtk_widget_show(w_pass);

    label = gtk_label_new(gettext("Base directory: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(tbox), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL,
      5, 2);
    gtk_widget_show(label);

    w_base = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(tbox), w_base, 1, 2, 2, 3, GTK_EXPAND,
      GTK_FILL, 5, 2);
    gtk_widget_show(w_base);

    label = gtk_label_new(gettext("Realm: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(tbox), label, 2, 3, 2, 3, GTK_FILL, GTK_FILL,
      5, 2);
    gtk_widget_show(label);

    w_realm = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(tbox), w_realm, 3, 4, 2, 3, GTK_EXPAND,
      GTK_FILL, 5, 2);
    gtk_widget_show(w_realm);


    frame = gtk_frame_new("Scheme");
    gtk_table_attach(GTK_TABLE(tbox), frame, 0, 4, 3, 4, GTK_FILL, GTK_FILL,
      5, 2);
    gtk_widget_show(frame);

    pbox = gtk_hbox_new(TRUE, 5);
    gtk_container_add(GTK_CONTAINER(frame), pbox);
    gtk_widget_show(pbox);

    w_type[0] =
      gtk_radio_button_new_with_label(NULL, gettext("User auth. scheme"));
    rg = gtk_radio_button_group(GTK_RADIO_BUTTON(w_type[0]));
    gtk_widget_show(w_type[0]);
    gtk_container_add(GTK_CONTAINER(pbox), w_type[0]);

    w_type[1] =
      gtk_radio_button_new_with_label(rg, gettext("Base auth. scheme"));
    rg = gtk_radio_button_group(GTK_RADIO_BUTTON(w_type[1]));
    gtk_widget_show(w_type[1]);
    gtk_container_add(GTK_CONTAINER(pbox), w_type[1]);

    w_type[2] =
      gtk_radio_button_new_with_label(rg, gettext("Digest auth. scheme"));
    rg = gtk_radio_button_group(GTK_RADIO_BUTTON(w_type[2]));
    gtk_widget_show(w_type[2]);
    gtk_container_add(GTK_CONTAINER(pbox), w_type[2]);

#ifdef ENABLE_NTLM
    w_type[3] =
      gtk_radio_button_new_with_label(rg, gettext("NTLM auth. scheme"));
    rg = gtk_radio_button_group(GTK_RADIO_BUTTON(w_type[3]));
    gtk_widget_show(w_type[3]);
    gtk_container_add(GTK_CONTAINER(pbox), w_type[3]);
#endif

    brow = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(brow), GTK_BUTTONBOX_SPREAD);
    gtk_table_attach(GTK_TABLE(tbox), brow, 0, 4, 4, 5, GTK_EXPAND, GTK_FILL,
      5, 5);
    gtk_widget_show(brow);

    button = guitl_pixmap_button(append_xpm, NULL, gettext("Append"));
    gtk_container_border_width(GTK_CONTAINER(button), 0);
    gtk_container_add(GTK_CONTAINER(brow), button);
    gtk_widget_show(button);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(Append), list);

    button = guitl_pixmap_button(modify_xpm, NULL, gettext("Modify"));
    gtk_container_border_width(GTK_CONTAINER(button), 0);
    gtk_container_add(GTK_CONTAINER(brow), button);
    gtk_widget_show(button);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(Modify), list);

    button = guitl_pixmap_button(clear_xpm, NULL, gettext("Clear"));
    gtk_container_border_width(GTK_CONTAINER(button), 0);
    gtk_container_add(GTK_CONTAINER(brow), button);
    gtk_widget_show(button);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(guitl_ListClear), list);

    button = guitl_pixmap_button(delete_xpm, NULL, gettext("Delete"));
    gtk_container_border_width(GTK_CONTAINER(button), 0);
    gtk_container_add(GTK_CONTAINER(brow), button);
    gtk_widget_show(button);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(guitl_ListDeleteSelected), list);

    brow = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(brow), GTK_BUTTONBOX_SPREAD);
    gtk_box_pack_start(GTK_BOX(box), brow, FALSE, TRUE, 5);
    gtk_widget_show(brow);

    button = guitl_pixmap_button(ok_xpm, NULL, gettext("OK"));
    gtk_container_border_width(GTK_CONTAINER(button), 0);
    gtk_container_add(GTK_CONTAINER(brow), button);
    gtk_widget_show(button);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(Apply), list);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(PopdownW), topl);

    button = guitl_pixmap_button(apply_xpm, NULL, gettext("Apply"));
    gtk_container_border_width(GTK_CONTAINER(button), 0);
    gtk_container_add(GTK_CONTAINER(brow), button);
    gtk_widget_show(button);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(Apply), list);

    button = guitl_pixmap_button(load_xpm, NULL, gettext("Load"));
    gtk_container_border_width(GTK_CONTAINER(button), 0);
    gtk_container_add(GTK_CONTAINER(brow), button);
    gtk_widget_show(button);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(Load), NULL);

    button = guitl_pixmap_button(save_xpm, NULL, gettext("Save"));
    gtk_container_border_width(GTK_CONTAINER(button), 0);
    gtk_container_add(GTK_CONTAINER(brow), button);
    gtk_widget_show(button);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(Apply), list);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(Save), NULL);

    button = guitl_pixmap_button(cancel_xpm, NULL, gettext("Cancel"));
    gtk_container_border_width(GTK_CONTAINER(button), 0);
    gtk_container_add(GTK_CONTAINER(brow), button);
    gtk_widget_show(button);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(PopdownW), topl);
  }
  gtk_widget_show(topl);
  if(GTK_WIDGET_REALIZED(topl))
    gdk_window_raise(topl->window);
}
#endif
