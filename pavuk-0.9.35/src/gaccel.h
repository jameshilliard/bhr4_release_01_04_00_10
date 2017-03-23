/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _gaccel_h_
#define _gaccel_h_

#include <gtk/gtk.h>
#include <stdio.h>

typedef struct
{
  GtkWidget *widget;
  gchar *sigid;
  gchar *name;
  GdkModifierType modifier_mask;
  guint accelerator_key;
  GtkAccelGroup *accel_group;
} gaccel;

extern void gaccel_window_activate(GtkAccelGroup *, GtkWidget *);
extern gaccel *gaccel_parse_str(guchar *);
extern guchar *gaccel_to_str(gaccel *);
extern void gaccel_bind_widget(gchar *, gchar *, GtkWidget *, GtkAccelGroup *,
  GtkWidget *);
extern gaccel *gaccel_find_by_name(gchar *);
extern void gaccel_add(gaccel *);
extern void gaccel_save_keys(FILE * f);

extern GSList *gaccel_list;

#endif
