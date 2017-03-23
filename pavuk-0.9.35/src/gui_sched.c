/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "gui.h"

#ifdef GTK_FACE
#include <gdk/gdkkeysyms.h>

#include "schedule.h"

#include "icons/cancel.xpm"
#include "icons/schedule.xpm"

static void Schedule(GtkObject * object, gpointer func_data)
{
  const char *p;

  cfg.time->tm_hour =
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gui_cfg.hour_label));
  cfg.time->tm_min =
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gui_cfg.min_label));

  gtk_calendar_get_date(GTK_CALENDAR(gui_cfg.calendar),
    &cfg.time->tm_year, &cfg.time->tm_mon, &cfg.time->tm_mday);
  cfg.time->tm_year -= 1900;

  _free(cfg.sched_cmd);
  p = gtk_entry_get_text(GTK_ENTRY(gui_cfg.sched_cmd));
  if(p && *p)
    cfg.sched_cmd = tl_strdup(p);

  cfg.reschedh =
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gui_cfg.resched));

  if(!at_schedule())
  {
    gtk_widget_hide(gui_cfg.cfg_sch);
    return;
  }
  xprintf(1, gettext("Error scheduling\n"));
  gdk_beep();

}

void gui_build_scheduler(int popup)
{
  GtkWidget *col, *brow, *button, *sep, *label, *ptab;
  GtkAccelGroup *accel_group;

  if(gui_cfg.cfg_sch)
  {
    if(popup)
    {
      gtk_widget_show_all(gui_cfg.cfg_sch);
      if(GTK_WIDGET_REALIZED(gui_cfg.cfg_sch))
        gdk_window_raise(gui_cfg.cfg_sch->window);
    }
    return;
  }

  gui_cfg.cfg_sch = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_border_width(GTK_CONTAINER(gui_cfg.cfg_sch), 3);
  gtk_window_set_title(GTK_WINDOW(gui_cfg.cfg_sch),
    gettext("Pavuk: Scheduler"));
  gtk_widget_realize(gui_cfg.cfg_sch);
  gtk_signal_connect(GTK_OBJECT(gui_cfg.cfg_sch), "destroy",
    GTK_SIGNAL_FUNC(gtk_widget_destroyed), &gui_cfg.cfg_sch);

  col =
    guitl_timesel_new(&gui_cfg.calendar, &gui_cfg.hour_label,
    &gui_cfg.min_label, &gui_cfg.month_combo, &gui_cfg.year_label);
  gtk_container_add(GTK_CONTAINER(gui_cfg.cfg_sch), col);
  gtk_widget_show(col);

  sep = gtk_hseparator_new();
  gtk_widget_show(sep);
  gtk_box_pack_start(GTK_BOX(col), sep, FALSE, TRUE, 3);

  ptab = gtk_table_new(2, 1, FALSE);
  gtk_box_pack_start(GTK_BOX(col), ptab, FALSE, TRUE, 1);
  gtk_widget_show(ptab);

  gui_cfg.sched_cmd = guitl_tab_add_entry(ptab,
    gettext("Scheduling command: "), 0, 0, FALSE);

  sep = gtk_hseparator_new();
  gtk_widget_show(sep);
  gtk_box_pack_start(GTK_BOX(col), sep, FALSE, TRUE, 3);

  brow = gtk_hbox_new(0, 5);
  gtk_widget_show(brow);
  gtk_box_pack_start(GTK_BOX(col), brow, FALSE, TRUE, 1);

  label = gtk_label_new(gettext("Reschedule after "));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(brow), label, TRUE, TRUE, 4);

  gui_cfg.resched = gtk_spin_button_new(
    (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 10000.0, 1.0, 5.0, 0.0),
    10.0, 0.0);
  gtk_widget_show(gui_cfg.resched);
  gtk_box_pack_start(GTK_BOX(brow), gui_cfg.resched, TRUE, TRUE, 4);

  label = gtk_label_new(gettext(" hours"));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(brow), label, TRUE, TRUE, 4);

  brow = gtk_hbutton_box_new();
  gtk_box_pack_end(GTK_BOX(col), brow, FALSE, TRUE, 4);
  gtk_hbutton_box_set_spacing_default(1);
  gtk_widget_show(brow);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(brow), GTK_BUTTONBOX_SPREAD);

  sep = gtk_hseparator_new();
  gtk_widget_show(sep);
  gtk_box_pack_end(GTK_BOX(col), sep, FALSE, TRUE, 4);

  button = guitl_pixmap_button(schedule_xpm, NULL, gettext("OK"));
  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(Schedule), (gpointer) NULL);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  button = guitl_pixmap_button(cancel_xpm, NULL, gettext("Cancel"));

  accel_group = gtk_accel_group_new();
  gtk_widget_add_accelerator(button, "clicked", accel_group,
    GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_window_add_accel_group(GTK_WINDOW(gui_cfg.cfg_sch), accel_group);

  gtk_container_add(GTK_CONTAINER(brow), button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_PopdownW), (gpointer) gui_cfg.cfg_sch);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_show(button);

  if(popup)
    gtk_widget_show(gui_cfg.cfg_sch);
}

#endif
