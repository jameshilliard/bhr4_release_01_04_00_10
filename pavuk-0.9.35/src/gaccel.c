/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "gui.h"

#ifdef GTK_FACE
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include "gaccel.h"
#include "tools.h"

GSList *gaccel_list = NULL;

static gaccel *gaccel_new(gchar * name, gchar * sigid)
{
  gaccel *retv = g_malloc(sizeof(gaccel));

  retv->name = g_strdup(name);
  retv->sigid = g_strdup(sigid);
  retv->widget = NULL;
  retv->modifier_mask = 0;
  retv->accelerator_key = 0;
  retv->accel_group = NULL;

  gaccel_list = g_slist_append(gaccel_list, retv);

  return retv;
}

gaccel *gaccel_parse_str(guchar * str)
{
  gaccel *retv = NULL;
  char *p1, *p2, *p3, *p;
  GdkModifierType modifier_mask = 0;
  guint accelerator_key = 0;

  if(!str)
    return NULL;

  p = g_strdup((char *) str);

  p1 = g_strdup(get_1qstr(p));
  p2 = g_strdup(get_1qstr(NULL));
  p3 = g_strdup(get_1qstr(NULL));

  if(p3)
    gtk_accelerator_parse(p3, &accelerator_key, &modifier_mask);

  if(p1 && p2 && p3 && accelerator_key != 0)
  {
    retv = g_malloc(sizeof(gaccel));

    retv->widget = NULL;
    retv->name = p1;
    retv->sigid = p2;
    retv->modifier_mask = modifier_mask;
    retv->accelerator_key = accelerator_key;
    retv->accel_group = NULL;
    g_free(p3);
  }
  else
  {
    g_free(p1);
    g_free(p2);
    g_free(p3);
  }

  g_free(p);
  return retv;
}

guchar *gaccel_to_str(gaccel * gaccelp)
{
  static gchar pom[256];
  gchar *p;

  if(!gaccelp->accelerator_key && !gaccelp->modifier_mask)
    return NULL;

  p = gtk_accelerator_name(gaccelp->accelerator_key, gaccelp->modifier_mask);
  snprintf(pom, sizeof(pom), "\"%s\" \"%s\" \"%s\"", gaccelp->name, gaccelp->sigid, p);

  p = pom;
  return (guchar *) p;
}

static void gaccel_widget_destroy(GtkWidget * widget, gaccel * gaccelp)
{
  gaccelp->widget = NULL;
}

#if GTK_FACE < 2
/* FIXME - This does not work for GTK2 */
static void gaccel_widget_add_accel(GtkWidget * widget, guint sigid,
  GtkAccelGroup * agroup, guint akey, GdkModifierType amod,
  GtkAccelFlags aflag, gaccel * gaccelp)
{
  if((aflag & GTK_ACCEL_VISIBLE) &&
    (sigid == gtk_signal_lookup(gaccelp->sigid, GTK_OBJECT_TYPE(widget))))
  {
    gaccelp->accelerator_key = akey;
    gaccelp->modifier_mask = amod;
  }
}

static void gaccel_widget_remove_accel(GtkWidget * widget,
  GtkAccelGroup * agroup, guint akey, GdkModifierType amod, gaccel * gaccelp)
{
  if((gaccelp->accelerator_key == akey) && (gaccelp->modifier_mask == amod))
  {
    gaccelp->accelerator_key = 0;
    gaccelp->modifier_mask = 0;
  }
}
#endif

void gaccel_bind_widget(gchar * name, gchar * sigid, GtkWidget * widget,
  GtkAccelGroup * accel_group, GtkWidget * parent)
{
  static GtkAccelGroup *daccel_group = NULL;
  gaccel *gaccelp;

  if(!widget)
    return;

  /*  if (!daccel_group) daccel_group = gtk_accel_group_get_default(); */
  if(!daccel_group)
    daccel_group = gtk_accel_group_new();

  if(!accel_group && parent)
  {
    accel_group =
      (GtkAccelGroup *) gtk_object_get_data(GTK_OBJECT(parent),
      "gaccel_accel_group");

    if(!accel_group)
    {
      accel_group = gtk_accel_group_new();
      gtk_window_add_accel_group(GTK_WINDOW(gtk_widget_get_toplevel(parent)),
        accel_group);
      gtk_object_set_data_full(GTK_OBJECT(parent), "gaccel_accel_group",
        accel_group, (GtkDestroyNotify) gtk_accel_group_unref);
    }
  }

  if(!accel_group)
    accel_group = daccel_group;

  gaccelp = gaccel_find_by_name(name);

  if(!gaccelp)
  {
    gaccelp = gaccel_new(name, sigid);
  }

  gaccelp->widget = widget;
  gaccelp->accel_group = accel_group;

  if(gaccelp->accelerator_key || gaccelp->modifier_mask)
    gtk_widget_add_accelerator(widget, gaccelp->sigid, accel_group,
      gaccelp->accelerator_key, gaccelp->modifier_mask, GTK_ACCEL_VISIBLE);

  gtk_signal_connect(GTK_OBJECT(widget), "destroy",
    GTK_SIGNAL_FUNC(gaccel_widget_destroy), gaccelp);
#if GTK_FACE < 2
  gtk_signal_connect(GTK_OBJECT(widget), "add_accelerator",
    GTK_SIGNAL_FUNC(gaccel_widget_add_accel), gaccelp);
  gtk_signal_connect(GTK_OBJECT(widget), "remove_accelerator",
    GTK_SIGNAL_FUNC(gaccel_widget_remove_accel), gaccelp);
#endif
}

gaccel *gaccel_find_by_name(gchar * name)
{
  GSList *lst = gaccel_list;

  while(lst)
  {
    if(!strcmp(((gaccel *) lst->data)->name, name))
      return (gaccel *) lst->data;
    lst = lst->next;
  }

  return NULL;
}

void gaccel_add(gaccel * gaccelp)
{
  gaccel_list = g_slist_append(gaccel_list, gaccelp);
}

void gaccel_window_activate(GtkAccelGroup * accelp, GtkWidget * widget)
{
  gtk_window_add_accel_group(GTK_WINDOW(widget), accelp);
}

void gaccel_save_keys(FILE * f)
{
  GSList *lst = gaccel_list;
  char *p;

  while(lst)
  {
    p = (char *) gaccel_to_str(lst->data);
    if(p)
    {
      fprintf(f, "MenuAccel: %s\n", p);
      g_free(p);
    }
    lst = lst->next;
  }
}
#endif /* GTK_FACE */
