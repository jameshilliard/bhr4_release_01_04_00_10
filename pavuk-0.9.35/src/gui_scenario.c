/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "gui.h"

#include <string.h>

#ifdef GTK_FACE

#include "ainterface.h"
#include "uconfig.h"
#include "net.h"

static void SaveScn(GtkObject * object, gpointer func_data)
{
  const char *fn =
    gtk_file_selection_get_filename(GTK_FILE_SELECTION(gui_cfg.
      scn_save_shell));

  if(fn && *fn)
  {
    cfg_dump(fn);
    gtk_widget_hide(gui_cfg.scn_save_shell);
  }
  else
    gdk_beep();
}

void gui_build_scenario_saver(int popup)
{
  if(gui_cfg.scn_save_shell)
  {
    if(popup)
    {
      gtk_widget_show_all(gui_cfg.scn_save_shell);
      if(GTK_WIDGET_REALIZED(gui_cfg.scn_save_shell))
        gdk_window_raise(gui_cfg.scn_save_shell->window);
    }
    return;
  }

  gui_cfg.scn_save_shell =
    gtk_file_selection_new(gettext("Pavuk: Scenario saver"));

  gtk_signal_connect(GTK_OBJECT(gui_cfg.scn_save_shell), "destroy",
    GTK_SIGNAL_FUNC(gtk_widget_destroyed), &gui_cfg.scn_save_shell);

  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(gui_cfg.scn_save_shell)->
      ok_button), "clicked", GTK_SIGNAL_FUNC(SaveScn),
    gui_cfg.scn_save_shell);

  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(gui_cfg.scn_save_shell)->
      cancel_button), "clicked", GTK_SIGNAL_FUNC(guitl_PopdownW),
    gui_cfg.scn_save_shell);

  if(popup)
    gtk_widget_show(gui_cfg.scn_save_shell);
}

static void LoadScn(GtkObject * object, gpointer func_data)
{
  const char *fn =
    gtk_file_selection_get_filename(GTK_FILE_SELECTION(gui_cfg.
      scn_load_shell));

  if(fn && *fn)
  {
    int rv;

    LOCK_GCFG;
    cfg_set_all_to_default();
    cfg.xi_face = TRUE;
    rv = cfg_load(fn);
    _MT_CFGSTAMP;
    UNLOCK_GCFG;

    memset(&cfg.local_ip_addr, '\0', sizeof(cfg.local_ip_addr));
    if(cfg.local_ip &&
      (net_host_to_in_addr(cfg.local_ip, &cfg.local_ip_addr)))
    {
      xherror(cfg.local_ip);
    }

    if(rv)
      gdk_beep();
    else
    {
      gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(gui_cfg.modegr[cfg.
            mode]), TRUE);

      if(!cfg.processing)
      {
        gtk_widget_set_sensitive(gui_cfg.bt_start, FALSE);
        gtk_widget_set_sensitive(gui_cfg.mea_start, FALSE);
        gtk_widget_set_sensitive(gui_cfg.mtb_start, FALSE);

        free_all();
        cfg.total_cnt = 0;
        cfg.urlstack = NULL;
        cfg.mode_started = FALSE;
      }


      if(gui_cfg.cfg_limits)
        xset_cfg_values_lim();
      if(gui_cfg.config_shell)
        xset_cfg_values_comm();
#ifdef DEBUG
      gui_set_debug_level_mi();
      gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(gui_cfg.me_debug),
        cfg.debug);
#endif
#ifdef HAVE_MT
      gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(gui_cfg.immessages),
        cfg.immessages);
#endif
      gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(gui_cfg.me_quiet),
        cfg.quiet);

      gtk_widget_hide(gui_cfg.scn_load_shell);
    }
  }
  else
    gdk_beep();
}

void gui_build_scenario_loader(int popup)
{
  if(gui_cfg.scn_load_shell)
  {
    if(popup)
    {
      gtk_widget_show_all(gui_cfg.scn_load_shell);
      if(GTK_WIDGET_REALIZED(gui_cfg.scn_load_shell))
        gdk_window_raise(gui_cfg.scn_load_shell->window);
    }
    return;
  }

  gui_cfg.scn_load_shell =
    gtk_file_selection_new(gettext("Pavuk: Scenario loader"));

  gtk_signal_connect(GTK_OBJECT(gui_cfg.scn_load_shell), "destroy",
    GTK_SIGNAL_FUNC(gtk_widget_destroyed), &gui_cfg.scn_load_shell);

  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(gui_cfg.scn_load_shell)->
      ok_button), "clicked", GTK_SIGNAL_FUNC(LoadScn),
    gui_cfg.scn_load_shell);

  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(gui_cfg.scn_load_shell)->
      cancel_button), "clicked", GTK_SIGNAL_FUNC(guitl_PopdownW),
    gui_cfg.scn_load_shell);

  if(popup)
    gtk_widget_show(gui_cfg.scn_load_shell);
}

static void AddScn(GtkObject * object, gpointer func_data)
{
  const char *fn =
    gtk_file_selection_get_filename(GTK_FILE_SELECTION(gui_cfg.
      scn_add_shell));

  if(fn && *fn)
  {
    int rv;

    LOCK_GCFG;
    rv = cfg_load(fn);
    _MT_CFGSTAMP;
    UNLOCK_GCFG;

    memset(&cfg.local_ip_addr, '\0', sizeof(cfg.local_ip_addr));
    if(cfg.local_ip &&
      (net_host_to_in_addr(cfg.local_ip, &cfg.local_ip_addr)))
    {
      xherror(cfg.local_ip);
    }

    if(rv)
      gdk_beep();
    else
    {
      gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(gui_cfg.modegr[cfg.
            mode]), TRUE);

      if(!cfg.processing)
      {
        gtk_widget_set_sensitive(gui_cfg.bt_start, FALSE);
        gtk_widget_set_sensitive(gui_cfg.mea_start, FALSE);
        gtk_widget_set_sensitive(gui_cfg.mtb_start, FALSE);

        free_all();
        cfg.total_cnt = 0;
        cfg.urlstack = NULL;
        cfg.mode_started = FALSE;
      }


      if(gui_cfg.cfg_limits)
        xset_cfg_values_lim();
      if(gui_cfg.config_shell)
        xset_cfg_values_comm();
#ifdef DEBUG
      gui_set_debug_level_mi();
      gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(gui_cfg.me_debug),
        cfg.debug);
#endif
#ifdef HAVE_MT
      gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(gui_cfg.immessages),
        cfg.immessages);
#endif
      gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(gui_cfg.me_quiet),
        cfg.quiet);

      gtk_widget_hide(gui_cfg.scn_add_shell);
    }
  }
  else
    gdk_beep();
}

void gui_build_scenario_adder(int popup)
{
  if(gui_cfg.scn_add_shell)
  {
    if(popup)
    {
      gtk_widget_show_all(gui_cfg.scn_add_shell);
      if(GTK_WIDGET_REALIZED(gui_cfg.scn_add_shell))
        gdk_window_raise(gui_cfg.scn_add_shell->window);
    }
    return;
  }

  gui_cfg.scn_add_shell =
    gtk_file_selection_new(gettext("Pavuk: Scenario add"));

  gtk_signal_connect(GTK_OBJECT(gui_cfg.scn_add_shell), "destroy",
    GTK_SIGNAL_FUNC(gtk_widget_destroyed), &gui_cfg.scn_add_shell);

  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(gui_cfg.scn_add_shell)->
      ok_button), "clicked", GTK_SIGNAL_FUNC(AddScn), gui_cfg.scn_add_shell);

  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(gui_cfg.scn_add_shell)->
      cancel_button), "clicked", GTK_SIGNAL_FUNC(guitl_PopdownW),
    gui_cfg.scn_add_shell);

  if(popup)
    gtk_widget_show(gui_cfg.scn_add_shell);
}

#endif /* GTK_FACE */
