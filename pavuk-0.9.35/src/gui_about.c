/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "gui.h"

#ifdef GTK_FACE
#include <stdio.h>
#include <gdk/gdkkeysyms.h>

#include "icons/cancel.xpm"
#include "pavuk_logo.xpm"

void gui_build_about(int popup)
{
  GtkWidget *col, *button, *frame, *label, *pixmap, *row;
  GtkAccelGroup *accel_group;
  static Icon *icon = NULL;
  char pom[1024];

  if(gui_cfg.about_shell)
  {
    if(popup)
    {
      gtk_widget_show_all(gui_cfg.about_shell);
      if(GTK_WIDGET_REALIZED(gui_cfg.about_shell))
        gdk_window_raise(gui_cfg.about_shell->window);
    }
    return;
  }


  gui_cfg.about_shell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(gui_cfg.about_shell),
    gettext("Pavuk: About"));
  gtk_container_border_width(GTK_CONTAINER(gui_cfg.about_shell), 3);
  gtk_signal_connect(GTK_OBJECT(gui_cfg.about_shell), "destroy",
    GTK_SIGNAL_FUNC(gtk_widget_destroyed), &gui_cfg.about_shell);

  col = gtk_vbox_new(0, 5);
  gtk_container_add(GTK_CONTAINER(gui_cfg.about_shell), col);
  gtk_widget_show(col);

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_container_add(GTK_CONTAINER(col), frame);

  row = gtk_hbox_new(0, 10);
  gtk_widget_show(row);
  gtk_container_add(GTK_CONTAINER(frame), row);

  if(!icon)
    icon = guitl_load_pixmap(pavuk_logo_xpm);
  pixmap = gtk_pixmap_new(icon->pixmap, icon->shape);
  gtk_container_add(GTK_CONTAINER(row), pixmap);
  gtk_widget_show(pixmap);

  snprintf(pom, sizeof(pom), gettext("Pavuk %s %s\n\n"
      "an automatic WEB file grabber\n\n"
      "By Stefan Ondrejicka\n\n"
      "URL: http://pavuk.sourceforge.net/\n"), VERSION, HOSTTYPE);

  label = gtk_label_new(pom);
  gtk_misc_set_padding(GTK_MISC(label), 15, 15);
  gtk_container_add(GTK_CONTAINER(row), label);
  gtk_widget_show(label);

  button = guitl_pixmap_button(cancel_xpm, NULL, gettext("Cancel"));
  gtk_container_add(GTK_CONTAINER(col), button);

  accel_group = gtk_accel_group_new();
  gtk_widget_add_accelerator(button, "clicked", accel_group,
    GDK_Escape, 0, GTK_ACCEL_VISIBLE);
  gtk_window_add_accel_group(GTK_WINDOW(gui_cfg.about_shell), accel_group);

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(guitl_PopdownW), (gpointer) gui_cfg.about_shell);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);


  if(popup)
    gtk_widget_show(gui_cfg.about_shell);
}

#endif
