/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "gui.h"
#include "jsbind.h"

#if defined(GTK_FACE) && defined(HAVE_MOZJS)
#include <gdk/gdkkeysyms.h>
#include <string.h>

#include "icons/cancel.xpm"
#include "icons/restart_small.xpm"
#include "icons/save.xpm"
#include "icons/load.xpm"
#include "icons/append.xpm"

static GtkWidget *pjs_source_text;
static GtkWidget *pjs_file;
static GtkWidget *pjs_prompt;

static void load_script_file(void)
{
  char *fn;
  char *fc;

  fn = gtk_entry_get_text(GTK_ENTRY(pjs_file));

  if(!fn || !fn[0])
  {
    gdk_beep();
    return;
  }

  _free(cfg.js_script_file);
  cfg.js_script_file = tl_strdup(fn);

  fc = tl_load_text_file(fn);

  gtk_text_set_point(GTK_TEXT(pjs_source_text), 0);
  gtk_text_forward_delete(GTK_TEXT(pjs_source_text),
    gtk_text_get_length(GTK_TEXT(pjs_source_text)));
  gtk_text_insert(GTK_TEXT(pjs_source_text),
    NULL, NULL, NULL, fc, strlen(fc));

  _free(fc);
}

static void save_script_file(void)
{
  char *fn;
  char *fc;
  int len;

  fn = gtk_entry_get_text(GTK_ENTRY(pjs_file));

  if(!fn || !fn[0])
  {
    gdk_beep();
    return;
  }

  _free(cfg.js_script_file);
  cfg.js_script_file = tl_strdup(fn);

  len = gtk_text_get_length(GTK_TEXT(pjs_source_text));
  fc = gtk_editable_get_chars(GTK_EDITABLE(pjs_source_text), 0, len);

  if(tl_save_text_file(fn, fc, len))
    gdk_beep();

  g_free(fc);
}

static void restart_runtime(void)
{
  char *fn;

  fn = gtk_entry_get_text(GTK_ENTRY(pjs_file));
  _free(cfg.js_script_file);

  if(fn && fn[0])
    cfg.js_script_file = tl_strdup(fn);

  pjs_destroy();
  pjs_init();
}

static void set_script(void)
{
  int len;
  char *fc;

  len = gtk_text_get_length(GTK_TEXT(pjs_source_text));
  fc = gtk_editable_get_chars(GTK_EDITABLE(pjs_source_text), 0, len);

  if(len && fc)
  {
    if(pjs_load_script_string(fc))
      gdk_beep();
  }
  else
    gdk_beep();
}


static void evaluate_script(void)
{
  char *fc;

  fc = gtk_entry_get_text(GTK_ENTRY(pjs_prompt));

  if(fc && fc[0])
  {
    if(!pjs_execute(fc))
      gdk_beep();
    else
      gtk_entry_select_region(GTK_ENTRY(pjs_prompt), 0, strlen(fc));
  }
  else
    gdk_beep();
}

void gui_pjs_console(int popup)
{
  GtkWidget *col, *row, *button, *sep;
  GtkWidget *hsb, *vsb;
  GtkAdjustment *hadj, *vadj;
  GtkAccelGroup *accel_group;

  if(gui_cfg.pjs_console_shell)
  {
    if(popup)
    {
      gtk_widget_show_all(gui_cfg.pjs_console_shell);
      if(GTK_WIDGET_REALIZED(gui_cfg.pjs_console_shell))
        gdk_window_raise(gui_cfg.pjs_console_shell->window);
    }
    return;
  }

  pjs_init();

  gui_cfg.pjs_console_shell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(gui_cfg.pjs_console_shell),
    gettext("Pavuk: JavaScript console"));
  gtk_container_border_width(GTK_CONTAINER(gui_cfg.pjs_console_shell), 3);
  gtk_signal_connect(GTK_OBJECT(gui_cfg.pjs_console_shell), "destroy",
    GTK_SIGNAL_FUNC(gtk_widget_destroyed), &gui_cfg.pjs_console_shell);

  col = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(gui_cfg.pjs_console_shell), col);
  gtk_widget_show(col);

  row = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
  gtk_box_pack_start(GTK_BOX(col), row, FALSE, FALSE, 1);
  gtk_toolbar_set_button_relief(GTK_TOOLBAR(row), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_size(GTK_TOOLBAR(row), 10);
  gtk_widget_show(row);

  button = guitl_toolbar_button(row, NULL,
    gettext("Reload script file"),
    load_xpm, GTK_SIGNAL_FUNC(load_script_file), NULL, NULL);

  button = guitl_toolbar_button(row, NULL,
    gettext("Save script to file"),
    save_xpm, GTK_SIGNAL_FUNC(save_script_file),
    (gpointer) gui_cfg.pjs_console_shell, NULL);

  gtk_toolbar_append_space(GTK_TOOLBAR(row));

  button = guitl_toolbar_button(row, NULL,
    gettext("Load script to JavaScript runtime"),
    append_xpm, GTK_SIGNAL_FUNC(set_script), NULL, NULL);

  button = guitl_toolbar_button(row, NULL,
    gettext("Restart JavaScript runtime"),
    restart_small_xpm, GTK_SIGNAL_FUNC(restart_runtime), NULL, NULL);

  gtk_toolbar_append_space(GTK_TOOLBAR(row));

  button = guitl_toolbar_button(row, NULL, gettext("Close"), cancel_xpm,
    GTK_SIGNAL_FUNC(guitl_PopdownW),
    (gpointer) gui_cfg.pjs_console_shell, NULL);

  accel_group = gtk_accel_group_new();
  gtk_widget_add_accelerator(button, "clicked", accel_group,
    GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_window_add_accel_group(GTK_WINDOW(gui_cfg.pjs_console_shell),
    accel_group);

  sep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(col), sep, FALSE, TRUE, 1);
  gtk_widget_show(sep);

  row = gtk_table_new(2, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(col), row, FALSE, FALSE, 2);
  gtk_widget_show(row);

  pjs_prompt = guitl_tab_add_entry(row, gettext("Prompt: "), 0, 0, FALSE);
  gtk_signal_connect(GTK_OBJECT(pjs_prompt), "activate",
    GTK_SIGNAL_FUNC(evaluate_script), NULL);

  sep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(col), sep, FALSE, TRUE, 1);
  gtk_widget_show(sep);

  row = gtk_table_new(2, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(col), row, FALSE, FALSE, 2);
  gtk_widget_show(row);

  pjs_file = guitl_tab_add_path_entry(row,
    gettext("JavaScript source file: "), 0, 0, FALSE);
  gtk_entry_set_text(GTK_ENTRY(pjs_file),
    cfg.js_script_file ? cfg.js_script_file : "");

  row = gtk_table_new(2, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(col), row, TRUE, TRUE, 2);
  gtk_widget_show(row);

  hadj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  hsb = gtk_hscrollbar_new(hadj);
  gtk_table_attach(GTK_TABLE(row), hsb, 0, 1, 1, 2,
    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(hsb);

  vadj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  vsb = gtk_vscrollbar_new(vadj);
  gtk_table_attach(GTK_TABLE(row), vsb, 1, 2, 0, 1,
    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(vsb);

  pjs_source_text = gtk_text_new(hadj, vadj);
  gtk_widget_set_usize(pjs_source_text, 400, 300);
  gtk_text_set_editable(GTK_TEXT(pjs_source_text), TRUE);
  gtk_table_attach(GTK_TABLE(row), pjs_source_text, 0, 1, 0, 1,
    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(pjs_source_text);
  if(cfg.js_script_file)
    load_script_file();

  if(popup)
  {
    gtk_widget_show(gui_cfg.pjs_console_shell);
    if(GTK_WIDGET_REALIZED(gui_cfg.pjs_console_shell))
      gdk_window_raise(gui_cfg.pjs_console_shell->window);
  }
}

#endif
