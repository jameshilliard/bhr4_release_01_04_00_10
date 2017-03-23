/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef GTK_FACE
#include "gui.h"

#include "icons/append.xpm"
#include "icons/ok.xpm"
#include "icons/browse.xpm"
#include "icons/clear.xpm"
#include "icons/delete.xpm"
#include "icons/modify.xpm"

GtkWidget *guitl_timesel_new(GtkWidget ** calendar, GtkWidget ** hour_entry,
  GtkWidget ** min_entry, GtkWidget ** month_combo, GtkWidget ** year_entry)
{
  GtkWidget *col, *frame, *brow, *label;
  GtkAdjustment *adj;

  col = gtk_vbox_new(0, 10);
  gtk_widget_show(col);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(col), frame, FALSE, FALSE, 5);

  *calendar = gtk_calendar_new();
  gtk_widget_show(*calendar);
  gtk_container_add(GTK_CONTAINER(frame), *calendar);
  gtk_calendar_display_options(GTK_CALENDAR(*calendar),
    GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES);
  *year_entry = NULL;
  *month_combo = NULL;

  brow = gtk_hbox_new(0, 5);
  gtk_widget_show(brow);
  gtk_box_pack_start(GTK_BOX(col), brow, FALSE, FALSE, 4);

  label = gtk_label_new(gettext("Time: "));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(brow), label, FALSE, FALSE, 0);

  adj = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 23.0, 1.0, 5.0, 0.0);
  *hour_entry = gtk_spin_button_new(adj, 0, 0);
  gtk_widget_show(*hour_entry);
  gtk_box_pack_start(GTK_BOX(brow), *hour_entry, FALSE, FALSE, 0);

  label = gtk_label_new(gettext(" : "));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(brow), label, FALSE, FALSE, 0);

  adj = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 59.0, 1.0, 5.0, 0.0);
  *min_entry = gtk_spin_button_new(adj, 0, 0);
  gtk_widget_show(*min_entry);
  gtk_box_pack_start(GTK_BOX(brow), *min_entry, FALSE, FALSE, 0);

  return col;
}

GtkWidget *guitl_tab_add_entry(GtkWidget * table, char *label, guint col,
  guint row, guint passw)
{
  GtkWidget *entry, *labelw;

  labelw = gtk_label_new(label);
  gtk_misc_set_alignment(GTK_MISC(labelw), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), labelw, col, col + 1, row, row + 1,
    GTK_FILL | GTK_SHRINK, GTK_FILL, 2, 2);
  gtk_widget_show(labelw);

  entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), entry, col + 1, col + 2, row, row + 1,
    GTK_FILL | GTK_EXPAND, GTK_FILL, 2, 2);
  if(passw)
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);

  gtk_widget_show(entry);

  return entry;
}

GtkWidget *guitl_tab_add_numentry(GtkWidget * table, char *label, guint col,
  guint row, guint max_val)
{
  GtkWidget *entry, *labelw;
  GtkAdjustment *adj;
  GdkFont *font;

  labelw = gtk_label_new(label);
  gtk_misc_set_alignment(GTK_MISC(labelw), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), labelw, col, col + 1, row, row + 1,
    GTK_FILL | GTK_SHRINK, GTK_FILL, 2, 2);
  gtk_widget_show(labelw);

  adj = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0,
    (float) max_val, 1.0, 5.0, 0.0);

  entry = gtk_spin_button_new(adj, 0, 0);
  gtk_table_attach(GTK_TABLE(table), entry, col + 1, col + 2, row, row + 1,
    0, GTK_FILL, 2, 2);
#if GTK_FACE < 2
  font = entry->style->font;
#else
  font = gtk_style_get_font(entry->style);
#endif

  gtk_widget_set_usize(entry, gdk_string_width(font, "000000000"), -1);
  gtk_widget_show(entry);

  return entry;
}

GtkWidget *guitl_tab_add_doubleentry(GtkWidget * table, char *label,
  guint col, guint row, guint max_val, guint decimals)
{
  GtkWidget *entry, *labelw;
  GtkAdjustment *adj;
  GdkFont *font;

  labelw = gtk_label_new(label);
  gtk_misc_set_alignment(GTK_MISC(labelw), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), labelw, col, col + 1, row, row + 1,
    GTK_FILL | GTK_SHRINK, GTK_FILL, 2, 2);
  gtk_widget_show(labelw);

  adj = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0,
    (float) max_val, 1.0, 5.0, 0.0);

  entry = gtk_spin_button_new(adj, 0, decimals);
  gtk_table_attach(GTK_TABLE(table), entry, col + 1, col + 2, row, row + 1,
    0, GTK_FILL, 2, 2);
#if GTK_FACE < 2
  font = entry->style->font;
#else
  font = gtk_style_get_font(entry->style);
#endif
  gtk_widget_set_usize(entry, gdk_string_width(font, "000000000.000"), -1);
  gtk_widget_show(entry);

  return entry;
}

GtkWidget *guitl_tab_add_enum(GtkWidget * table, char *label, guint col,
  guint row, const char **vals, guint edit)
{
  GtkWidget *entry, *labelw;
  GList *ptr;
  int i;

  labelw = gtk_label_new(label);
  gtk_misc_set_alignment(GTK_MISC(labelw), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), labelw, col, col + 1, row, row + 1,
    GTK_FILL | GTK_SHRINK, GTK_FILL, 2, 2);
  gtk_widget_show(labelw);

  entry = gtk_combo_new();
  ptr = NULL;
  for(i = 0; vals[i]; i++)
    ptr = g_list_append(ptr, (char *) vals[i]);
  gtk_combo_set_popdown_strings(GTK_COMBO(entry), ptr);
  gtk_table_attach(GTK_TABLE(table), entry, col + 1, col + 2, row, row + 1,
    GTK_FILL | GTK_EXPAND, GTK_FILL, 2, 2);
  gtk_widget_show(entry);

  while(ptr)
    ptr = g_list_remove_link(ptr, ptr);

  return GTK_COMBO(entry)->entry;
}

static void DestroyW(GtkObject * object, gpointer func_data)
{
  gtk_widget_destroy(GTK_WIDGET(func_data));
}


struct edit_entry_info
{
  GtkWidget *entry;
  GtkWidget *tl;
  GtkWidget *list;
  GtkWidget *list_entry;
  char *label;
  char *tlabel;
  guint quoted;
};

static void edit_entry_ok(GtkWidget * w, struct edit_entry_info *info)
{
  char *p = NULL;
  char *pom;
  int i;

  for(i = 0; i < GTK_CLIST(info->list)->rows; i++)
  {
    gtk_clist_get_text(GTK_CLIST(info->list), i, 0, &pom);

    if(info->quoted)
    {
      char *p1;
      p1 = escape_str(pom, "\\\"");
      p = tl_str_concat(p, p ? " " : "", "\"", p1, "\"", NULL);
      _free(p1);
    }
    else
      p = tl_str_concat(p, p ? "," : "", pom, NULL);

  }

  gtk_entry_set_text(GTK_ENTRY(info->entry), p ? p : "");
  _free(p);

  gtk_widget_destroy(info->tl);
}

static void edit_entry_start_edit(GtkWidget * w, struct edit_entry_info *info)
{
  const char *p;

  if(!info->tl)
  {
    GtkWidget *brow, *col, *button, *frame;
    char pom[2048];

    snprintf(pom, sizeof(pom), gettext("Pavuk: edit %s"),
      info->tlabel ? info->tlabel : gettext("entry"));

    info->tl = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_border_width(GTK_CONTAINER(info->tl), 5);
    gtk_window_set_title(GTK_WINDOW(info->tl), pom);
    gtk_signal_connect(GTK_OBJECT(info->tl), "destroy",
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &(info->tl));

    col = gtk_vbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(info->tl), col);
    gtk_widget_show(col);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(col), frame);
    gtk_widget_show(frame);

    brow = guitl_new_edit_list(&info->list, &info->list_entry,
      info->label, NULL, NULL, NULL, NULL, TRUE, NULL);
    gtk_container_add(GTK_CONTAINER(frame), brow);
    gtk_widget_show(brow);

    brow = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(brow), GTK_BUTTONBOX_SPREAD);
    gtk_box_pack_end(GTK_BOX(col), brow, FALSE, FALSE, 1);
    gtk_widget_show(brow);

    button = guitl_pixmap_button(ok_xpm, NULL, gettext("OK"));
    gtk_container_add(GTK_CONTAINER(brow), button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(edit_entry_ok), (gpointer) info);
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(button);
    gtk_widget_show(button);

    button = guitl_pixmap_button(ok_xpm, NULL, gettext("Cancel"));
    gtk_container_add(GTK_CONTAINER(brow), button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(DestroyW), (gpointer) info->tl);
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(button);
    gtk_widget_show(button);
  }

  p = gtk_entry_get_text(GTK_ENTRY(info->entry));
  gtk_clist_clear(GTK_CLIST(info->list));
  if(info->quoted)
  {
    if(*p == '\"')
    {
      for(p = get_1qstr(p); p; p = get_1qstr(NULL))
        gtk_clist_append(GTK_CLIST(info->list), (gchar **) &p);
    }
    else if(*p)
      gtk_clist_append(GTK_CLIST(info->list), (gchar **) &p);
  }
  else
  {
    char **v;
    int i;

    v = tl_str_split(p, ",");
    for(i = 0; v && v[i]; i++)
      gtk_clist_append(GTK_CLIST(info->list), &(v[i]));
    tl_strv_free(v);
  }

  if(GTK_WIDGET_REALIZED(info->tl))
    gdk_window_raise(info->tl->window);
  gtk_widget_show(info->tl);
}

static void edit_entry_info_destroy(GtkWidget * w,
  struct edit_entry_info *info)
{
  if(info->tl)
  {
    gtk_signal_disconnect_by_func(GTK_OBJECT(info->tl),
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &(info->tl));
    gtk_widget_destroy(info->tl);

  }
  _free(info->label);
  _free(info->tlabel);
  _free(info);
}

static void edit_entry_info_hide(GtkWidget * w, struct edit_entry_info *info)
{
  if(info->tl)
    gtk_widget_destroy(info->tl);
}

GtkWidget *guitl_tab_add_edit_entry(GtkWidget * table, char *label,
  char *elabel, guint col, guint row, guint quoted)
{
  GtkWidget *entry, *labelw, *pom, *button, *box;
  struct edit_entry_info *info;
  int ofs = 0;

  info = _malloc(sizeof(struct edit_entry_info));
  memset(info, '\0', sizeof(struct edit_entry_info));
  info->tlabel = elabel ? tl_strdup(elabel) :
    (label ? tl_strndup(label, strlen(label) - 2) : NULL);
  info->label = tl_strdup(label);
  info->quoted = quoted;

  box = gtk_hbox_new(0, 0);
  gtk_widget_show(box);
  gtk_table_attach(GTK_TABLE(table), box, col, col + 1, row, row + 1,
    GTK_FILL | GTK_SHRINK, GTK_FILL, 2, 2);

  if(label)
  {
    labelw = gtk_label_new(label);
    gtk_box_pack_start(GTK_BOX(box), labelw, FALSE, TRUE, 0);
    gtk_misc_set_alignment(GTK_MISC(labelw), 0.0, 0.5);
    gtk_widget_show(labelw);
    ofs = 1;
  }

  pom = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(pom);
  gtk_table_attach(GTK_TABLE(table), pom, col + ofs, col + ofs + 1,
    row, row + 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 2, 2);

  info->entry = entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(pom), entry, TRUE, TRUE, 0);
  gtk_widget_show(entry);

  gtk_signal_connect(GTK_OBJECT(entry), "destroy",
    GTK_SIGNAL_FUNC(edit_entry_info_destroy), (gpointer) info);

  gtk_signal_connect(GTK_OBJECT(gtk_widget_get_toplevel(GTK_WIDGET(entry))),
    "hide", GTK_SIGNAL_FUNC(edit_entry_info_hide), (gpointer) info);

  button = guitl_pixmap_button(browse_xpm, NULL, gettext("Edit ..."));
  gtk_box_pack_end(GTK_BOX(pom), button, FALSE, TRUE, 2);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(edit_entry_start_edit), (gpointer) info);

  return entry;
}

struct path_entry_info
{
  GtkWidget *entry;
  GtkWidget *fs;
  char *label;
  char *fslabel;
  int isdir;
};

static void SetFileEntry(GtkObject * object, struct path_entry_info *info)
{
  const gchar *p;
  gboolean ok = TRUE;

  p = gtk_file_selection_get_filename(GTK_FILE_SELECTION(info->fs));

  if(!info->isdir)
  {
    struct stat estat;

    if(!stat(p, &estat) && !S_ISREG(estat.st_mode))
      ok = FALSE;
  }

  if(ok)
  {
    gtk_entry_set_text(GTK_ENTRY(info->entry), p);
    gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(object)));
  }
  else
  {
    gdk_beep();
  }
}

static void GetFile(GtkObject * object, struct path_entry_info *info)
{
  if(!info->fs)
  {
    const gchar *p;
    char pom[2048];

    snprintf(pom, sizeof(pom), gettext("Pavuk: select %s"),
      info->label ? info->label : gettext("file"));
    info->fs = gtk_file_selection_new(info->fslabel ? info->fslabel : pom);

    if(info->isdir)
      gtk_widget_set_sensitive(GTK_FILE_SELECTION(info->fs)->file_list,
        FALSE);

    gtk_signal_connect(GTK_OBJECT(info->fs), "destroy",
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &(info->fs));

    p = gtk_entry_get_text(GTK_ENTRY(info->entry));

    if(p && p[0])
      gtk_file_selection_set_filename(GTK_FILE_SELECTION(info->fs), p);

    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(info->fs)->ok_button),
      "clicked", GTK_SIGNAL_FUNC(SetFileEntry), (gpointer) info);

    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(info->fs)->
        cancel_button), "clicked", GTK_SIGNAL_FUNC(DestroyW),
      (gpointer) info->fs);
  }
  if(GTK_WIDGET_REALIZED(info->fs))
    gdk_window_raise(info->fs->window);
  gtk_widget_show(info->fs);
}

static void path_entry_info_destroy(GtkWidget * w,
  struct path_entry_info *info)
{
  if(info->fs)
  {
    gtk_signal_disconnect_by_func(GTK_OBJECT(info->fs),
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &(info->fs));
    gtk_widget_destroy(info->fs);
  }
  _free(info->label);
  _free(info->fslabel);
  _free(info);
}

static void path_entry_info_hide(GtkWidget * w, struct path_entry_info *info)
{
  if(info->fs)
    gtk_widget_destroy(info->fs);
}

GtkWidget *guitl_tab_add_path_entry_full(GtkWidget * table, char *label,
  guint col, guint row, int dir, char *fslabel)
{
  GtkWidget *entry, *labelw, *pom, *button, *box;
  struct path_entry_info *info;
  int ofs = 0;

  info = _malloc(sizeof(struct path_entry_info));
  info->fs = NULL;
  info->label = label ? tl_strndup(label, strlen(label) - 2) : NULL;
  info->fslabel = tl_strdup(fslabel);
  info->isdir = dir;

  box = gtk_hbox_new(0, 0);
  gtk_widget_show(box);
  gtk_table_attach(GTK_TABLE(table), box, col, col + 1, row, row + 1,
    GTK_FILL | GTK_SHRINK, GTK_FILL, 2, 2);

  if(label)
  {
    labelw = gtk_label_new(label);
    gtk_box_pack_start(GTK_BOX(box), labelw, FALSE, TRUE, 0);
    gtk_misc_set_alignment(GTK_MISC(labelw), 0.0, 0.5);
    gtk_widget_show(labelw);
    ofs = 1;
  }

  pom = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(pom);
  gtk_table_attach(GTK_TABLE(table), pom, col + ofs, col + ofs + 1,
    row, row + 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 2, 2);

  info->entry = entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(pom), entry, TRUE, TRUE, 0);
  gtk_widget_show(entry);

  gtk_signal_connect(GTK_OBJECT(entry), "destroy",
    GTK_SIGNAL_FUNC(path_entry_info_destroy), (gpointer) info);

  gtk_signal_connect(GTK_OBJECT(gtk_widget_get_toplevel(GTK_WIDGET(entry))),
    "hide", GTK_SIGNAL_FUNC(path_entry_info_hide), (gpointer) info);

  button = guitl_pixmap_button(browse_xpm, NULL, gettext("Browse ..."));
  gtk_box_pack_end(GTK_BOX(pom), button, FALSE, TRUE, 2);
  gtk_widget_show(button);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(GetFile), (gpointer) info);

  return entry;
}

GtkWidget *guitl_tab_add_path_entry(GtkWidget * table, char *label,
  guint col, guint row, int dir)
{
  return guitl_tab_add_path_entry_full(table, label, col, row, dir, NULL);
}

void guitl_ListDeleteSelected(GtkObject * object, gpointer func_data)
{
  GtkWidget *w = (GtkWidget *) func_data;

  gtk_clist_freeze(GTK_CLIST(w));

  if(GTK_CLIST(w)->selection)
  {
    gtk_clist_remove(GTK_CLIST(w),
      GPOINTER_TO_INT(GTK_CLIST(w)->selection->data));
  }

  gtk_clist_thaw(GTK_CLIST(w));

  if(!GTK_CLIST(w)->rows)
    gtk_signal_emit_by_name(GTK_OBJECT(func_data), "select_row", 0, 0, NULL);
}

void guitl_ListInsertEntry(GtkObject * object, gpointer func_data)
{
  listanfo *lnfo = (listanfo *) func_data;
  const char *p;

  p = gtk_entry_get_text(GTK_ENTRY(lnfo->entry));

  if(p && *p)
  {
    gtk_clist_append(GTK_CLIST(lnfo->list), (gchar **) &p);
  }
  else
    gdk_beep();
}

void guitl_ListModifyEntry(GtkObject * object, gpointer func_data)
{
  listanfo *lnfo = (listanfo *) func_data;
  const char *p;

  p = gtk_entry_get_text(GTK_ENTRY(lnfo->entry));

  if(p && *p && GTK_CLIST(lnfo->list)->selection)
  {
    int row = GPOINTER_TO_INT(GTK_CLIST(lnfo->list)->selection->data);
    gtk_clist_set_text(GTK_CLIST(lnfo->list), row, 0, p);
  }
  else
    gdk_beep();
}

void guitl_ListClear(GtkObject * object, gpointer func_data)
{
  gtk_clist_freeze(GTK_CLIST(func_data));
  gtk_clist_clear(GTK_CLIST(func_data));
  gtk_clist_thaw(GTK_CLIST(func_data));

  gtk_signal_emit_by_name(GTK_OBJECT(func_data), "select_row", 0, 0, NULL);
}

void guitl_ListCopyToEntry(GtkObject * object, int row, int col,
  GdkEvent * event, gpointer func_data)
{
  GtkWidget *entry = (GtkWidget *) func_data;
  char *p = NULL;

  if(GTK_CLIST(object)->selection)
    gtk_clist_get_text(GTK_CLIST(object), row, 0, &p);

  if(!p)
    p = "";

  if(entry)
    gtk_entry_set_text(GTK_ENTRY(entry), p);
}

void guitl_ListInsertList(GtkObject * object, gpointer func_data)
{
  listanfo *lnfo = (listanfo *) func_data;
  GList *dlist = GTK_CLIST(lnfo->entry)->selection;
  char *p;

  if(!dlist)
    gdk_beep();

  while(dlist)
  {
    gtk_clist_get_text(GTK_CLIST(lnfo->entry),
      GPOINTER_TO_INT(dlist->data), 0, &p);

    gtk_clist_append(GTK_CLIST(lnfo->list), &p);
    dlist = dlist->next;
  }
}

GtkWidget *guitl_new_edit_list(GtkWidget ** list, GtkWidget ** entry,
  char *label, GtkWidget ** dbutton, GtkWidget ** mbutton,
  GtkWidget ** cbutton, GtkWidget ** abutton, gboolean connect_entry,
  const char **vals)
{
  GtkWidget *swin;
  GtkWidget *box, *brow, *button;
  listanfo *p = NULL;

  box = gtk_table_new(3, 1, FALSE);
  gtk_widget_show(box);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_table_attach_defaults(GTK_TABLE(box), swin, 0, 1, 0, 1);
  gtk_widget_show(swin);

  *list = gtk_clist_new(1);
  gtk_clist_set_selection_mode(GTK_CLIST(*list), GTK_SELECTION_BROWSE);
  gtk_clist_set_reorderable(GTK_CLIST(*list), TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(*list), 0, TRUE);
  gtk_container_add(GTK_CONTAINER(swin), *list);
  gtk_widget_show(*list);

  brow = gtk_table_new(1, 2, FALSE);
  gtk_table_attach(GTK_TABLE(box), brow, 0, 1, 1, 2,
    GTK_EXPAND | GTK_FILL, GTK_FILL, 2, 5);
  gtk_widget_show(brow);

  if(vals)
    *entry = guitl_tab_add_enum(brow, label, 0, 0, vals, TRUE);
  else
    *entry = guitl_tab_add_entry(brow, label, 0, 0, FALSE);

  if(connect_entry)
    gtk_signal_connect(GTK_OBJECT(*list), "select_row",
      GTK_SIGNAL_FUNC(guitl_ListCopyToEntry), (gpointer) * entry);

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

  if(!abutton)
  {
    p = g_malloc(sizeof(listanfo));

    p->list = *list;
    p->entry = *entry;

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(guitl_ListInsertEntry), (gpointer) p);

    gtk_signal_connect(GTK_OBJECT(*entry), "activate",
      GTK_SIGNAL_FUNC(guitl_ListInsertEntry), (gpointer) p);
  }
  else
    *abutton = button;

  button = guitl_pixmap_button(modify_xpm, NULL, gettext("Modify"));
  gtk_container_border_width(GTK_CONTAINER(button), 0);
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_widget_show(button);

  if(!mbutton)
  {
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(guitl_ListModifyEntry), (gpointer) p);
  }
  else
    *mbutton = button;

  button = guitl_pixmap_button(clear_xpm, NULL, gettext("Clear"));
  gtk_container_border_width(GTK_CONTAINER(button), 0);
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_widget_show(button);

  if(!cbutton)
  {
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(guitl_ListClear), (gpointer) * list);
  }
  else
    *cbutton = button;

  button = guitl_pixmap_button(delete_xpm, NULL, gettext("Delete"));
  gtk_container_border_width(GTK_CONTAINER(button), 0);
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_widget_show(button);

  if(!dbutton)
  {
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(guitl_ListDeleteSelected), (gpointer) * list);
  }
  else
    *dbutton = button;

  return box;
}

Icon *guitl_load_pixmap(char **pmap)
{
  GdkWindow *win = GTK_WIDGET(gui_cfg.toplevel)->window;
  Icon *icon = malloc(sizeof(Icon));

  icon->pixmap = gdk_pixmap_create_from_xpm_d(win, &icon->shape, NULL, pmap);

  return icon;
}

GtkWidget *guitl_pixmap_button(char **pm, char *alticon, char *label)
{
  GtkWidget *btn;
  GtkWidget *pixmap = NULL;
  Icon *icon;

  btn = gtk_button_new();

  if(alticon)
  {
    GdkPixmap *pm;
    GdkBitmap *mask;

    pm =
      gdk_pixmap_create_from_xpm(GTK_WIDGET(gui_cfg.toplevel)->window, &mask,
      NULL, alticon);

    if(pm)
    {
      pixmap = gtk_pixmap_new(pm, mask);
    }
  }
  if(!pixmap)
  {
    icon = guitl_load_pixmap(pm);

    pixmap = gtk_pixmap_new(icon->pixmap, icon->shape);
    _free(icon);
  }

  if(label)
  {
    GtkWidget *box;
    GtkWidget *lbl;

    box = gtk_hbox_new(FALSE, 1);
    gtk_container_add(GTK_CONTAINER(btn), box);
    gtk_widget_show(box);

    gtk_box_pack_start(GTK_BOX(box), pixmap, TRUE, TRUE, 0);
    gtk_misc_set_alignment(GTK_MISC(pixmap), 1.0, 0.5);
    gtk_widget_show(pixmap);

    lbl = gtk_label_new(label);
    gtk_misc_set_alignment(GTK_MISC(lbl), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(box), lbl, TRUE, TRUE, 0);
    gtk_widget_show(lbl);

  }
  else
  {
    gtk_container_add(GTK_CONTAINER(btn), pixmap);
    gtk_widget_show(pixmap);
  }

  return btn;
}

void guitl_PopdownW(GtkObject * object, gpointer func_data)
{
  GtkWidget *w = (GtkWidget *) func_data;

  gtk_widget_hide(w);
}

static void menu_kill(GtkWidget * w, GtkWidget * menu)
{
  gtk_widget_destroy(menu);
}

void guitl_menu_attach(GtkWidget * menu, GtkWidget * parent)
{
  gtk_signal_connect(GTK_OBJECT(parent), "destroy",
    GTK_SIGNAL_FUNC(menu_kill), menu);
  gtk_object_set_data(GTK_OBJECT(menu), "guitl_menu_parent", parent);
}

GtkWidget *guitl_menu_parent(GtkWidget * menu)
{
  return gtk_object_get_data(GTK_OBJECT(menu), "guitl_menu_parent");
}

static char *guitl_selection_content = NULL;
enum
{
  GUITL_SEL_TYPE_NONE,
  GUITL_COMPOUND_TEXT,
  GUITL_STRING,
  GUITL_TEXT,
  GUITL_LAST_SEL_TYPE
};

static gint guitl_selection_clear(GtkWidget * widget,
  GdkEventSelection * event)
{
  g_free(guitl_selection_content);
  guitl_selection_content = NULL;

  return TRUE;
}

static void guitl_selection_get(GtkWidget * widget,
  GtkSelectionData * selection_data, guint info, guint time, gpointer data)
{
  GdkAtom type = GDK_NONE;
  switch (info)
  {
  case GUITL_COMPOUND_TEXT:
    type = gdk_atom_intern("COMPOUND_TEXT", FALSE);
    break;
  case GUITL_TEXT:
    type = gdk_atom_intern("TEXT", FALSE);
    break;
  case GUITL_STRING:
    type = gdk_atom_intern("STRING", FALSE);
    break;
  }
  gtk_selection_data_set(selection_data, type, 8 * sizeof(gchar),
    guitl_selection_content, strlen(guitl_selection_content));
}

void guitl_set_clipboard_content(char *content)
{
  GdkAtom clipboard_atom;
  static bool_t initialized = FALSE;
  static GtkTargetEntry targetlist[] = {
    {"STRING", 0, GUITL_STRING},
    {"TEXT", 0, GUITL_TEXT},
    {"COMPOUND_TEXT", 0, GUITL_COMPOUND_TEXT}
  };

  if(!initialized)
  {
    initialized = TRUE;
    clipboard_atom = gdk_atom_intern("CLIPBOARD", FALSE);

    gtk_selection_add_targets(gui_cfg.toplevel,
      GDK_SELECTION_PRIMARY,
      targetlist, sizeof(targetlist) / sizeof(targetlist[0]));
    gtk_selection_add_targets(gui_cfg.toplevel, clipboard_atom,
      targetlist, sizeof(targetlist) / sizeof(targetlist[0]));
    gtk_signal_connect(GTK_OBJECT(gui_cfg.toplevel),
      "selection_get", GTK_SIGNAL_FUNC(guitl_selection_get), NULL);
    gtk_signal_connect(GTK_OBJECT(gui_cfg.toplevel),
      "selection_clear_event", GTK_SIGNAL_FUNC(guitl_selection_clear), NULL);
  }

  g_free(guitl_selection_content);
  guitl_selection_content = g_strdup(content);

  gtk_selection_owner_set(gui_cfg.toplevel,
    GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
  clipboard_atom = gdk_atom_intern("CLIPBOARD", FALSE);
  gtk_selection_owner_set(gui_cfg.toplevel, clipboard_atom, GDK_CURRENT_TIME);
}

void guitl_clist_selection_to_clipboard(GtkWidget * w, GtkWidget * clist)
{
  char *t = NULL, *p;
  GList *sel;

  for(sel = GTK_CLIST(clist)->selection; sel; sel = sel->next)
  {
    int row = GPOINTER_TO_INT(sel->data);
    gtk_clist_get_text(GTK_CLIST(clist), row, 0, &p);
    t = tl_str_concat(t, p, "\n", NULL);
  }

  if(t)
  {
    guitl_set_clipboard_content(t);
    _free(t);
  }
}

GtkWidget *guitl_toolbar_button(GtkWidget * parent, char *label, char *help,
  char **pmap, GtkSignalFunc cb, gpointer cb_data, char *alticon)
{
  GtkWidget *button = NULL;
  GtkWidget *pixmap = NULL;
  Icon *icon;

  if(alticon)
  {
    GdkPixmap *pm;
    GdkBitmap *mask;

    pm = gdk_pixmap_create_from_xpm(GTK_WIDGET(gui_cfg.toplevel)->window,
      &mask, NULL, alticon);

    if(pm)
      pixmap = gtk_pixmap_new(pm, mask);
  }
  if(pmap && !pixmap)
  {
    icon = guitl_load_pixmap(pmap);

    pixmap = gtk_pixmap_new(icon->pixmap, icon->shape);
    _free(icon);
  }

  button = gtk_toolbar_append_item(GTK_TOOLBAR(parent), label,
    help, "", pixmap, cb, cb_data);

  return button;
}
#endif
