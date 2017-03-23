/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "gui.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#ifdef GTK_FACE

#ifdef __CYGWIN__
#include <windows.h>
#include "pavukrc.h"
#include <gdk/gdkx.h>
#else /* !__CYGWIN__ */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#ifdef HAVE_XMU
#include <X11/Xmu/WinUtil.h>
#endif
#include <gdk/gdkx.h>
#endif /* !__CYGWIN__ */

#include "gaccel.h"
#include "gkeys.h"
#include "gprop.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "gui_api.h"
#include "gauthinfo.h"
#include "form.h"
#include "tools.h"
#include "uconfig.h"
#include "ainterface.h"
#include "recurse.h"
#include "icons.h"
#include "stats.h"
#include "dns.h"
#include "times.h"

#include "icons/configure.xpm"
#include "icons/limits.xpm"
#include "icons/gobg.xpm"
#include "icons/exit.xpm"
#include "icons/stop.xpm"
#include "icons/break.xpm"
#include "icons/continue.xpm"
#include "icons/restart.xpm"
#include "icons/break_small.xpm"
#include "icons/close_small.xpm"
#include "icons/continue_small.xpm"
#include "icons/maximize_small.xpm"
#include "icons/minimize_small.xpm"
#include "icons/restart_small.xpm"
#include "icons/stop_small.xpm"

Gtk_nfo gui_cfg;

static bool_t _restart_iface = TRUE;
#ifdef GETTEXT_NLS
static char *last_lang = NULL;
#endif
static bool_t miniaturized = FALSE;
static bool_t resizing = FALSE;

#ifdef HAVE_MT
static GtkWidget *logw_pane = NULL;
#endif

static enum
{
  GUI_ACT_GOBG,
  GUI_ACT_RESTART,
  GUI_ACT_CONTINUE,
  GUI_ACT_CHANGELANG,

  GUI_ACT_NONE
} gui_action = GUI_ACT_NONE;

const GtkTargetEntry dragtypes[] = {
  {"STRING", 0, 0},
  {"text/plain", 0, 0},
  {"application/x-rootwin-drop", 0, 1}
};

#ifdef DEBUG
void gui_set_debug_level_mi(void)
{
  int i;
  GList *ch = GTK_MENU_SHELL(gui_cfg.debug_level_m)->children;

  for(i = 0; cfg_debug_levels[i].id; i++)
  {
    if(ch)
    {
      gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(ch->data),
        (cfg.debug_level & cfg_debug_levels[i].id) != 0);
      ch = ch->next;
    }
  }
}
#endif

static GtkWidget *menu_item(gchar * field)
{
  GtkWidget *item, *label;
  guint accelerator_key;

  item = gtk_menu_item_new();

  label = gtk_accel_label_new(field);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(item), label);
  gtk_accel_label_set_accel_widget(GTK_ACCEL_LABEL(label), item);
  gtk_widget_show(label);

  accelerator_key = gtk_label_parse_uline(GTK_LABEL(label), field);

  if(accelerator_key != GDK_VoidSymbol)
  {
    gtk_widget_add_accelerator(item,
#if GTK_FACE < 2
      "activate_item",
#else
      "activate",
#endif
      gui_cfg.accel_group, accelerator_key, GDK_MOD1_MASK, GTK_ACCEL_LOCKED);
  }

  return item;
}

static void ResetCfg(GtkObject * object, gpointer func_data)
{
  cfg_set_all_to_default();

  cfg.xi_face = TRUE;

  if(gui_cfg.cfg_limits)
    xset_cfg_values_lim();
  if(gui_cfg.config_shell)
    xset_cfg_values_comm();

  if(cfg.use_prefs)
    cfg_dump_pref();
}

static void Save_rc(GtkObject * object, gpointer func_data)
{
  char pom[PATH_MAX];

  snprintf(pom, sizeof(pom), "%s/.pavukrc", cfg.path_to_home);
  cfg_dump(pom);
}

#ifdef GETTEXT_NLS
static void ChangeLang(GtkObject * object, gpointer func_data)
{
  if(!cfg.done)
    return;

  if(last_lang && func_data && !strcmp(last_lang, func_data))
    return;
  else if(!last_lang && !func_data)
    return;

  if(cfg.processing)
  {
    gdk_beep();
    xprintf(1,
      gettext("Unable to change language during processing time!\n"));
    return;
  }


  _free(cfg.language);
  cfg.language = tl_strdup((char *) func_data);

  _INIT_NLS _free(last_lang);
  last_lang = tl_strdup((char *) func_data);

  _restart_iface = TRUE;
  gui_action = GUI_ACT_CHANGELANG;
  gtk_main_quit();
}
#endif

/*** DROP SIGNAL HANDLER ***/
void gui_window_drop_url(GtkWidget * widget, GdkDragContext * context,
  gint x, gint y, GtkSelectionData * seldata, guint info, guint time,
  gpointer data)
{
  gchar *drag_url;
  gchar *p_seldata;
  url_info *ui;
  char *pp[2];

  if(!seldata || !seldata->data)
  {
    gtk_drag_finish(context, FALSE, FALSE, time);
    return;
  }

  p_seldata = (gchar *) seldata->data;

  /* strip away '\n' */
  drag_url = tl_strndup(p_seldata, strcspn(p_seldata, "\r\n"));

  ui = url_info_new(drag_url);
  if(gui_cfg.config_shell)
  {
    url_info *cui;
    int row;

    pp[0] = drag_url;
    pp[1] = NULL;

    row = gtk_clist_append(GTK_CLIST(gui_cfg.url_list), pp);
    cui = url_info_duplicate(ui);
    gtk_clist_set_row_data_full(GTK_CLIST(gui_cfg.url_list), row, cui,
      (GtkDestroyNotify) url_info_free);
  }

  cfg.request = dllist_append(cfg.request, (dllist_t) ui);

#ifdef DEBUG
  if(cfg.debug)
    xprintf(0, gettext("Dropped URL : %s\n"), drag_url);
#endif

  _free(drag_url);

  gtk_drag_finish(context, TRUE, FALSE, time);
}

static void DeMiniaturize(GtkObject * object, gpointer func_data)
{
  gint w, h;
  char pom[128];

  miniaturized = FALSE;
  resizing = TRUE;

  gtk_widget_hide(GTK_WIDGET(gui_cfg.mini_toolbar));
  gtk_widget_show(GTK_WIDGET(gui_cfg.main_window_hide));
  gtk_widget_show(GTK_WIDGET(gui_cfg.logw_swin));

  w = -1;
  h = -1;
  snprintf(pom, sizeof(pom), "(%s)-toplevel_n_width", cfg.language ? cfg.language : "C");
  gprop_get_int(pom, &w);
  snprintf(pom, sizeof(pom), "(%s)-toplevel_n_height", cfg.language ? cfg.language : "C");
  gprop_get_int(pom, &h);
  gtk_window_set_default_size(GTK_WINDOW(gui_cfg.toplevel), w, h);

#ifdef HAVE_MT
  if(gprop_get_int("logw_height", &h))
    gtk_paned_set_position(GTK_PANED(logw_pane), h);
#endif

  resizing = FALSE;
}

static void Miniaturize(GtkObject * object, gpointer func_data)
{
  gint w, h;
  char pom[128];

  miniaturized = TRUE;
  resizing = TRUE;

  gtk_widget_show(GTK_WIDGET(gui_cfg.mini_toolbar));
  gtk_widget_hide(GTK_WIDGET(gui_cfg.main_window_hide));
  gtk_widget_hide(GTK_WIDGET(gui_cfg.logw_swin));

  w = -1;
  h = -1;
  snprintf(pom, sizeof(pom), "(%s)-toplevel_m_width", cfg.language ? cfg.language : "C");
  gprop_get_int(pom, &w);
  snprintf(pom, sizeof(pom), "(%s)-toplevel_m_height", cfg.language ? cfg.language : "C");
  gprop_get_int(pom, &h);
  gtk_window_set_default_size(GTK_WINDOW(gui_cfg.toplevel), w, h);

#ifdef HAVE_MT
  if(gprop_get_int("logw_height", &h))
    gtk_paned_set_position(GTK_PANED(logw_pane), 0);
#endif
  resizing = FALSE;
}

void gui_PopdownWC(GtkObject * object, gpointer func_data)
{
  GtkWidget *w = (GtkWidget *) func_data;
  int rv = 0;

  if(w == gui_cfg.cfg_limits)
    rv = xget_cfg_values_lim();
  if(w == gui_cfg.config_shell)
    rv = xget_cfg_values_comm();

  if(!rv)
  {
    gtk_widget_hide(w);
    if(cfg.use_prefs)
      cfg_dump_pref();
  }
  else
    gdk_beep();
}

void gui_PopupW(GtkObject * object, gpointer func_data)
{
  int which = (int) func_data;

  switch (which)
  {
  case PAVUK_ABOUT:
    gui_build_about(TRUE);
    break;
  case PAVUK_CFGCOMM:
    gui_build_config_common(TRUE);
    break;
  case PAVUK_CFGSCH:
    gui_build_scheduler(TRUE);
    {
      time_t t = time(NULL);
      struct tm *ltime;

      LOCK_TIME;
      ltime = new_tm(localtime(&t));
      UNLOCK_TIME;

      ltime->tm_year += 1900;

      gtk_calendar_select_month(GTK_CALENDAR(gui_cfg.calendar), ltime->tm_mon,
        ltime->tm_year);
      gtk_calendar_select_day(GTK_CALENDAR(gui_cfg.calendar), ltime->tm_mday);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(gui_cfg.hour_label),
        (gfloat) ltime->tm_hour);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(gui_cfg.min_label),
        (gfloat) ltime->tm_min);
      gtk_entry_set_text(GTK_ENTRY(gui_cfg.sched_cmd), cfg.sched_cmd);

      free(ltime);
    }
    break;
  case PAVUK_CFGLIM:
    gui_build_config_limits(TRUE);
    break;
#ifdef WITH_TREE
  case PAVUK_TREE:
    gui_build_tree_preview(TRUE);
    break;
#endif
  case PAVUK_SCNLD:
    gui_build_scenario_loader(TRUE);
    break;
  case PAVUK_SCNADD:
    gui_build_scenario_adder(TRUE);
    break;
  case PAVUK_SCNSV:
    gui_build_scenario_saver(TRUE);
    break;
#ifdef HAVE_MOZJS
  case PAVUK_JSCONS:
    gui_pjs_console(TRUE);
    break;
#endif
  default:
    xprintf(1, gettext("Unknown window to popup"));
  }
}

static void ClearLog(GtkObject * object, gpointer func_data)
{
  LOCK_GTKLOG;
  gtk_clist_clear(GTK_CLIST(gui_cfg.logw));
  UNLOCK_GTKLOG;
}

static void ToggleBool(GtkObject * object, gpointer func_data)
{
  bool_t *v = (bool_t *) func_data;

  if(GTK_IS_CHECK_MENU_ITEM(object))
    *v = GTK_CHECK_MENU_ITEM(object)->active;

  if(GTK_IS_TOGGLE_BUTTON(object) || GTK_IS_CHECK_BUTTON(object))
    *v = GTK_TOGGLE_BUTTON(object)->active;
}

static void OpenURL(GtkObject * object, gpointer func_data)
{
  gtk_notebook_set_page(GTK_NOTEBOOK(gui_cfg.cb_comcfg), 0);
}

static void FetchURL(GtkWidget * widget, GtkSelectionData * selection_data,
  gpointer data)
{
  char *p, *cb_url;
  url_info *ui;

  if(selection_data->length <= 0)
  {
    gdk_beep();
    return;
  }

  p = tl_strndup((char *) selection_data->data, selection_data->length);
  cb_url = tl_strndup(p, strcspn(p, "\r\n"));
  _free(p);

  ui = url_info_new(cb_url);
  if(gui_cfg.config_shell)
  {
    url_info *cui;
    int row = gtk_clist_append(GTK_CLIST(gui_cfg.url_list), &cb_url);
    cui = url_info_duplicate(ui);
    gtk_clist_set_row_data_full(GTK_CLIST(gui_cfg.url_list), row, cui,
      (GtkDestroyNotify) url_info_free);
  }

  cfg.request = dllist_append(cfg.request, (dllist_t) ui);

#ifdef DEBUG
  if(cfg.debug)
    xprintf(0, gettext("Fetched URL from clipboard : %s\n"), cb_url);
#endif

  _free(cb_url);
}

static void FetchCBUrl(GtkObject * object, gpointer func_data)
{
  GdkAtom atom;

  atom = gdk_atom_intern("TEXT", FALSE);

  gtk_selection_convert(GTK_WIDGET(object), GDK_SELECTION_PRIMARY, atom,
    GDK_CURRENT_TIME);
}

#ifdef HAVE_XMU
static Atom XA_MOZILLA_URL = None;
static Atom XA_MOZILLA_NAME = None;

static void AddBrowserURL(GtkWidget * w)
{
  url_info *ui;
  char *p = gtk_object_get_data(GTK_OBJECT(w), "url");

  if(p)
  {
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


#ifdef DEBUG
    if(cfg.debug)
      xprintf(0, gettext("Fetched URL from browser : %s\n"), p);
#endif
  }
}

static char *get_string_atom_value(Display * dpy, Window win, Atom atom)
{
  int result;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *data = 0;

  result = XGetWindowProperty(dpy, win, atom,
    0, (65536 / sizeof(long)), False,
    XA_STRING, &actual_type, &actual_format, &nitems, &bytes_after, &data);

  if((result == Success) && data && data[0])
  {
    return g_strdup(data);
  }
  return NULL;
}

static void fill_browser_menu(GtkWidget * menu)
{
  Window root, parent, *chld;
  int num;
  int i;
  Display *dpy = GDK_DISPLAY();

  if(!XA_MOZILLA_URL)
    XA_MOZILLA_URL = XInternAtom(dpy, "_MOZILLA_URL", False);
  if(!XA_MOZILLA_NAME)
    XA_MOZILLA_NAME = XInternAtom(dpy, "WM_ICON_NAME", False);

  XGrabServer(dpy);
  XQueryTree(dpy, GDK_ROOT_WINDOW(), &root, &parent, &chld, &num);

  for(i = 0; i < num; i++)
  {
    Window cw = XmuClientWindow(dpy, chld[i]);
    GtkWidget *mi;
    char *u, *l;

    u = get_string_atom_value(dpy, cw, XA_MOZILLA_URL);
    if(u)
    {
      l = get_string_atom_value(dpy, cw, XA_MOZILLA_NAME);
      mi = gtk_menu_item_new_with_label(l + 10);
      g_free(l);
      gtk_menu_append(GTK_MENU(menu), mi);
      gtk_widget_show(mi);
      gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(AddBrowserURL), NULL);
      gtk_object_set_data_full(GTK_OBJECT(mi), "url", u, g_free);
    }
  }
  XUngrabServer(dpy);
  XSync(dpy, False);
  if(chld)
    XFree(chld);
}

static void BrowserMenuPopup(GtkWidget * w)
{
  while(GTK_MENU_SHELL(w)->children)
  {
    gtk_widget_destroy(GTK_WIDGET(GTK_MENU_SHELL(w)->children->data));
  }
  fill_browser_menu(w);
}
#endif

static void SwitchMode(GtkObject * object, gpointer func_data)
{
  cfg.mode = (int) func_data;

  if(cfg.done && cfg.mode_started)
  {
    if(cfg.mode != cfg.prev_mode)
    {
      gtk_widget_set_sensitive(gui_cfg.bt_start, FALSE);
      gtk_widget_set_sensitive(gui_cfg.mea_start, FALSE);
      gtk_widget_set_sensitive(gui_cfg.mtb_start, FALSE);
    }
    else
    {
      gtk_widget_set_sensitive(gui_cfg.bt_start, TRUE);
      gtk_widget_set_sensitive(gui_cfg.mea_start, TRUE);
      gtk_widget_set_sensitive(gui_cfg.mtb_start, TRUE);
    }
  }
}

static void StartRecurse(int restart)
{
  gui_start_download(TRUE);
  cfg.processing = TRUE;

#ifndef HAVE_MT
  if(restart || cfg.mode != cfg.prev_mode || !cfg.mode_started)
  {
    absi_restart();
  }
  else
  {
    absi_cont();
  }

  if(cfg.xi_face)
    gui_finish_download(TRUE);
#else
  {
    pthread_attr_t tattr;

    pthread_attr_init(&tattr);
    pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setstacksize(&tattr, MT_STACK_SIZE);

    if(restart || cfg.mode != cfg.prev_mode || !cfg.mode_started)
    {
      if(pthread_create(&cfg.mainthread, &tattr, (void *) absi_restart, NULL))
      {
        xperror("Create main thread");
        gui_finish_download(TRUE);
      }
    }
    else
    {
      if(pthread_create(&cfg.mainthread, &tattr, (void *) absi_cont, NULL))
      {
        xperror("Create main thread");
        gui_finish_download(TRUE);
      }
    }
  }
#endif

}

static void Start(GtkObject * object, gpointer func_data)
{
  if(cfg.processing)
  {
    gdk_beep();
    return;
  }

  gui_action = func_data ? GUI_ACT_RESTART : GUI_ACT_CONTINUE;

  gtk_main_quit();
}

static void Stop(GtkObject * object, gpointer func_data)
{
  cfg.stop = TRUE;
}

static void Break(GtkObject * object, gpointer func_data)
{
  cfg.rbreak = TRUE;
  cfg.stop = TRUE;
  errno = EINTR;

#ifdef HAVE_MT
  if(cfg.processing)
  {
    int i;

    for(i = 0; i < cfg.allthreadsnr; i++)
    {
      pthread_kill(cfg.allthreads[i], SIGINT);
    }
/*
    pthread_join(cfg.mainthread, NULL);
*/
  }
#else
  _Xt_EscLoop;
#endif
}

static void Quit(GtkObject * object, gpointer func_data)
{
  if(!gui_cfg._go_bg && !_restart_iface)
  {
#ifdef HAVE_MT
    if(cfg.processing)
    {
      int i;

      for(i = 0; i < cfg.allthreadsnr; i++)
      {
        pthread_kill(cfg.allthreads[i], SIGQUIT);
        pthread_join(cfg.allthreads[i], NULL);
      }

      pthread_kill(cfg.mainthread, SIGQUIT);
      pthread_join(cfg.mainthread, NULL);
    }
#endif

    if(cfg.use_prefs)
      cfg_dump_pref();

    gkey_save();

    gtk_exit(0);
  }
}

static void Gobg(GtkObject * object, gpointer func_data)
{
  gui_cfg._go_bg = TRUE;

  if(cfg.use_prefs)
    cfg_dump_pref();

  gkey_save();

  if(!cfg.processing)
  {
    gui_action = GUI_ACT_GOBG;
    gtk_main_quit();
  }
}

static const struct
{
  char *label;
  char *accel;
  char *prope;
  GtkToolbarStyle tbtype;
} tbar_setup_rec[] =
{
  {gettext_nop("Both"), "config/tb_both", "tb_both_e", GTK_TOOLBAR_BOTH},
  {gettext_nop("Icons only"), "config/tb_icons", "tb_icons_e",
    GTK_TOOLBAR_ICONS},
  {gettext_nop("Text only"), "config/tb_text", "tb_text_e", GTK_TOOLBAR_TEXT}
};

#ifdef HAVE_MT

static const struct
{
  char *label;
  char *accel;
  char *prope;
  char *propw;
  int width;
} statusbar_col_rec[] =
{
  {gettext_nop("Nr."), NULL, NULL, "status_col_nr_width", 20},
  {gettext_nop("URL"), "config/sb_url", "status_col_url_e",
    "status_col_url_width", 150},
  {gettext_nop("Status"), "config/sb_status", "status_col_status_e",
    "status_col_status_width", 150},
  {gettext_nop("Size"), "config/sb_size", "status_col_size_e",
    "status_col_size_width", 150},
  {gettext_nop("Transfer rate"), "config/sb_rate", "status_col_rate_e",
    "status_col_rate_width", 60},
  {gettext_nop("Elapsed time"), "config/sb_et", "status_col_et_e",
    "status_col_et_width", 80},
  {gettext_nop("Remaining time"), "config/sb_rt", "status_col_rt_e",
    "status_col_rt_width", 80}
};

static void statusbar_col_set(GtkObject * object, gpointer func_data)
{
  gtk_clist_set_column_visibility(GTK_CLIST(gui_cfg.status_list),
    (int) func_data, GTK_CHECK_MENU_ITEM(object)->active);

  gprop_set_bool_t(statusbar_col_rec[(int) func_data].prope,
    GTK_CHECK_MENU_ITEM(object)->active);
}
#endif

static void toolbar_set(GtkObject * object, gpointer func_data)
{
  int i;

  if(GTK_CHECK_MENU_ITEM(object)->active)
  {
    gtk_toolbar_set_style(GTK_TOOLBAR(gui_cfg.toolbar),
      tbar_setup_rec[(int) func_data].tbtype);

    if(cfg.done)
    {
      for(i = 0; i < (sizeof(tbar_setup_rec) / sizeof(tbar_setup_rec[0]));
        i++)
      {
        gprop_set_bool_t(tbar_setup_rec[i].prope, i == (int) func_data);
      }
    }

    gtk_widget_queue_resize(GTK_WIDGET(gui_cfg.toolbar)->parent);
  }
}

static void toolbar_onoff(GtkWidget * object, gpointer func_data)
{
  GtkWidget *w;

  gprop_set_bool_t("tb_onoff", GTK_CHECK_MENU_ITEM(object)->active);

  w = GTK_WIDGET(gui_cfg.toolbar)->parent;

  if(GTK_CHECK_MENU_ITEM(object)->active)
    gtk_widget_show(w);
  else
    gtk_widget_hide(w);

  gtk_widget_queue_resize(gui_cfg.toplevel);
}

#ifdef DEBUG
static void switch_debug_level(GtkWidget * w, gpointer data)
{
  int dl = (int) data;

  if(GTK_CHECK_MENU_ITEM(w)->active)
    cfg.debug_level |= dl;
  else
    cfg.debug_level &= ~dl;
}

static void toggle_debug(GtkWidget * w, gpointer data)
{
  cfg.debug = GTK_CHECK_MENU_ITEM(w)->active;

  gtk_widget_set_sensitive(gui_cfg.debug_level_mi, cfg.debug);
  gtk_widget_set_sensitive(gui_cfg.debug_level_m, cfg.debug);
}

static void set_debug_level(GtkWidget * w, gpointer data)
{
  int l = (int) data;
  cfg.debug_level = l;
  gui_set_debug_level_mi();
}
#endif

static void ToggleTooltips(GtkWidget * w)
{
  int have = GTK_CHECK_MENU_ITEM(w)->active;

  gprop_set_bool_t("tooltips_enabled", have);
  if(have)
    gtk_tooltips_enable(gui_cfg.help_tooltips);
  else
    gtk_tooltips_disable(gui_cfg.help_tooltips);
}

static void build_menu(GtkWidget * parent)
{
  GtkWidget *mbar, *menu, *mbb, *mi, *hbox, *lbox;
  GSList *rg;
  GtkWidget *smenu, *button;
  int i;

  lbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(lbox);

  button = guitl_pixmap_button(minimize_small_xpm, cfg.bt_icon_mtb, NULL);
  gtk_box_pack_end(GTK_BOX(lbox), button, FALSE, TRUE, 1);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(Miniaturize), (gpointer) parent);
  gtk_widget_show(button);

  mbar = gtk_menu_bar_new();
  gtk_box_pack_start(GTK_BOX(lbox), mbar, TRUE, TRUE, 1);
  gtk_widget_show(mbar);

  gui_cfg.accel_group = gtk_accel_group_new();

  hbox = gtk_handle_box_new();
  gtk_handle_box_set_shadow_type(GTK_HANDLE_BOX(hbox), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, TRUE, 0);
  gtk_box_reorder_child(GTK_BOX(parent), hbox, 0);
  gtk_widget_show(hbox);
  gtk_container_add(GTK_CONTAINER(hbox), lbox);


/*** FILE MENU ***/

  menu = gtk_menu_new();
  gtk_widget_realize(menu);

  mi = gtk_tearoff_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  mi = menu_item(gettext("Open _URL ..."));
  gaccel_bind_widget("file/open_url", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(gui_PopupW), (gpointer) PAVUK_CFGCOMM);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(OpenURL), (gpointer) NULL);

  mi = menu_item(gettext("Append URL ..."));
  gaccel_bind_widget("file/add_url", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(gui_build_addurl), (gpointer) NULL);

  mi = menu_item(gettext("Fetch URL from Clipboard"));
  gaccel_bind_widget("file/fetch_url", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(FetchCBUrl), (gpointer) NULL);
  gtk_signal_connect(GTK_OBJECT(mi), "selection_received",
    GTK_SIGNAL_FUNC(FetchURL), (gpointer) NULL);
#ifdef HAVE_XMU
  smenu = gtk_menu_new();
  gtk_widget_realize(smenu);
  gtk_signal_connect(GTK_OBJECT(smenu), "show",
    GTK_SIGNAL_FUNC(BrowserMenuPopup), (gpointer) NULL);

  mi = menu_item(gettext("Fetch URL from browser"));
  gaccel_bind_widget("file/browser_url", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), smenu);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
#endif
  mi = gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  mi = menu_item(gettext("Load scenario ..."));
  gaccel_bind_widget("file/load_scn", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(gui_PopupW), (gpointer) PAVUK_SCNLD);

  mi = menu_item(gettext("Add scenario ..."));
  gaccel_bind_widget("file/add_scn", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(gui_PopupW), (gpointer) PAVUK_SCNADD);

  mi = menu_item(gettext("Save scenario ..."));
  gaccel_bind_widget("file/save_scn", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(gui_PopupW), (gpointer) PAVUK_SCNSV);

  mi = menu_item(gettext("Save settings to ~/.pavukrc"));
  gaccel_bind_widget("file/save_pavukrc", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(Save_rc), (gpointer) NULL);

  mi = gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  mi = menu_item(gettext("Schedule ..."));
  gaccel_bind_widget("file/schedule", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(gui_PopupW), (gpointer) PAVUK_CFGSCH);

  mi = menu_item(gettext("Auth. info editor ..."));
  gaccel_bind_widget("file/auth_edit", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(gauthinfo_run), (gpointer) NULL);

  mi = gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  mi = menu_item(gettext("E_xit"));
  gaccel_bind_widget("file/exit", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(Quit), (gpointer) NULL);

  mbb = menu_item(gettext("_File"));
  gtk_widget_show(mbb);
  gtk_menu_bar_append(GTK_MENU_BAR(mbar), mbb);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(mbb), menu);

/*** VIEW MENU ***/
  menu = gtk_menu_new();
  gtk_widget_realize(menu);

  mi = gtk_tearoff_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

#ifdef WITH_TREE
  mi = menu_item(gettext("Document _Tree ..."));
  gaccel_bind_widget("view/doc_tree", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(gui_PopupW), (gpointer) PAVUK_TREE);
#endif

  mi = menu_item(gettext("Status page ..."));
  gaccel_bind_widget("view/status_page", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(stats_show), (gpointer) NULL);

  mi = menu_item(gettext("HTML forms editor ..."));
  gaccel_bind_widget("view/form_editor", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(form_edit_dlg), (gpointer) NULL);

#ifdef HAVE_MOZJS
  mi = menu_item(gettext("Javascript console ..."));
  gaccel_bind_widget("view/js_console", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(gui_PopupW), (gpointer) PAVUK_JSCONS);
#endif

  mi = gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  mi = menu_item(gettext("Clear log window"));
  gaccel_bind_widget("view/clear_log", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(ClearLog), (gpointer) NULL);

  mbb = menu_item(gettext("_View"));
  gtk_widget_show(mbb);
  gtk_menu_bar_append(GTK_MENU_BAR(mbar), mbb);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(mbb), menu);

/*** MODE MENU ***/

  menu = gtk_menu_new();
  gtk_widget_realize(menu);

  mi = gtk_tearoff_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  rg = NULL;

  mi = gtk_radio_menu_item_new_with_label(rg, gettext("normal recurse"));
  gaccel_bind_widget("mode/normal_recurse", "activate", mi, NULL,
    gui_cfg.toplevel);
  rg = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(mi));
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(SwitchMode), (gpointer) MODE_NORMAL);
  gui_cfg.modegr[MODE_NORMAL] = mi;

  mi = gtk_radio_menu_item_new_with_label(rg, gettext("synchronize"));
  gaccel_bind_widget("mode/synchronize", "activate", mi, NULL,
    gui_cfg.toplevel);
  rg = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(mi));
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(SwitchMode), (gpointer) MODE_SYNC);
  gui_cfg.modegr[MODE_SYNC] = mi;

  mi = gtk_radio_menu_item_new_with_label(rg, gettext("mirror"));
  gaccel_bind_widget("mode/mirror", "activate", mi, NULL, gui_cfg.toplevel);
  rg = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(mi));
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(SwitchMode), (gpointer) MODE_MIRROR);
  gui_cfg.modegr[MODE_MIRROR] = mi;

  mi = gtk_radio_menu_item_new_with_label(rg, gettext("single page"));
  gaccel_bind_widget("mode/single_page", "activate", mi, NULL,
    gui_cfg.toplevel);
  rg = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(mi));
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(SwitchMode), (gpointer) MODE_SINGLE);
  gui_cfg.modegr[MODE_SINGLE] = mi;

  mi = gtk_radio_menu_item_new_with_label(rg, gettext("update local links"));
  gaccel_bind_widget("mode/update_local", "activate", mi, NULL,
    gui_cfg.toplevel);
  rg = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(mi));
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(SwitchMode), (gpointer) MODE_LNUPD);
  gui_cfg.modegr[MODE_LNUPD] = mi;

  mi = gtk_radio_menu_item_new_with_label(rg, gettext("resume files"));
  gaccel_bind_widget("mode/resume_files", "activate", mi, NULL,
    gui_cfg.toplevel);
  rg = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(mi));
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(SwitchMode), (gpointer) MODE_RESUME);
  gui_cfg.modegr[MODE_RESUME] = mi;

  mi = gtk_radio_menu_item_new_with_label(rg, gettext("unlimited reget"));
  gaccel_bind_widget("mode/unlimited_reget", "activate", mi, NULL,
    gui_cfg.toplevel);
  rg = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(mi));
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(SwitchMode), (gpointer) MODE_SREGET);
  gui_cfg.modegr[MODE_SREGET] = mi;

  mi =
    gtk_radio_menu_item_new_with_label(rg,
    gettext("transfer but don't store"));
  gaccel_bind_widget("mode/nostore", "activate", mi, NULL, gui_cfg.toplevel);
  rg = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(mi));
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(SwitchMode), (gpointer) MODE_NOSTORE);
  gui_cfg.modegr[MODE_NOSTORE] = mi;

  mi = gtk_radio_menu_item_new_with_label(rg, gettext("reminder"));
  gaccel_bind_widget("mode/reminder", "activate", mi, NULL, gui_cfg.toplevel);
  rg = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(mi));
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(SwitchMode), (gpointer) MODE_REMIND);
  gui_cfg.modegr[MODE_REMIND] = mi;

  mi = gtk_radio_menu_item_new_with_label(rg, gettext("list ftp directory"));
  gaccel_bind_widget("mode/ftpdir", "activate", mi, NULL, gui_cfg.toplevel);
  rg = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(mi));
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(SwitchMode), (gpointer) MODE_FTPDIR);
  gui_cfg.modegr[MODE_FTPDIR] = mi;

  mbb = menu_item(gettext("_Mode"));
  gtk_widget_show(mbb);
  gtk_menu_bar_append(GTK_MENU_BAR(mbar), mbb);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(mbb), menu);

  gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(gui_cfg.modegr[cfg.mode]),
    TRUE);

/*** CONFIG MENU ***/

  gui_cfg.cfg_menu = menu = gtk_menu_new();
  gtk_widget_realize(menu);

  mi = gtk_tearoff_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  mi = menu_item(gettext("C_ommon ..."));
  gaccel_bind_widget("config/common_cfg", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(gui_PopupW), (gpointer) PAVUK_CFGCOMM);

  mi = menu_item(gettext("_Limitations ..."));
  gaccel_bind_widget("config/limit_cfg", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(gui_PopupW), (gpointer) PAVUK_CFGLIM);

  mi = menu_item(gettext("Reset configuration"));
  gaccel_bind_widget("config/reset_cfg", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(ResetCfg), (gpointer) NULL);

/*** toolbar menu ***/
  mi = gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  mi = menu_item(gettext("Toolbar"));
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  smenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), smenu);
  gtk_widget_show(menu);

  mi = gtk_check_menu_item_new_with_label(gettext("Toggle toolbar"));
  gaccel_bind_widget("config/tb_toggle", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(smenu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    (GtkSignalFunc) toolbar_onoff, (gpointer) NULL);
  if(gprop_get_bool_t("tb_onoff", &i))
  {
    gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(mi), i);
    toolbar_onoff(mi, NULL);
  }
  else
  {
    gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(mi), TRUE);
    toolbar_onoff(mi, NULL);
  }

  mi = gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(smenu), mi);
  gtk_widget_show(mi);

  rg = NULL;

#define TBSET_ME(label, tb_style)\
  mi = gtk_radio_menu_item_new_with_label(rg, gettext(label));\
  rg = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (mi));\
  gtk_menu_append(GTK_MENU(smenu), mi);\
  gtk_widget_show (mi);\
  gtk_signal_connect(GTK_OBJECT(mi), "activate",\
      (GtkSignalFunc) toolbar_set,\
      (gpointer)tb_style);

  for(i = 0; i < (sizeof(tbar_setup_rec) / sizeof(tbar_setup_rec[0])); i++)
  {
    int rv;

    TBSET_ME(tbar_setup_rec[i].label, i);
    gaccel_bind_widget(tbar_setup_rec[i].accel, "activate", mi, NULL,
      gui_cfg.toplevel);
    if(gprop_get_bool_t(tbar_setup_rec[i].prope, &rv))
      gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(mi), rv);
  }
/*** toolbar menu end ***/

#ifdef HAVE_MT
/*** status bar menu start ***/
#define SBSET_ME(label, cn)\
  mi = gtk_check_menu_item_new_with_label(gettext(label));\
  gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(mi), TRUE);\
  gtk_menu_append(GTK_MENU(smenu), mi);\
  gtk_widget_show (mi);\
  gtk_signal_connect(GTK_OBJECT(mi), "activate",\
      (GtkSignalFunc) statusbar_col_set,\
      (gpointer)cn);

  mi = menu_item(gettext("Progressbar"));
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  smenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), smenu);
  gtk_widget_show(menu);

  for(i = 1; i < (sizeof(statusbar_col_rec) / sizeof(statusbar_col_rec[0]));
    i++)
  {
    int rv;

    SBSET_ME(statusbar_col_rec[i].label, i);
    gaccel_bind_widget(statusbar_col_rec[i].accel, "activate", mi, NULL,
      gui_cfg.toplevel);
    if(gprop_get_bool_t(statusbar_col_rec[i].prope, &rv))
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi), rv);
  }

/*** status bar menu end ***/
#endif /* HAVE_MT */

#ifdef GETTEXT_NLS
  {
    static char **al = NULL;

    if(al || (al = get_available_languages()))
    {
      mi = gtk_menu_item_new();
      gtk_menu_append(GTK_MENU(menu), mi);
      gtk_widget_show(mi);

      mi = gtk_menu_item_new_with_label(gettext("Language"));
      gtk_menu_append(GTK_MENU(menu), mi);
      gtk_widget_show(mi);

      smenu = gtk_menu_new();
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), smenu);

      rg = NULL;
      mi = gtk_radio_menu_item_new_with_label(rg, gettext_nop("English"));
      gaccel_bind_widget("lang/en", "activate", mi, NULL, gui_cfg.toplevel);
      rg = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(mi));
      gtk_menu_append(GTK_MENU(smenu), mi);
      gtk_widget_show(mi);
      if(!last_lang)
        gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(mi), TRUE);

      gtk_signal_connect(GTK_OBJECT(mi), "activate",
        GTK_SIGNAL_FUNC(ChangeLang), (gpointer) "C");

      for(i = 0; al[i]; i++)
      {
        gchar idstr[256];
        const char *name;

        name = nls_langcat_name(al[i]);

        mi = gtk_radio_menu_item_new_with_label(rg, name ? name : al[i]);
        snprintf(idstr, sizeof(idstr), "lang/%s", al[i]);
        gaccel_bind_widget(idstr, "activate", mi, NULL, gui_cfg.toplevel);
        rg = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(mi));
        gtk_menu_append(GTK_MENU(smenu), mi);
        gtk_widget_show(mi);
        if(last_lang && !strcmp(al[i], last_lang))
          gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(mi), TRUE);

        gtk_signal_connect(GTK_OBJECT(mi), "activate",
          GTK_SIGNAL_FUNC(ChangeLang), (gpointer) al[i]);
      }
    }
  }
#endif

  mi = gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

#ifdef DEBUG
  gui_cfg.debug_level_mi = mi = menu_item(gettext("Debug level"));
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_widget_set_sensitive(mi, cfg.debug);

  gui_cfg.debug_level_m = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), gui_cfg.debug_level_m);

  for(i = 0; cfg_debug_levels[i].id; i++)
  {
    mi =
      gtk_check_menu_item_new_with_label(gettext(cfg_debug_levels[i].label));
    gtk_menu_append(GTK_MENU(gui_cfg.debug_level_m), mi);
    gtk_widget_show(mi);
    gtk_signal_connect(GTK_OBJECT(mi), "activate",
      GTK_SIGNAL_FUNC(switch_debug_level), (gpointer) cfg_debug_levels[i].id);
  }
  gui_set_debug_level_mi();

  mi = gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(gui_cfg.debug_level_m), mi);
  gtk_widget_show(mi);

  mi = gtk_menu_item_new_with_label(gettext("All"));
  gtk_menu_append(GTK_MENU(gui_cfg.debug_level_m), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(set_debug_level), (gpointer) 0xffffffff);

  mi = gtk_menu_item_new_with_label(gettext("None"));
  gtk_menu_append(GTK_MENU(gui_cfg.debug_level_m), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(set_debug_level), (gpointer) 0);

  gui_cfg.me_debug = mi =
    gtk_check_menu_item_new_with_label(gettext("Debug"));
  gaccel_bind_widget("config/debug", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(toggle_debug), (gpointer) & cfg.debug);

  gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(mi), cfg.debug);

  mi = gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
#endif

  mi = gtk_check_menu_item_new_with_label(gettext("Allow tooltips"));
  gaccel_bind_widget("config/tooltips", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(mi),
    !gprop_get_bool_t("tooltips_enabled", &i) || i);

  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(ToggleTooltips), NULL);

  mi = gtk_check_menu_item_new_with_label(gettext("Log window autoscroll"));
  gaccel_bind_widget("config/log_sroll", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(mi), cfg.log_autoscroll);
  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(ToggleBool), (gpointer) & cfg.log_autoscroll);

  mi = gtk_check_menu_item_new_with_label(gettext("Use preferences"));
  gaccel_bind_widget("config/use_pref", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(mi), cfg.use_prefs);
  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(ToggleBool), (gpointer) & cfg.use_prefs);

  gui_cfg.me_quiet = mi =
    gtk_check_menu_item_new_with_label(gettext("Quiet"));
  gaccel_bind_widget("config/quiet", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(mi), cfg.quiet);

  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(ToggleBool), (gpointer) & cfg.quiet);

#ifdef HAVE_MT
  gui_cfg.immessages = mi =
    gtk_check_menu_item_new_with_label(gettext("Immediate messages"));
  gaccel_bind_widget("config/immediate_msg", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(mi), cfg.immessages);

  gtk_signal_connect(GTK_OBJECT(mi), "toggled",
    GTK_SIGNAL_FUNC(ToggleBool), (gpointer) & cfg.immessages);
#endif

  mbb = menu_item(gettext("_Config"));
  gtk_widget_show(mbb);
  gtk_menu_bar_append(GTK_MENU_BAR(mbar), mbb);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(mbb), menu);

/*** ACTION MENU ***/

  menu = gtk_menu_new();
  gtk_widget_realize(menu);

  mi = gtk_tearoff_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  mi = menu_item(gettext("_Restart"));
  gaccel_bind_widget("action/restart", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gui_cfg.mea_rest = mi;

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(Start), (gpointer) TRUE);

  mi = menu_item(gettext("Co_ntinue"));
  gaccel_bind_widget("action/continue", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gui_cfg.mea_start = mi;

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(Start), (gpointer) FALSE);

  mi = menu_item(gettext("Sto_p"));
  gaccel_bind_widget("action/stop", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gui_cfg.mea_stop = mi;

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(Stop), (gpointer) NULL);

  mi = menu_item(gettext("_Break"));
  gaccel_bind_widget("action/break", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);
  gui_cfg.mea_break = mi;

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(Break), (gpointer) NULL);

  mbb = menu_item(gettext("_Action"));
  gtk_widget_show(mbb);
  gtk_menu_bar_append(GTK_MENU_BAR(mbar), mbb);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(mbb), menu);

  gtk_widget_set_sensitive(gui_cfg.mea_start, FALSE);
  gtk_widget_set_sensitive(gui_cfg.mea_stop, FALSE);
  gtk_widget_set_sensitive(gui_cfg.mea_break, FALSE);

/*** HELP MENU ***/

  menu = gtk_menu_new();
  gtk_widget_realize(menu);

  mi = gtk_tearoff_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  mi = menu_item(gettext("About ..."));
  gaccel_bind_widget("help/about", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(menu), mi);
  gtk_widget_show(mi);

  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(gui_PopupW), (gpointer) PAVUK_ABOUT);

  mbb = menu_item(gettext("_Help"));
  gtk_menu_item_right_justify(GTK_MENU_ITEM(mbb));
  gtk_widget_show(mbb);
  gtk_menu_bar_append(GTK_MENU_BAR(mbar), mbb);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(mbb), menu);
}

static void build_toolbar(GtkWidget * parent)
{
  GtkWidget *row, *hbox;

#if GTK_FACE < 2
  gui_cfg.toolbar = row = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL,
    GTK_TOOLBAR_BOTH);

  gtk_toolbar_set_button_relief(GTK_TOOLBAR(row), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_size(GTK_TOOLBAR(row), 10);
#else
  gui_cfg.toolbar = row = gtk_toolbar_new();
  gtk_toolbar_set_orientation(GTK_TOOLBAR(gui_cfg.toolbar),
  GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style(GTK_TOOLBAR(gui_cfg.toolbar), GTK_TOOLBAR_BOTH);

  /* FIXME - There is no description, how to manage this in GTK 2.x */
  /* gtk_toolbar_set_button_relief(GTK_TOOLBAR(row), GTK_RELIEF_NONE); */
  /* gtk_toolbar_set_space_size(GTK_TOOLBAR(row), 10); */
#endif

  hbox = gtk_handle_box_new();
  gtk_handle_box_set_shadow_type(GTK_HANDLE_BOX(hbox), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);
  gtk_widget_show(hbox);
  gtk_container_add(GTK_CONTAINER(hbox), row);

  gtk_widget_show(row);

  gui_cfg.bt_cfg = guitl_toolbar_button(row, gettext("Config"),
    gettext("Popup config window"), configure_xpm,
    GTK_SIGNAL_FUNC(gui_PopupW), (gpointer) PAVUK_CFGCOMM, cfg.bt_icon_cfg);

  gui_cfg.bt_lim = guitl_toolbar_button(row, gettext("Limits"),
    gettext("Popup limits window"), limits_xpm,
    GTK_SIGNAL_FUNC(gui_PopupW), (gpointer) PAVUK_CFGLIM, cfg.bt_icon_lim);

  gtk_toolbar_append_space(GTK_TOOLBAR(row));

  gui_cfg.bt_bg = guitl_toolbar_button(row, gettext("Go bg"),
    gettext("Destroy window as soon as posible and continue on terminal"),
    gobg_xpm, GTK_SIGNAL_FUNC(Gobg), NULL, cfg.bt_icon_gobg);

  gui_cfg.bt_rest = guitl_toolbar_button(row, gettext("Restart"),
    gettext("Start working on currently set starting URLs"),
    restart_xpm, GTK_SIGNAL_FUNC(Start), (gpointer) TRUE, cfg.bt_icon_rest);

  gui_cfg.bt_start = guitl_toolbar_button(row, gettext("Continue"),
    gettext("Continue after stop or break"), continue_xpm,
    GTK_SIGNAL_FUNC(Start), (gpointer) FALSE, cfg.bt_icon_cont);

  gtk_toolbar_append_space(GTK_TOOLBAR(row));

  gui_cfg.bt_stop = guitl_toolbar_button(row, gettext("Stop"),
    gettext("Finish this transfer and stop"), stop_xpm,
    GTK_SIGNAL_FUNC(Stop), (gpointer) NULL, cfg.bt_icon_stop);

  gui_cfg.bt_break = guitl_toolbar_button(row, gettext("Break"),
    gettext("Break transfer and stop"), break_xpm,
    GTK_SIGNAL_FUNC(Break), (gpointer) NULL, cfg.bt_icon_brk);

  gtk_toolbar_append_space(GTK_TOOLBAR(row));

  gui_cfg.bt_exit = guitl_toolbar_button(row, gettext("Exit"),
    gettext("Immediately quit the program"), exit_xpm,
    GTK_SIGNAL_FUNC(Quit), (gpointer) NULL, cfg.bt_icon_exit);

  gtk_widget_set_sensitive(gui_cfg.bt_start, FALSE);
  gtk_widget_set_sensitive(gui_cfg.bt_stop, FALSE);
  gtk_widget_set_sensitive(gui_cfg.bt_break, FALSE);

}

static void build_minitoolbar(GtkWidget * col)
{
  GtkWidget *tb;
#ifndef HAVE_MT
  GtkWidget *frame;
#endif

  gui_cfg.mini_toolbar = gtk_hbox_new(0, 5);
  gtk_box_pack_start(GTK_BOX(col), gui_cfg.mini_toolbar, FALSE, TRUE, 1);

#ifndef HAVE_MT
  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(gui_cfg.mini_toolbar), frame, TRUE, TRUE, 4);

  gui_cfg.minitb_label = gtk_entry_new();
  gtk_widget_show(gui_cfg.minitb_label);
  gtk_widget_set_usize(gui_cfg.minitb_label, 400, -1);
  gtk_entry_set_editable(GTK_ENTRY(gui_cfg.minitb_label), FALSE);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.minitb_label);
#endif

#if GTK_FACE < 2
  tb = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief(GTK_TOOLBAR(tb), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_size(GTK_TOOLBAR(tb), 2);
#else
  tb = gtk_toolbar_new();
  gtk_toolbar_set_orientation(GTK_TOOLBAR(gui_cfg.toolbar),
  GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style(GTK_TOOLBAR(gui_cfg.toolbar), GTK_TOOLBAR_ICONS);

  /* FIXME - There is no description, how to manage this in GTK 2.x */
  /* gtk_toolbar_set_button_relief(GTK_TOOLBAR(tb), GTK_RELIEF_NONE); */
  /* gtk_toolbar_set_space_size(GTK_TOOLBAR(tb), 2); */
#endif

#ifdef HAVE_MT
  gtk_box_pack_start(GTK_BOX(gui_cfg.mini_toolbar), tb, FALSE, FALSE, 1);
#else
  gtk_box_pack_end(GTK_BOX(gui_cfg.mini_toolbar), tb, FALSE, FALSE, 1);
#endif
  gtk_widget_show(tb);

  gui_cfg.mtb_rest = guitl_toolbar_button(tb, NULL,
    gettext("Restart"), restart_small_xpm,
    GTK_SIGNAL_FUNC(Start), (gpointer) TRUE, cfg.bt_icon_rest_s);

  gui_cfg.mtb_start = guitl_toolbar_button(tb, NULL,
    gettext("Continue"), continue_small_xpm,
    GTK_SIGNAL_FUNC(Start), (gpointer) FALSE, cfg.bt_icon_cont_s);

  gui_cfg.mtb_stop = guitl_toolbar_button(tb, NULL,
    gettext("Stop"), stop_small_xpm,
    GTK_SIGNAL_FUNC(Stop), (gpointer) NULL, cfg.bt_icon_stop_s);

  gui_cfg.mtb_break = guitl_toolbar_button(tb, NULL,
    gettext("Break"), break_small_xpm,
    GTK_SIGNAL_FUNC(Break), (gpointer) NULL, cfg.bt_icon_brk_s);

  guitl_toolbar_button(tb, NULL,
    gettext("Show whole main window"), maximize_small_xpm,
    GTK_SIGNAL_FUNC(DeMiniaturize), (gpointer) col, cfg.bt_icon_mtb_s);

  guitl_toolbar_button(tb, NULL,
    gettext("Quit"), close_small_xpm,
    GTK_SIGNAL_FUNC(Quit), (gpointer) NULL, cfg.bt_icon_exit_s);

  gtk_widget_set_sensitive(gui_cfg.mtb_start, FALSE);
  gtk_widget_set_sensitive(gui_cfg.mtb_stop, FALSE);
  gtk_widget_set_sensitive(gui_cfg.mtb_break, FALSE);
}

static guint gui_logw_list_events(GtkWidget * widget, GdkEvent * event)
{
  GdkEventButton *bevent;

  switch (event->type)
  {
  case GDK_BUTTON_PRESS:
    bevent = (GdkEventButton *) event;
    if(bevent->button == 3)
    {
      gtk_widget_set_sensitive(gui_cfg.logw_copy_me,
        (GTK_CLIST(gui_cfg.logw)->selection != NULL));
      gtk_menu_popup(GTK_MENU(gui_cfg.logw_menu),
        NULL, NULL, NULL, NULL, 3, bevent->time);
    }
    break;
  default:
    break;
  }
  return FALSE;
}

static void logw_save_content_do(GtkWidget * w, GtkWidget * fs)
{
  char *t = NULL;
  const char *p;
  int row, fd;
  int err = FALSE;

  for(row = 0; row < GTK_CLIST(gui_cfg.logw)->rows; row++)
  {
    char *s;
    gtk_clist_get_text(GTK_CLIST(gui_cfg.logw), row, 0, &s);
    t = tl_str_concat(t, s, "\n", NULL);
  }

  p = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));

  if((fd = open(p, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
  {
    xperror(p);
    gdk_beep();
    err = TRUE;
  }
  else
  {
    row = strlen(t);
    if(write(fd, t, row) != row)
    {
      xperror(p);
      gdk_beep();
      err = TRUE;
    }
    close(fd);
  }

  _free(t);

  if(!err)
    gtk_widget_destroy(fs);
}

static void logw_save_content(void)
{
  static GtkWidget *fs = NULL;

  if(!fs)
  {
    fs = gtk_file_selection_new(gettext("Pavuk: save log"));
    gtk_signal_connect(GTK_OBJECT(fs), "destroy",
      GTK_SIGNAL_FUNC(gtk_widget_destroyed), &fs);
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button),
      "clicked", GTK_SIGNAL_FUNC(logw_save_content_do), fs);
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button),
      "clicked", GTK_SIGNAL_FUNC(guitl_PopdownW), fs);

  }
  gtk_widget_show(fs);
  if(GTK_WIDGET_REALIZED(fs))
    gdk_window_raise(fs->window);
}

static void logw_select_all(void)
{
  gtk_clist_select_all(GTK_CLIST(gui_cfg.logw));
}

static void logw_clear_selection(void)
{
  gtk_clist_unselect_all(GTK_CLIST(gui_cfg.logw));
}

static void build_logw(GtkWidget * pane)
{
  GtkWidget *swin, *par;
  GtkWidget *mi;

  par = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(pane), par);
  gtk_widget_show(par);

  gui_cfg.logw = gtk_clist_new(1);
  gtk_clist_set_column_title(GTK_CLIST(gui_cfg.logw), 0, gettext("Log"));
  gtk_clist_column_titles_show(GTK_CLIST(gui_cfg.logw));
  gtk_clist_set_selection_mode(GTK_CLIST(gui_cfg.logw),
    GTK_SELECTION_EXTENDED);
  gtk_widget_set_usize(gui_cfg.logw, -1, 150);
  gtk_clist_set_column_auto_resize(GTK_CLIST(gui_cfg.logw), 0, TRUE);

  gui_cfg.logw_swin = swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
  gtk_box_pack_start(GTK_BOX(par), swin, TRUE, TRUE, 0);
  gtk_widget_show(swin);
  gtk_container_add(GTK_CONTAINER(swin), gui_cfg.logw);
  gui_cfg.logvadj =
    gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(swin));
  gtk_widget_show(gui_cfg.logw);

  gtk_signal_connect(GTK_OBJECT(gui_cfg.logw), "event",
    GTK_SIGNAL_FUNC(gui_logw_list_events), NULL);

  gui_cfg.logw_menu = gtk_menu_new();
  gtk_widget_realize(gui_cfg.logw_menu);

  mi = menu_item(gettext("Select all"));
  gaccel_bind_widget("logw/select_all", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(gui_cfg.logw_menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(logw_select_all), gui_cfg.logw);

  mi = menu_item(gettext("Clear selection"));
  gaccel_bind_widget("logw/clear_selection", "activate", mi, NULL,
    gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(gui_cfg.logw_menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(logw_clear_selection), gui_cfg.logw);

  gui_cfg.logw_copy_me = mi = menu_item(gettext("Copy selection"));
  gaccel_bind_widget("logw/copy", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(gui_cfg.logw_menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(guitl_clist_selection_to_clipboard), gui_cfg.logw);

  mi = gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(gui_cfg.logw_menu), mi);
  gtk_widget_show(mi);

  mi = menu_item(gettext("Save log ..."));
  gaccel_bind_widget("logw/save", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(gui_cfg.logw_menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(logw_save_content), NULL);

  mi = menu_item(gettext("Clear log"));
  gaccel_bind_widget("logw/clear", "activate", mi, NULL, gui_cfg.toplevel);
  gtk_menu_append(GTK_MENU(gui_cfg.logw_menu), mi);
  gtk_widget_show(mi);
  gtk_signal_connect(GTK_OBJECT(mi), "activate",
    GTK_SIGNAL_FUNC(ClearLog), NULL);
}


#ifdef HAVE_MT
static void status_list_resize_column(GtkWidget * w, int col, int width,
  gpointer fdata)
{
  gprop_set_int(statusbar_col_rec[col].propw, width);
}

static void build_statusbar(GtkWidget * pane)
{
  GtkWidget *lbox, *frame, *hbox, *par;
  int i;
  GtkWidget *swin;

  par = gtk_vbox_new(FALSE, 2);
  gtk_container_add(GTK_CONTAINER(pane), par);
  gtk_widget_show(par);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_usize(swin, -1, 100);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
  gtk_box_pack_start(GTK_BOX(par), swin, TRUE, TRUE, 1);
  gtk_widget_show(swin);

  /* thrnr, URL, status, size, rate, etime, rtime */
  gui_cfg.status_list = gtk_clist_new(7);
  gtk_widget_set_usize(gui_cfg.status_list, 400, -1);
  gtk_signal_connect(GTK_OBJECT(gui_cfg.status_list), "resize_column",
    GTK_SIGNAL_FUNC(status_list_resize_column), NULL);
  for(i = 0; i < (sizeof(statusbar_col_rec) / sizeof(statusbar_col_rec[0]));
    i++)
  {
    int w;

    gtk_clist_set_column_title(GTK_CLIST(gui_cfg.status_list), i,
      gettext(statusbar_col_rec[i].label));
    if(!gprop_get_int(statusbar_col_rec[i].propw, &w))
      w = statusbar_col_rec[i].width;
    gtk_clist_set_column_width(GTK_CLIST(gui_cfg.status_list), i, w);
  }

  gtk_clist_column_titles_show(GTK_CLIST(gui_cfg.status_list));
  gtk_container_add(GTK_CONTAINER(swin), gui_cfg.status_list);
  gtk_widget_show(gui_cfg.status_list);

  lbox = gtk_handle_box_new();
  gtk_handle_box_set_shadow_type(GTK_HANDLE_BOX(lbox), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(par), lbox, FALSE, TRUE, 0);
  gtk_widget_show(lbox);

  hbox = gtk_vbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(lbox), hbox);
  gtk_widget_show(hbox);

  /*** first status line ***/
  lbox = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(hbox), lbox, FALSE, TRUE, 1);
  gtk_widget_show(lbox);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(lbox), frame);

  gui_cfg.status_done = gtk_label_new(gettext("Processed:       "));
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.status_done), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.status_done);
  gtk_widget_show(gui_cfg.status_done);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(lbox), frame);

  gui_cfg.status_fail = gtk_label_new(gettext("Failed:       "));
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.status_fail), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.status_fail);
  gtk_widget_show(gui_cfg.status_fail);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(lbox), frame);

  gui_cfg.status_queue = gtk_label_new(gettext("Queued:       "));
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.status_queue), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.status_queue);
  gtk_widget_show(gui_cfg.status_queue);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(lbox), frame);

  gui_cfg.status_rej = gtk_label_new(gettext("Rejected:       "));
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.status_rej), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.status_rej);
  gtk_widget_show(gui_cfg.status_rej);


  /**** second status line ****/
  lbox = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(hbox), lbox, FALSE, TRUE, 1);
  gtk_widget_show(lbox);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(lbox), frame);

  gui_cfg.status_msg = gtk_label_new(" ");
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.status_msg), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.status_msg);
  gtk_widget_show(gui_cfg.status_msg);
}

#else /* HAVE_MT */
static void build_statusbar(GtkWidget * par)
{
  GtkWidget *lbox, *frame, *hbox;

  lbox = gtk_handle_box_new();
  gtk_handle_box_set_shadow_type(GTK_HANDLE_BOX(lbox), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(par), lbox, FALSE, TRUE, 0);
  gtk_widget_show(lbox);

  hbox = gtk_vbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(lbox), hbox);
  gtk_widget_show(hbox);

  /**** first status line ****/
  lbox = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(hbox), lbox, FALSE, TRUE, 1);
  gtk_widget_show(lbox);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(lbox), frame);

  gui_cfg.status_size = gtk_label_new(gettext("S:                     "));
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.status_size), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.status_size);
  gtk_widget_show(gui_cfg.status_size);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(lbox), frame);

  gui_cfg.status_rate = gtk_label_new(gettext("R:          "));
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.status_rate), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.status_rate);
  gtk_widget_show(gui_cfg.status_rate);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(lbox), frame);

  gui_cfg.status_et = gtk_label_new(gettext("ET:          "));
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.status_et), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.status_et);
  gtk_widget_show(gui_cfg.status_et);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(lbox), frame);

  gui_cfg.status_rt = gtk_label_new(gettext("RT:          "));
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.status_rt), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.status_rt);
  gtk_widget_show(gui_cfg.status_rt);

  /*** second status line ***/
  lbox = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(hbox), lbox, FALSE, TRUE, 1);
  gtk_widget_show(lbox);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(lbox), frame);

  gui_cfg.status_done = gtk_label_new(gettext("Processed:       "));
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.status_done), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.status_done);
  gtk_widget_show(gui_cfg.status_done);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(lbox), frame);

  gui_cfg.status_fail = gtk_label_new(gettext("Failed:       "));
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.status_fail), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.status_fail);
  gtk_widget_show(gui_cfg.status_fail);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(lbox), frame);

  gui_cfg.status_queue = gtk_label_new(gettext("Queued:       "));
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.status_queue), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.status_queue);
  gtk_widget_show(gui_cfg.status_queue);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(lbox), frame);

  gui_cfg.status_rej = gtk_label_new(gettext("Rejected:       "));
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.status_rej), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.status_rej);
  gtk_widget_show(gui_cfg.status_rej);


  /**** third status line ****/
  lbox = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(hbox), lbox, FALSE, TRUE, 1);
  gtk_widget_show(lbox);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(lbox), frame);

  gui_cfg.status_msg = gtk_label_new(" ");
  gtk_misc_set_alignment(GTK_MISC(gui_cfg.status_msg), 0.0, 0.5);
  gtk_container_add(GTK_CONTAINER(frame), gui_cfg.status_msg);
  gtk_widget_show(gui_cfg.status_msg);
}
#endif /* !HAVE_MT */

static void toplevel_resize(GtkWidget * w, GtkAllocation * sallocation,
  gpointer fdata)
{
  char pom[64];

  if(resizing)
    return;

  if(miniaturized)
  {
    snprintf(pom, sizeof(pom), "(%s)-toplevel_m_width", cfg.language ? cfg.language : "C");
    gprop_set_int(pom, sallocation->width);
    snprintf(pom, sizeof(pom), "(%s)-toplevel_m_height", cfg.language ? cfg.language : "C");
    gprop_set_int(pom, sallocation->height);
  }
  else
  {
    snprintf(pom, sizeof(pom), "(%s)-toplevel_n_width", cfg.language ? cfg.language : "C");
    gprop_set_int(pom, sallocation->width);
    snprintf(pom, sizeof(pom), "(%s)-toplevel_n_height", cfg.language ? cfg.language : "C");
    gprop_set_int(pom, sallocation->height);
  }
}

#ifdef HAVE_MT
static void pane_resize(GtkWidget * w, GtkAllocation * sallocation,
  gpointer fdata)
{
  if(resizing || sallocation->height < 2)
    return;

  gprop_set_int("logw_height", sallocation->height);
}
#endif /* HAVE_MT */

#define BUILD_SH(sh, build_fnc) \
  if (sh) \
  {\
    gdk_window_get_position(GTK_WIDGET(sh)->window, &x, &y);\
    is_up = GTK_WIDGET_VISIBLE (sh);\
    gtk_widget_destroy(sh);\
    if (is_up)\
    {\
      build_fnc(TRUE);\
      gtk_widget_set_uposition(sh, x, y);\
    }\
    else sh = NULL;\
    _Xt_Serve;\
  }

static void build_main(void)
{
  gint is_up;
  bool_t consens = FALSE;
  bool_t stop_looping;
  gint x, y, w, h;
  GtkWidget *col, *sep;
  char pom[64];

  if(gui_cfg.toplevel)
  {
    consens = GTK_WIDGET_SENSITIVE(gui_cfg.bt_start);
    gdk_window_get_position(GTK_WIDGET(gui_cfg.toplevel)->window, &x, &y);
    is_up = TRUE;
    gtk_widget_destroy(gui_cfg.toplevel);
  }
  else
    is_up = FALSE;

  gui_cfg.toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  w = -1;
  h = -1;
  snprintf(pom, sizeof(pom), "(%s)-toplevel_n_width", cfg.language ? cfg.language : "C");
  gprop_get_int(pom, &w);
  snprintf(pom, sizeof(pom), "(%s)-toplevel_n_height", cfg.language ? cfg.language : "C");
  gprop_get_int(pom, &h);
  gtk_window_set_default_size(GTK_WINDOW(gui_cfg.toplevel), w, h);
  gtk_window_set_policy(GTK_WINDOW(gui_cfg.toplevel), TRUE, TRUE, TRUE);

  /*** DRAG'N'DROP ***/
  gtk_drag_dest_set(gui_cfg.toplevel, GTK_DEST_DEFAULT_ALL,
    dragtypes, NUM_ELEM(dragtypes), GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gtk_signal_connect(GTK_OBJECT(gui_cfg.toplevel),
    "drag_data_received", GTK_SIGNAL_FUNC(gui_window_drop_url), NULL);

  gtk_signal_connect(GTK_OBJECT(gui_cfg.toplevel), "destroy",
    GTK_SIGNAL_FUNC(Quit), NULL);
  gtk_signal_connect(GTK_OBJECT(gui_cfg.toplevel), "size_allocate",
    GTK_SIGNAL_FUNC(toplevel_resize), NULL);

  if(is_up)
    gtk_widget_set_uposition(gui_cfg.toplevel, x, y);

  gtk_widget_realize(gui_cfg.toplevel);
  gtk_container_border_width(GTK_CONTAINER(gui_cfg.toplevel), 0);
  gtk_window_set_title(GTK_WINDOW(gui_cfg.toplevel), "Pavuk");

  col = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(gui_cfg.toplevel), col);
  gtk_widget_show(col);

  gui_cfg.main_window_hide = gtk_vbox_new(FALSE, 0);
#ifdef HAVE_MT
  gtk_box_pack_start(GTK_BOX(col), gui_cfg.main_window_hide, FALSE, FALSE, 0);
#else
  gtk_box_pack_start(GTK_BOX(col), gui_cfg.main_window_hide, TRUE, TRUE, 0);
#endif
  gtk_widget_show(gui_cfg.main_window_hide);

  BUILD_SH(gui_cfg.about_shell, gui_build_about);
  BUILD_SH(gui_cfg.config_shell, gui_build_config_common);
  BUILD_SH(gui_cfg.cfg_limits, gui_build_config_limits);
  BUILD_SH(gui_cfg.scn_load_shell, gui_build_scenario_loader);
  BUILD_SH(gui_cfg.scn_add_shell, gui_build_scenario_adder);
  BUILD_SH(gui_cfg.scn_save_shell, gui_build_scenario_saver);
  BUILD_SH(gui_cfg.cfg_sch, gui_build_scheduler);
#ifdef HAVE_MOZJS
  BUILD_SH(gui_cfg.pjs_console_shell, gui_pjs_console);
#endif

#ifdef WITH_TREE
  gui_build_tree_preview(gui_cfg.tree_shell ?
    GTK_WIDGET_VISIBLE(gui_cfg.tree_shell) : FALSE);
#endif

  sep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(gui_cfg.main_window_hide), sep, FALSE, TRUE, 1);
  gtk_widget_show(sep);

  build_toolbar(gui_cfg.main_window_hide);

#ifdef HAVE_MT
  build_minitoolbar(col);

  logw_pane = gtk_vpaned_new();
  gtk_box_pack_start(GTK_BOX(col), logw_pane, TRUE, TRUE, 1);
  gtk_widget_show(logw_pane);

  build_logw(logw_pane);
  build_statusbar(logw_pane);

  if(gprop_get_int("logw_height", &h))
    gtk_paned_set_position(GTK_PANED(logw_pane), h);

  gtk_signal_connect(GTK_OBJECT(GTK_PANED(logw_pane)->child1),
    "size_allocate", GTK_SIGNAL_FUNC(pane_resize), NULL);
#else
  build_logw(gui_cfg.main_window_hide);
  build_minitoolbar(col);
  build_statusbar(col);
#endif

  build_menu(gui_cfg.main_window_hide);

  gtk_widget_set_sensitive(gui_cfg.bt_start, consens);
  gtk_widget_set_sensitive(gui_cfg.mea_start, consens);

  gaccel_window_activate(gui_cfg.accel_group, gui_cfg.toplevel);

  gtk_signal_connect(GTK_OBJECT(gui_cfg.toplevel), "destroy",
    GTK_SIGNAL_FUNC(gtk_widget_destroyed), &gui_cfg.toplevel);

  gtk_widget_show_now(gui_cfg.toplevel);

  icons_load();

#ifdef __CYGWIN__
  SendMessage(GDK_WINDOW_XWINDOW(gui_cfg.toplevel->window),
    WM_SETICON, ICON_SMALL,
    (struct _HICON *) LoadImage(GetModuleHandle(NULL),
      MAKEINTRESOURCE(PAVUK_ICON), IMAGE_ICON,
      GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
  SendMessage(GDK_WINDOW_XWINDOW(gui_cfg.toplevel->window),
    WM_SETICON, ICON_BIG,
    (struct _HICON *) LoadImage(GetModuleHandle(NULL),
      MAKEINTRESOURCE(PAVUK_ICON), IMAGE_ICON,
      GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
#endif

  cfg.done = TRUE;

  _restart_iface = FALSE;

  if(gui_cfg.config_shell)
    xset_cfg_values_comm();
  if(gui_cfg.cfg_limits)
    xset_cfg_values_lim();

  if(cfg.run_iface)
  {
    gui_action = GUI_ACT_RESTART;
    cfg.run_iface = FALSE;
  }
  else
    gui_action = GUI_ACT_NONE;

  stop_looping = FALSE;

  while(!stop_looping)
  {
    switch (gui_action)
    {
    case GUI_ACT_RESTART:
      StartRecurse(TRUE);
      break;
    case GUI_ACT_CONTINUE:
      StartRecurse(FALSE);
      break;
    case GUI_ACT_CHANGELANG:
    case GUI_ACT_GOBG:
      stop_looping = TRUE;
      break;
    default:
      break;
    }

    if(gui_cfg._go_bg)
      stop_looping = TRUE;

    if(!stop_looping)
      gtk_main();
  }

  cfg.done = FALSE;
}

void gui_main(void)
{
  GdkFont *font;
  int tooltips;

  cfg.done = FALSE;
  cfg.log_autoscroll = TRUE;

  if(cfg.fontname && (font = gdk_font_load(cfg.fontname)))
  {
#if GTK_FACE < 2
    gtk_widget_get_default_style()->font = font;
#else
    gtk_style_set_font(gtk_widget_get_default_style(), font);
#endif
  }

#ifdef GETTEXT_NLS
  last_lang = tl_strdup(cfg.language);

  if(!last_lang)
  {
    last_lang = tl_strdup(getenv("LC_MESSAGES"));
  }

  if(!last_lang)
  {
    last_lang = tl_strdup(getenv("LANG"));
  }
#endif

  gui_cfg.help_tooltips = gtk_tooltips_new();
  if(!gprop_get_bool_t("tooltips_enabled", &tooltips) || tooltips)
    gtk_tooltips_enable(gui_cfg.help_tooltips);
  else
    gtk_tooltips_disable(gui_cfg.help_tooltips);

  while(_restart_iface && !gui_cfg._go_bg)
  {
    build_main();
  }

  if(cfg.rbreak)
    return;

  gui_do_ui_cleanup();

  if(!cfg.processing)
  {
    if(cfg.mode != cfg.prev_mode || !cfg.mode_started)
      absi_restart();
    else
      absi_cont();
  }
#ifdef HAVE_MT
  else
  {
    pthread_join(cfg.mainthread, NULL);
  }
#endif

  exit(cfg.fail_cnt ? PAVUK_EXIT_DOC_ERR : PAVUK_EXIT_OK);
}

#endif
