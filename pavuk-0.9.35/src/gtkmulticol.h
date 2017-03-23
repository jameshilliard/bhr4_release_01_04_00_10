/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef __GTK_MULTICOL_H__
#define __GTK_MULTICOL_H__


#include <gdk/gdk.h>
#include <gtk/gtkbox.h>


#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


#define GTK_MULTICOL(obj)          GTK_CHECK_CAST (obj, gtk_multicol_get_type (), GtkMultiCol)
#define GTK_MULTICOL_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gtk_multicol_get_type (), GtkMultiColClass)
#define GTK_IS_MULTICOL(obj)       GTK_CHECK_TYPE (obj, gtk_multicol_get_type ())


  typedef struct _GtkMultiCol GtkMultiCol;
  typedef struct _GtkMultiColClass GtkMultiColClass;

  struct _GtkMultiCol
  {
    GtkBox box;

    gint nrows;

    gint vspacing;
    gint hspacing;
  };

  struct _GtkMultiColClass
  {
    GtkBoxClass parent_class;
  };

  guint gtk_multicol_get_type(void);
  GtkWidget *gtk_multicol_new(gint nrows);
  void gtk_multicol_set_number_of_rows(GtkMultiCol * multicol, gint nrows);
  void gtk_multicol_set_spacing(GtkMultiCol * multicol,
    gint hspacing, gint vspacing);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */


#endif                          /* __GTK_MULTICOL_H__ */
