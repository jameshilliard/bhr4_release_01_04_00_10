/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/
#include "config.h"

#ifdef GTK_FACE
#include "gui.h"

#include <gdk/gdkkeysyms.h>
#include <string.h>

#include "uconfig.h"
#include "lfname.h"
#include "lang.h"
#include "charset.h"
#include "form.h"
#include "http.h"
#include "ftp.h"
#include "html.h"
#include "jstrans.h"

#include "icons/append.xpm"
#include "icons/modify.xpm"
#include "icons/clear.xpm"
#include "icons/delete.xpm"
#include "icons/ok.xpm"
#include "icons/apply.xpm"
#include "icons/limit.xpm"
#include "icons/cancel.xpm"

static void CfgCommon(GtkObject * object, gpointer func_data)
{
  if(!xget_cfg_values_comm())
  {
    if(cfg.use_prefs)
      cfg_dump_pref();
  }
}

/*** URL ***/

struct cfg_url_dlg_t
{
  GtkWidget *url_list;
  GtkWidget *url_entry;
  GtkWidget *localname_entry;
  GtkWidget *ext_sw;
  GtkWidget *ext_frame;
  GtkWidget *method;
  GtkWidget *encoding;
  GtkWidget *field_list;
  GtkWidget *field_type;
  GtkWidget *field_name;
  GtkWidget *field_value_l;
  GtkWidget *field_value_t;
  GtkWidget *field_value_f;
  GtkWidget *tab;
};

static url_info *cfg_url_dlg_get_url_info(struct cfg_url_dlg_t *urldlg)
{
  url_info *ui;
  char *p;
  int row;

  p = (gchar *) gtk_entry_get_text(GTK_ENTRY(urldlg->url_entry));
  ui = url_info_new(p);

  ui->type = URLI_FORM;

  if(urldlg->localname_entry)
  {
    p = (gchar *) gtk_entry_get_text(GTK_ENTRY(urldlg->localname_entry));
    if(p && *p)
      ui->localname = tl_strdup(p);
  }

  if(urldlg->method)
    ui->method = GTK_OPTION_MENU(urldlg->method)->menu_item ?
      (int) gtk_object_get_user_data(GTK_OBJECT(GTK_OPTION_MENU(urldlg->
          method)->menu_item)) : FORM_M_GET;

  if(urldlg->encoding)
    ui->encoding = GTK_OPTION_MENU(urldlg->encoding)->menu_item ?
      (int) gtk_object_get_user_data(GTK_OBJECT(GTK_OPTION_MENU(urldlg->
          encoding)->menu_item)) : FORM_E_URLENCODED;

  for(row = 0; row < GTK_CLIST(urldlg->field_list)->rows; row++)
  {
    form_field *ff;

    ff = _malloc(sizeof(form_field));

    ff->type =
      (int) gtk_clist_get_row_data(GTK_CLIST(urldlg->field_list), row);

    gtk_clist_get_text(GTK_CLIST(urldlg->field_list), row, 1, &p);
    ff->name = form_decode_urlencoded_str(p, strlen(p));

    gtk_clist_get_text(GTK_CLIST(urldlg->field_list), row, 2, &p);
    ff->value = form_decode_urlencoded_str(p, strlen(p));

    ui->fields = dllist_append(ui->fields, (dllist_t) ff);
  }

  return ui;
}

static void cfg_url_dlg_append(GtkWidget * w, struct cfg_url_dlg_t *urldlg)
{
  char *p, *s, *ln;
  char *pp[2];
  bool_t ok = TRUE;
  int row;

  p = (gchar *) gtk_entry_get_text(GTK_ENTRY(urldlg->url_entry));

  if(p && *p)
  {
    url_info *ui = NULL;
    protocol pt = URLT_UNKNOWN;

    s = url_parse_scheme(p);

    if(s)
    {
      pt = url_scheme_to_schemeid(s);
      ok = prottable[pt].supported;
      _free(s);
    }
    else
      pt = URLT_HTTP;

    if(!ok)
    {
      /* unsupported URL type */
      gdk_beep();
      return;
    }

    if((urldlg->ext_sw && GTK_TOGGLE_BUTTON(urldlg->ext_sw)->active) ||
      !urldlg->ext_sw)
    {
      if(pt != URLT_HTTP && pt != URLT_HTTPS)
      {
        /* extended request supported only with HTTP */
        gdk_beep();
        return;
      }
      ui = cfg_url_dlg_get_url_info(urldlg);
      ln = ui->localname;
    }
    else
      ln = NULL;

    if(!ui && urldlg->localname_entry)
    {
      ln = (gchar *) gtk_entry_get_text(GTK_ENTRY(urldlg->localname_entry));
      if(ln && *ln)
      {
        ui = url_info_new(p);
        ui->type = URLI_NORMAL;
        ui->localname = tl_strdup(ln);
      }
    }

    gtk_clist_freeze(GTK_CLIST(urldlg->url_list));

    pp[0] = p;
    pp[1] = ln;

    row = gtk_clist_append(GTK_CLIST(urldlg->url_list), pp);

    gtk_clist_set_row_data_full(GTK_CLIST(urldlg->url_list), row, ui,
      (GtkDestroyNotify) url_info_free);
    gtk_clist_thaw(GTK_CLIST(urldlg->url_list));
    gtk_clist_select_row(GTK_CLIST(urldlg->url_list), row, 0);
  }
  else
    gdk_beep();
}

static void cfg_url_dlg_modify(GtkWidget * w, struct cfg_url_dlg_t *urldlg)
{
  char *p, *s, *ln;
  bool_t ok = TRUE;
  int row;

  if(!GTK_CLIST(urldlg->url_list)->selection)
  {
    gdk_beep();
    return;
  }

  row = GPOINTER_TO_INT(GTK_CLIST(urldlg->url_list)->selection->data);

  p = (gchar *) gtk_entry_get_text(GTK_ENTRY(urldlg->url_entry));

  if(p && *p)
  {
    url_info *ui = NULL;
    protocol pt = URLT_UNKNOWN;

    s = url_parse_scheme(p);

    if(s)
    {
      pt = url_scheme_to_schemeid(s);
      ok = prottable[pt].supported;
      _free(s);
    }
    else
      pt = URLT_HTTP;

    if(!ok)
    {
      /* unsupported URL type !!! */
    }

/*
    ui = (url_info *) gtk_clist_get_row_data(GTK_CLIST(urldlg->url_list), row);
    if (ui)
      url_info_free(ui);
    ui = NULL;
*/

    if((urldlg->ext_sw && GTK_TOGGLE_BUTTON(urldlg->ext_sw)->active) ||
      !urldlg->ext_sw)
    {
      if(pt != URLT_HTTP && pt != URLT_HTTPS)
      {
        /* extended request supported only with HTTP */
        gdk_beep();
        return;
      }
      ui = cfg_url_dlg_get_url_info(urldlg);
      ln = ui->localname;
    }
    else
      ln = NULL;

    if(!ui && urldlg->localname_entry)
    {
      ln = (gchar *) gtk_entry_get_text(GTK_ENTRY(urldlg->localname_entry));

      if(ln && *ln)
      {
        ui = url_info_new(p);
        ui->type = URLI_NORMAL;
        ui->localname = tl_strdup(ln);
      }
      gtk_clist_set_text(GTK_CLIST(urldlg->url_list), row, 1, ln);
    }

    gtk_clist_set_text(GTK_CLIST(urldlg->url_list), row, 1, ln);
    gtk_clist_set_text(GTK_CLIST(urldlg->url_list), row, 0, p);

    gtk_clist_set_row_data_full(GTK_CLIST(urldlg->url_list), row, ui,
      (GtkDestroyNotify) url_info_free);
    gtk_clist_select_row(GTK_CLIST(urldlg->url_list), row, 0);
  }
  else
    gdk_beep();
}

static void cfg_url_dlg_einfo_clear(struct cfg_url_dlg_t *urldlg)
{
  guitl_ListClear(NULL, urldlg->field_list);
}

static void cfg_url_dlg_switch(GtkObject * w, int row, int col,
  GdkEvent * event, struct cfg_url_dlg_t *urldlg)
{
  if(GTK_CLIST(w)->selection)
  {
    url_info *ui;
    char *p;

    ui =
      (url_info *) gtk_clist_get_row_data(GTK_CLIST(urldlg->url_list), row);

    if(urldlg->ext_sw)
    {
      gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(urldlg->ext_sw),
        ui ? (ui->type != URLI_NORMAL) : FALSE);
      gtk_widget_set_sensitive(urldlg->ext_frame,
        ui ? (ui->type != URLI_NORMAL) : FALSE);
    }

    gtk_clist_get_text(GTK_CLIST(urldlg->url_list), row, 0, &p);

    gtk_entry_set_text(GTK_ENTRY(urldlg->url_entry), p);

    if(urldlg->localname_entry)
    {
      p = "";
      gtk_clist_get_text(GTK_CLIST(urldlg->url_list), row, 1, &p);
      gtk_entry_set_text(GTK_ENTRY(urldlg->localname_entry), p);
    }

    if(ui && ui->type != URLI_NORMAL)
    {
      dllist *ptr;

      if(urldlg->method)
        gtk_option_menu_set_history(GTK_OPTION_MENU(urldlg->method),
          ui->method);
      if(urldlg->encoding)
        gtk_option_menu_set_history(GTK_OPTION_MENU(urldlg->encoding),
          ui->encoding);

      gtk_clist_clear(GTK_CLIST(urldlg->field_list));

      ptr = ui->fields;
      while(ptr)
      {
        char *p3[3];
        int row;
        form_field *ff = (form_field *) ptr->data;

        switch (ff->type)
        {
        case FORM_T_FILE:
          p3[0] = "FILE";
          break;
        case FORM_T_TEXTAREA:
          p3[0] = "LONG TEXT";
          break;
        default:
          p3[0] = "TEXT";
        }

        p3[1] = form_encode_urlencoded_str(ff->name);
        p3[2] = form_encode_urlencoded_str(ff->value);

        row = gtk_clist_append(GTK_CLIST(urldlg->field_list), p3);
        gtk_clist_set_row_data(GTK_CLIST(urldlg->field_list),
          row, (gpointer) ff->type);

        _free(p3[1]);
        _free(p3[2]);
        ptr = ptr->next;
      }
      if(ui->fields)
        gtk_clist_select_row(GTK_CLIST(urldlg->field_list), 0, 0);
    }
    else
      cfg_url_dlg_einfo_clear(urldlg);
  }
  else
  {
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(urldlg->ext_sw), FALSE);
    gtk_widget_set_sensitive(urldlg->ext_frame, FALSE);
    gtk_entry_set_text(GTK_ENTRY(urldlg->url_entry), "");
    if(urldlg->localname_entry)
      gtk_entry_set_text(GTK_ENTRY(urldlg->localname_entry), "");
    cfg_url_dlg_einfo_clear(urldlg);
  }
}

static void cfg_url_dlg_field_append(GtkWidget * w,
  struct cfg_url_dlg_t *urldlg)
{
  char *p3[3];
  int type;

  type = GTK_OPTION_MENU(urldlg->field_type)->menu_item ?
    (int) gtk_object_get_user_data(GTK_OBJECT(GTK_OPTION_MENU(urldlg->
        field_type)->menu_item)) : 0;

  switch (type)
  {
  case FORM_T_FILE:
    p3[0] = "FILE";
    p3[2] = form_encode_urlencoded_str(
      (gchar *) gtk_entry_get_text(GTK_ENTRY(urldlg->field_value_f)));
    break;
  case FORM_T_TEXTAREA:
    {
      int len;
      char *p;

      p3[0] = "LONG TEXT";
#if GTK_FACE < 2
      len = gtk_text_get_length(GTK_TEXT(urldlg->field_value_t));
#else
      len = gtk_text_buffer_get_char_count(
          gtk_text_view_get_buffer(GTK_TEXT_VIEW(urldlg->field_value_t)));
#endif
      p = gtk_editable_get_chars(GTK_EDITABLE(urldlg->field_value_t), 0, len);
      p3[2] = form_encode_urlencoded_str(p);
      g_free(p);
    }
    break;
  default:
    p3[0] = "TEXT";
    p3[2] = form_encode_urlencoded_str(
      (gchar *) gtk_entry_get_text(GTK_ENTRY(urldlg->field_value_l)));
  }

  p3[1] =
    form_encode_urlencoded_str((gchar *) gtk_entry_get_text(GTK_ENTRY(urldlg->
        field_name)));

  if(p3[0] && p3[1] && *p3[0] && *p3[1])
  {
    int row = gtk_clist_append(GTK_CLIST(urldlg->field_list), p3);
    gtk_clist_set_row_data(GTK_CLIST(urldlg->field_list),
      row, (gpointer) type);
  }
  else
    gdk_beep();

  _free(p3[2]);
  _free(p3[1]);
}

static void cfg_url_dlg_field_modify(GtkWidget * w,
  struct cfg_url_dlg_t *urldlg)
{
  char *p3[3];
  int type;
  int row;

  if(!GTK_CLIST(urldlg->field_list)->selection)
  {
    gdk_beep();
    return;
  }

  row = GPOINTER_TO_INT(GTK_CLIST(urldlg->field_list)->selection->data);

  type = GTK_OPTION_MENU(urldlg->field_type)->menu_item ?
    (int) gtk_object_get_user_data(GTK_OBJECT(GTK_OPTION_MENU(urldlg->
        field_type)->menu_item)) : 0;

  switch (type)
  {
  case FORM_T_FILE:
    p3[0] = "FILE";
    p3[2] = form_encode_urlencoded_str(
      (gchar *) gtk_entry_get_text(GTK_ENTRY(urldlg->field_value_f)));
    break;
  case FORM_T_TEXTAREA:
    {
      int len;
      char *p;

      p3[0] = "LONG TEXT";
#if GTK_FACE < 2
      len = gtk_text_get_length(GTK_TEXT(urldlg->field_value_t));
#else
      len = gtk_text_buffer_get_char_count(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(urldlg->field_value_t)));
#endif
      p = gtk_editable_get_chars(GTK_EDITABLE(urldlg->field_value_t), 0, len);
      p3[2] = form_encode_urlencoded_str(p);
      g_free(p);
    }
    break;
  default:
    p3[0] = "TEXT";
    p3[2] = form_encode_urlencoded_str(
      (gchar *) gtk_entry_get_text(GTK_ENTRY(urldlg->field_value_l)));
  }

  p3[1] =
    form_encode_urlencoded_str((gchar *) gtk_entry_get_text(GTK_ENTRY(urldlg->
        field_name)));

  if(p3[0] && p3[1] && *p3[0] && *p3[1])
  {
    gtk_clist_set_text(GTK_CLIST(urldlg->field_list), row, 0, p3[0]);
    gtk_clist_set_text(GTK_CLIST(urldlg->field_list), row, 1, p3[1]);
    gtk_clist_set_text(GTK_CLIST(urldlg->field_list), row, 2, p3[2]);
    gtk_clist_set_row_data(GTK_CLIST(urldlg->field_list),
      row, (gpointer) type);
  }
  else
    gdk_beep();

  _free(p3[2]);
  _free(p3[1]);
}

static void cfg_url_dlg_set_type(GtkWidget * w, struct cfg_url_dlg_t *urldlg)
{
  int type;
  int tab;

  type = (int) gtk_object_get_user_data(GTK_OBJECT(w));

  switch (type)
  {
  case FORM_T_FILE:
    tab = 1;
    break;
  case FORM_T_TEXTAREA:
    tab = 2;
    break;
  default:
    tab = 0;
  }

  gtk_notebook_set_page(GTK_NOTEBOOK(urldlg->tab), tab);
}

static void cfg_url_dlg_field_switch(GtkObject * w, int row, int col,
  GdkEvent * event, struct cfg_url_dlg_t *urldlg)
{
  if(GTK_CLIST(w)->selection)
  {
    char *p;
    int type;

    type = (int) gtk_clist_get_row_data(GTK_CLIST(urldlg->field_list), row);

    gtk_clist_get_text(GTK_CLIST(urldlg->field_list), row, 1, &p);
    p = form_decode_urlencoded_str(p, strlen(p));
    gtk_entry_set_text(GTK_ENTRY(urldlg->field_name), p);
    _free(p);

    gtk_clist_get_text(GTK_CLIST(urldlg->field_list), row, 2, &p);
    p = form_decode_urlencoded_str(p, strlen(p));

    switch (type)
    {
    case FORM_T_FILE:
      gtk_option_menu_set_history(GTK_OPTION_MENU(urldlg->field_type), 1);
      gtk_entry_set_text(GTK_ENTRY(urldlg->field_value_f), p);
      gtk_notebook_set_page(GTK_NOTEBOOK(urldlg->tab), 1);
      break;
    case FORM_T_TEXTAREA:
      gtk_option_menu_set_history(GTK_OPTION_MENU(urldlg->field_type), 2);
#if GTK_FACE < 2
      gtk_text_set_point(GTK_TEXT(urldlg->field_value_t), 0);
      gtk_text_forward_delete(GTK_TEXT(urldlg->field_value_t),
        gtk_text_get_length(GTK_TEXT(urldlg->field_value_t)));
      gtk_text_insert(GTK_TEXT(urldlg->field_value_t), NULL, NULL, NULL, p,
        strlen(p));
#else
      gtk_text_buffer_set_text(gtk_text_view_get_buffer(
      GTK_TEXT_VIEW(urldlg->field_value_t)), p, -1);
#endif
      gtk_notebook_set_page(GTK_NOTEBOOK(urldlg->tab), 2);
      break;
    default:
      gtk_option_menu_set_history(GTK_OPTION_MENU(urldlg->field_type), 0);
      gtk_entry_set_text(GTK_ENTRY(urldlg->field_value_l), p);
      gtk_notebook_set_page(GTK_NOTEBOOK(urldlg->tab), 0);
    }
    _free(p);
  }
  else
  {
    gtk_entry_set_text(GTK_ENTRY(urldlg->field_name), "");
    gtk_entry_set_text(GTK_ENTRY(urldlg->field_value_f), "");
    gtk_entry_set_text(GTK_ENTRY(urldlg->field_value_l), "");
#if GTK_FACE < 2
    gtk_text_set_point(GTK_TEXT(urldlg->field_value_t), 0);
    gtk_text_forward_delete(GTK_TEXT(urldlg->field_value_t),
      gtk_text_get_length(GTK_TEXT(urldlg->field_value_t)));
#else
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(
    GTK_TEXT_VIEW(urldlg->field_value_t)), "", 0);
#endif
  }
}

static void cfg_url_dlg_set_sensitive(GtkWidget * w,
  struct cfg_url_dlg_t *urldlg)
{
  gtk_widget_set_sensitive(urldlg->ext_frame,
    GTK_TOGGLE_BUTTON(urldlg->ext_sw)->active);
}

static void cfgtab_url(GtkWidget * notebook)
{
  GtkWidget *label, *box, *tbox, *ptab, *prow, *menu, *mi;
  GtkWidget *swin, *brow, *button, *pbox;
  GtkWidget *hsb, *vsb;
  GtkAdjustment *hadj, *vadj;
  static struct cfg_url_dlg_t urldlg;

  tbox = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(tbox);
  label = gtk_label_new(gettext("URL"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tbox, label);

  box = gtk_table_new(3, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(tbox), box, TRUE, TRUE, 1);
  gtk_widget_show(box);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_table_attach_defaults(GTK_TABLE(box), swin, 0, 1, 0, 1);
  gtk_widget_show(swin);

  gui_cfg.url_list = gtk_clist_new(2);
  urldlg.url_list = gui_cfg.url_list;
  gtk_clist_set_selection_mode(GTK_CLIST(gui_cfg.url_list),
    GTK_SELECTION_BROWSE);
  gtk_clist_set_column_title(GTK_CLIST(gui_cfg.url_list), 0, gettext("URL"));
  gtk_clist_set_column_title(GTK_CLIST(gui_cfg.url_list), 1,
    gettext("Local filename"));
  gtk_clist_column_titles_show(GTK_CLIST(gui_cfg.url_list));
  gtk_clist_set_column_width(GTK_CLIST(gui_cfg.url_list), 0, 200);
  gtk_clist_set_reorderable(GTK_CLIST(gui_cfg.url_list), TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(gui_cfg.url_list), 1, TRUE);
  gtk_container_add(GTK_CONTAINER(swin), gui_cfg.url_list);
  gtk_widget_show(gui_cfg.url_list);

  gtk_signal_connect(GTK_OBJECT(gui_cfg.url_list), "select_row",
    GTK_SIGNAL_FUNC(cfg_url_dlg_switch), (gpointer) & urldlg);

  brow = gtk_table_new(2, 2, FALSE);
  gtk_table_attach(GTK_TABLE(box), brow, 0, 1, 1, 2,
    GTK_EXPAND | GTK_FILL, GTK_FILL, 2, 5);
  gtk_widget_show(brow);

  gui_cfg.url_entry =
    guitl_tab_add_entry(brow, gettext("Request URL: "), 0, 0, FALSE);
  urldlg.url_entry = gui_cfg.url_entry;

  urldlg.localname_entry =
    guitl_tab_add_path_entry(brow, gettext("Local filename: "), 0, 2, FALSE);

  brow = gtk_hbutton_box_new();
  gtk_table_attach(GTK_TABLE(box), brow, 0, 1, 2, 3,
    GTK_EXPAND | GTK_FILL, GTK_FILL, 2, 5);
  gtk_hbutton_box_set_spacing_default(1);
  gtk_widget_show(brow);

  gtk_button_box_set_layout(GTK_BUTTON_BOX(brow), GTK_BUTTONBOX_SPREAD);

  button = guitl_pixmap_button(append_xpm, NULL, gettext("Append"));
  gtk_container_border_width(GTK_CONTAINER(button), 0);
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(cfg_url_dlg_append), (gpointer) & urldlg);

  gtk_signal_connect(GTK_OBJECT(gui_cfg.url_entry), "activate",
    GTK_SIGNAL_FUNC(cfg_url_dlg_append), (gpointer) & urldlg);

  button = guitl_pixmap_button(modify_xpm, NULL, gettext("Modify"));
  gtk_container_border_width(GTK_CONTAINER(button), 0);
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(cfg_url_dlg_modify), (gpointer) & urldlg);

  button = guitl_pixmap_button(clear_xpm, NULL, gettext("Clear"));
  gtk_container_border_width(GTK_CONTAINER(button), 0);
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_ListClear), (gpointer) gui_cfg.url_list);

  button = guitl_pixmap_button(delete_xpm, NULL, gettext("Delete"));
  gtk_container_border_width(GTK_CONTAINER(button), 0);
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_ListDeleteSelected), (gpointer) gui_cfg.url_list);

  /*** DRAG'N'DROP ***/
  gtk_drag_dest_set(box, GTK_DEST_DEFAULT_ALL,
    dragtypes, NUM_ELEM(dragtypes) - 1, GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gtk_signal_connect(GTK_OBJECT(box),
    "drag_data_received", GTK_SIGNAL_FUNC(gui_window_drop_url), NULL);

  box = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(tbox), box, TRUE, TRUE, 1);
  gtk_widget_show(box);

  urldlg.ext_sw =
    gtk_check_button_new_with_label(gettext
    ("Extended informations for HTTP POST request"));
  gtk_box_pack_start(GTK_BOX(box), urldlg.ext_sw, FALSE, FALSE, 1);
  gtk_widget_show(urldlg.ext_sw);

  gtk_signal_connect(GTK_OBJECT(urldlg.ext_sw), "toggled",
    GTK_SIGNAL_FUNC(cfg_url_dlg_set_sensitive), &urldlg);

  urldlg.ext_frame = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(box), urldlg.ext_frame, FALSE, FALSE, 1);
  gtk_widget_show(urldlg.ext_frame);

  ptab = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(urldlg.ext_frame), ptab);
  gtk_widget_show(ptab);

  prow = gtk_table_new(2, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(ptab), prow, FALSE, FALSE, 1);
  gtk_widget_show(prow);

  label = gtk_label_new(gettext("Request method: "));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(prow), label, 0, 1, 0, 1, GTK_SHRINK | GTK_FILL,
    GTK_FILL, 2, 2);
  gtk_widget_show(label);

  urldlg.method = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(prow), urldlg.method, 1, 2, 0, 1,
    GTK_SHRINK | GTK_FILL, GTK_FILL, 2, 2);
  menu = gtk_menu_new();
  mi = gtk_menu_item_new_with_label("GET");
  gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) FORM_M_GET);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  mi = gtk_menu_item_new_with_label("POST");
  gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) FORM_M_POST);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(urldlg.method), menu);
  gtk_widget_show(urldlg.method);

  label = gtk_label_new(gettext("Request encoding: "));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(prow), label, 0, 1, 1, 2, GTK_SHRINK | GTK_FILL,
    GTK_FILL, 2, 2);
  gtk_widget_show(label);

  urldlg.encoding = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(prow), urldlg.encoding, 1, 2, 1, 2,
    GTK_SHRINK | GTK_FILL, GTK_FILL, 2, 2);
  menu = gtk_menu_new();
  mi = gtk_menu_item_new_with_label("multipart/form-data");
  gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) FORM_E_MULTIPART);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  mi = gtk_menu_item_new_with_label("application/x-www-form-urlencoded");
  gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) FORM_E_URLENCODED);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(urldlg.encoding), menu);
  gtk_widget_show(urldlg.encoding);

  label = gtk_label_new(gettext("Query fields: "));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(ptab), label, FALSE, FALSE, 2);
  gtk_widget_show(label);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_usize(swin, -1, 100);
  gtk_box_pack_start(GTK_BOX(ptab), swin, TRUE, TRUE, 2);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show(swin);

  urldlg.field_list = gtk_clist_new(3);
  gtk_clist_set_selection_mode(GTK_CLIST(urldlg.field_list),
    GTK_SELECTION_BROWSE);
  gtk_clist_set_column_title(GTK_CLIST(urldlg.field_list), 0,
    gettext("Type"));
  gtk_clist_set_column_title(GTK_CLIST(urldlg.field_list), 1,
    gettext("Name"));
  gtk_clist_set_column_title(GTK_CLIST(urldlg.field_list), 2,
    gettext("Value"));
  gtk_clist_column_titles_show(GTK_CLIST(urldlg.field_list));
  gtk_clist_set_column_auto_resize(GTK_CLIST(urldlg.field_list), 0, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(urldlg.field_list), 1, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(urldlg.field_list), 2, TRUE);
  gtk_container_add(GTK_CONTAINER(swin), urldlg.field_list);
  gtk_widget_show(urldlg.field_list);

  gtk_signal_connect(GTK_OBJECT(urldlg.field_list), "select_row",
    GTK_SIGNAL_FUNC(cfg_url_dlg_field_switch), (gpointer) & urldlg);

  prow = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(ptab), prow, FALSE, FALSE, 1);
  gtk_widget_show(prow);

  button = guitl_pixmap_button(append_xpm, NULL, gettext("Append"));
  gtk_container_add(GTK_CONTAINER(prow), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(cfg_url_dlg_field_append), &urldlg);

  button = guitl_pixmap_button(modify_xpm, NULL, gettext("Modify"));
  gtk_container_add(GTK_CONTAINER(prow), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(cfg_url_dlg_field_modify), &urldlg);

  button = guitl_pixmap_button(delete_xpm, NULL, gettext("Delete"));
  gtk_container_add(GTK_CONTAINER(prow), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_ListDeleteSelected), (gpointer) urldlg.field_list);

  prow = gtk_table_new(2, 3, FALSE);
  gtk_box_pack_start(GTK_BOX(ptab), prow, FALSE, FALSE, 1);
  gtk_widget_show(prow);

  label = gtk_label_new(gettext("Type: "));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(prow), label, 0, 1, 0, 1, GTK_SHRINK | GTK_FILL,
    GTK_FILL, 2, 2);
  gtk_widget_show(label);

  urldlg.field_type = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(prow), urldlg.field_type, 1, 2, 0, 1,
    GTK_SHRINK | GTK_FILL, GTK_FILL, 2, 2);
  menu = gtk_menu_new();
  mi = gtk_menu_item_new_with_label("TEXT");
  gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) FORM_T_TEXT);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(cfg_url_dlg_set_type), (gpointer) & urldlg);
  mi = gtk_menu_item_new_with_label("FILE");
  gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) FORM_T_FILE);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(cfg_url_dlg_set_type), (gpointer) & urldlg);
  mi = gtk_menu_item_new_with_label("LONG TEXT");
  gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) FORM_T_TEXTAREA);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(cfg_url_dlg_set_type), (gpointer) & urldlg);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(urldlg.field_type), menu);
  gtk_widget_show(urldlg.field_type);

  urldlg.field_name =
    guitl_tab_add_entry(prow, gettext("Name: "), 0, 1, FALSE);

  label = gtk_label_new(gettext("Value: "));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
  gtk_table_attach(GTK_TABLE(prow), label, 0, 1, 2, 3, GTK_SHRINK | GTK_FILL,
    GTK_FILL, 2, 2);
  gtk_widget_show(label);

  urldlg.tab = gtk_notebook_new();
  gtk_notebook_set_show_border(GTK_NOTEBOOK(urldlg.tab), FALSE);
  gtk_notebook_set_show_tabs(GTK_NOTEBOOK(urldlg.tab), FALSE);
  gtk_table_attach(GTK_TABLE(prow), urldlg.tab, 1, 2, 2, 3,
    GTK_SHRINK | GTK_FILL, GTK_FILL, 2, 2);
  gtk_widget_show(urldlg.tab);

  label = gtk_label_new("TEXT");
  brow = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(brow);
  gtk_notebook_append_page(GTK_NOTEBOOK(urldlg.tab), brow, label);

  urldlg.field_value_l = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(brow), urldlg.field_value_l, FALSE, FALSE, 1);
  gtk_widget_show(urldlg.field_value_l);

  label = gtk_label_new("FILE");
  brow = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(brow);
  gtk_notebook_append_page(GTK_NOTEBOOK(urldlg.tab), brow, label);

  pbox = gtk_table_new(1, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(brow), pbox, FALSE, FALSE, 1);
  gtk_widget_show(pbox);

  urldlg.field_value_f = guitl_tab_add_path_entry_full(pbox, NULL, 0, 0,
    FALSE, gettext("Pavuk: Choose form field file"));

  label = gtk_label_new("LONG TEXT");
  brow = gtk_table_new(2, 2, FALSE);
  gtk_widget_show(brow);
  gtk_notebook_append_page(GTK_NOTEBOOK(urldlg.tab), brow, label);

  hadj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  hsb = gtk_hscrollbar_new(hadj);
  gtk_table_attach(GTK_TABLE(brow), hsb, 0, 1, 1, 2,
    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(hsb);

  vadj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  vsb = gtk_vscrollbar_new(vadj);
  gtk_table_attach(GTK_TABLE(brow), vsb, 1, 2, 0, 1,
    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(vsb);

#if GTK_FACE < 2
  urldlg.field_value_t = gtk_text_new(hadj, vadj);
#else
  urldlg.field_value_t = gtk_text_view_new();
#endif
  gtk_widget_set_usize(urldlg.field_value_t, -1, 100);
#if GTK_FACE < 2
  gtk_text_set_editable(GTK_TEXT(urldlg.field_value_t), TRUE);
#else
  gtk_text_view_set_editable(GTK_TEXT_VIEW(urldlg.field_value_t), TRUE);
#endif
  gtk_table_attach(GTK_TABLE(brow), urldlg.field_value_t, 0, 1, 0, 1,
    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(urldlg.field_value_t);

  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(urldlg.ext_sw), FALSE);
  gtk_widget_set_sensitive(urldlg.ext_frame, FALSE);
}


/* FORM DATA */
static void cfgtab_formdata(GtkWidget * notebook)
{
  GtkWidget *label, *box, *tbox, *ptab, *prow, *menu, *mi;
  GtkWidget *swin, *brow, *button, *pbox;
  GtkWidget *hsb, *vsb;
  GtkAdjustment *hadj, *vadj;
  static struct cfg_url_dlg_t urldlg;

  urldlg.ext_sw = NULL;
  urldlg.encoding = NULL;
  urldlg.method = NULL;
  urldlg.localname_entry = NULL;

  tbox = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(tbox);
  label = gtk_label_new(gettext("Form data"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tbox, label);

  box = gtk_table_new(3, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(tbox), box, TRUE, TRUE, 1);
  gtk_widget_show(box);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_table_attach_defaults(GTK_TABLE(box), swin, 0, 1, 0, 1);
  gtk_widget_show(swin);

  gui_cfg.formdata_list = gtk_clist_new(1);
  urldlg.url_list = gui_cfg.formdata_list;
  gtk_clist_set_selection_mode(GTK_CLIST(gui_cfg.formdata_list),
    GTK_SELECTION_BROWSE);
  gtk_clist_set_reorderable(GTK_CLIST(gui_cfg.formdata_list), TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(gui_cfg.formdata_list), 0, TRUE);
  gtk_container_add(GTK_CONTAINER(swin), gui_cfg.formdata_list);
  gtk_widget_show(gui_cfg.formdata_list);

  brow = gtk_table_new(1, 2, FALSE);
  gtk_table_attach(GTK_TABLE(box), brow, 0, 1, 1, 2,
    GTK_EXPAND | GTK_FILL, GTK_FILL, 2, 5);
  gtk_widget_show(brow);

  gui_cfg.formdata_entry =
    guitl_tab_add_entry(brow, gettext("Matching action URL: "), 0, 0, FALSE);
  urldlg.url_entry = gui_cfg.formdata_entry;

  gtk_signal_connect(GTK_OBJECT(gui_cfg.formdata_list), "select_row",
    GTK_SIGNAL_FUNC(cfg_url_dlg_switch), (gpointer) & urldlg);

  brow = gtk_hbutton_box_new();
  gtk_table_attach(GTK_TABLE(box), brow, 0, 1, 2, 3,
    GTK_EXPAND | GTK_FILL, GTK_FILL, 2, 5);
  gtk_hbutton_box_set_spacing_default(1);
  gtk_widget_show(brow);

  gtk_button_box_set_layout(GTK_BUTTON_BOX(brow), GTK_BUTTONBOX_SPREAD);

  button = guitl_pixmap_button(append_xpm, NULL, gettext("Append"));
  gtk_container_border_width(GTK_CONTAINER(button), 0);
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(cfg_url_dlg_append), (gpointer) & urldlg);

  gtk_signal_connect(GTK_OBJECT(gui_cfg.formdata_entry), "activate",
    GTK_SIGNAL_FUNC(cfg_url_dlg_append), (gpointer) & urldlg);

  button = guitl_pixmap_button(modify_xpm, NULL, gettext("Modify"));
  gtk_container_border_width(GTK_CONTAINER(button), 0);
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(cfg_url_dlg_modify), (gpointer) & urldlg);

  button = guitl_pixmap_button(clear_xpm, NULL, gettext("Clear"));
  gtk_container_border_width(GTK_CONTAINER(button), 0);
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_ListClear), (gpointer) gui_cfg.formdata_list);

  button = guitl_pixmap_button(delete_xpm, NULL, gettext("Delete"));
  gtk_container_border_width(GTK_CONTAINER(button), 0);
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_ListDeleteSelected),
    (gpointer) gui_cfg.formdata_list);

  /*** DRAG'N'DROP ***/
  gtk_drag_dest_set(box, GTK_DEST_DEFAULT_ALL,
    dragtypes, NUM_ELEM(dragtypes) - 1, GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gtk_signal_connect(GTK_OBJECT(box),
    "drag_data_received", GTK_SIGNAL_FUNC(gui_window_drop_url), NULL);

  box = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(tbox), box, TRUE, TRUE, 1);
  gtk_widget_show(box);

  urldlg.ext_frame = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(box), urldlg.ext_frame, FALSE, FALSE, 1);
  gtk_widget_show(urldlg.ext_frame);

  ptab = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(urldlg.ext_frame), ptab);
  gtk_widget_show(ptab);

  label = gtk_label_new(gettext("Query fields: "));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(ptab), label, FALSE, FALSE, 2);
  gtk_widget_show(label);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_usize(swin, -1, 100);
  gtk_box_pack_start(GTK_BOX(ptab), swin, TRUE, TRUE, 2);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show(swin);

  urldlg.field_list = gtk_clist_new(3);
  gtk_clist_set_selection_mode(GTK_CLIST(urldlg.field_list),
    GTK_SELECTION_BROWSE);
  gtk_clist_set_column_title(GTK_CLIST(urldlg.field_list), 0,
    gettext("Type"));
  gtk_clist_set_column_title(GTK_CLIST(urldlg.field_list), 1,
    gettext("Name"));
  gtk_clist_set_column_title(GTK_CLIST(urldlg.field_list), 2,
    gettext("Value"));
  gtk_clist_column_titles_show(GTK_CLIST(urldlg.field_list));
  gtk_clist_set_column_auto_resize(GTK_CLIST(urldlg.field_list), 0, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(urldlg.field_list), 1, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(urldlg.field_list), 2, TRUE);
  gtk_container_add(GTK_CONTAINER(swin), urldlg.field_list);
  gtk_widget_show(urldlg.field_list);

  gtk_signal_connect(GTK_OBJECT(urldlg.field_list), "select_row",
    GTK_SIGNAL_FUNC(cfg_url_dlg_field_switch), (gpointer) & urldlg);

  prow = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(ptab), prow, FALSE, FALSE, 1);
  gtk_widget_show(prow);

  button = guitl_pixmap_button(append_xpm, NULL, gettext("Append"));
  gtk_container_add(GTK_CONTAINER(prow), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(cfg_url_dlg_field_append), &urldlg);

  button = guitl_pixmap_button(modify_xpm, NULL, gettext("Modify"));
  gtk_container_add(GTK_CONTAINER(prow), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(cfg_url_dlg_field_modify), &urldlg);

  button = guitl_pixmap_button(delete_xpm, NULL, gettext("Delete"));
  gtk_container_add(GTK_CONTAINER(prow), button);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_ListDeleteSelected), (gpointer) urldlg.field_list);

  prow = gtk_table_new(2, 3, FALSE);
  gtk_box_pack_start(GTK_BOX(ptab), prow, FALSE, FALSE, 1);
  gtk_widget_show(prow);

  label = gtk_label_new(gettext("Type: "));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(prow), label, 0, 1, 0, 1, GTK_SHRINK | GTK_FILL,
    GTK_FILL, 2, 2);
  gtk_widget_show(label);

  urldlg.field_type = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(prow), urldlg.field_type, 1, 2, 0, 1,
    GTK_SHRINK | GTK_FILL, GTK_FILL, 2, 2);
  menu = gtk_menu_new();
  mi = gtk_menu_item_new_with_label("TEXT");
  gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) FORM_T_TEXT);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(cfg_url_dlg_set_type), (gpointer) & urldlg);
  mi = gtk_menu_item_new_with_label("FILE");
  gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) FORM_T_FILE);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(cfg_url_dlg_set_type), (gpointer) & urldlg);
  mi = gtk_menu_item_new_with_label("LONG TEXT");
  gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) FORM_T_TEXTAREA);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(cfg_url_dlg_set_type), (gpointer) & urldlg);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(urldlg.field_type), menu);
  gtk_widget_show(urldlg.field_type);

  urldlg.field_name =
    guitl_tab_add_entry(prow, gettext("Name: "), 0, 1, FALSE);

  label = gtk_label_new(gettext("Value: "));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
  gtk_table_attach(GTK_TABLE(prow), label, 0, 1, 2, 3, GTK_SHRINK | GTK_FILL,
    GTK_FILL, 2, 2);
  gtk_widget_show(label);

  urldlg.tab = gtk_notebook_new();
  gtk_notebook_set_show_border(GTK_NOTEBOOK(urldlg.tab), FALSE);
  gtk_notebook_set_show_tabs(GTK_NOTEBOOK(urldlg.tab), FALSE);
  gtk_table_attach(GTK_TABLE(prow), urldlg.tab, 1, 2, 2, 3,
    GTK_SHRINK | GTK_FILL, GTK_FILL, 2, 2);
  gtk_widget_show(urldlg.tab);

  label = gtk_label_new("TEXT");
  brow = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(brow);
  gtk_notebook_append_page(GTK_NOTEBOOK(urldlg.tab), brow, label);

  urldlg.field_value_l = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(brow), urldlg.field_value_l, FALSE, FALSE, 1);
  gtk_widget_show(urldlg.field_value_l);

  label = gtk_label_new("FILE");
  brow = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(brow);
  gtk_notebook_append_page(GTK_NOTEBOOK(urldlg.tab), brow, label);

  pbox = gtk_table_new(1, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(brow), pbox, FALSE, FALSE, 1);
  gtk_widget_show(pbox);

  urldlg.field_value_f = guitl_tab_add_path_entry_full(pbox, NULL, 0, 0,
    FALSE, gettext("Pavuk: Choose form field file"));

  label = gtk_label_new("LONG TEXT");
  brow = gtk_table_new(2, 2, FALSE);
  gtk_widget_show(brow);
  gtk_notebook_append_page(GTK_NOTEBOOK(urldlg.tab), brow, label);

  hadj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  hsb = gtk_hscrollbar_new(hadj);
  gtk_table_attach(GTK_TABLE(brow), hsb, 0, 1, 1, 2,
    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(hsb);

  vadj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  vsb = gtk_vscrollbar_new(vadj);
  gtk_table_attach(GTK_TABLE(brow), vsb, 1, 2, 0, 1,
    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(vsb);

#if GTK_FACE < 2
  urldlg.field_value_t = gtk_text_new(hadj, vadj);
#else
  urldlg.field_value_t = gtk_text_view_new();
#endif
  gtk_widget_set_usize(urldlg.field_value_t, -1, 100);
#if GTK_FACE < 2
  gtk_text_set_editable(GTK_TEXT(urldlg.field_value_t), TRUE);
#else
  gtk_text_view_set_editable(GTK_TEXT_VIEW(urldlg.field_value_t), TRUE);
#endif
  gtk_table_attach(GTK_TABLE(brow), urldlg.field_value_t, 0, 1, 0, 1,
    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(urldlg.field_value_t);
}

/*** GRABER I ***/
static void cfgtab_graberI(GtkWidget * notebook)
{
  GtkWidget *tbox, *label, *box, *frame, *ptab;
  const char *sample_index_names[] = {
    "",
    "_._.html",
    "index.html",
    "index.htm",
    "default.htm",
    NULL
  };
  const char *sample_browsers[] = {
    "",
    "gnome-moz-remote",
    "netscape",
    "xterm -e lynx",
    "rxvt -e links",
    NULL,
  };
  const char *sample_identity[] = {
    "",
    PACKAGE "/" VERSION " (" HOSTTYPE ")",
    "Mozilla/4.0 (compatible; " PACKAGE " " VERSION
      "; Linux 2.2.14 i486; X11)",
    "Mozilla/4.7 [en] (X11; Linux 2.2.14 i486; I)",
    "Mozilla/4.0 (compatible; MSIE 5.0; Windows NT; DigExt)",
    "Lynx/2.7 libWWW-FM/2.14",
    "NCSA_Mosaic/2.7b5 (X11;Linux 2.0.31 i486)  libwww/2.12 modified",
    "Opera/3.0",
    NULL,
  };

  tbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(tbox);
  label = gtk_label_new(gettext("Grabber I"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tbox, label);

  box = gtk_table_new(2, 11, FALSE);
  gtk_box_pack_start(GTK_BOX(tbox), box, FALSE, FALSE, 0);
  gtk_widget_show(box);

  gui_cfg.cdir_label = guitl_tab_add_path_entry(box,
    gettext("Cache Directory: "), 0, 0, TRUE);

  gui_cfg.default_prefix = guitl_tab_add_entry(box,
    gettext("Default URL prefix: "), 0, 1, FALSE);

  gui_cfg.info_dir = guitl_tab_add_path_entry(box,
    gettext("Separate info directory: "), 0, 2, TRUE);

  gui_cfg.index_name = guitl_tab_add_enum(box,
    gettext("Index file name: "), 0, 3, sample_index_names, TRUE);

  gui_cfg.store_name = guitl_tab_add_entry(box,
    gettext("Store file name: "), 0, 4, FALSE);

  gui_cfg.identity = guitl_tab_add_enum(box,
    gettext("Identity string: "), 0, 5, sample_identity, TRUE);

#ifdef HAVE_BDB_18x
  gui_cfg.ns_cache_dir = guitl_tab_add_path_entry(box,
    gettext("Netscape browser cache directory: "), 0, 6, TRUE);

  gui_cfg.moz_cache_dir = guitl_tab_add_path_entry(box,
    gettext("Mozilla browser cache directory: "), 0, 7, TRUE);
#endif

#ifdef WITH_TREE
  gui_cfg.browser_label = guitl_tab_add_enum(box,
    gettext("Browser: "), 0, 8, sample_browsers, FALSE);
#endif

  gui_cfg.remind_cmd = guitl_tab_add_entry(box,
    gettext("Reminder command: "), 0, 9, FALSE);

  gui_cfg.post_cmd = guitl_tab_add_entry(box,
    gettext("Post command: "), 0, 10, FALSE);

/*********/
  box = gtk_table_new(1, 2, TRUE);
  gtk_box_pack_start(GTK_BOX(tbox), box, FALSE, FALSE, 0);
  gtk_widget_show(box);

  frame = gtk_frame_new(NULL);
  gtk_table_attach(GTK_TABLE(box), frame, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 1,
    1);
  gtk_widget_show(frame);

  ptab = gtk_table_new(6, 3, FALSE);
  gtk_container_add(GTK_CONTAINER(frame), ptab);
  gtk_widget_show(ptab);

  gui_cfg.retry_label = guitl_tab_add_numentry(ptab,
    gettext("How many times retry on fail: "), 0, 0, USHRT_MAX);

  gui_cfg.redir_label = guitl_tab_add_numentry(ptab,
    gettext("How many moved links to follow: "), 0, 1, USHRT_MAX);

  gui_cfg.reget_label = guitl_tab_add_numentry(ptab,
    gettext("How many times to reget file: "), 0, 2, USHRT_MAX);

  gui_cfg.ddays_label = guitl_tab_add_numentry(ptab,
    gettext("Document age before syncing with server: "), 0, 3, USHRT_MAX);

  label = gtk_label_new(gettext(" days "));
  gtk_table_attach(GTK_TABLE(ptab), label, 2, 3, 3, 4,
    GTK_SHRINK, GTK_FILL, 2, 2);
  gtk_widget_show(label);

  gui_cfg.bufsize = guitl_tab_add_numentry(ptab,
    gettext("Read buffer size: "), 0, 4, 10000);
  gtk_adjustment_clamp_page(GTK_ADJUSTMENT(GTK_SPIN_BUTTON(gui_cfg.bufsize)->
      adjustment), 1.0, 1024.0);

  label = gtk_label_new(gettext(" kB "));
  gtk_table_attach(GTK_TABLE(ptab), label, 2, 3, 4, 5,
    GTK_SHRINK, GTK_FILL, 2, 2);
  gtk_widget_show(label);

  gui_cfg.hash_size = guitl_tab_add_numentry(ptab,
    gettext("Hash tables size: "), 0, 5, SHRT_MAX);

  label = gtk_label_new(gettext(" entries "));
  gtk_table_attach(GTK_TABLE(ptab), label, 2, 3, 5, 6,
    GTK_SHRINK, GTK_FILL, 2, 2);
  gtk_widget_show(label);
/*****/
  frame = gtk_frame_new(NULL);
  gtk_table_attach(GTK_TABLE(box), frame, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 1,
    1);
  gtk_widget_show(frame);

  ptab = gtk_table_new(6, 4, FALSE);
  gtk_container_add(GTK_CONTAINER(frame), ptab);
  gtk_widget_show(ptab);

  gui_cfg.sleep_label = guitl_tab_add_numentry(ptab,
    gettext("Sleep time between transfers: "), 0, 0, INT_MAX);

  label = gtk_label_new(gettext(" sec."));
  gtk_table_attach(GTK_TABLE(ptab), label, 2, 3, 0, 1,
    GTK_SHRINK, GTK_FILL, 2, 2);
  gtk_widget_show(label);

  gui_cfg.rsleep = gtk_check_button_new_with_label(gettext("randomize"));
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.rsleep, 3, 4, 0, 1,
    GTK_SHRINK, GTK_FILL, 2, 2);
  gtk_widget_show(gui_cfg.rsleep);

  gui_cfg.rollback_label = guitl_tab_add_numentry(ptab,
    gettext("Rollback amount on reget: "), 0, 1, INT_MAX);

  label = gtk_label_new(gettext(" bytes"));
  gtk_table_attach(GTK_TABLE(ptab), label, 2, 3, 1, 2,
    GTK_SHRINK, GTK_FILL, 2, 2);
  gtk_widget_show(label);

  gui_cfg.trans_quota = guitl_tab_add_numentry(ptab,
    gettext("Transfer quota: "), 0, 2, INT_MAX);

  label = gtk_label_new(gettext(" kB"));
  gtk_table_attach(GTK_TABLE(ptab), label, 2, 3, 2, 3,
    GTK_SHRINK, GTK_FILL, 2, 2);
  gtk_widget_show(label);

  gui_cfg.file_quota = guitl_tab_add_numentry(ptab,
    gettext("File size quota: "), 0, 3, INT_MAX);

  label = gtk_label_new(gettext(" kB"));
  gtk_table_attach(GTK_TABLE(ptab), label, 2, 3, 3, 4,
    GTK_SHRINK, GTK_FILL, 2, 2);
  gtk_widget_show(label);

#if defined HAVE_FSTATFS || HAVE_FSTATVFS
  gui_cfg.fs_quota = guitl_tab_add_numentry(ptab,
    gettext("Filesystem freespace quota: "), 0, 4, INT_MAX);

  label = gtk_label_new(gettext(" kB"));
  gtk_table_attach(GTK_TABLE(ptab), label, 2, 3, 4, 5,
    GTK_SHRINK, GTK_FILL, 2, 2);
  gtk_widget_show(label);
#endif

#ifdef HAVE_MT
  {
    int n = 61;

#ifdef PTHREAD_THREADS_MAX
    n = PTHREAD_THREADS_MAX - 3;
#elif defined _POSIX_THREADS_THREAD_MAX
    n = _POSIX_THREADS_THREAD_MAX - 3;
#endif
    gui_cfg.nthr = guitl_tab_add_numentry(ptab,
      gettext("Number of downloading threads: "), 0, 5, n);
  }
#endif
}

/*** GRABER II ***/
static void cfgtab_graberII(GtkWidget * notebook)
{
  GtkWidget *brow, *label, *frame, *ptab;
  GtkWidget *menu, *mi;
  int i;

  brow = gtk_hbox_new(FALSE, 5);
  gtk_widget_show(brow);
  label = gtk_label_new(gettext("Grabber II"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), brow, label);

  frame = gtk_frame_new(gettext("Misc settings"));
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(brow), frame, FALSE, TRUE, 1);

  brow = gtk_vbox_new(0, 1);
  gtk_widget_show(brow);
  gtk_container_add(GTK_CONTAINER(frame), brow);

  gui_cfg.unique_doc =
    gtk_check_button_new_with_label(gettext
    ("Always generate unique name for document"));
  gtk_widget_show(gui_cfg.unique_doc);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.unique_doc, FALSE, TRUE, 1);

  gui_cfg.del_after =
    gtk_check_button_new_with_label(gettext
    ("Delete FTP document after succesful download"));
  gtk_widget_show(gui_cfg.del_after);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.del_after, FALSE, TRUE, 1);

  gui_cfg.ptime =
    gtk_check_button_new_with_label(gettext
    ("Preserve document modification time"));
  gtk_widget_show(gui_cfg.ptime);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.ptime, FALSE, TRUE, 1);

  gui_cfg.preserve_perm =
    gtk_check_button_new_with_label(gettext
    ("Preserve FTP document permissions"));
  gtk_widget_show(gui_cfg.preserve_perm);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.preserve_perm, FALSE, TRUE, 1);

  gui_cfg.preserve_links =
    gtk_check_button_new_with_label(gettext
    ("Preserve FTP symbolic links paths"));
  gtk_widget_show(gui_cfg.preserve_links);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.preserve_links, FALSE, TRUE, 1);

  gui_cfg.retrieve_slink =
    gtk_check_button_new_with_label(gettext
    ("Retrieve FTP symbolic links like files"));
  gtk_widget_show(gui_cfg.retrieve_slink);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.retrieve_slink, FALSE, TRUE, 1);


  gui_cfg.freget_sw =
    gtk_check_button_new_with_label(gettext
    ("Whole reget when partial not supported"));
  gtk_widget_show(gui_cfg.freget_sw);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.freget_sw, FALSE, TRUE, 1);


  gui_cfg.enc_sw =
    gtk_check_button_new_with_label(gettext
    ("Use gzip encoding for transfer"));
  gtk_widget_show(gui_cfg.enc_sw);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.enc_sw, FALSE, TRUE, 1);

  gui_cfg.oldrm_sw =
    gtk_check_button_new_with_label(gettext("Remove improper documents"));
  gtk_widget_show(gui_cfg.oldrm_sw);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.oldrm_sw, FALSE, TRUE, 1);

  gui_cfg.check_size =
    gtk_check_button_new_with_label(gettext
    ("Check transferred size of document"));
  gtk_widget_show(gui_cfg.check_size);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.check_size, FALSE, TRUE, 1);

  gui_cfg.store_index =
    gtk_check_button_new_with_label(gettext
    ("Store directory URLs as index files"));
  gtk_widget_show(gui_cfg.store_index);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.store_index, FALSE, TRUE, 1);

  gui_cfg.enable_info =
    gtk_check_button_new_with_label(gettext
    ("Store info files with each document"));
  gtk_widget_show(gui_cfg.enable_info);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.enable_info, FALSE, TRUE, 1);

  gui_cfg.auto_referer =
    gtk_check_button_new_with_label(gettext
    ("Send self URL as Referer for starting URLs"));
  gtk_widget_show(gui_cfg.auto_referer);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.auto_referer, FALSE, TRUE, 1);

  gui_cfg.referer =
    gtk_check_button_new_with_label(gettext
    ("Send Referer with HTTP requests"));
  gtk_widget_show(gui_cfg.referer);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.referer, FALSE, TRUE, 1);

  gui_cfg.read_css =
    gtk_check_button_new_with_label(gettext
    ("Fetch objects mentioned in style sheets"));
  gtk_widget_show(gui_cfg.read_css);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.read_css, FALSE, TRUE, 1);

  gui_cfg.send_if_range =
    gtk_check_button_new_with_label(gettext
    ("Send If-Range header field when regeting"));
  gtk_widget_show(gui_cfg.send_if_range);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.send_if_range, FALSE, TRUE, 1);

  gui_cfg.show_time =
    gtk_check_button_new_with_label(gettext
    ("Show time of start and end of downloading"));
  gtk_widget_show(gui_cfg.show_time);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.show_time, FALSE, TRUE, 1);

  ptab = gtk_table_new(2, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(brow), ptab, FALSE, FALSE, 1);
  gtk_widget_show(ptab);

  label = gtk_label_new(gettext("URL scheduling strategy: "));
  gtk_table_attach(GTK_TABLE(ptab), label, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 1, 1);
  gtk_widget_show(label);

  gui_cfg.scheduling_strategie = gtk_option_menu_new();

  menu = gtk_menu_new();

  for(i = 0; i < SSTRAT_LAST; i++)
  {
    mi = gtk_menu_item_new_with_label(get_strategie_label(i));
    gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) i);
    gtk_menu_append(GTK_MENU(menu), mi);
    gtk_widget_show(mi);
  }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(gui_cfg.scheduling_strategie),
    menu);

  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.scheduling_strategie,
    1, 2, 0, 1, GTK_FILL, GTK_FILL, 1, 1);
  gtk_widget_show(gui_cfg.scheduling_strategie);
}

/*** HTML ***/
static void HtmlswSens(GtkWidget * w, gpointer data)
{
  gboolean act = GTK_TOGGLE_BUTTON(w)->active;

  gtk_widget_set_sensitive(gui_cfg.all_to_local, act);
  gtk_widget_set_sensitive(gui_cfg.sel_to_local, act);
  gtk_widget_set_sensitive(gui_cfg.url_to_local, act);
  gtk_widget_set_sensitive(gui_cfg.all_to_remote, act);
  gtk_widget_set_sensitive(gui_cfg.post_update, act);
}

static void cfgtab_html(GtkWidget * notebook)
{
  GtkWidget *brow, *label, *frame, *ptab, *brow2;
  GSList *rg;

  brow = gtk_table_new(2, 2, FALSE);
  gtk_widget_show(brow);
  label = gtk_label_new(gettext("HTML"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), brow, label);

  frame = gtk_frame_new(gettext("HTML document URL rewriting rules"));
  gtk_widget_show(frame);
  gtk_table_attach(GTK_TABLE(brow), frame, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 2, 2);

  brow2 = gtk_vbox_new(0, 1);
  gtk_widget_show(brow2);
  gtk_container_add(GTK_CONTAINER(frame), brow2);

  gui_cfg.noreloc_sw =
    gtk_check_button_new_with_label(gettext("Rewrite URLs inside HTML doc."));
  gtk_widget_show(gui_cfg.noreloc_sw);
  gtk_box_pack_start(GTK_BOX(brow2), gui_cfg.noreloc_sw, FALSE, TRUE, 2);
  gtk_signal_connect(GTK_OBJECT(gui_cfg.noreloc_sw), "toggled",
    GTK_SIGNAL_FUNC(HtmlswSens), NULL);

  rg = NULL;
  gui_cfg.url_to_local = gtk_radio_button_new_with_label(rg,
    gettext("Rewrite URLs to local when stored locally"));
  rg = gtk_radio_button_group(GTK_RADIO_BUTTON(gui_cfg.url_to_local));
  gtk_widget_show(gui_cfg.url_to_local);
  gtk_box_pack_start(GTK_BOX(brow2), gui_cfg.url_to_local, FALSE, TRUE, 1);

  gui_cfg.sel_to_local = gtk_radio_button_new_with_label(rg,
    gettext("Rewrite all suitable URLs to local"));
  rg = gtk_radio_button_group(GTK_RADIO_BUTTON(gui_cfg.sel_to_local));
  gtk_widget_show(gui_cfg.sel_to_local);
  gtk_box_pack_start(GTK_BOX(brow2), gui_cfg.sel_to_local, FALSE, TRUE, 1);

  gui_cfg.all_to_local = gtk_radio_button_new_with_label(rg,
    gettext("Rewrite all URLs to local immediately"));
  rg = gtk_radio_button_group(GTK_RADIO_BUTTON(gui_cfg.all_to_local));
  gtk_widget_show(gui_cfg.all_to_local);
  gtk_box_pack_start(GTK_BOX(brow2), gui_cfg.all_to_local, FALSE, TRUE, 1);

  gui_cfg.all_to_remote = gtk_radio_button_new_with_label(rg,
    gettext("Rewrite all URLs to remote immediately"));
  rg = gtk_radio_button_group(GTK_RADIO_BUTTON(gui_cfg.all_to_remote));
  gtk_widget_show(gui_cfg.all_to_remote);
  gtk_box_pack_start(GTK_BOX(brow2), gui_cfg.all_to_remote, FALSE, TRUE, 1);

  gui_cfg.post_update = gtk_radio_button_new_with_label(rg,
    gettext("Rewrite only one currently download URL"));
  rg = gtk_radio_button_group(GTK_RADIO_BUTTON(gui_cfg.post_update));
  gtk_widget_show(gui_cfg.post_update);
  gtk_box_pack_start(GTK_BOX(brow2), gui_cfg.post_update, FALSE, TRUE, 1);
/***/
  frame = gtk_frame_new(gettext("Tuning of HTML rewriting engine"));
  gtk_widget_show(frame);
  gtk_table_attach(GTK_TABLE(brow), frame, 1, 2, 0, 1,
    GTK_FILL, GTK_FILL, 2, 2);

  ptab = gtk_table_new(2, 4, FALSE);
  gtk_container_add(GTK_CONTAINER(frame), ptab);
  gtk_widget_show(ptab);

  gui_cfg.dont_touch_url_pattern = guitl_tab_add_edit_entry(ptab,
    gettext("Don't touch URL wildcard pattern: "), NULL, 0, 0, FALSE);

#ifdef HAVE_REGEX
  gui_cfg.dont_touch_url_rpattern = guitl_tab_add_edit_entry(ptab,
    gettext("Don't touch URL RE pattern: "), NULL, 0, 1, TRUE);

  gui_cfg.dont_touch_tag_rpattern = guitl_tab_add_edit_entry(ptab,
    gettext("Don't touch HTML tag RE pattern: "), NULL, 0, 2, TRUE);
#endif
}

/*** NET ***/
static void FTPHSSelectRow(void)
{
  char *p;
  int row = GPOINTER_TO_INT(GTK_CLIST(gui_cfg.ftp_login_hs)->selection->data);

  gtk_clist_get_text(GTK_CLIST(gui_cfg.ftp_login_hs), row, 0, &p);
  gtk_entry_set_text(GTK_ENTRY(gui_cfg.ftp_login_hs_host), p);

  gtk_clist_get_text(GTK_CLIST(gui_cfg.ftp_login_hs), row, 1, &p);
  gtk_entry_set_text(GTK_ENTRY(gui_cfg.ftp_login_hs_handshake), p);
}

static void FTPHSNew(int row)
{
  char *pp[2];
  ftp_handshake_info *fhi;

  pp[0] = (gchar *) gtk_entry_get_text(GTK_ENTRY(gui_cfg.ftp_login_hs_host));
  pp[1] =
    (gchar *) gtk_entry_get_text(GTK_ENTRY(gui_cfg.ftp_login_hs_handshake));

  if(!(fhi = ftp_handshake_info_parse(pp[0], pp[1])))
  {
    gdk_beep();
    return;
  }
  ftp_handshake_info_free(fhi);

  if(row < 0)
    gtk_clist_append(GTK_CLIST(gui_cfg.ftp_login_hs), pp);
  else
  {
    int i;
    for(i = 0; i < 2; i++)
      gtk_clist_set_text(GTK_CLIST(gui_cfg.ftp_login_hs), row, i, pp[i]);
  }
}

static void FTPHSModify(void)
{
  if(!GTK_CLIST(gui_cfg.ftp_login_hs)->selection)
  {
    gdk_beep();
    return;
  }

  FTPHSNew(GPOINTER_TO_INT(GTK_CLIST(gui_cfg.ftp_login_hs)->selection->data));
}

static void FTPHSAppend(void)
{
  FTPHSNew(-1);
}


static void cfgtab_net(GtkWidget * notebook)
{
  GtkWidget *col, *brow, *box, *frame, *label, *rb, *vbox;
  GtkWidget *swin, *ptab, *bbox, *button;
  GSList *rg;
  const char *hssamples[] = {
    "",
    "user %u\\331\\pass %p\\230",
    "user %u@%h %s\\331\\pass %p\\230",
    "user %U\\331\\pass %P\\230\\user %u@%h %s\\331\\pass %p\\230",
    NULL,
  };

  box = gtk_hbox_new(0, 5);
  gtk_widget_show(box);
  label = gtk_label_new(gettext("Net"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);

  col = gtk_table_new(11, 3, FALSE);
  gtk_widget_show(col);
  gtk_container_add(GTK_CONTAINER(box), col);

  frame = gtk_frame_new(gettext("Allowed protocols"));
  gtk_widget_show(frame);
  gtk_table_attach(GTK_TABLE(col), frame, 1, 2, 0, 1,
    GTK_FILL, GTK_FILL, 5, 5);

  brow = gtk_table_new(3, 2, FALSE);
  gtk_widget_show(brow);
  gtk_container_add(GTK_CONTAINER(frame), brow);

  gui_cfg.http_sw = gtk_check_button_new_with_label(gettext("HTTP"));
  gtk_widget_show(gui_cfg.http_sw);
  gtk_table_attach(GTK_TABLE(brow), gui_cfg.http_sw, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 5, 5);

  gui_cfg.ftp_sw = gtk_check_button_new_with_label(gettext("FTP"));
  gtk_widget_show(gui_cfg.ftp_sw);
  gtk_table_attach(GTK_TABLE(brow), gui_cfg.ftp_sw, 0, 1, 1, 2,
    GTK_FILL, GTK_FILL, 5, 5);

  gui_cfg.gopher_sw = gtk_check_button_new_with_label(gettext("Gopher"));
  gtk_widget_show(gui_cfg.gopher_sw);
  gtk_table_attach(GTK_TABLE(brow), gui_cfg.gopher_sw, 1, 2, 0, 1,
    GTK_FILL, GTK_FILL, 5, 5);

#ifdef USE_SSL
  gui_cfg.https_sw = gtk_check_button_new_with_label(gettext("HTTPS"));
  gtk_widget_show(gui_cfg.https_sw);
  gtk_table_attach(GTK_TABLE(brow), gui_cfg.https_sw, 1, 2, 1, 2,
    GTK_FILL, GTK_FILL, 5, 5);

  gui_cfg.ftps_sw = gtk_check_button_new_with_label(gettext("FTPS"));
  gtk_widget_show(gui_cfg.ftps_sw);
  gtk_table_attach(GTK_TABLE(brow), gui_cfg.ftps_sw, 2, 3, 0, 1,
    GTK_FILL, GTK_FILL, 5, 5);
#endif

  frame = gtk_frame_new(gettext("FTP data connection type "));
  gtk_widget_show(frame);
  gtk_table_attach(GTK_TABLE(col), frame, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 5, 5);

  brow = gtk_table_new(2, 1, FALSE);
  gtk_widget_show(brow);
  gtk_container_add(GTK_CONTAINER(frame), brow);

  rb = gtk_radio_button_new_with_label(NULL, gettext("Active"));
  rg = gtk_radio_button_group(GTK_RADIO_BUTTON(rb));
  gtk_widget_show(rb);
  gtk_table_attach(GTK_TABLE(brow), rb, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 5, 5);
  gui_cfg.ftpmodegr[1] = rb;

  rb = gtk_radio_button_new_with_label(rg, gettext("Passive"));
  rg = gtk_radio_button_group(GTK_RADIO_BUTTON(rb));
  gtk_widget_show(rb);
  gtk_table_attach(GTK_TABLE(brow), rb, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 5, 5);
  gui_cfg.ftpmodegr[0] = rb;

  label = gtk_label_new(gettext("Communication timeout: "));
  gtk_table_attach(GTK_TABLE(col), label, 0, 1, 1, 2,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);

  gui_cfg.timeout_label = gtk_spin_button_new(
    (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 9999, 1.0, 10.0, 0.0),
    0, 2);
  gtk_table_attach(GTK_TABLE(col), gui_cfg.timeout_label, 1, 2, 1, 2,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_widget_show(gui_cfg.timeout_label);

  label = gtk_label_new(gettext(" min."));
  gtk_table_attach(GTK_TABLE(col), label, 2, 3, 1, 2,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);

  label = gtk_label_new(gettext("Maximal transfer rate: "));
  gtk_table_attach(GTK_TABLE(col), label, 0, 1, 3, 4,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);

  gui_cfg.maxrate = gtk_spin_button_new(
    (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 1000000.0, 1.0, 10.0, 0.0),
    0, 3);
  gtk_table_attach(GTK_TABLE(col), gui_cfg.maxrate, 1, 2, 3, 4,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_widget_show(gui_cfg.maxrate);

  label = gtk_label_new(gettext(" kB/s"));
  gtk_table_attach(GTK_TABLE(col), label, 2, 3, 3, 4,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);

  label = gtk_label_new(gettext("Minimal transfer rate: "));
  gtk_table_attach(GTK_TABLE(col), label, 0, 1, 4, 5,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);

  gui_cfg.minrate = gtk_spin_button_new(
    (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 1000000.0, 1.0, 10.0, 0.0),
    0, 3);
  gtk_table_attach(GTK_TABLE(col), gui_cfg.minrate, 1, 2, 4, 5,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_widget_show(gui_cfg.minrate);

  label = gtk_label_new(gettext(" kB/s"));
  gtk_table_attach(GTK_TABLE(col), label, 2, 3, 4, 5,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);

  label = gtk_label_new(gettext("Local interface address: "));
  gtk_table_attach(GTK_TABLE(col), label, 0, 1, 5, 6,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);

  gui_cfg.local_ip = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(col), gui_cfg.local_ip, 1, 3, 5, 6,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_widget_show(gui_cfg.local_ip);

  label = gtk_label_new(gettext("Additional HTTP headers: "));
  gtk_table_attach(GTK_TABLE(col), label, 0, 1, 6, 7,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);

  gui_cfg.http_headers = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(col), gui_cfg.http_headers, 1, 3, 6, 7,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_widget_show(gui_cfg.http_headers);

  label = gtk_label_new(gettext("Additional FTP list options: "));
  gtk_table_attach(GTK_TABLE(col), label, 0, 1, 7, 8,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);

  gui_cfg.ftp_list_options = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(col), gui_cfg.ftp_list_options, 1, 3, 7, 8,
    GTK_FILL, GTK_FILL, 2, 5);
  gtk_widget_show(gui_cfg.ftp_list_options);


  gui_cfg.ftp_list =
    gtk_check_button_new_with_label(gettext
    ("Use wide listing of FTP directories"));
  gtk_widget_show(gui_cfg.ftp_list);
  gtk_table_attach(GTK_TABLE(col), gui_cfg.ftp_list, 0, 3, 8, 9,
    GTK_FILL, GTK_FILL, 5, 5);

  gui_cfg.fix_wuftpd =
    gtk_check_button_new_with_label(gettext
    ("Fix detection of nonexisting FTP directories on WuFTPD FTP servers"));
  gtk_widget_show(gui_cfg.fix_wuftpd);
  gtk_table_attach(GTK_TABLE(col), gui_cfg.fix_wuftpd, 0, 3, 9, 10,
    GTK_FILL, GTK_FILL, 5, 5);

  gui_cfg.use_http11 =
    gtk_check_button_new_with_label(gettext
    ("Use HTTP/1.1 protocol for HTTP communication"));
  gtk_widget_show(gui_cfg.use_http11);
  gtk_table_attach(GTK_TABLE(col), gui_cfg.use_http11, 0, 3, 10, 11,
    GTK_FILL, GTK_FILL, 5, 5);

/*******/
  frame = gtk_frame_new(gettext("FTP login handshake rules"));
  gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 1);
  gtk_widget_show(frame);

  vbox = gtk_vbox_new(0, 5);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(vbox);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(vbox), swin, TRUE, TRUE, 5);
  gtk_widget_show(swin);

  gui_cfg.ftp_login_hs = gtk_clist_new(2);
  gtk_clist_set_column_title(GTK_CLIST(gui_cfg.ftp_login_hs), 0,
    gettext("Host"));
  gtk_clist_set_column_title(GTK_CLIST(gui_cfg.ftp_login_hs), 1,
    gettext("Login handshake"));
  gtk_clist_column_titles_show(GTK_CLIST(gui_cfg.ftp_login_hs));
  gtk_clist_set_reorderable(GTK_CLIST(gui_cfg.ftp_login_hs), TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(gui_cfg.ftp_login_hs), 0, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(gui_cfg.ftp_login_hs), 1, TRUE);
  gtk_signal_connect(GTK_OBJECT(gui_cfg.ftp_login_hs), "select_row",
    GTK_SIGNAL_FUNC(FTPHSSelectRow), NULL);
  gtk_container_add(GTK_CONTAINER(swin), gui_cfg.ftp_login_hs);
  gtk_widget_show(gui_cfg.ftp_login_hs);

  ptab = gtk_table_new(2, 5, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), ptab, FALSE, TRUE, 2);
  gtk_widget_show(ptab);

  gui_cfg.ftp_login_hs_host = guitl_tab_add_entry(ptab,
    gettext("Host: "), 0, 0, FALSE);

  gui_cfg.ftp_login_hs_handshake = guitl_tab_add_enum(ptab,
    gettext("Handshake: "), 0, 1, hssamples, TRUE);

  bbox = gtk_hbutton_box_new();
  gtk_widget_show(bbox);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
  gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 5);

  button = guitl_pixmap_button(append_xpm, NULL, gettext("Append"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(FTPHSAppend), NULL);

  button = guitl_pixmap_button(modify_xpm, NULL, gettext("Modify"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(FTPHSModify), NULL);

  button = guitl_pixmap_button(clear_xpm, NULL, gettext("Clear"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_ListClear), gui_cfg.js_transform);

  button = guitl_pixmap_button(delete_xpm, NULL, gettext("Delete"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_ListDeleteSelected), gui_cfg.js_transform);

}


/*** PROXY ***/
static void cfgtab_proxy(GtkWidget * notebook)
{
  GtkWidget *col, *brow, *label, *box, *frame;

  box = gtk_vbox_new(0, 5);
  gtk_widget_show(box);
  label = gtk_label_new(gettext("Proxy"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);

  col = gtk_table_new(3, 2, FALSE);
  gtk_widget_show(col);
  gtk_container_add(GTK_CONTAINER(box), col);
  frame = gtk_frame_new(gettext("Gopher proxy"));
  gtk_widget_show(frame);
  gtk_table_attach(GTK_TABLE(col), frame, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 5, 5);

  brow = gtk_table_new(3, 2, FALSE);
  gtk_widget_show(brow);
  gtk_container_add(GTK_CONTAINER(frame), brow);

  gui_cfg.gopher_proxyh_label = guitl_tab_add_entry(brow,
    gettext("Host: "), 0, 0, FALSE);

  gui_cfg.gopher_proxyp_label = guitl_tab_add_numentry(brow,
    gettext("Port: "), 0, 1, USHRT_MAX);


  gui_cfg.gopher_httpgw =
    gtk_check_button_new_with_label(gettext("Gopher via HTTP proxy"));
  gtk_widget_show(gui_cfg.gopher_httpgw);
  gtk_table_attach(GTK_TABLE(brow), gui_cfg.gopher_httpgw, 0, 2, 2, 3,
    GTK_FILL, GTK_FILL, 5, 5);

  frame = gtk_frame_new(gettext("FTP proxy"));
  gtk_widget_show(frame);
  gtk_table_attach(GTK_TABLE(col), frame, 1, 2, 0, 1,
    GTK_FILL, GTK_FILL, 5, 5);

  brow = gtk_table_new(4, 2, FALSE);
  gtk_widget_show(brow);
  gtk_container_add(GTK_CONTAINER(frame), brow);


  gui_cfg.ftp_proxyh_label = guitl_tab_add_entry(brow,
    gettext("Host: "), 0, 0, FALSE);

  gui_cfg.ftp_proxyp_label = guitl_tab_add_numentry(brow,
    gettext("Port: "), 0, 1, USHRT_MAX);

  gui_cfg.ftp_httpgw =
    gtk_check_button_new_with_label(gettext("FTP via HTTP proxy"));
  gtk_widget_show(gui_cfg.ftp_httpgw);
  gtk_table_attach(GTK_TABLE(brow), gui_cfg.ftp_httpgw, 0, 2, 2, 3,
    GTK_FILL, GTK_FILL, 5, 5);

  gui_cfg.ftp_dirtyp =
    gtk_check_button_new_with_label(gettext("FTP via HTTP tunneling proxy"));
  gtk_widget_show(gui_cfg.ftp_dirtyp);
  gtk_table_attach(GTK_TABLE(brow), gui_cfg.ftp_dirtyp, 0, 2, 3, 4,
    GTK_FILL, GTK_FILL, 5, 5);


#ifdef USE_SSL
  frame = gtk_frame_new(gettext("SSL proxy"));
  gtk_widget_show(frame);
  gtk_table_attach(GTK_TABLE(col), frame, 2, 3, 0, 1,
    GTK_FILL, GTK_FILL, 5, 5);

  brow = gtk_table_new(2, 2, FALSE);
  gtk_widget_show(brow);
  gtk_container_add(GTK_CONTAINER(frame), brow);

  gui_cfg.ssl_proxyh_label = guitl_tab_add_entry(brow,
    gettext("Host: "), 0, 0, FALSE);

  gui_cfg.ssl_proxyp_label = guitl_tab_add_numentry(brow,
    gettext("Port: "), 0, 1, USHRT_MAX);

#endif

  frame = gtk_frame_new(gettext("HTTP proxy"));
  gtk_widget_show(frame);
  gtk_table_attach(GTK_TABLE(col), frame, 0, 2, 1, 2,
    GTK_FILL, GTK_FILL, 5, 5);

  brow =
    guitl_new_edit_list(&gui_cfg.http_proxy_list, &gui_cfg.http_proxyh_label,
    gettext("Proxy: "), NULL, NULL, NULL, NULL, FALSE, NULL);

  gtk_container_add(GTK_CONTAINER(frame), brow);

  gui_cfg.cache_sw =
    gtk_check_button_new_with_label(gettext("Allow caching of documents"));
  gtk_widget_show(gui_cfg.cache_sw);
  gtk_table_attach(GTK_TABLE(col), gui_cfg.cache_sw, 0, 1, 2, 3,
    GTK_FILL, GTK_FILL, 5, 5);
}

static void LangNewRow(int row, GtkWidget * entry)
{
  char *p;

  p = (gchar *) gtk_entry_get_text(GTK_ENTRY(entry));

  p = tl_strndup(p, strcspn(p, " "));

  if(row < 0)
    gtk_clist_append(GTK_CLIST(gui_cfg.alanglist), &p);
  else
    gtk_clist_set_text(GTK_CLIST(gui_cfg.alanglist), row, 0, p);

  _free(p);

}

static void LangAppend(GtkWidget * w, GtkWidget * entry)
{
  LangNewRow(-1, entry);
}

static void LangModify(GtkWidget * w, GtkWidget * entry)
{
  if(!GTK_CLIST(gui_cfg.alanglist)->selection)
  {
    gdk_beep();
    return;
  }

  LangNewRow(GPOINTER_TO_INT(GTK_CLIST(gui_cfg.alanglist)->selection->data),
    entry);
}

/*** LANG ***/
static void cfgtab_lang(GtkWidget * notebook)
{
  GtkWidget *col, *box, *label, *frame, *entry;
  GtkWidget *abtn, *mbtn;


  box = gtk_hbox_new(0, 5);
  gtk_widget_show(box);
  label = gtk_label_new(gettext("Languages"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);

  frame = gtk_frame_new(gettext("Preffered languages"));
  gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 5);
  gtk_widget_show(frame);

  col = guitl_new_edit_list(&gui_cfg.alanglist, &entry,
    gettext("Language code: "), NULL, &mbtn, NULL, &abtn, TRUE, iso_lang);

  gtk_signal_connect(GTK_OBJECT(abtn), "clicked",
    GTK_SIGNAL_FUNC(LangAppend), entry);
  gtk_signal_connect(GTK_OBJECT(entry), "activate",
    GTK_SIGNAL_FUNC(LangAppend), entry);
  gtk_signal_connect(GTK_OBJECT(mbtn), "clicked",
    GTK_SIGNAL_FUNC(LangModify), entry);

  gtk_container_add(GTK_CONTAINER(frame), col);

  frame = gtk_frame_new(gettext("Preffered character sets"));
  gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 5);
  gtk_widget_show(frame);

  col = guitl_new_edit_list(&gui_cfg.acharset_list, &entry,
    gettext("Character set code: "), NULL, NULL, NULL, NULL, TRUE, char_sets);

  gtk_container_add(GTK_CONTAINER(frame), col);
}

/*** AUTH ***/
static void cfgtab_auth(GtkWidget * notebook)
{
  GtkWidget *label, *box, *frame, *ptab, *smenu, *mi, *pbox, *tbox;
  int i;
  struct
  {
    char *name;
    http_auth_type_t id;
  } authtab[] =
  {
    {gettext_nop("User auth. scheme"), HTTP_AUTH_USER},
    {gettext_nop("Basic auth. scheme"), HTTP_AUTH_BASIC},
    {gettext_nop("Digest auth. scheme"), HTTP_AUTH_DIGEST},
#ifdef ENABLE_NTLM
    {gettext_nop("NTLM auth. scheme"), HTTP_AUTH_NTLM},
#endif
    {NULL, HTTP_AUTH_NONE}
  };

  box = gtk_vbox_new(0, 5);
  gtk_widget_show(box);
  label = gtk_label_new(gettext("Auth"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);

  pbox = gtk_table_new(2, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(box), pbox, FALSE, FALSE, 2);
  gtk_widget_show(pbox);

  frame = gtk_frame_new(gettext("User authentification"));
  gtk_table_attach(GTK_TABLE(pbox), frame, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 5, 5);
  gtk_widget_show(frame);

  ptab = gtk_table_new(3, 3, FALSE);
  gtk_widget_show(ptab);
  gtk_container_add(GTK_CONTAINER(frame), ptab);

  label = gtk_label_new(gettext("Scheme: "));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(ptab), label, 0, 1, 0, 1,
    GTK_FILL, GTK_FILL, 2, 5);

  gui_cfg.http_auth_scheme = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.http_auth_scheme,
    1, 2, 0, 1, GTK_FILL, GTK_FILL, 2, 5);

  smenu = gtk_menu_new();
  gtk_widget_realize(smenu);

  for(i = 0; authtab[i].name; i++)
  {
    mi = gtk_menu_item_new_with_label(gettext(authtab[i].name));
    gtk_menu_append(GTK_MENU(smenu), mi);
    gtk_widget_show(mi);

    gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) authtab[i].id);
  }
  gtk_option_menu_set_menu(GTK_OPTION_MENU(gui_cfg.http_auth_scheme), smenu);
  gtk_widget_show(gui_cfg.http_auth_scheme);


  gui_cfg.auth_label = guitl_tab_add_entry(ptab,
    gettext("User name: "), 0, 1, FALSE);

  gui_cfg.pass_label = guitl_tab_add_entry(ptab,
    gettext("Password: "), 0, 2, TRUE);

  gui_cfg.auth_ntlm_domain = guitl_tab_add_entry(ptab,
    gettext("NTLM domain: "), 0, 3, FALSE);

  gui_cfg.auth_reuse_nonce =
    gtk_check_button_new_with_label(gettext
    ("Reuse HTTP Digest access nonce"));
  gtk_widget_show(gui_cfg.auth_reuse_nonce);
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.auth_reuse_nonce, 0, 2, 4, 5,
    GTK_FILL, GTK_FILL, 5, 5);

  frame = gtk_frame_new(gettext("HTTP proxy user authentification"));
  gtk_table_attach(GTK_TABLE(pbox), frame, 1, 2, 0, 1,
    GTK_FILL, GTK_FILL, 5, 5);
  gtk_widget_show(frame);

  ptab = gtk_table_new(2, 5, FALSE);
  gtk_container_add(GTK_CONTAINER(frame), ptab);
  gtk_widget_show(ptab);

  label = gtk_label_new(gettext("Scheme: "));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(ptab), label,
    0, 1, 0, 1, GTK_FILL, GTK_FILL, 2, 5);
  gtk_widget_show(label);

  gui_cfg.http_proxy_auth_scheme = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.http_proxy_auth_scheme,
    1, 2, 0, 1, GTK_FILL, GTK_FILL, 2, 5);

  smenu = gtk_menu_new();
  gtk_widget_realize(smenu);

  for(i = 0; authtab[i].name; i++)
  {
    mi = gtk_menu_item_new_with_label(gettext(authtab[i].name));
    gtk_menu_append(GTK_MENU(smenu), mi);
    gtk_widget_show(mi);

    gtk_object_set_user_data(GTK_OBJECT(mi), (gpointer) authtab[i].id);
  }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(gui_cfg.http_proxy_auth_scheme),
    smenu);
  gtk_widget_show(gui_cfg.http_proxy_auth_scheme);


  gui_cfg.proxy_auth_label = guitl_tab_add_entry(ptab,
    gettext("User name: "), 0, 1, FALSE);

  gui_cfg.proxy_pass_label = guitl_tab_add_entry(ptab,
    gettext("Password: "), 0, 2, TRUE);

  gui_cfg.auth_proxy_ntlm_domain = guitl_tab_add_entry(ptab,
    gettext("NTLM domain: "), 0, 3, FALSE);

  gui_cfg.auth_reuse_proxy_nonce =
    gtk_check_button_new_with_label(gettext
    ("Reuse HTTP Digest access nonce"));
  gtk_widget_show(gui_cfg.auth_reuse_proxy_nonce);
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.auth_reuse_proxy_nonce, 0, 2, 4,
    5, GTK_FILL, GTK_FILL, 5, 5);

  frame = gtk_frame_new(gettext("FTP proxy user authentification"));
  gtk_table_attach(GTK_TABLE(pbox), frame, 0, 1, 1, 2,
    GTK_FILL, GTK_FILL, 5, 5);
  gtk_widget_show(frame);

  ptab = gtk_table_new(2, 5, FALSE);
  gtk_container_add(GTK_CONTAINER(frame), ptab);
  gtk_widget_show(ptab);

  gui_cfg.ftp_proxy_user = guitl_tab_add_entry(ptab,
    gettext("User name: "), 0, 1, FALSE);

  gui_cfg.ftp_proxy_pass = guitl_tab_add_entry(ptab,
    gettext("Password: "), 0, 2, TRUE);

  frame = gtk_frame_new(gettext("Misc"));
  gtk_table_attach(GTK_TABLE(pbox), frame, 1, 2, 1, 2,
    GTK_FILL, GTK_FILL, 5, 5);
  gtk_widget_show(frame);

  tbox = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(frame), tbox);
  gtk_widget_show(tbox);

  ptab = gtk_table_new(2, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(tbox), ptab, FALSE, FALSE, 2);
  gtk_widget_show(ptab);

  gui_cfg.from_label = guitl_tab_add_entry(ptab,
    gettext("E-mail address: "), 0, 5, FALSE);

  gui_cfg.send_from =
    gtk_check_button_new_with_label(gettext
    ("Send From: header with HTTP request"));
  gtk_box_pack_start(GTK_BOX(tbox), gui_cfg.send_from, FALSE, FALSE, 2);
  gtk_widget_show(gui_cfg.send_from);
}

static void cfgtab_ssl(GtkWidget * notebook)
{
#ifdef USE_SSL
  GtkWidget *label, *box, *frame, *ptab, *rb, *hbox;
  GSList *rg;

  box = gtk_vbox_new(0, 5);
  gtk_widget_show(box);
  label = gtk_label_new(gettext("SSL"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);

  frame = gtk_frame_new(gettext("SSL client certificate"));
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 5);

  ptab = gtk_table_new(5, 2, FALSE);
  gtk_widget_show(ptab);
  gtk_container_add(GTK_CONTAINER(frame), ptab);

#ifdef USE_SSL_IMPL_OPENSSL
  gui_cfg.ssl_cert_passwd_en = guitl_tab_add_entry(ptab,
    gettext("Certificate password: "), 0, 0, TRUE);

  gui_cfg.ssl_cert_file_en = guitl_tab_add_path_entry(ptab,
    gettext("Certificate PEM file: "), 0, 1, FALSE);

  gui_cfg.ssl_key_file_en = guitl_tab_add_path_entry(ptab,
    gettext("Certificate key file: "), 0, 2, FALSE);
#endif

#ifdef USE_SSL_IMPL_NSS
  gui_cfg.nss_cert_dir = guitl_tab_add_path_entry(ptab,
    gettext("NSS certificate config directory: "), 0, 0, TRUE);

  gui_cfg.ssl_cert_passwd_en = guitl_tab_add_entry(ptab,
    gettext("Certificate password: "), 0, 1, TRUE);

  gui_cfg.nss_accept_unknown_cert =
    gtk_check_button_new_with_label(gettext("Accept unknown certificates"));
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.nss_accept_unknown_cert, 0, 2, 2,
    3, GTK_FILL, GTK_FILL, 2, 2);
  gtk_widget_show(gui_cfg.nss_accept_unknown_cert);

  gui_cfg.nss_domestic_policy =
    gtk_check_button_new_with_label(gettext("Domestic SSL ciphers policy"));
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.nss_domestic_policy, 0, 2, 3, 4,
    GTK_FILL, GTK_FILL, 2, 2);
  gtk_widget_show(gui_cfg.nss_domestic_policy);
#
#endif

  gui_cfg.ssl_cipher_list = guitl_tab_add_entry(ptab,
    gettext("List of preffered ciphers: "), 0, 4, FALSE);

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 5);
  gtk_widget_show(hbox);

  frame = gtk_frame_new(gettext("SSL protocol version"));
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 5);

  ptab = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(frame), ptab);
  gtk_widget_show(ptab);

  rb = gtk_radio_button_new_with_label(NULL, gettext("SSLv23"));
  rg = gtk_radio_button_group(GTK_RADIO_BUTTON(rb));
  gtk_widget_show(rb);
  gtk_container_add(GTK_CONTAINER(ptab), rb);
  gui_cfg.ssl_version[0] = rb;

  rb = gtk_radio_button_new_with_label(rg, gettext("SSLv2"));
  rg = gtk_radio_button_group(GTK_RADIO_BUTTON(rb));
  gtk_widget_show(rb);
  gtk_container_add(GTK_CONTAINER(ptab), rb);
  gui_cfg.ssl_version[1] = rb;

  rb = gtk_radio_button_new_with_label(rg, gettext("SSLv3"));
  rg = gtk_radio_button_group(GTK_RADIO_BUTTON(rb));
  gtk_widget_show(rb);
  gtk_container_add(GTK_CONTAINER(ptab), rb);
  gui_cfg.ssl_version[2] = rb;

#if defined(WITH_SSL_TLS1) || defined(USE_SSL_IMPL_NSS)
  rb = gtk_radio_button_new_with_label(rg, gettext("TLSv1"));
  rg = gtk_radio_button_group(GTK_RADIO_BUTTON(rb));
  gtk_widget_show(rb);
  gtk_container_add(GTK_CONTAINER(ptab), rb);
  gui_cfg.ssl_version[3] = rb;
#endif

  frame = gtk_frame_new(gettext("Miscelanous SSL settings"));
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 5);

  ptab = gtk_table_new(2, 2, FALSE);
  gtk_container_add(GTK_CONTAINER(frame), ptab);
  gtk_widget_show(ptab);

#ifdef USE_SSL_IMPL_OPENSSL
#ifdef HAVE_RAND_EGD
  gui_cfg.egd_socket = guitl_tab_add_path_entry(ptab,
    gettext("EGD daemon socket path: "), 0, 0, FALSE);
#endif
#endif

  gui_cfg.unique_sslid =
    gtk_check_button_new_with_label(gettext
    ("Unique ID for all SSL sessions"));
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.unique_sslid, 0, 2, 1, 2,
    GTK_FILL, GTK_FILL, 2, 2);
  gtk_widget_show(gui_cfg.unique_sslid);
#endif
}

/*** LOG ***/
static void cfgtab_log(GtkWidget * notebook)
{
  GtkWidget *col, *label, *box;

  box = gtk_vbox_new(0, 5);
  gtk_widget_show(box);
  label = gtk_label_new(gettext("Log"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);

  col = gtk_table_new(2, 4, FALSE);
  gtk_container_add(GTK_CONTAINER(box), col);
  gtk_widget_show(col);

  gui_cfg.gen_logname =
    gtk_check_button_new_with_label(gettext
    ("Try to find unique name, when original log file locked"));
  gtk_table_attach(GTK_TABLE(col), gui_cfg.gen_logname, 0, 2, 0, 1, GTK_FILL,
    GTK_FILL, 1, 1);
  gtk_widget_show(gui_cfg.gen_logname);

  gui_cfg.log_label = guitl_tab_add_path_entry(col,
    gettext("Log file: "), 0, 1, FALSE);

  gui_cfg.slog_label = guitl_tab_add_path_entry(col,
    gettext("Shortlog file: "), 0, 2, FALSE);

  gui_cfg.xloglen_label = guitl_tab_add_numentry(col,
    gettext("Log window length: "), 0, 3, INT_MAX);

}

/*** COOKIES ***/
static void cfgtab_cookies(GtkWidget * notebook)
{
  GtkWidget *label, *box, *tbox, *hbox, *pom, *frame;
  GtkAdjustment *adj;

  tbox = gtk_hbox_new(0, 5);
  gtk_widget_show(tbox);
  label = gtk_label_new(gettext("Cookies"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tbox, label);

  frame = gtk_frame_new(gettext("Disabled cookie domains"));
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(tbox), frame, TRUE, TRUE, 5);

  box =
    guitl_new_edit_list(&gui_cfg.cookie_domain_list,
    &gui_cfg.cookie_domain_entry, gettext("Domain: "), NULL, NULL, NULL, NULL,
    TRUE, NULL);
  gtk_container_add(GTK_CONTAINER(frame), box);

  frame = gtk_frame_new(gettext("Cookies settings"));
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(tbox), frame, TRUE, TRUE, 5);

  hbox = gtk_vbox_new(0, 5);
  gtk_container_add(GTK_CONTAINER(frame), hbox);
  gtk_widget_show(hbox);

  gui_cfg.sw_cookie_update =
    gtk_check_button_new_with_label(gettext("Update cookies"));
  gtk_widget_show(gui_cfg.sw_cookie_update);
  gtk_box_pack_start(GTK_BOX(hbox), gui_cfg.sw_cookie_update, FALSE, FALSE,
    0);

  gui_cfg.sw_cookie_send =
    gtk_check_button_new_with_label(gettext("Send cookies"));
  gtk_widget_show(gui_cfg.sw_cookie_send);
  gtk_box_pack_start(GTK_BOX(hbox), gui_cfg.sw_cookie_send, FALSE, FALSE, 0);

  gui_cfg.sw_cookie_recv =
    gtk_check_button_new_with_label(gettext("Accept cookies"));
  gtk_widget_show(gui_cfg.sw_cookie_recv);
  gtk_box_pack_start(GTK_BOX(hbox), gui_cfg.sw_cookie_recv, FALSE, FALSE, 0);

  gui_cfg.cookie_check_domain =
    gtk_check_button_new_with_label(gettext("Check cookies domain"));
  gtk_widget_show(gui_cfg.cookie_check_domain);
  gtk_box_pack_start(GTK_BOX(hbox), gui_cfg.cookie_check_domain, FALSE, FALSE,
    0);

  box = gtk_hbox_new(0, 5);
  gtk_widget_show(box);
  gtk_box_pack_start(GTK_BOX(hbox), box, FALSE, FALSE, 0);

  pom = gtk_label_new(gettext("Cookies maximal number: "));
  gtk_widget_show(pom);
  gtk_box_pack_start(GTK_BOX(box), pom, FALSE, FALSE, 0);

  adj =
    (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 10000.0, 1.0, 10.0, 0.0);
  gui_cfg.en_cookie_max = gtk_spin_button_new(adj, 0, 0);
  gtk_widget_show(gui_cfg.en_cookie_max);
  gtk_box_pack_start(GTK_BOX(box), gui_cfg.en_cookie_max, TRUE, FALSE, 0);


  box = gtk_table_new(2, 1, FALSE);
  gtk_widget_show(box);
  gtk_box_pack_start(GTK_BOX(hbox), box, FALSE, FALSE, 0);

  gui_cfg.en_cookie_file = guitl_tab_add_path_entry(box,
    gettext("Cookie file: "), 0, 0, FALSE);
}

static void RulesSelectRow(GtkWidget * w, gpointer func_data)
{
  gchar *p;
  int row = GPOINTER_TO_INT(GTK_CLIST(gui_cfg.rules_list)->selection->data);

#ifdef HAVE_REGEX
  gtk_clist_get_text(GTK_CLIST(gui_cfg.rules_list), row, 0, &p);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(gui_cfg.ptrn_fnmatch),
    (*p == 'F'));
#endif

  gtk_clist_get_text(GTK_CLIST(gui_cfg.rules_list), row, 1, &p);
  gtk_entry_set_text(GTK_ENTRY(gui_cfg.mpt_entry), p);

  gtk_clist_get_text(GTK_CLIST(gui_cfg.rules_list), row, 2, &p);
  gtk_entry_set_text(GTK_ENTRY(gui_cfg.rule_entry), p);
}

static void RulesNew(int row)
{
  const gchar *p[3];

  p[1] = gtk_entry_get_text(GTK_ENTRY(gui_cfg.mpt_entry));
  p[2] = gtk_entry_get_text(GTK_ENTRY(gui_cfg.rule_entry));

#ifdef HAVE_REGEX
  p[0] = GTK_TOGGLE_BUTTON(gui_cfg.ptrn_fnmatch)->active ? "F" : "R";

  if(!lfname_check_pattern(GTK_TOGGLE_BUTTON(gui_cfg.ptrn_fnmatch)->active ?
      LFNAME_FNMATCH : LFNAME_REGEX, p[1]))
  {
    gdk_beep();
    return;
  }
#else
  p[0] = "F";
#endif


  if(p[1] && p[2] && *p[1] && *p[2] && lfname_check_rule(p[2]))
  {
    if(row < 0)
      gtk_clist_append(GTK_CLIST(gui_cfg.rules_list), (gchar **) p);
    else
    {
      gtk_clist_set_text(GTK_CLIST(gui_cfg.rules_list), row, 0, p[0]);
      gtk_clist_set_text(GTK_CLIST(gui_cfg.rules_list), row, 1, p[1]);
      gtk_clist_set_text(GTK_CLIST(gui_cfg.rules_list), row, 2, p[2]);
    }
  }
  else
    gdk_beep();
}

static void RulesAppend(void)
{
  RulesNew(-1);
}

static void RulesModify(void)
{
  if(!GTK_CLIST(gui_cfg.rules_list)->selection)
  {
    gdk_beep();
    return;
  }

  RulesNew(GPOINTER_TO_INT(GTK_CLIST(gui_cfg.rules_list)->selection->data));
}

static void cfgtab_filename(GtkWidget * notebook)
{
  GtkWidget *label, *box, *frame, *ptab, *hbox, *swin, *button, *bbox;

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_widget_show(hbox);
  label = gtk_label_new(gettext("Filename"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), hbox, label);

  frame = gtk_frame_new(gettext("Local filename conversion rules"));
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 5);

  box = gtk_vbox_new(FALSE, 5);
  gtk_widget_show(box);
  gtk_container_add(GTK_CONTAINER(frame), box);

  frame = gtk_frame_new(gettext("Replace String1 with String2 in filename"));
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 5);

  ptab = gtk_table_new(2, 2, FALSE);
  gtk_container_add(GTK_CONTAINER(frame), ptab);
  gtk_widget_show(ptab);

  gui_cfg.tr_str_s1 = guitl_tab_add_entry(ptab,
    gettext("String1: "), 0, 0, FALSE);

  gui_cfg.tr_str_s2 = guitl_tab_add_entry(ptab,
    gettext("String2: "), 0, 1, FALSE);

  frame = gtk_frame_new(gettext("Delete characters from filename"));
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 5);

  ptab = gtk_table_new(2, 1, FALSE);
  gtk_container_add(GTK_CONTAINER(frame), ptab);
  gtk_widget_show(ptab);

  gui_cfg.tr_del_chr = guitl_tab_add_entry(ptab,
    gettext("Character set: "), 0, 0, FALSE);

  frame =
    gtk_frame_new(gettext
    ("Replace chars from Set1 with chars from Set2 in filename"));
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 5);

  ptab = gtk_table_new(2, 2, FALSE);
  gtk_container_add(GTK_CONTAINER(frame), ptab);
  gtk_widget_show(ptab);

  gui_cfg.tr_chr_s1 = guitl_tab_add_entry(ptab,
    gettext("Character set1: "), 0, 0, FALSE);

  gui_cfg.tr_chr_s2 = guitl_tab_add_entry(ptab,
    gettext("Character set2: "), 0, 1, FALSE);

  ptab = gtk_table_new(2, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(box), ptab, FALSE, FALSE, 5);
  gtk_widget_show(ptab);

  gui_cfg.base_level_label = guitl_tab_add_numentry(ptab,
    gettext("Base level of tree: "), 0, 0, USHRT_MAX);

  frame = gtk_frame_new(gettext("Local filename mapping rules"));
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 5);

  box = gtk_vbox_new(0, 5);
  gtk_widget_show(box);
  gtk_container_add(GTK_CONTAINER(frame), box);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(box), swin, TRUE, TRUE, 5);
  gtk_widget_show(swin);

  gui_cfg.rules_list = gtk_clist_new(3);
  gtk_clist_set_column_title(GTK_CLIST(gui_cfg.rules_list), 0,
    gettext("Type"));
  gtk_clist_set_column_title(GTK_CLIST(gui_cfg.rules_list), 1,
    gettext("Match pattern"));
  gtk_clist_set_column_title(GTK_CLIST(gui_cfg.rules_list), 2,
    gettext("Rule"));
  gtk_clist_column_titles_show(GTK_CLIST(gui_cfg.rules_list));
  gtk_clist_set_reorderable(GTK_CLIST(gui_cfg.rules_list), TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(gui_cfg.rules_list), 0, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(gui_cfg.rules_list), 1, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(gui_cfg.rules_list), 2, TRUE);
  gtk_signal_connect(GTK_OBJECT(gui_cfg.rules_list), "select_row",
    GTK_SIGNAL_FUNC(RulesSelectRow), NULL);
  gtk_container_add(GTK_CONTAINER(swin), gui_cfg.rules_list);
  gtk_widget_show(gui_cfg.rules_list);

#ifdef HAVE_REGEX
  {
    GSList *rg = NULL;

    ptab = gtk_table_new(2, 1, FALSE);
    gtk_box_pack_start(GTK_BOX(box), ptab, FALSE, TRUE, 2);
    gtk_widget_show(ptab);

    gui_cfg.ptrn_fnmatch =
      gtk_radio_button_new_with_label(NULL, gettext("wildcard pattern"));
    rg = gtk_radio_button_group(GTK_RADIO_BUTTON(gui_cfg.ptrn_fnmatch));
    gtk_widget_show(gui_cfg.ptrn_fnmatch);
    gtk_table_attach(GTK_TABLE(ptab), gui_cfg.ptrn_fnmatch, 0, 1, 0, 1,
      GTK_FILL, GTK_FILL, 2, 2);

    gui_cfg.ptrn_regex =
      gtk_radio_button_new_with_label(rg, gettext("RE pattern"));
    gtk_widget_show(gui_cfg.ptrn_regex);
    gtk_table_attach(GTK_TABLE(ptab), gui_cfg.ptrn_regex, 1, 2, 0, 1,
      GTK_FILL, GTK_FILL, 2, 2);
  }
#endif

  ptab = gtk_table_new(2, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(box), ptab, FALSE, TRUE, 2);
  gtk_widget_show(ptab);

  gui_cfg.mpt_entry = guitl_tab_add_entry(ptab,
    gettext("Matching pattern: "), 0, 0, FALSE);

  ptab = gtk_table_new(2, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(box), ptab, FALSE, TRUE, 2);
  gtk_widget_show(ptab);

  gui_cfg.rule_entry = guitl_tab_add_entry(ptab,
    gettext("Construction rule: "), 0, 0, FALSE);

  bbox = gtk_hbutton_box_new();
  gtk_widget_show(bbox);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
  gtk_box_pack_start(GTK_BOX(box), bbox, FALSE, TRUE, 5);

  button = guitl_pixmap_button(append_xpm, NULL, gettext("Append"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(RulesAppend), NULL);

  button = guitl_pixmap_button(modify_xpm, NULL, gettext("Modify"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(RulesModify), NULL);

  button = guitl_pixmap_button(clear_xpm, NULL, gettext("Clear"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_ListClear), gui_cfg.rules_list);

  button = guitl_pixmap_button(delete_xpm, NULL, gettext("Delete"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_ListDeleteSelected), gui_cfg.rules_list);
}

static void cfgtab_advert(GtkWidget * notebook)
{
#ifdef HAVE_REGEX
  GtkWidget *hbox, *vbox, *label, *box, *entry;

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_widget_show(hbox);
  label = gtk_label_new(gettext("Advertisement"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), hbox, label);

  vbox = gtk_vbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 1);
  gtk_widget_show(vbox);

  gui_cfg.remove_adv =
    gtk_check_button_new_with_label(gettext
    ("Enable removing of advertisement banners"));
  gtk_box_pack_start(GTK_BOX(vbox), gui_cfg.remove_adv, FALSE, FALSE, 1);
  gtk_widget_show(gui_cfg.remove_adv);

  box = guitl_new_edit_list(&gui_cfg.advert_res, &entry,
    gettext("RE for advertisement URLs: "), NULL, NULL, NULL, NULL, TRUE,
    NULL);
  gtk_box_pack_start(GTK_BOX(vbox), box, TRUE, TRUE, 1);
#endif
}


#ifdef HAVE_REGEX
static void JSTSelectRow(void)
{
  char *p;
  int row = GPOINTER_TO_INT(GTK_CLIST(gui_cfg.js_transform)->selection->data);

  gtk_clist_get_text(GTK_CLIST(gui_cfg.js_transform), row, 0, &p);
  gtk_entry_set_text(GTK_ENTRY(gui_cfg.jst_pattern), p);

  gtk_clist_get_text(GTK_CLIST(gui_cfg.js_transform), row, 1, &p);
  gtk_entry_set_text(GTK_ENTRY(gui_cfg.jst_rule), p);

  gtk_clist_get_text(GTK_CLIST(gui_cfg.js_transform), row, 2, &p);
  gtk_entry_set_text(GTK_ENTRY(gui_cfg.jst_tag), p);

  gtk_clist_get_text(GTK_CLIST(gui_cfg.js_transform), row, 3, &p);
  gtk_entry_set_text(GTK_ENTRY(gui_cfg.jst_attrib), p);

  gtk_clist_get_text(GTK_CLIST(gui_cfg.js_transform), row, 4, &p);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(gui_cfg.jst_rewrite),
    *p == 'X');
}

static void JSTNew(int row)
{
  const char *pp[5];
  js_transform_t *jt;
  int type;

  type = GTK_TOGGLE_BUTTON(gui_cfg.jst_rewrite)->active;
  pp[0] = gtk_entry_get_text(GTK_ENTRY(gui_cfg.jst_pattern));
  pp[1] = gtk_entry_get_text(GTK_ENTRY(gui_cfg.jst_rule));
  pp[2] = gtk_entry_get_text(GTK_ENTRY(gui_cfg.jst_tag));
  pp[3] = gtk_entry_get_text(GTK_ENTRY(gui_cfg.jst_attrib));
  pp[4] = type ? "X" : " ";

  if(!(jt = js_transform_new(pp[0], pp[1], pp[2], pp[3], type)))
  {
    gdk_beep();
    return;
  }
  js_transform_free(jt);

  if(row < 0)
    gtk_clist_append(GTK_CLIST(gui_cfg.js_transform), (gchar **) pp);
  else
  {
    int i;
    for(i = 0; i < 5; i++)
      gtk_clist_set_text(GTK_CLIST(gui_cfg.js_transform), row, i, pp[i]);
  }
}

static void JSTModify(void)
{
  if(!GTK_CLIST(gui_cfg.js_transform)->selection)
  {
    gdk_beep();
    return;
  }

  JSTNew(GPOINTER_TO_INT(GTK_CLIST(gui_cfg.js_transform)->selection->data));
}

static void JSTAppend(void)
{
  JSTNew(-1);
}

#endif

static void cfgtab_js(GtkWidget * notebook)
{
#ifdef HAVE_REGEX
  GtkWidget *hbox, *vbox, *label, *box, *entry, *ptab, *bbox, *button;
  GtkWidget *frame, *swin;
  int i;
  char **tags;
  const char *attribs[] = {
    "",
    "onclick",
    "ondblclick",
    "onmousedown",
    "onmouseup",
    "onmouseover",
    "onmousemove",
    "onmouseout",
    "onkeypress",
    "onkeydown",
    "onkeyup",
    "onfocus",
    "onblur",
    "onload",
    "onunload",
    "onsubmit",
    "onreset",
    "onselect",
    "onchange",
    NULL,
  };

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_widget_show(hbox);
  label = gtk_label_new(gettext("Javascript"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), hbox, label);

  vbox = gtk_vbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 1);
  gtk_widget_show(vbox);

  gui_cfg.enable_js =
    gtk_check_button_new_with_label(gettext("Processing of javascript"));
  gtk_box_pack_start(GTK_BOX(vbox), gui_cfg.enable_js, FALSE, TRUE, 1);
  gtk_widget_show(gui_cfg.enable_js);

  frame = gtk_frame_new(gettext("Javascript patterns"));
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 1);
  gtk_widget_show(frame);

  box = guitl_new_edit_list(&gui_cfg.js_patterns, &entry,
    gettext("RE for Javascript patterns: "), NULL, NULL, NULL, NULL, TRUE,
    NULL);
  gtk_container_add(GTK_CONTAINER(frame), box);



/*****/
  frame = gtk_frame_new(gettext("Javascript patterns with transform rules"));
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 1);
  gtk_widget_show(frame);

  vbox = gtk_vbox_new(0, 5);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(vbox);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(vbox), swin, TRUE, TRUE, 5);
  gtk_widget_show(swin);

  gui_cfg.js_transform = gtk_clist_new(5);
  gtk_clist_set_column_title(GTK_CLIST(gui_cfg.js_transform), 0,
    gettext("Pattern"));
  gtk_clist_set_column_title(GTK_CLIST(gui_cfg.js_transform), 1,
    gettext("Transform rule"));
  gtk_clist_set_column_title(GTK_CLIST(gui_cfg.js_transform), 2,
    gettext("HTML tag"));
  gtk_clist_set_column_title(GTK_CLIST(gui_cfg.js_transform), 3,
    gettext("HTML tag attribute"));
  gtk_clist_set_column_title(GTK_CLIST(gui_cfg.js_transform), 4,
    gettext("Rewrite"));
  gtk_clist_column_titles_show(GTK_CLIST(gui_cfg.js_transform));
  gtk_clist_set_reorderable(GTK_CLIST(gui_cfg.js_transform), TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(gui_cfg.js_transform), 0, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(gui_cfg.js_transform), 1, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(gui_cfg.js_transform), 2, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(gui_cfg.js_transform), 3, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(gui_cfg.js_transform), 4, TRUE);
  gtk_signal_connect(GTK_OBJECT(gui_cfg.js_transform), "select_row",
    GTK_SIGNAL_FUNC(JSTSelectRow), NULL);
  gtk_container_add(GTK_CONTAINER(swin), gui_cfg.js_transform);
  gtk_widget_show(gui_cfg.js_transform);


  ptab = gtk_table_new(2, 5, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), ptab, FALSE, TRUE, 2);
  gtk_widget_show(ptab);

  gui_cfg.jst_pattern = guitl_tab_add_entry(ptab,
    gettext("Pattern: "), 0, 0, FALSE);

  gui_cfg.jst_rule = guitl_tab_add_entry(ptab,
    gettext("Tranform rule: "), 0, 1, FALSE);

  tags = _malloc((html_link_tags_num() + 3) * sizeof(char *));
  tags[0] = "";
  tags[1] = "*";
  for(i = 0; i < html_link_tags_num(); i++)
    tags[i + 2] = html_link_tags[i].tag;
  tags[i] = NULL;
  gui_cfg.jst_tag = guitl_tab_add_enum(ptab,
    gettext("HTML tag: "), 0, 2, (const char **) tags, TRUE);
  _free(tags);

  gui_cfg.jst_attrib = guitl_tab_add_enum(ptab,
    gettext("HTML tag attribute: "), 0, 3, attribs, TRUE);

  gui_cfg.jst_rewrite =
    gtk_check_button_new_with_label(gettext
    ("Rewrite URL part in HTML document"));
  gtk_table_attach(GTK_TABLE(ptab), gui_cfg.jst_rewrite, 0, 2, 4, 5, GTK_FILL,
    GTK_FILL, 2, 2);
  gtk_widget_show(gui_cfg.jst_rewrite);

  bbox = gtk_hbutton_box_new();
  gtk_widget_show(bbox);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
  gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 5);

  button = guitl_pixmap_button(append_xpm, NULL, gettext("Append"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(JSTAppend), NULL);

  button = guitl_pixmap_button(modify_xpm, NULL, gettext("Modify"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(JSTModify), NULL);

  button = guitl_pixmap_button(clear_xpm, NULL, gettext("Clear"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_ListClear), gui_cfg.js_transform);

  button = guitl_pixmap_button(delete_xpm, NULL, gettext("Delete"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_ListDeleteSelected), gui_cfg.js_transform);
#endif
}

/*** COMMON CFG ***/
void gui_build_config_common(int popup)
{
  GtkWidget *col, *brow, *button, *notebook;
  GtkAccelGroup *accel_group;

  if(gui_cfg.config_shell)
  {
    if(popup)
    {
      if(!GTK_WIDGET_VISIBLE(gui_cfg.config_shell))
        xset_cfg_values_comm();
      gtk_widget_show_all(gui_cfg.config_shell);
      if(GTK_WIDGET_REALIZED(gui_cfg.config_shell))
        gdk_window_raise(gui_cfg.config_shell->window);
    }
    return;
  }

  gui_cfg.config_shell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_border_width(GTK_CONTAINER(gui_cfg.config_shell), 3);
  gtk_window_set_title(GTK_WINDOW(gui_cfg.config_shell),
    gettext("Pavuk: Common config"));
  gtk_widget_realize(gui_cfg.config_shell);
  gtk_signal_connect(GTK_OBJECT(gui_cfg.config_shell), "destroy",
    GTK_SIGNAL_FUNC(gtk_widget_destroyed), &gui_cfg.config_shell);

  col = gtk_table_new(2, 1, FALSE);
  gtk_container_add(GTK_CONTAINER(gui_cfg.config_shell), col);
  gtk_widget_show(col);

  gui_cfg.cb_comcfg = notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
  gtk_table_attach_defaults(GTK_TABLE(col), notebook, 0, 1, 0, 1);
  gtk_widget_show(notebook);

  cfgtab_url(notebook);

  cfgtab_formdata(notebook);

  cfgtab_graberI(notebook);

  cfgtab_graberII(notebook);

  cfgtab_html(notebook);

  cfgtab_net(notebook);

  cfgtab_proxy(notebook);

  cfgtab_lang(notebook);

  cfgtab_auth(notebook);

  cfgtab_ssl(notebook);

  cfgtab_log(notebook);

  cfgtab_cookies(notebook);

  cfgtab_filename(notebook);

  cfgtab_advert(notebook);

  cfgtab_js(notebook);

  brow = gtk_hbutton_box_new();
  gtk_table_attach(GTK_TABLE(col), brow, 0, 1, 1, 2,
    GTK_EXPAND | GTK_FILL, GTK_FILL, 2, 5);
  gtk_hbutton_box_set_spacing_default(1);
  gtk_widget_show(brow);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(brow), GTK_BUTTONBOX_SPREAD);

  button = guitl_pixmap_button(ok_xpm, NULL, gettext("OK"));
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(gui_PopdownWC), (gpointer) gui_cfg.config_shell);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  button = guitl_pixmap_button(apply_xpm, NULL, gettext("Apply"));
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(CfgCommon), (gpointer) gui_cfg.config_shell);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_show(button);

  button = guitl_pixmap_button(limit_xpm, NULL, gettext("Limitations ..."));
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(gui_PopupW), (gpointer) PAVUK_CFGLIM);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_show(button);

  button = guitl_pixmap_button(cancel_xpm, NULL, gettext("Cancel"));

  accel_group = gtk_accel_group_new();
  gtk_widget_add_accelerator(button, "clicked", accel_group,
    GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_window_add_accel_group(GTK_WINDOW(gui_cfg.config_shell), accel_group);

  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_PopdownW), (gpointer) gui_cfg.config_shell);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_show(button);

  if(popup)
    gtk_widget_show(gui_cfg.config_shell);

  xset_cfg_values_comm();
}

#endif /* GTK_FACE */
